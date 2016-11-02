#include "BoundedBuffer.h"
#include "diskdevice_full.h"
#include "freesectordescriptorstore_full.h"
#include "sectordescriptorcreator.h"
#include "voucher.h"

#include <stdio.h>
#include <pthread.h>

#define BUFFER_SIZE 30


/* pthread lock and condition for vouchers */
pthread_mutex_t thread_lock;
pthread_cond_t thread_cond;

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
    pthread_mutex_t    lock;
    pthread_cond_t     condition;
    SectorDescriptor  *sector;
};

/* array of Vouchers */
Voucher vouchers[BUFFER_SIZE];

/* buffers */
BoundedBuffer  write_bb;
BoundedBuffer  read_bb;
BoundedBuffer  free_voucher_bb;


int fill_free_voucher_bb()
{
    int i;
    for(i = 0; i < BUFFER_SIZE; i++)
    {
        vouchers[i].lock      = thread_lock;
        vouchers[i].condition = thread_cond;

        if(nonblockingWriteBB(&free_voucher_bb, &(vouchers[i])) == 0)
            return 0;
    }

    return 1;
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
    pthread_mutex_init(&thread_lock, NULL);
    pthread_cond_init(&thread_cond, NULL);

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

}




/* Worker threads */

void write_worker()
{
    while(1)
    {
        Voucher *v = (Voucher *) blockingReadBB(&write_bb);

        pthread_mutex_lock(&(v->lock));
        v->status = (write_sector(device, v->sector) == 1) ? COMPLETE : ERROR;
        pthread_cond_signal(&(v->condition));  //might need to broadcast instead
        pthread_mutex_unlock(&(v->lock));
    }
}

void read_worker()
{
    while(1)
    {

        Voucher *v = (Voucher *) blockingReadBB(&read_bb);

        pthread_mutex_lock(&(v->lock));
        v->status = (read_sector(device, v->sector) == 1) ? COMPLETE : ERROR;
        pthread_cond_signal(&(v->condition));  //might need to broadcast instead
        pthread_mutex_unlock(&(v->lock));
    }
}

/* helper for setting vouchers */

void set_voucher(Voucher *v, SectorDescriptor *sd, voucher_type v_type)
{
    v->status = INCOMPLETE;
    v->type   = v_type;
    v->sector = sd;
}


/* Calls made by apps into disk driver */

void blocking_write_sector(SectorDescriptor *sd, Voucher **v)
{
    *v = (Voucher*) blockingReadBB(&free_voucher_bb);

    set_voucher(*v, sd, READ);

    blockingWriteBB(&write_bb, &v);
}

int nonblocking_write_sector(SectorDescriptor *sd, Voucher **v)
{

    if(nonblockingReadBB(&free_voucher_bb, (void**) v) == 0)
        return 0;

    set_voucher(*v, sd, WRITE);

    return nonblockingWriteBB(&write_bb, *v);
}

void blocking_read_sector(SectorDescriptor *sd, Voucher **v)
{
    *v = (Voucher*) blockingReadBB(&free_voucher_bb);

    set_voucher(*v, sd, READ);

    blockingWriteBB(&read_bb, &v);
}

int nonblocking_read_sector(SectorDescriptor *sd, Voucher **v)
{

    if(nonblockingReadBB(&free_voucher_bb, (void**) v) == 0)
        return 0;

    set_voucher(*v, sd, READ);

    return nonblockingWriteBB(&read_bb, *v);

}


int redeem_voucher(Voucher *v, SectorDescriptor **sd)
{
    pthread_mutex_lock(&(v->lock));
    while(v->status == INCOMPLETE)
        pthread_cond_wait(&(v->condition), &(v->lock));
    blockingWriteBB(&free_voucher_bb, v);
    return v->status == COMPLETE ? 1 : 0;
}
