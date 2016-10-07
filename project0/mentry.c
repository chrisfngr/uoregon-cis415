#include "mentry.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

MEntry *me_get(FILE *fd)
{
    MEntry* entry = (MEntry*) malloc(sizeof(MEntry));
    
    entry->house_number = 0;
    entry->zipcode      = (char*) malloc(sizeof(char)*6);
    entry->full_address = (char*) malloc(sizeof(char)*1024);
    entry->surname      = (char*) malloc(sizeof(char)*1024);

    char zero = '0';
    memcpy(entry->full_address, &zero, 1024);

    char buffer[1024];
    buffer[1023] = '\0';
    

    /* get surname */

    fgets(buffer, 1024, fd);
 
    if(feof(fd)){
        free(entry->zipcode);
        free(entry->full_address);
        free(entry->surname);
        free(entry);


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

void str_to_upper(char* orig_str, char* new_str)
{
    unsigned int i = 0;
    while(i < strlen(orig_str)){
        new_str[i] = toupper(orig_str[i++]);
    }
}

int me_compare(MEntry *me1, MEntry *me2)
{
    int tmp_result;

    char sur1[strlen(me1->surname)+1];
    char sur2[strlen(me2->surname)+1];

    str_to_upper(me1->surname, sur1);
    str_to_upper(me2->surname, sur2);

    printf("New surname1: %s\n", sur1);
    printf("New surname2: %s\n", sur2);

    /* compare surname */
    tmp_result = strcmp(sur1, sur2);
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



    char zip1[strlen(me1->zipcode)+1];
    char zip2[strlen(me2->zipcode)+1];

    str_to_upper(me1->zipcode, zip1);
    str_to_upper(me2->zipcode, zip2);

    /* compare zipcode */
    tmp_result = strcmp(zip1, zip2);
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
}


