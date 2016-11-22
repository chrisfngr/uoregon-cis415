#include "BoundedBuffer.h"
#include "diskdevice_full.h"
#include "freesectordescriptorstore_full.h"
#include "sectordescriptorcreator.h"
#include "voucher.h"
#include "generic_queue.h"


#include <stdio.h>
#include <pthread.h>

#define BUFFER_SIZE 30

/* pthreads */
pthread_t write_thread, read_thread;


/* --------- for testing --------- */
int sd_freed = 0, vouchers_freed = 0, sd_used = 0, vouchers_used = 0;



/* free sector descriptor store */
FreeSectorDescriptorStore *sector_store;

/* disk device - in charge of executing reads and writes */
DiskDevice *device = NULL;


/*** enums for voucher ***/

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


int fill_free_voucher_bb()
{
    int i;
    for(i = 0; i < BUFFER_SIZE; i++)
    {
       
        /* add lock and condition to voucher */ 
        pthread_mutex_init(&(vouchers[i].lock), NULL);
        pthread_cond_init(&(vouchers[i].condition), NULL);

        if(nonblockingWriteBB(free_voucher_bb, &(vouchers[i])) == 0)
            return 0;
    }

    return 1;
}


void * write_worker()
{
    while(1)
    {
        Voucher *v = (Voucher *) blockingReadBB(write_bb);

        if(v == NULL)
            printf("voucher null!!\n");

        if(v->sector == NULL)
            printf("sector null!!\n");


        pthread_mutex_lock(&(v->lock));
        v->status = (write_sector(device, v->sector) == 1) ? COMPLETE : ERROR;
        pthread_cond_broadcast(&(v->condition));  //might need to broadcast instead
        pthread_mutex_unlock(&(v->lock));
    }
}


void * read_worker()
{
    while(1)
    {

        Voucher *v = (Voucher *) blockingReadBB(read_bb);

        pthread_mutex_lock(&(v->lock));
        v->status = (read_sector(device, v->sector) == 1) ? COMPLETE : ERROR;
        pthread_cond_broadcast(&(v->condition));  //might need to broadcast instead
        pthread_mutex_unlock(&(v->lock));
    }
}

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

    /* initialize threads */
    if(pthread_create(&write_thread, NULL, &write_worker, NULL) != 0)
        printf("error in creating write_thread\n");
    if(pthread_create(&read_thread, NULL, &read_worker, NULL) != 0)
        printf("error in creating read_thread\n");

}




/* helper for setting vouchers */

void set_voucher(Voucher *v, SectorDescriptor *sd, voucher_type v_type)
{
    v->status    = INCOMPLETE;
    v->type      = v_type;
    v->sector    = sd;

    vouchers_used++;

}


/* Calls made by apps into disk driver */

void blocking_write_sector(SectorDescriptor *sd, Voucher **v)
{
    sd_used++;
    *v = (Voucher*) blockingReadBB(free_voucher_bb);

    set_voucher(*v, sd, WRITE);
    blockingWriteBB(write_bb, *v);
}

int nonblocking_write_sector(SectorDescriptor *sd, Voucher **v)
{

    sd_used++;
    if(nonblockingReadBB(free_voucher_bb, (void**) v) == 0)
    {
        blocking_put_sd(sector_store, sd);
        sd_freed++;
        return 0;
    }

    set_voucher(*v, sd, WRITE);

    if(nonblockingWriteBB(write_bb, *v) == 0)
    {
        blockingWriteBB(free_voucher_bb, *v);
        blocking_put_sd(sector_store, sd);
        sd_freed++;
        vouchers_freed++;
        return 0;
    }

    return 1;
}

void blocking_read_sector(SectorDescriptor *sd, Voucher **v)
{
    *v = (Voucher*) blockingReadBB(free_voucher_bb);

    set_voucher(*v, sd, READ);

    blockingWriteBB(read_bb, *v);
}

int nonblocking_read_sector(SectorDescriptor *sd, Voucher **v)
{

    if(nonblockingReadBB(free_voucher_bb, (void**) v) == 0)
        return 0;

    set_voucher(*v, sd, READ);

    if(nonblockingWriteBB(read_bb, *v) == 0)
    {
        blockingWriteBB(free_voucher_bb, *v);
        vouchers_freed++;
        sd_freed++;
        return 0;
    }

    return 1;

}


int redeem_voucher(Voucher *v, SectorDescriptor **sd)
{
    printf("-------- REDEEM VOUCHER --------\n");
    pthread_mutex_lock(&(v->lock));
    while(v->status == INCOMPLETE)
        pthread_cond_wait(&(v->condition), &(v->lock));
    blockingWriteBB(free_voucher_bb, v);
    printf("-------- RETURNED VOUCHER --------\n");
    vouchers_freed++;

    if(v->type == READ)
        *sd = v->sector;
    else{
        blocking_put_sd(sector_store, v->sector);
        printf("-------- RETURNED SECTOR DESCRIPTOR --------\n");
        sd_freed++;
    }

    printf("sd used: %d\t\tvouchers used: %d\nsd free: %d\t\tvouchers free: %d\n",
        sd_used, vouchers_used, sd_freed, vouchers_freed);
    pthread_mutex_unlock(&(v->lock));

    return (v->status == COMPLETE);
}
