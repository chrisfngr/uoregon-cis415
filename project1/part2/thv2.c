#include "p1fxns.h"

#include <stdlib.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <unistd.h>
#include <time.h>
#include <stdio.h>
#include <signal.h>


//number of processes and processors
int nprocesses  = -1; 
int nprocessors = -1;

//command string
char *command = NULL;

//time vars
struct timespec start, end;




//routine for exiting program
void exitRoutine(int status){
    clock_gettime(CLOCK_REALTIME, &end);

    long double starttime, endtime;
    starttime = (long double) start.tv_sec + (((long double) start.tv_nsec)/1000000000);
    endtime   = (long double) end.tv_sec + (((long double) end.tv_nsec)/1000000000);

    printf("The elapsed time to execute %d copies of \"%s\" on %d processors is %7.3fsec\n",
            nprocesses, command, nprocessors,
            (double) (endtime - starttime) );

    if(command != NULL)
        free(command);
    exit(status);
}



void usage(){
    printf("USAGE: program [--number=<nprocesses>] [--processors=<nprocessors>] --command='command'\n");
    exit(1);
}


void parseArgs(int argc, char* argv[])
{
    int i;
	
	int location = -1;
	
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
}






int main(int argc, char* argv[])
{
    int i;


    parseArgs(argc, argv);


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

            if(execvp(file, args) < 0){
                 exit(1);
            }
            
        }
    }

    status = sigprocmask(SIG_UNBLOCK, &set, NULL);

    clock_gettime(CLOCK_REALTIME, &start);

    for(i = 0; i < nprocesses; i++){
        kill(pid[i], SIGUSR1);
    }


    for(i = 0; i < nprocesses; i++){
        kill(pid[i], SIGSTOP);
    }


    for(i = 0; i < nprocesses; i++){
        kill(pid[i], SIGCONT);
    }

    for(i = 0; i < nprocesses; i++){
        waitpid(pid[i], &status, 0);
    }


    exitRoutine(EXIT_SUCCESS);

    return 1;

}

