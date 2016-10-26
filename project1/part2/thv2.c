#include "p1fxns.h"

#include <stdlib.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <unistd.h>
#include <time.h>
#include <stdio.h>
#include <signal.h>

void usage(){
    exit(1);
}

void handler(int sig){

    printf("caught signal %d in thread %ld\n", sig, getpid());
}

void stophand(int sig){

    printf("STOP: caught signal %d in thread %ld\n", sig, getpid());
}

int main(int argc, const char* argv[])
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

    int status, s, sig;
    
    sigset_t set;

    sigemptyset(&set);
    sigaddset(&set, SIGUSR1);
    sigaddset(&set, SIGSTOP);
    sigaddset(&set, SIGCONT);
    s = sigprocmask(SIG_BLOCK, &set, NULL);
    if(s != 0)
        printf("error in sigprocmask creation!!\n");
 
    char* args[1];
    char file[p1strlen(command)+1];
    p1strcpy(file, command);
    args[0] = command;


    for(i = 0; i < nprocesses; i++){
       
        if((pid[i] = fork()) == 0){

            status = sigwait(&set, &sig); 

            signal(SIGSTOP, stophand);
            signal(SIGCONT, handler);

            

            if(status != 0)
                printf("error in sigwait!");

            status = sigprocmask(SIG_UNBLOCK, &set, NULL);

            printf("thread %ld caught signal %d\n", getpid(), sig);
            raise(SIGSTOP);
            raise(SIGCONT);

            if(execvp(file, args) < 0){
                 exit(1);
            }
            
        }
    }


    //start = clock();
    for(i = 0; i < nprocesses; i++){
        printf("sugusr1 being sent to %ld\n", pid[i]);
        kill(pid[i], SIGUSR1);
    }


    for(i = 0; i < nprocesses; i++){
        printf("about to send SIGSTOP to %ld\n", pid[i]);
        kill(pid[i], SIGSTOP);
    }


    for(i = 0; i < nprocesses; i++){
        printf("about to send SIGCONT to %ld\n", pid[i]);
        kill(pid[i], SIGCONT);
    }

    for(i = 0; i < nprocesses; i++){
        waitpid(pid[i], &status, 0);
    }

    //end = clock();


    // seems to be some error here but I will figure that out later 
    //printf("time elapsed: %ld - %ld / %d = %f\n", end, start, CLOCKS_PER_SEC, ((double) end - start) / CLOCKS_PER_SEC);

    free(command);

    return 1;

}

