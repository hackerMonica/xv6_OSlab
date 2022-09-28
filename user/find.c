#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fs.h"

void find(char*,char*);
char* fmtname(char *path);

int main(int argc,char* argv[]){
    if (argc!=3)
    {
        printf("find needs two arguments!\n");
        exit(0);
    }
    find( argv[1] , argv[2] );
    exit(0);
}

void find(char* path, char* file_name ){
    int fd;
    struct stat st;
    struct dirent de;
    char buf[512],*p;

    if((fd = open(path, 0)) < 0){
        fprintf(2, "find: cannot open %s\n", path);
        return;
    }

    if(fstat(fd, &st) < 0){
        fprintf(2, "find: cannot stat %s\n", path);
        close(fd);
        return;
    }
    // printf("test point 1\n");
    switch (st.type){
        case T_DEVICE:
        case T_FILE:
            // printf("test point 2\tfilename:%s,aim_name:%s\n",fmtname(path),file_name);
            if (strcmp(fmtname(path),file_name)==0)
            {
                printf("%s\n",path);
            }
            break;
    
        case T_DIR:
            // printf("test point 3\n");
            
            if(strlen(path) + 1 + DIRSIZ + 1 > sizeof buf){
                printf("ls: path too long\n");
                break;
            }
            strcpy(buf, path);
            p = buf+strlen(buf);
            *p++ = '/';
            // printf("test point 4\n");
            while(read(fd, &de, sizeof(de)) == sizeof(de)){
                // printf("test point 5\n");
                if(de.inum == 0)
                    continue;
                memmove(p, de.name, DIRSIZ);
                p[DIRSIZ] = 0;
                if(stat(buf, &st) < 0){
                    printf("ls: cannot stat %s\n", buf);
                    continue;
                }
                // printf("test point 6\n");
                //skip this directory(".") and up direcotry("..") 
                if ( st.type==T_DIR && (strcmp(de.name,".")==0
                || strcmp(de.name,"..")==0) ){
                    // printf("break point\n");
                    continue;
                }
                // printf("test point 7\n");
                find(buf,file_name);
            }
            break;
        default:
            printf("get file type error!\n");
            exit(0);
    }
    close(fd) ;
}

char*
fmtname(char *path)
{
  static char buf[DIRSIZ+1];
  char *p;

  // Find first character after last slash.
  for(p=path+strlen(path); p >= path && *p != '/'; p--)
    ;
  p++;

  // Return blank-padded name.
  if(strlen(p) >= DIRSIZ)
    return p;
  memmove(buf, p, strlen(p));
  memset(buf+strlen(p), 0, DIRSIZ-strlen(p));
  return buf;
}