#include "p1fxns.h"

#include <stdlib.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <unistd.h>


void usage(){
    exit(1);
}

int main(int argc, const char* argv[])
{

    int i;
    
    int nprocesses  = -1; //p1atoi(getenv("TH_NPROCESSES"));
    int nprocessors = -1; //p1atoi(getenv("TH_NPROCESSORS"));    
    int location    = -1;



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
    
    



    long long starttime; //figure this out later, but should be "start time"

    pid_t pid[nprocesses];

    int status;

    for(i = 0; i < nprocesses; i++){
        pid[i] = fork();
        if(pid[i] == 0){
            
            char* args[1];
            char file[p1strlen(command)+1];
            p1strcpy(file, command);
            args[0] = command;
            if(execvp(file, args) < 0){
                 
                 exit(1);
            }
            
        }
    }

    for(i = 0; i < nprocesses; i++){
        waitpid(pid[i], &status, 0);
    }

    free(command);

    return 1;

}

