#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
int main(int argc, char *argv[]){
    sleep(1);

    srand((double) clock());
    int a;
    a = rand();
    printf("I made a random number: %d!\n", a % 50);
}
