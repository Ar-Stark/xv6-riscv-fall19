#include "kernel/types.h"
#include "user/user.h"

int main(int argc, char *argv[]){
	if(argc < 2){
		fprintf(2, "2 arguments needed!\n");
		exit();
	}
	int n = atoi(argv[1]);        //n是睡眠的时钟周期数
	printf("Sleep 10\n");
	sleep(n);
	exit();
}
