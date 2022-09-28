#include "../kernel/types.h"
#include "user.h"

#define MaxNum 35
void func(int,int[18][2]);


int main(int argc,char* argv[]){
    int pid;
    
    int p[18][2];
    int p_location=1;   //start from "1"lcoation, discard "0"

    if (pipe(&(p[1][0])) == -1 ){
        printf("create pipe error!%d\n",p_location);
        exit(0);
    }

    if ( (pid=fork() )==0 )
    {
        close(p[p_location][1]);
        func(p_location,p);
        exit(0);
    }else if (pid>0){
        close(p[p_location][0]);
        printf("prime 2\n");
        for (int i = 3; i <= 35; i++)
        {
            if (i%2!=0){
                write(p[p_location][1],&i,sizeof(int));
            }
        }
        close(p[p_location][1]);
        
    }else{
        printf("create child process error!\n");
        exit(0);
    }
    wait(&pid);
    exit(0);
}

void func(int p_location ,int p[18][2]){
    int pid;
    int first;
    if (read(p[p_location][0],&first,sizeof(int)) == 0)
    {
        exit(0);
    }
    printf("prime %d\n",first);

    if (pipe(&(p[p_location+1][0])) == -1 ){
        printf("create pipe error!%d\n",p_location);
        exit(0);
    }

    if ( (pid=fork())==0 ){
        //child process
        close(p[p_location+1][1]);
        func(p_location+1,p);
    }else if ( pid>0 ){
        //parent process
        //create next pipe
        
        // printf("%d\n",getpid());

        int term;
        close(p[p_location][1]);
        close(p[p_location+1][0]);
        
        
        while (read(p[p_location][0],&term,sizeof(int)))
        {
            if ( term%first!=0 )
            {
                write(p[p_location+1][1],&term,sizeof(int));
            }
            
        }
        close(p[p_location][0]);
        close(p[p_location+1][1]);
        wait(&pid);
        exit(0);
        
    }else{
        printf("create child process error!\n");
        exit(0);
    }
    
}
