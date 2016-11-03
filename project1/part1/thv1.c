#include "p1fxns.h"

#include <stdlib.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <unistd.h>
#include <time.h>
#include <stdio.h>

//number of processes and processors
int nprocesses  = -1; 
int nprocessors = -1;

//command string
char *command = NULL;

//time vars
struct timespec start, end;


//args to send to execvp
char **args  = NULL;
int argsSize = 0;

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
    int i = 0;
    if(args != NULL){
		while(i < argsSize){
			free(args[i++]);
		}
		free(args);
	}
    exit(status);
}



void usage(){
    printf("USAGE: program [--number=<nprocesses>] [--processors=<nprocessors>] --command='command'\n");
    exit(1);
}


void parseCommands(){
    args = (char**) malloc(sizeof(char*));
    char buffer[256];
    int location = 0;
    int i = 0;
    
    while(location != -1){
   	    location = p1getword(command, location, buffer);
   	    args[i] = p1strdup(buffer);
   	    
   	    args = (char**) realloc(args, sizeof(char*)*(++i + 1));
   	    args[i] = NULL;
	}
	
	argsSize = i + 1;
    
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
        
        parseCommands();
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

    int status;


    for(i = 0; i < nprocesses; i++){
        pid[i] = fork();
        if(pid[i] == 0){
            
            if(execvp(args[0], args) < 0){
                 
                 exit(1);
            }
            
        }
    }

    clock_gettime(CLOCK_REALTIME, &start);


    for(i = 0; i < nprocesses; i++){
        waitpid(pid[i], &status, 0);
    }

    exitRoutine(EXIT_SUCCESS);

    return 1;

}

