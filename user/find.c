#include "kernel/types.h"
#include "kernel/fcntl.h"
#include "kernel/stat.h"
#include "kernel/fs.h"
#include "user/user.h"

//返回path最后一个斜杠后的文件名
char* fmt_name(char *path){
    static char buf[DIRSIZ+1];
    char *p;
    // Find first character after last slash.
    for(p=path+strlen(path); p >= path && *p != '/'; p--);
    p++;
    memmove(buf, p, strlen(p)+1);
    return buf;
}

void find(char *path, char *findName){
	int fd;
	struct stat st;	
	if((fd = open(path, O_RDONLY)) < 0){
		fprintf(2, "find: cannot open %s\n", path); //打开文件失败
		return;
	}
	if(fstat(fd, &st) < 0){          //通过fstat看指向文件的类型
		fprintf(2, "find: cannot stat %s\n", path); //失败
		close(fd);
		return;
	}
	char buf[512], *p;	
	struct dirent de;
	switch(st.type){	//判断文件类型
		case T_FILE:    //普通文件
            if(strcmp(fmt_name(path), findName) == 0){//如果系统文件名与要查找的文件名一致，打印系统文件完整路径
	    	printf("%s\n", path);
	        }		
			break;
		case T_DIR:     //目录文件
			if(strlen(path) + 1 + DIRSIZ + 1 > sizeof buf){
				printf("find: path too long\n");
				break;
			}
			strcpy(buf, path);
			p = buf+strlen(buf);
			*p++ = '/';
			while(read(fd, &de, sizeof(de)) == sizeof(de)){//得到其子文件/目录名
				if(de.inum == 0 || de.inum == 1 || strcmp(de.name, ".")==0 || strcmp(de.name, "..")==0)
					continue;				
				memmove(p, de.name, strlen(de.name));
				p[strlen(de.name)] = 0;
				find(buf, findName);//递归搜索整个目录树
			}
			break;
	}
	close(fd);	
}

int main(int argc, char *argv[]){
	if(argc < 3){
		printf("find: find <path> <fileName>\n");
		exit();
	}
	find(argv[1], argv[2]);
	exit();
}
