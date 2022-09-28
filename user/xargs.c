#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fs.h"
#include "kernel/param.h"

void child_loop(char** , int );

int main(int argc,char* argv[]){
    int pid;
    char *argv_new[MAXARG];
    if (argc<3)
    {
        printf("xargs needs at least three arguments!\n");
        exit(0);
    }
    for (int i = 0; i < MAXARG; i++)
    {
        argv_new[i]=(char*)malloc(512);
    }
    
    
    for (int i = 1; i < argc; i++)
    {
        argv_new[i-1]=argv[i];
    }

    if ( (pid=fork() )==0 ){
        //child process
        child_loop(argv_new,argc);
        exit(0);
        
    }else if (pid<0){
        printf("create child process error!\n");
        exit(0);
    }
    wait(&pid);
    exit(0);
}

void child_loop(char** argv , int paraNum){
    char buf[512];
    int pid;

    char* term=gets(buf,512);
    // printf("test point1\n");
    while (strcmp(term,"\0")!=0)
    {
        pid=0;
        if ( strcmp(term,"\n")==0|| strcmp(term,"\r")==0 )
        {
            term=gets(buf,512);
            continue;
        }else if (term[strlen(term)-1]=='\n'||term[strlen(term)-1]=='\r')
        {
            term[strlen(term)-1]='\0';
        }
        if ( (pid=fork())==0 )
        {
            // printf("test point2\n");
            int wordLen=0,paraLoc=paraNum-1;
            char*p=term;
            //analyse parameters
            for (int i = 0; i < strlen(term); i++)
            {
                if (term[i]==' ')
                {
                    
                    if (wordLen==0)
                    {
                        p=term+i+1;
                        continue;
                    }
                    
                    memmove(argv[paraLoc++],p,wordLen);
                    wordLen=0;
                    p=term+i+1;
                }else{
                    wordLen++;
                }
                
            }
            memmove(argv[paraLoc++],p,wordLen);
            // printf("test point4\n");
            argv[paraLoc]=0;
            
            exec(argv[0],argv);
        }else if (pid<0){
            printf("create child process error!\n");
            exit(0);
        }else
        {
            wait(&pid);
        }
        term=gets(buf,512);
    }
    // printf("test point 5\n");
    exit(0);
}