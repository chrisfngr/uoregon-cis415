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

    char* buffer = malloc(sizeof(char)*1024);


    fgets(buffer, 1024, fd);
 
    if(feof(fd)){
        return NULL;
    }

    /* get surname */

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

    printf("%s", entry->full_address);

    return entry;
}

unsigned long me_hash(MEntry *me, unsigned long size)
{
    return 0L;
}

void me_print(MEntry *me, FILE *fd)
{

}

int me_compare(MEntry *me1, MEntry *me2)
{
    return 1;
}

void me_destroy(MEntry *me)
{

}
