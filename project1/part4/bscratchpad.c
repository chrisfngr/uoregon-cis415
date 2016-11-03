#include <stdio.h>
#include <signal.h>
#include <stdlib.h>


void handle(){
    signal(SIGUSR1, handle);
    printf("caught signal in %ld\n", getpid());
}

main(){

int pid, st;
sigset_t set;
sigemptyset(&set);
sigaddset(&set, SIGUSR1);
st = sigprocmask(SIG_BLOCK, &set, NULL);

if((pid = fork()) < 0){
perror("fork");
exit(1);
}

if(pid == 0)
{

for(;;){
int s, sig;
s = sigwait( (void *) &set, &sig);
if( s != 0)
    printf("shit went wrong\n");
printf("signal handling thread got signal %d\n", sig);

}

//signal(SIGUSR1, handle);
//raise(SIGUSR1);
//printf("worked!");
//for(;;);

}

else{
sleep(3);
printf("about to send signal to child..\n");
kill(pid, SIGUSR1);
}

return 1;

}
