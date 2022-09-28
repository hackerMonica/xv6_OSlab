#include "kernel/types.h"
#include "user.h"

int main(int argc,char* argv[]){

    // for (int i = 0; i < argc; i++)
    // {
    //     printf("argv[%d]:\t%s\n",i,argv[i]);
    // }
    // exit(0);

    int pid;
    char buffer[6];
    int p1[2];
    int p2[2];
    // printf("test1\n");
    if (pipe(p1) == -1 || pipe(p2) == -1 ){
        printf("create pipe error!\n");
        exit(0);
    }

    if ((pid=fork())<0 ){
        printf("create child error!\n");
        exit(0);
    }else if (pid==0){
        //child process
        // printf("this is child process\n");
        close(p1[1]);   //close write port
        close(p2[0]);   //close read port
         
        read(p1[0],buffer,5);
        printf("%d: received %s\n",getpid(),buffer);
        close(p1[0]);

        
        write(p2[1],"pong",5);
        close(p2[1]);

        exit(0);
    }else{
        //parent process
        // printf("this is parent process\n");
        close(p1[0]);   //close read port
        close(p2[1]);   //close write port
        write(p1[1],"ping",5);
        close(p1[1]);

        read(p2[0],buffer,5);
        printf("%d: received %s\n",getpid(),buffer);
        close(p2[0]);
    }
    wait(&pid);
    exit(0);
}