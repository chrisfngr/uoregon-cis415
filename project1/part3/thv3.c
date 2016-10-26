#include "p1fxns.h"

#include <sys/time.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>
#include <signal.h>

void usage(){
    printf("USAGE: program [--number=<nprocesses>] [--processors=<nprocessors>] --command='command'\n");
    exit(1);
}


void scheduler(){
    /*
    -----PSUEDO CODE-----
    broadcast(SIGSTOP);
    signal(SIGCONT) to the next NPROCESSORS from pid[] that are still alive
                    (which means this has to be able to access pid[])
    */

    printf("scheduler called\n");

}


int main(int argc, char* argv[])
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
        }else{
            usage();
        }
    }
    
    
    if(getenv("TH_NPROCESSES") == NULL && nprocesses == -1){
        printf("TH_NPROCESSES not given or in env vars!\n");
        exit(0);
    }
    if(getenv("TH_NPROCESSORS") == NULL && nprocessors == -1){
        printf("TH_NPROCESSORS not given or in env vars!\n");
        exit(0);
    }

    if(nprocesses == -1)
        nprocesses  = p1atoi(getenv("TH_NPROCESSES"));
    if(nprocessors == -1)
        nprocessors = p1atoi(getenv("TH_NPROCESSORS"));

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
 
    char* args[2];
    char file[p1strlen(command)+1];
    p1strcpy(file, command);
    args[0] = command;

    args[1] = NULL;

    for(i = 0; i < nprocesses; i++){
       
        if((pid[i] = fork()) == 0){

            status = sigwait(&set, &sig); 

            signal(SIGCONT, SIG_DFL);


            if(status != 0)
                printf("error in sigwait!");

            if(status != 0)
                printf("sig_unblock didn't work :)\n");

            printf("thread %ld caught signal %d\n", (long int) getpid(), sig);
            //raise(SIGSTOP);
            //raise(SIGCONT);

            printf("about to call execvp(%s, %s, %s)\n", file, args[0], args[1]);
            if(execvp(file, args) < 0){
                 exit(1);
            }
            
        }
    }


    status = sigprocmask(SIG_UNBLOCK, &set, NULL);

    //start = clock();

    struct itimerval timer;
    timer.it_interval.tv_sec  = 0;
    timer.it_interval.tv_usec = 250000;
    timer.it_value.tv_sec     = 0;
    timer.it_value.tv_usec    = 250000;

    signal(SIGALRM, scheduler);
    printf("about to set timer\n");
    setitimer(ITIMER_REAL, &timer, NULL);
    //end = clock();
    while(1); // just for testing so I can see the timer handler called (SIGALRM)
    // seems to be some error here but I will figure that out later 
    //printf("time elapsed: %ld - %ld / %d = %f\n", end, start, CLOCKS_PER_SEC, ((double) end - start) / CLOCKS_PER_SEC);

    free(command);

    return 1;

}

