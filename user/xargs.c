#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/param.h"

int
main(int argc, char *argv[])
{
    char * args[MAXARG];
    char buf[100];
    int flag=0;
    for(int i=1;i<argc;i++){
        args[flag++]=argv[i];//将xargs之后的参数存入args[]
    }
    args[flag]=0;
    char *p=buf;
    int len;
    int d=flag;
    while((len=read(0,buf,sizeof(buf)))!=0){
        for(int i=0;i<len;i++){
            if(buf[i]=='\n'){//读到换行符
                buf[i]=0;
                args[flag++]=p;//将参数送入args[]中
                args[flag]=0;//最后一个参数args[size-1]必须为0
                flag=d;//flag读取第二行参数前的位置
                p=buf;//p指回buf首地址
                if(fork()==0){
                    exec(argv[1],args);//argv[1]即为命令，args为二维参数数组
                }
                wait();
            }else if(buf[i]==' '){//读到空格
                buf[i]=0;
                args[flag++]=p;//将参数送入args[]中
                p=&buf[i+1];//p指向下一个参数
            }
        }
    }
    exit();
}