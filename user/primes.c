#include "kernel/types.h"
#include "user/user.h"

void get_prime(int p[2]){
    close(p[1]);
    int n;
    int tag = read(p[0], &n, 4);
    if(!tag){
        close(p[0]);
        exit(0);
    }
    printf("prime %d \n",n);
    int q[2];
    pipe(q);
    int pid = fork();
    if(!pid) get_prime(q);
    else if(pid > 0){
        int m;
        while(read(p[0], &m, 4)){
            if(m % n){
                write(q[1], &m, 4);
            }
        }
        close(p[0]);
        close(q[1]);
        close(q[0]);
        wait(0);
    }
    exit(0);
}

int main(int argc, char* argv[]){
   int p[2];
   pipe(p);
   for(int i = 2; i <= 35; i++){
        write(p[1], &i, 4);
   }
   get_prime(p);
   exit(0);
}