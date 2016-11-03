#include "p1fxns.h"
#include <sys/time.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <unistd.h>
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

//args to send to execvp
char **args  = NULL;
int argsSize = 0;


//routine for exiting program
void exitRoutine(int status){
    clock_gettime(CLOCK_REALTIME, &end);
    
    long int tmp_nsecs, secs, msecs;
    
    tmp_nsecs = end.tv_nsec - start.tv_nsec;
    
    secs  = (tmp_nsecs > 0) ? (long int) (end.tv_sec - start.tv_sec) :  (long int) ((end.tv_sec - start.tv_sec) - 1);
    
    msecs = (tmp_nsecs > 0) ? (long int) ((tmp_nsecs)/1000000) : (long int) ((1000000000 + (tmp_nsecs))/1000000);


    char msecs_str[32];
    char secs_str[32];
    
    p1itoa(secs, secs_str);
    p1itoa(msecs, msecs_str);
    
    secs_str[7]  = '\0';
    msecs_str[3] = '\0';

    p1putstr(1, "The elapsed time to execute ");
    p1putint(1, nprocesses);
    p1putstr(1, " copies of \"");
    p1putstr(1, command);
    p1putstr(1, "\" on ");
    p1putint(1, nprocessors);
    p1putstr(1, " processors is ");
    p1putstr(1, secs_str);
    p1putstr(1, ".");
    if(msecs < 100){
        p1putstr(1, "0");
        if(msecs < 10){
		    p1putstr(1, "0");
		}
	}
    p1putstr(1, msecs_str);
    p1putstr(1, "sec\n");
           
    if(command != NULL)
        free(command);
    exit(status);
}


void parseCommands(){

    char buffer[256];
    int location = 0;
    int i = 0;
    
    while(command[location] != '\0'){
   	location = p1getword(command, location, buffer);
   	i++;
    }

    argsSize = i+1;
    
    args = (char**) malloc(sizeof(char*)*argsSize);
    location = 0;
    i = 0;
    while(command[location] != '\0'){
        location = p1getword(command, location, buffer);
        args[i++] = p1strdup(buffer);
    }
    args[i] = NULL;
}


// usage rules for program
void usage(){
    p1putstr(2, "USAGE: program [--number=<nprocesses>] [--processors=<nprocessors>] --command='command'\n");
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
        p1perror(2, "error in sigprocmask creation!!\n");

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
    }else if(queue->tail == NULL)
    {
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
        p1perror(2, "weird... pop was null in cycleElem\n");
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
            p1perror(2, "wait returned with error...\n");
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
        p1perror(2, "queue is empty\n");
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
            p1perror(2, "cycleElem returned NULL\n");
        kill(pid_elem->pid, SIGCONT);
    }

    signal(SIGALRM, scheduler);
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
        p1perror(2, "TH_NPROCESSES not given or in env vars!\n");
        exit(0);
    }
    if(getenv("TH_NPROCESSORS") == NULL && nprocessors == -1){
        p1perror(2, "TH_NPROCESSORS not given or in env vars!\n");
        exit(0);
    }
    parseCommands();   

    if(nprocesses == -1)
        nprocesses  = p1atoi(getenv("TH_NPROCESSES"));
    if(nprocessors == -1)
        nprocessors = p1atoi(getenv("TH_NPROCESSORS"));
}

int main(int argc, char* argv[])
{
    int i;

    //setup gobal sigchld mask
    sig_chld_alrm_mask_setup();
    

    parseArgs(argc, argv);

    int status, s, sig;
    
    sigset_t set;

    sigemptyset(&set);
    sigaddset(&set, SIGUSR1);
    sigaddset(&set, SIGSTOP);
    sigaddset(&set, SIGCONT);
    

    s = sigprocmask(SIG_BLOCK, &set, NULL);

    if(s != 0)
        p1perror(2, "error in sigprocmask creation!!\n");
 

    pid_t pid_status;

    for(i = 0; i < nprocesses; i++){
        
        if((pid_status = fork())== 0){
            status = sigwait(&set, &sig); 

            signal(SIGCONT, SIG_DFL);


            if(status != 0)
                p1perror(2, "error in sigwait!\n");

            if(status != 0)
                p1perror(2, "sig_unblock encountered error\n");

            if(execvp(args[0], args) < 0){
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
   

    while(1); // continues running until exitRutine is called



    return 1;

}

