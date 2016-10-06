#include "mentry.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

MEntry *me_get(FILE *fd)
{
    MEntry* entry = (MEntry*) malloc(sizeof(MEntry));

    entry->house_number = 0;
    entry->zipcode      = (char*) malloc(sizeof(char)*6);
    entry->full_address = (char*) malloc(sizeof(char)*1024);
    entry->surname      = (char*) malloc(sizeof(char)*1024);

    char* buffer = (char*) malloc(sizeof(char)*1024);

    /* get surname */

    fgets(buffer, 1024, fd);
 
    if(feof(fd)){
        free(entry->zipcode);
        free(entry->full_address);
        free(entry->surname);
        free(entry);
        free(buffer);

        return NULL;
    }

    sscanf(buffer, "%s\n", entry->surname);
    strcat(entry->full_address, buffer);

   
     /* get house number */

    fgets(buffer, 1024, fd);

    if(feof(fd)){
        return NULL;
    }
    
    sscanf(buffer, "%d", &(entry->house_number));

    strcat(entry->full_address, buffer);
 
    
    /* get zipcode */

    fgets(buffer, 1024, fd);

    if(feof(fd)){
        return NULL;
    }

    sscanf(buffer, "%s", entry->zipcode);
    strcat(entry->full_address, buffer);
    free(buffer);
    return entry;
}

unsigned long me_hash(MEntry *me, unsigned long size)
{
    return 0L;
}

void me_print(MEntry *me, FILE *fd)
{
    fprintf(fd, me->full_address);
}

int me_compare(MEntry *me1, MEntry *me2)
{
    int tmp_result, result;
    
    /* compare surname */
    tmp_result = strcmp(me1->surname, me2->surname);
    if(tmp_result < 0){
        return -1;
    }else if(tmp_result > 0){
        return 1;
    }

    /* compare house number */
    tmp_result = (me1->house_number < me2->house_number) ? 1 : -1;
    if(me1->house_number == me2->house_number){
        /* skip over compares, because tmp_result will == -1 */
    }else if(tmp_result < 0){
        return -1;
    }else{
        return 1;
    }

    /* compare zipcode */
    tmp_result = strcmp(me1->zipcode, me2->zipcode);
    if(tmp_result < 0){
        return -1;
    }else if(tmp_result > 0){
        return 1;
    }

    return 0;
}

void me_destroy(MEntry *me)
{

    free(me->surname);
    free(me->zipcode);
    free(me->full_address);
    free(me);    

    me->surname      = NULL;
    me->zipcode      = NULL;
    me->full_address = NULL;
    me               = NULL;
}
