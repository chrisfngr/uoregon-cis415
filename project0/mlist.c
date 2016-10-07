#include "mlist.h"
#include "mentry.h"

#define HASHSIZE 20

typedef struct mlistnode {   /* bucket for hash table */
    MList  *next;            /* next entry in list    */
    MEntry *entry;           /* node's data           */
} MListNode;

struct mlist {
    struct mlistnode *first;
    struct mlistnode *last;
    unsigned long     size;
};

static MList *hashtable[HASHSIZE];

/* ml_create - create a new mailing list */
MList *ml_create(void)
{
    MList *list;

    if((list = (MList*) malloc(sizeof(MList))) != NULL)
    {
        list->first  = NULL;
        list->last   = NULL;
        list->size   = 0;
    }

    return list;
}

/* ml_add - addsd a new MEntry to the list;
 * returns 1 if successful, 0 if error (malloc)
 * returns 1 if it is a duplicate */
int ml_add(MList **ml, MEntry *me)
{
    MList *p;
    MListNode *q;

    p = *ml;
    if(ml_lookup(p, me) != NULL)
        return 1;
    if((q = (MListNode *) malloc(sizeof(MListNode))) == NULL)
        return 0;
    q->entry = me;
    q->next  = p->first;
    p->first = q;
    if(!(p->last))
        p->last = q;
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
