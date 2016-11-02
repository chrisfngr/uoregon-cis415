#include "p1fxns.h"

#include <stdlib.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <unistd.h>
#include <time.h>
#include <stdio.h>

void usage(){
    exit(1);
}

int main(int argc, char* argv[])
{

    int i;
    
    int nprocesses  = -1; //p1atoi(getenv("TH_NPROCESSES"));
    int nprocessors = -1; //p1atoi(getenv("TH_NPROCESSORS"));    
    int location    = -1;
    
    if(getenv("TH_NPROCESSES") == NULL){
        setenv("TH_NPROCESSES", "5", 1);
    }
    if(getenv("TH_NPROCESSORS") == NULL){
        setenv("TH_NPROCESSORS", "1", 1);
    }

    nprocesses  = p1atoi(getenv("TH_NPROCESSES"));
    nprocessors = p1atoi(getenv("TH_NPROCESSORS"));


    char *command;

    for(i = 1; i < argc; i++){
        if(p1strneq(argv[i], "--number=", p1strlen("--number=")) == 1){
            if((location = p1strchr(argv[i], '=')) == -1){
                usage();
            }

            // use p1atoi to convert str to int

            nprocesses = p1atoi(&(argv[i][location+1]));
 
        }else if(p1strneq(argv[i], "--processors=", p1strlen("--processors=")) == 1){
            if((location = p1strchr(argv[i], '=')) == -1){
                usage();
            }

            nprocessors = p1atoi(&(argv[i][location+1]));
    
        }else if(p1strneq(argv[i], "--command=", p1strlen("--command=")) == 1){
            command = (char *) malloc(p1strlen(argv[i])+1);
            command[0] = '\0';
            if((location = p1strchr(argv[i], '=')) == -1){
                usage();
            }

            p1strcpy(command, &(argv[i][location+1]));
        }
    }
    
    



    clock_t start, end; //figure this out later, but should be "start time"



    pid_t pid[nprocesses];

    int status;

    char* args[2];
    char file[p1strlen(command)+1];
    p1strcpy(file, command);
    args[0] = command;
    args[1] = NULL;

    start = clock();

    for(i = 0; i < nprocesses; i++){
        pid[i] = fork();
        if(pid[i] == 0){
            
            if(execvp(file, args) < 0){
                 
                 exit(1);
            }
            
        }
    }

    for(i = 0; i < nprocesses; i++){
        waitpid(pid[i], &status, 0);
    }

    end = clock();


    // seems to be some error here but I will figure that out later 
    printf("time elapsed: %ld - %ld / %d = %f\n", end, start, (int) CLOCKS_PER_SEC, ((double) end - start) / CLOCKS_PER_SEC);

    free(command);

    return 1;

}

