#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int main(int argc, char* argv[]){
    int p[2],q[2];
    char *buf = {"a"};
    pipe(p);
    pipe(q);
    int pid = fork();
    if(pid > 0){
        close(p[0]);
        close(q[1]);
        write(p[1], buf, 2);
        read(q[0], buf, sizeof(buf));
        printf("%d: received pong \n", getpid());
        exit(0);
    }else{
        close(p[1]);
        close(q[0]);
        printf("%d: received ping \n", getpid());
        write(q[1], buf, 2);
        exit(0);
    }
}