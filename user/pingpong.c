#include <kernel/types.h>
#include <user/user.h>

int main(){
    int parent_fd[2],child_fd[2];
    char buf[10];//传输字符串的数组
    pipe(parent_fd);//创建管道：写端父进程，读端子进程
    pipe(child_fd);//创建管道：写端子进程，读端父进程
	//子进程
    if(fork() == 0){
		if(read(parent_fd[0], buf, 4) == 4){          //子进程则通过从parent_fd [0]读取来接收字节
			printf("%d: received ping\n", getpid());
		}
		write(child_fd[1], "pong", 4);//子进程从父进程收到后，通过将“pong”写入child_fd [1]来回复
    }
	//父进程
	else{
		write(parent_fd[1], "ping", 4);//父进程通过将”ping”写入parent_fd [1]来发送子进程
		if(read(child_fd[0], buf, 4) == 4){           //父进程从child_fd[0]读取
			printf("%d: received pong\n", getpid());
		}
	}
	exit();
}
