#include "mlist.h"
#include "mentry.h"
#include <stdlib.h>
static int HASH_SIZE     = 20;
static int HASH_INCREASE = 20;
static int RESIZES       = 0;
typedef struct mlistnode {   /* bucket for hash table */
    struct mlistnode *next;  /* next entry in list    */
    MEntry           *entry; /* node's data           */
} MListNode;

typedef struct bucketlist {
    struct   mlistnode *first;
    struct   mlistnode *last;
    unsigned long       size;
} BucketList;

struct mlist {
    BucketList **table;
    unsigned long size;
};



/* ml_create - create a new mailing list */
MList *ml_create(void)
{
    MList *list;

    if((list = (MList*) malloc(sizeof(MList))) != NULL)
    {
        list->table  = (BucketList**) malloc(sizeof(BucketList*)*HASH_SIZE); 
        list->size   = HASH_SIZE;
    }

    return list;
}


/* ml_resize - resizes hashtable, placing elements in buckets
 * by new hash values (in O(nm) time)
 */
MList* ml_resize(MList *ml)
{
    
    MList *new_list = ml_create(); 
    if(new_list == NULL)
    {
        return NULL;
    }

    new_list->size = HASH_SIZE;
    fprintf(stderr, "New list created, and size set\n");
    int i = 0;
    MListNode *elem = NULL;
    while(i < (HASH_SIZE - HASH_INCREASE))
    {
        fprintf(stderr, "about to start work on column %d\n", i);
        
        if(ml->table[i] != NULL)
            elem = ml->table[i++]->first;
        else
            i++;

        fprintf(stderr, "started work on column %d\n", i-1);
        while(elem->next != NULL)
        {
            fprintf(stderr, "about to add element from old array to new array\n");
            ml_add(&new_list, elem->entry);
            fprintf(stderr, "add function exited");
            elem = elem->next;
        }
        
    }
    ml_destroy(ml);
    return new_list;
}


/* ml_add - addsd a new MEntry to the list;
 * returns 1 if successful, 0 if error (malloc)
 * returns 1 if it is a duplicate */
int ml_add(MList **ml, MEntry *me)
{

    unsigned long hash_val = me_hash(me, HASH_SIZE);
    fprintf(stderr, "Hash: %lu\n", hash_val); 
    BucketList *bucket = NULL;

    if((*ml)->table[hash_val] == NULL)
    {
        bucket = (BucketList*) malloc(sizeof(BucketList));

        if(bucket == NULL)
        {
            return 0;
        }else{

            MListNode *new_node = (MListNode*) malloc(sizeof(MListNode));
    
            new_node->entry = me;
            new_node->next = NULL;

            (*ml)->table[hash_val]        = bucket;
            (*ml)->table[hash_val]->first = new_node;
            (*ml)->table[hash_val]->size  = 1;
            (*ml)->table[hash_val]->last  = new_node;
           
            fprintf(stderr, "created new bucket\n");
 
            return 1;
        }
    }else{
        if(ml_lookup(*ml, me) != NULL)
        {
            return 1;
        }
    }


    (*ml)->table[hash_val]->size++;

    if((*ml)->table[hash_val]->size > 20)
    {
        fprintf(stderr, "Bucket exceeded 20 elements. Had to resize hashtable from %d to %d", HASH_SIZE, HASH_SIZE + HASH_INCREASE);
        //HASH_SIZE = HASH_SIZE + HASH_INCREASE;
        //*ml = ml_resize(*ml);
    }

    hash_val = me_hash(me, HASH_SIZE);
    MListNode *new_node          = (MListNode*) malloc(sizeof(MListNode));
    new_node->entry              = me;    
    new_node->next               = NULL;

    fprintf(stderr, "about to add value to bucket %lu\n", hash_val);

    (*ml)->table[hash_val]->last->next = new_node;
    (*ml)->table[hash_val]->last       = new_node;

    fprintf(stderr, "added value to bucket %lu\n", hash_val);

    return 1;
}

/* ml_lookup - looks for MEntry in the list, returns matching entry or NULL */
MEntry *ml_lookup(MList *ml, MEntry *me)
{

    fprintf(stderr, "entering ml_lookup\n");
    unsigned long hash_val = me_hash(me, HASH_SIZE - RESIZES*HASH_INCREASE);
    MListNode* listEntry;
    fprintf(stderr, "got hash_val: %lu\n", hash_val); 
    if(ml->table[hash_val] == NULL){
        fprintf(stderr, "bucket null\n");
        return NULL;
    }
    fprintf(stderr, "about to set listEntry\n");
    listEntry = ml->table[hash_val]->first;
    fprintf(stderr, "just set listEntry\n");
    if(listEntry == NULL){

        fprintf(stderr, "first null\n");
        
        return NULL;
    }
    int compared = 0;
 
    while(listEntry != NULL){

        compared = me_compare(listEntry->entry, me);
        listEntry = listEntry->next;
        if(compared == 0){
            fprintf(stderr, "compare returned %d", compared);
            return listEntry->entry;
        }
    }
    fprintf(stderr, "ended ml_lookup, ret null\n");
    return NULL;
}

/* ml_destroy - destroy the mailing list */
void ml_destroy(MList *ml)
{  
    MListNode *node_del, *node_next;
    int i = 0;
    while( i < ml->size){
        node_del = ml->table[i++]->first;
        while(node_del != NULL){
            node_next = node_del->next;
            free(node_del);
            node_del = node_next;
        }
    }

    free(ml->table);
    free(ml);
}
