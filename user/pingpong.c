#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int main(int argc, char* argv[]){
    int p[2],q[2];  // 创建两个管道：p用于父进程到子进程的通信，q用于子进程到父进程通信
    char *buf = {"a"};
    pipe(p);
    pipe(q);
    int pid = fork();
    if(pid > 0){    // 父进程
        close(p[0]);    // 关闭子进程端的读和写端口
        close(q[1]);
        write(p[1], buf, 2);    // 父进程向子进程发送字符
        read(q[0], buf, sizeof(buf));   // 从子进程那边读取传过来的字符
        printf("%d: received pong \n", getpid());
        exit(0);
    }else{  // 子进程
        close(p[1]);    // 关闭父进程端的读和写端口
        close(q[0]);
        printf("%d: received ping \n", getpid());
        write(q[1], buf, 2);    // 子进程向父进程发送字符
        exit(0);
    }
}