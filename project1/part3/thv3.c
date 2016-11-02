#include "p1fxns.h"
#include <sys/time.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>
#include <signal.h>
#include <time.h>

typedef struct pidQueueElem {
    pid_t pid;
    struct pidQueueElem* next;
} pid_queue_elem;

typedef struct pidQueue {
    pid_queue_elem *head;
    pid_queue_elem *tail;
} pid_queue;

//global pid queue. NOTE: should only be manipulated by 1 thread (the main thread)
pid_queue queue = { NULL, NULL };

//number of processes and processors
int nprocesses  = -1; 
int nprocessors = -1;

//command string
char *command = NULL;

//time vars
struct timespec start, end;

//mask for sigchld;
sigset_t sig_chld_alrm_mask;
int s;

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

// usage rules for program
void usage(){
    printf("USAGE: program [--number=<nprocesses>] [--processors=<nprocessors>] --command='command'\n");
    exitRoutine(EXIT_FAILURE);
}

//sets up global mask
void sig_chld_alrm_mask_setup(){
    sigemptyset(&sig_chld_alrm_mask);
    sigaddset(&sig_chld_alrm_mask, SIGCHLD);
    sigaddset(&sig_chld_alrm_mask, SIGALRM);
}

int queueIsEmpty(pid_queue *queue){
    return (queue->head == NULL);
}

//finds element in queue
pid_queue_elem* findElem(int pid, pid_queue *queue){
    pid_queue_elem *elem;
    elem = queue->head;


    while(elem != NULL){
        if(elem->pid == pid)
            return elem;
        elem = elem->next;
    }
    return NULL;
}

//removes element from queue
int removeElem(pid_t pid, pid_queue *queue){


    //s = sigprocmask(SIG_BLOCK, &sig_chld_alrm_mask, NULL);

    if(s != 0)
        printf("error in sigprocmask creation!!\n");

    pid_queue_elem *elem_to_rm, *elem;
    elem_to_rm = NULL;
    elem       = NULL;



    if((elem_to_rm = findElem(pid, queue)) == NULL)
        return -1;


    elem = queue->head;

    while(elem->next != NULL){
        if(elem->next == elem_to_rm)
            break;
        elem = elem->next;
    }


    if(elem_to_rm == queue->head){
        queue->head = elem_to_rm->next;
    }else if(elem_to_rm == queue->tail){
        queue->tail = elem;
        elem->next  = NULL;
    }else{
        elem->next = elem_to_rm->next;
    }


    free(elem_to_rm);

    if(queueIsEmpty(queue))
        exitRoutine(EXIT_SUCCESS);


    return 1;
}

// pushes element to queue
void push(pid_queue_elem *elem, pid_queue *queue){
    
    if(queue->head == NULL){
        queue->head       = elem;
        queue->head->next = NULL;
    }else if(queue->tail == NULL){
        queue->tail       = elem;
        queue->tail->next = NULL;
        queue->head->next = elem;
    }else{
        queue->tail->next = elem;
        queue->tail       = elem;
        queue->tail->next = NULL;
    }

}

// non-destructively pops element from head of queue
pid_queue_elem* pop(pid_queue *queue)
{
    if(queue->head == NULL)
        return NULL;

    pid_queue_elem *newHead, *result;
    newHead = queue->head->next;
    result  = queue->head;

    queue->head = newHead;
    return result;
}


//creates new pid_queue_elem with pid=pid
pid_queue_elem* createPIDQueueElem(pid_t pid)
{
    pid_queue_elem *elem = NULL;
    if((elem = (pid_queue_elem *) malloc(sizeof(pid_queue_elem))) == NULL)
        return NULL;
    elem->pid = pid;
    elem->next = NULL;

    return elem;
}

/* adds new pid_queue_elem to tail of queue */
pid_t addPIDToQueue(pid_t pid, pid_queue *queue)
{
    pid_queue_elem *elem = NULL;
    if((elem = createPIDQueueElem(pid)) == NULL)
        return -1;

    push(elem, queue);
    return pid;
}


// pops and returns element from head of queue then pushes it to tail
pid_queue_elem* cycleElem(pid_queue *queue){
    pid_queue_elem *elem = NULL;
    if((elem = pop(queue)) == NULL){
        printf("weird... pop was null in cycleElem\n");
        return NULL;
    }
    push(elem, queue);
    
    return elem;
}


void childDeadHandler(){
    // remove dead child from queue somehow...

    pid_t pid;

    while(1){

        pid = waitpid(-1, NULL, WNOHANG);   

        if(pid == 0)
            return;

        if(pid == -1)
        {
            printf("shit went wrong in wait...\n");
            return;
        }


        removeElem(pid, &queue);
    }

    return;
}

// runs on SIGALRM, stops running processes, runs next NPROCESSSORS processes
void scheduler(){

    signal(SIGALRM, SIG_IGN);

    if(queueIsEmpty(&queue)){
        printf("queue is empty\n");
        exitRoutine(EXIT_SUCCESS);
    }

    pid_queue_elem *elem = queue.head;
 

    while(elem != NULL){
        kill(elem->pid, SIGSTOP);
        elem = elem->next;
    }

    int i;
    pid_queue_elem *pid_elem = NULL;
    for(i = 0; i < nprocessors; i++){
        if((pid_elem = cycleElem(&queue)) == NULL)
            printf("cycleElem returned NULL on i = %d\n", i);
        kill(pid_elem->pid, SIGCONT);
    }

    signal(SIGALRM, scheduler);
}


int main(int argc, char* argv[])
{
    int i;

    int location    = -1;
    


    //setup gobal sigchld mask
    sig_chld_alrm_mask_setup();

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




    //pid_t pid[nprocesses];




    int status, s, sig;
    
    sigset_t set;

    sigemptyset(&set);
    sigaddset(&set, SIGUSR1);
    sigaddset(&set, SIGSTOP);
    sigaddset(&set, SIGCONT);

    //struct sigaction action;
    //action.sa_handler = SIG_DFL;
    

    s = sigprocmask(SIG_BLOCK, &set, NULL);

    if(s != 0)
        printf("error in sigprocmask creation!!\n");
 
    char* args[2];
    char file[p1strlen(command)+1];
    p1strcpy(file, command);
    args[0] = command;

    args[1] = NULL;
    
    pid_t pid_status;

    for(i = 0; i < nprocesses; i++){
        
        if((pid_status = fork())== 0){
            status = sigwait(&set, &sig); 

            signal(SIGCONT, SIG_DFL);


            if(status != 0)
                printf("error in sigwait!");

            if(status != 0)
                printf("sig_unblock didn't work :(\n");

            if(execvp(file, args) < 0){
                 exit(1);
            }
            
        }else{
            addPIDToQueue(pid_status, &queue);
        }
    }


    status = sigprocmask(SIG_UNBLOCK, &set, NULL);

    

    struct itimerval timer;
    timer.it_interval.tv_sec  = 0;
    timer.it_interval.tv_usec = 250000;
    timer.it_value.tv_sec     = 0;
    timer.it_value.tv_usec    = 250000;

    signal(SIGCHLD, childDeadHandler);
    signal(SIGALRM, scheduler);

    setitimer(ITIMER_REAL, &timer, NULL);
 
    clock_gettime(CLOCK_REALTIME, &start);
   

    while(1); // just for testing so I can see the timer handler called (SIGALRM)



    return 1;

}

