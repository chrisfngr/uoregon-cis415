#include <stdio.h>
#include <sys/types.h>
#include <stdlib.h>
int main(int argc, char* argv[]){

pid_t pid = fork();

if(pid == 0){
    if(execvp(argv[1], &(argv[1])) < 0){
        fprintf(stderr, "ahh shit\n");
        exit(1);
    }
}
int status;
waitpid(pid, &status, 0);

return 0;
}
