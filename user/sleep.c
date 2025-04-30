#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int main(int argc, char* argv[]){
    if(argc != 2){
        fprintf(2,"Not Input the correct params! \n");
        exit(1);
    }
    int a = atoi(argv[1]);
    sleep(a);
    exit(0);
}