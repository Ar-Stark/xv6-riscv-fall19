#include "kernel/types.h"
#include "user/user.h"
#include "kernel/fcntl.h"
 
#define MAX 50
char whitespace[] = " \t\r\n\v";

//仿照sh.c  
int 
getcmd(char *buf, int nbuf)
{
    fprintf(2, "@ ");
    memset(buf, 0, nbuf);
    gets(buf, nbuf);
    if (buf[0] == 0) // EOF
        return -1;
    return 0;
}

//解析参数，存入argv数组中
void parse_argv(char *buf, char* argv[],int* argc)
{
    int i = 0, j = 0; 
    while(buf[i] != '\n' && buf[i] != '\0')
    {
        argv[j]=buf+i;//找到参数，让argv[j]指向它
        while (strchr(whitespace,buf[i])==0){
            i++;
        } 
        buf[i]='\0';//找到参数后的空格，赋值为\0
        i++;
        j++;
    }
    argv[j]=0;
    *argc=j;
}

//运行命令
void runcmd(char*argv[],int argc)
{
    for(int i=1;i<argc;i++){
        if(!strcmp(argv[i],"|")){//判断命令中存在管道符的情况
                int i=0;
                for(;i<argc;i++){
                    if(!strcmp(argv[i],"|")){
                    argv[i]=0;
                    break;
                    }
                }
                int p[2];
                pipe(p);
                if(fork()==0){
                close(1);//关闭标准输出
                dup(p[1]);
                close(p[0]);
                close(p[1]);
                runcmd(argv,i);//执行管道符左边的命令
                }else{
                close(0);//关闭标准输入
                dup(p[0]);
                close(p[0]);
                close(p[1]);
                runcmd(argv+i+1,argc-i-1);//执行管道符右边的命令
                }
            }
    }
    for(int i=1;i<argc;i++){//判断命令中的输入输出重定向
        if(!strcmp(argv[i],">")){//命令中包含'>',输出重定向
            close(1);//由于是输出重定向，关闭标准输出
            open(argv[i+1],O_CREATE|O_WRONLY);
            argv[i]=0;
        }
        if(!strcmp(argv[i],"<")){//命令中包含'<',输入重定向
            close(0);////由于是输入重定向，关闭标准输入
            open(argv[i+1],O_RDONLY);
            argv[i]=0;
        }
    }
    exec(argv[0], argv);
}
 
int main()
{
    char buf[MAX];
    while (getcmd(buf, sizeof(buf)) >= 0)
    {
        if (fork() == 0)
        {
            char* argv[MAX];//保存参数
            int argc=-1;//argc记录参数个数
            parse_argv(buf, argv,&argc);
            runcmd(argv,argc);
        }
        wait(0);
    }
    exit(0);
}