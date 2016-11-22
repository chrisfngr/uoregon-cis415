/* Duck ID: josephy
 * CIS 415 Project 2
 * UO ID: 951 485 015
 * This is my own work.
 */

#include "BoundedBuffer.h"
#include "diskdevice_full.h"
#include "freesectordescriptorstore_full.h"
#include "sectordescriptorcreator.h"
#include "voucher.h"

#include <stdio.h>
#include <pthread.h>

#define BUFFER_SIZE 30

/* threads for concurrent writing and reading functionality */
pthread_t write_thread, read_thread;

/* global pointer to apps' free sector descriptor store */
FreeSectorDescriptorStore *sector_store;

/* disk device - in charge of executing reads and writes */
DiskDevice *device = NULL;


/*** enums for voucher - for readability ***/

/* status of voucher */
typedef enum {
    INCOMPLETE,
    COMPLETE,
    ERROR
} voucher_status;

/* type of voucher */
typedef enum {
    WRITE,
    READ
} voucher_type;


/* Voucher struct - item that will be passed between driver, device, and apps */
struct voucher {
    voucher_status     status;
    voucher_type       type;
    pthread_mutex_t    lock;
    pthread_cond_t     condition;
    SectorDescriptor  *sector;
};

/* array of Vouchers */
Voucher vouchers[BUFFER_SIZE];

/* buffers */
BoundedBuffer *write_bb;
BoundedBuffer *read_bb;
BoundedBuffer *free_voucher_bb;

/*
 * initializes locks and conditions for all vouchers
 * and adds them to free voucher buffer
 * returns 1 on success, otherwise 0
 */

int fill_free_voucher_bb()
{
    int i;
    for(i = 0; i < BUFFER_SIZE; i++)
    {
       
        /* initialize lock and condition for voucher */ 
        pthread_mutex_init(&(vouchers[i].lock), NULL);
        pthread_cond_init(&(vouchers[i].condition), NULL);

        /* add voucher to free buffer array and return 0 if can't do it */
        if(nonblockingWriteBB(free_voucher_bb, &(vouchers[i])) == 0)
            return 0;
    }

    return 1;
}

/* function called by write thread */
void * write_worker()
{
    /* while running, continuously loop */
    while(1)
    {
        /* get the next voucher from the bounded buffer */
        Voucher *v = (Voucher *) blockingReadBB(write_bb);


        /* lock for critical section, about to change voucher */
        pthread_mutex_lock(&(v->lock));

        /*
         * write sector to device, set voucher status to COMPLETE on success
         * otherwise, set to ERROR
         */
        v->status = (write_sector(device, v->sector) == 1) ? COMPLETE : ERROR;

        /* broadcast condition (this means the status has changed */
        pthread_cond_broadcast(&(v->condition));

        /* give back lock */
        pthread_mutex_unlock(&(v->lock));
    }
}

/* function called by read thread */
void * read_worker()
{
    /* while running, continuously loop */
    while(1)
    {

        /* Get voucher from read buffer */
        Voucher *v = (Voucher *) blockingReadBB(read_bb);

        /* get lock, about to enter critical zone. Multiple parts of code
         * has access to the voucher right now
         */
        pthread_mutex_lock(&(v->lock));

        /* have device read sector, then return status of read as COMPLETE or ERROR */
        v->status = (read_sector(device, v->sector) == 1) ? COMPLETE : ERROR;

        /* broadcast that the status of the voucher has been update */
        pthread_cond_broadcast(&(v->condition));  //might need to broadcast instead

        /* release lock */
        pthread_mutex_unlock(&(v->lock));
    }
}


/* the first function called on start up */
void init_disk_driver(DiskDevice *dd, void *mem_start,
                      unsigned long mem_length,
                      FreeSectorDescriptorStore **fsds)
{
    /* set the global device */
    device = dd;

    /* create and init fsds */
    if((*(fsds) = create_fsds()) == NULL)
        printf("error in creating fsds!\n");


    create_free_sector_descriptors(*fsds, mem_start, mem_length);

    sector_store = *fsds;

    /* create bounded buffers for write, read, and free vouchers */
    if((write_bb = createBB(BUFFER_SIZE)) == NULL)
        printf("error in creating write_bb\n");

    if((read_bb = createBB(BUFFER_SIZE)) == NULL)
        printf("error in creating read_bb\n");

    if((free_voucher_bb = createBB(BUFFER_SIZE)) == NULL)
        printf("error in creating free_voucher_bb\n");

    /* fill free_voucher_bb with vouchers */
    if(fill_free_voucher_bb() == 0)
        printf("error in fulling free_voucher_bb\n");

    /* initialize read and write threads */
    if(pthread_create(&write_thread, NULL, &write_worker, NULL) != 0)
        printf("error in creating write_thread\n");
    if(pthread_create(&read_thread, NULL, &read_worker, NULL) != 0)
        printf("error in creating read_thread\n");

}




/* helper for setting vouchers */
void set_voucher(Voucher *v, SectorDescriptor *sd, voucher_type v_type)
{
    /* set status, type, and sector */
    v->status    = INCOMPLETE;
    v->type      = v_type;
    v->sector    = sd;
}


/* Calls made by apps into disk driver */

/* blocks until write is complete */
void blocking_write_sector(SectorDescriptor *sd, Voucher **v)
{
    /* wait until a free voucher is available */
    *v = (Voucher*) blockingReadBB(free_voucher_bb);

    /* set voucher */
    set_voucher(*v, sd, WRITE);

    /* add voucher to write buffer so the write thread can process it */
    blockingWriteBB(write_bb, *v);
}

/* attempts to add voucher to write buffer,
 * returns 1 if success, otherwise 0
 */
int nonblocking_write_sector(SectorDescriptor *sd, Voucher **v)
{

    /* attempt to get free voucher, return 0 if none available */
    if(nonblockingReadBB(free_voucher_bb, (void**) v) == 0)
    {
        /* clean up the allocated sector */
        blocking_put_sd(sector_store, sd);
        return 0;
    }

    set_voucher(*v, sd, WRITE);

    /* attempt to add to write buffer, return 0 if no room */
    if(nonblockingWriteBB(write_bb, *v) == 0)
    {
        /* clean up routine */

        /* put voucher back in free voucher buffer */
        blockingWriteBB(free_voucher_bb, *v);

        /* return sector to free sector descriptor store */
        blocking_put_sd(sector_store, sd);

        return 0;
    }

    return 1;
}

/* blocks until able to add add voucher to read queue */
void blocking_read_sector(SectorDescriptor *sd, Voucher **v)
{

    /* get voucher from free voucher buffer */
    *v = (Voucher*) blockingReadBB(free_voucher_bb);

    set_voucher(*v, sd, READ);

    /* add voucher to read buffer */
    blockingWriteBB(read_bb, *v);
}

/* attempts to add voucher to read buffer, returns 1 on success
 * otherwise return 0
 */
int nonblocking_read_sector(SectorDescriptor *sd, Voucher **v)
{

    /* attempts to get voucher if one is available */
    if(nonblockingReadBB(free_voucher_bb, (void**) v) == 0)
        return 0;

    set_voucher(*v, sd, READ);

    /* attempts to add voucher to read buffer, returns 1 on success,
     * otherwise return 0
     */
    if(nonblockingWriteBB(read_bb, *v) == 0)
    {
        /* return unused voucher to free voucher buffer */
        blockingWriteBB(free_voucher_bb, *v);
        return 0;
    }

    return 1;

}

/* called by app to retrieve completed voucher
 * returns 1 if routine was successful, otherwise 0
 */
int redeem_voucher(Voucher *v, SectorDescriptor **sd)
{
    /* get lock, since we will be accessing shared data */
    pthread_mutex_lock(&(v->lock));

    /* wait until status is no longer INCOMPLETE
     * in other words, wait until status is COMPLETE or ERROR
     */
    while(v->status == INCOMPLETE)
        pthread_cond_wait(&(v->condition), &(v->lock));

    /* return voucher to free voucher buffer */
    blockingWriteBB(free_voucher_bb, v);

    /* if type of voucher was READ, return read data to app
     * otherwise, if the type was WRITE return the sector
     * to the free sector descriptor store
     */
    if(v->type == READ)
        *sd = v->sector;
    else
        blocking_put_sd(sector_store, v->sector);
    
    /* release the lock */
    pthread_mutex_unlock(&(v->lock));

    /* return 1 on success, otherwise 0 */
    return (v->status == COMPLETE);
}
