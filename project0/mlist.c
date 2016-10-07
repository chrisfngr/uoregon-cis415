#include "mlist.h"
#include "mentry.h"

#define HASHSIZE 20

typedef struct mlist{   /* bucket for hash table */
    MList *next;        /* next entry in list    */
    MEntry *entry;      /* node's data           */
} MList;

static MList *hashtable[HASHSIZE];

/* ml_create - create a new mailing list */
MList *ml_create(void)
{
    
}

/* ml_add - addsd a new MEntry to the list;
 * returns 1 if successful, 0 if error (malloc)
 * returns 1 if it is a duplicate */
int ml_add(MList **ml, MEntry *me)
{
    return 1;
}

/ * ml_lookup - looks for MEntry in the list, returns matching entry or NULL */
MEntry *ml_lookup(MList *ml, Mentry *me)
{
    return NULL;
}

/* ml_destroy - destroy the mailing list */
void ml_destroy(MList *ml)
{  
    
}
