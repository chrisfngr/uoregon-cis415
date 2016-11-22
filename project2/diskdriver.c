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

/* pthread lock and condition for vouchers */
pthread_mutex_t read_lock, write_lock;
pthread_cond_t  read_cond, write_cond;
/* disk device - in charge of executing reads and writes */
DiskDevice *device = NULL;


/* enums for voucher */

// status of voucher
typedef enum {
    INCOMPLETE,
    COMPLETE,
    ERROR
} voucher_status;

// type of voucher
typedef enum {
    WRITE,
    READ
} voucher_type;


/* Voucher struct - item that will be passed between driver, device, and apps*/
struct voucher {
    voucher_status     status;
    voucher_type       type;
    pthread_mutex_t   *lock;
    pthread_cond_t    *condition;
    SectorDescriptor  *sector;
};

/* array of Vouchers */
Voucher vouchers[BUFFER_SIZE];

/* buffers */
BoundedBuffer* write_bb;
BoundedBuffer* read_bb;
BoundedBuffer* free_voucher_bb;


int fill_free_voucher_bb()
{
    int i;
    for(i = 0; i < BUFFER_SIZE; i++)
    {
        vouchers[i].lock      = NULL;
        vouchers[i].condition = NULL;

        if(nonblockingWriteBB(free_voucher_bb, &(vouchers[i])) == 0)
            return 0;
    }

    return 1;
}


void * write_worker()
{
    while(1)
    {
        printf("about to get voucher \n");
        Voucher *v = (Voucher *) blockingReadBB(write_bb);
        printf("got voucher!!\n");
        if(v == NULL)
            printf("voucher null!!\n");
        //Voucher *v;

        if(v->sector == NULL)
            printf("sector null!!\n");

        //if((nonblockingReadBB(write_bb, (void **) &v)) == 0)
        //    printf("coudnt get shit from writebaby\n");

 

        pthread_mutex_lock(&(v->lock));
        fprintf(stderr, "about to run write_sector...\n");
        v->status = (write_sector(device, v->sector) == 1) ? COMPLETE : ERROR;
        pthread_cond_signal(v->condition);  //might need to broadcast instead
        printf("status: %d\n", v->status);
        pthread_mutex_unlock(v->lock);
    }
}


void * read_worker()
{
    while(1)
    {

        Voucher *v = (Voucher *) blockingReadBB(read_bb);

        pthread_mutex_lock(v->lock);
        v->status = (read_sector(device, v->sector) == 1) ? COMPLETE : ERROR;
        pthread_cond_signal(v->condition);  //might need to broadcast instead
        pthread_mutex_unlock(v->lock);
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


    /* init pthread mutex and condition */
    pthread_mutex_init(&write_lock, NULL);
    pthread_cond_init(&write_cond, NULL);

    pthread_mutex_init(&read_lock, NULL);
    pthread_cond_init(&read_cond, NULL);

    /* create bounded buffers for write, read, and free vouchers */
    if((write_bb = createBB(BUFFER_SIZE)) == NULL)
        printf("error in creating write_bb\n");

    if((read_bb = createBB(BUFFER_SIZE)) == NULL)
        printf("error in creating write_bb\n");

    if((free_voucher_bb = createBB(BUFFER_SIZE)) == NULL)
        printf("error in creating write_bb\n");

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
    v->lock      = (v_type == WRITE) ? &write_lock : &read_lock;
    v->condition = (v_type == WRITE) ? &write_cond : &read_cond;
}


/* Calls made by apps into disk driver */

void blocking_write_sector(SectorDescriptor *sd, Voucher **v)
{
    *v = (Voucher*) blockingReadBB(free_voucher_bb);

    set_voucher(*v, sd, READ);

    blockingWriteBB(write_bb, &v);
}

int nonblocking_write_sector(SectorDescriptor *sd, Voucher **v)
{

    if(nonblockingReadBB(free_voucher_bb, (void**) v) == 0)
        return 0;

    set_voucher(*v, sd, WRITE);

    return nonblockingWriteBB(write_bb, *v);
}

void blocking_read_sector(SectorDescriptor *sd, Voucher **v)
{
    *v = (Voucher*) blockingReadBB(free_voucher_bb);

    set_voucher(*v, sd, READ);

    blockingWriteBB(read_bb, &v);
}

int nonblocking_read_sector(SectorDescriptor *sd, Voucher **v)
{

    if(nonblockingReadBB(free_voucher_bb, (void**) v) == 0)
        return 0;

    set_voucher(*v, sd, READ);

    return nonblockingWriteBB(read_bb, *v);

}


int redeem_voucher(Voucher *v, SectorDescriptor **sd)
{
    pthread_mutex_lock(v->lock);
    while(v->status == INCOMPLETE)
        pthread_cond_wait(v->condition, v->lock);
    blockingWriteBB(free_voucher_bb, v);

    sd = &(v->sector);
    pthread_mutex_unlock(v->lock);
    return (v->status == COMPLETE);
}
