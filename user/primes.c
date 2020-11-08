#include "kernel/types.h"
#include "user/user.h"


int main(){
    int arr[34];
	for(int i = 0; i <= 33; i++){
		arr[i] = i + 2;//数组中存入2~35
	}
	int num = 34;//num记录当前arr中数字个数
	int p[2];
	while (num > 0){
		pipe(p);//创建管道
		int prime = arr[0];//每次筛选后数组第一个数是素数
		printf("prime %d\n", prime);
		if(fork() == 0){
			int buf;//存放从管道读取的数
			int count = 0;
			close(p[1]);//关闭写端，否则读端无法判断传输的结束
			while(read(p[0], &buf, 4) != 0){
				if(buf % prime != 0){
					arr[count++] = buf;//若读取的数不是当前素数的倍数，则存入数组arr
				}
			}
			num = count;
			close(p[0]);
		}
		else{
			for(int i = 0; i < num; i++){
				write(p[1], &arr[i], 4);//将数组中的数写入p[1]
			}
			close(p[1]);
			wait();
			break;
		}
	}
	exit();
}
