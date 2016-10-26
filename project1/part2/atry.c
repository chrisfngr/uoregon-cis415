#include <signal.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/wait.h>

void diedHandler(int signal){
    printf("thread %ld died", getpid());
}

int main(int argc, char *argv[]){

    int i = 0;

    int SIZE = 10;

    pid_t pid[SIZE];
    int status;

    sigset_t mask;
    sigemptyset(&mask);
    sigaddset(&mask, SIGUSR1);
    sigaddset(&mask, SIGCONT);
    sigaddset(&mask, SIGSTOP);

    sigprocmask(SIG_SETMASK, &mask, NULL);

    int sig;

    

    for(i = 0; i < SIZE; i++){
        pid[i] = fork();

        if(pid[i] == 0){
            sigprocmask(SIG_UNBLOCK, &mask, NULL);
            sigwait(&mask, &sig);
            printf("caught signal in thread %ld\n", getpid());
            exit(1);
        }
    }

    for(i = 0; i < SIZE; i++)
    {
        kill(pid[i], SIGUSR1);
    }

    return 1;
}
