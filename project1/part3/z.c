#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
int main(int argc, char *argv[]){
    int i = 0; 

    srand((double) clock());
    int a;

    for(i = 0; i < 100000000; i++){
        a = (rand() + a) % 50;
    }


    printf("I made a random number: %d!\n", a);
}
