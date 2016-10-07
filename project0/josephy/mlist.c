/* Joseph Yaconelli
 * Duck ID: josephy
 * Project: CIS 415 Project 0
 * This is my own work except that I was in Deschutes 100 with many other students in the 
 * class while working on this, so while I didn't specifically work with anyone, general
 * ideas were discussed among us. I didn't use any specific ideas and didn't see anyone 
 * else's code while working.
 */

#include "mlist.h"
#include "mentry.h"
#include <stdlib.h>

static int HASH_SIZE     = 20;
static int HASH_INCREASE = 20;

typedef struct mlistnode {
    struct mlistnode *next;
    MEntry           *entry;
} MListNode;

typedef struct bucketlist {
    struct mlistnode *first;
    struct mlistnode *last;
    int size;
} BucketList;

struct mlist {
    BucketList **table;
    int size;
};



MList *ml_create(void)
{

    MList *list;


    if((list = (MList*) malloc(sizeof(MList))) != NULL){
        list->table = (BucketList**) malloc(sizeof(BucketList*)*HASH_SIZE);
        list->size  = HASH_SIZE;
    } else {
        return NULL;
    }

    return list;

}

MList* resize(MList *ml)
{
    MList *new_ml = ml_create();
    

       
    int i = 0;
    int j = 0;
    MListNode *node;
    for(i = 0; i < ml->size; i++){
        MListNode *node = ml->table[i]->first;
        for(j = 0; j < ml->table[i]->size; j++){
            ml_add(&new_ml, node->entry);

            node = node->next;
        }
    }

    ml_destroy(ml);

    return new_ml;
}

int ml_add(MList **ml, MEntry *me)
{
    
    unsigned long hash_val = me_hash(me, HASH_SIZE);

    MListNode *new_node = (MListNode*) malloc(sizeof(MListNode));

    new_node->entry = me;
    new_node->next  = NULL;
    
    //if bucket doesn't exist, create it, set node to start
    if((*ml)->table[hash_val] == NULL){
        BucketList *bucket = (BucketList*) malloc(sizeof(BucketList));
        (*ml)->table[hash_val] = bucket;
        (*ml)->table[hash_val]->first = new_node;
        (*ml)->table[hash_val]->last  = new_node;
        (*ml)->table[hash_val]->size  = 1;
    }else{
        if(ml_lookup(*ml, me) != NULL){
            return 1;
        }
        if((*ml)->table[hash_val]->size == 1){
            (*ml)->table[hash_val]->first->next = new_node;
            (*ml)->table[hash_val]->last = new_node;
            (*ml)->table[hash_val]->size = 2;
        }else{
            (*ml)->table[hash_val]->last->next = new_node;
            (*ml)->table[hash_val]->last       = new_node;
            (*ml)->table[hash_val]->size++;
        }
        if((*ml)->table[hash_val]->size > 20){
            fprintf(stderr, "Bucket exceeds 20 elements. Must resize\n");
            HASH_SIZE = HASH_SIZE + HASH_INCREASE;

            *ml = resize(*ml);
        }
    }

   return 1;

}

MEntry *ml_lookup(MList *ml, MEntry *me){
    unsigned long hash = me_hash(me, ml->size);

    if(ml->table[hash] == NULL)
        return NULL;

    int i = 0;
    MListNode *node = ml->table[hash]->first;
    for(i = 0; i < ml->table[hash]->size; i++){
        if(me_compare(me, node->entry) == 0){
            return node->entry;
        }
    }
    return NULL;
    
}

void ml_destroy(MList *ml){
    /*int i = 0;
    int j = 0;

    MListNode *node, *tmp_node;
    for(i = 0; i < ml->size; i++){
        node = ml->table[i]->first;
        for(j = 0; j < ml->table[i]->size; j++){
            tmp_node = node->next;
            free(node);
        }
        free(ml->table[i]);
    }

    free(ml->table);
    free(ml);*/
}
