#include "BoundedBuffer.h"
#include "diskdevice_full.h"
#include "freesectordescriptorstore_full.h"
#include "pid.h"
#include "sectordescriptorcreator.h"
#include "sectordescriptor.h"
#include "voucher.h"

#include <stdio.h>
#include <pthread.h>

DiskDevice *device = NULL;

/* buffers */
Voucher **write_bb         = NULL;
Voucher **read_bb          = NULL;
Voucher **free_voucher_bb  = NULL;

/* enums for voucher */

// status of voucher
typedef enum {
    INCOMPLETE,
    COMPLETE
} status;

// type of voucher
typedef enum {
    WRITE,
    READ
} voucher_type;


/* Voucher struct - item that will be passed between driver, device, and apps*/
Voucher {
    status             voucher_status;
    voucher_type       type;
    pthread_mutex_lock lock;
    pthread_cond       condition;
    SectorDescriptor  *sector;
};

void write_worker()
{

}

void read_worker()
{

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

    create_free_sectors_descriptors(fsds, mem_start, mem_length);


}
