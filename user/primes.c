#include "kernel/types.h"
#include "user/user.h"

void get_prime(int p[2]){
    close(p[1]);    // 关闭管道写端口
    int n;
    int tag = read(p[0], &n, 4);    // 从管道读端读取第一个数
    if(!tag){   // 若没有数据，关闭读端退出
        close(p[0]);
        exit(0);
    }
    printf("prime %d \n",n);    // 打印第一个数
    int q[2];
    pipe(q);    // 创建新管道
    int pid = fork();   // 创建子进程
    if(!pid) get_prime(q);  // 子进程调用处理新管道
    else if(pid > 0){
        int m;
        while(read(p[0], &m, 4)){   // 从管道中读取数
            if(m % n){  // 若m不能被当前素数n整除
                write(q[1], &m, 4); // 将m写入管道
            }
        }
        close(p[0]);    // 关闭原读端
        close(q[1]);    // 关闭新管道读写端
        close(q[0]);
        wait(0);
    }
    exit(0);
}

int main(int argc, char* argv[]){
   int p[2];    // 创建管道
   pipe(p);
   for(int i = 2; i <= 35; i++){    // 写入2~35在管道里
        write(p[1], &i, 4);
   }
   get_prime(p);
   exit(0);
}