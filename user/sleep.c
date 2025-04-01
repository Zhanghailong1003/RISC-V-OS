#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int main(int argc, char* argv[]){
    if(argc != 2){
        fprintf(2,"Not input correct params! \n");
        exit(1);
    }
    int a = atoi(argv[1]);
    int ret = sleep(a);
    exit(ret);
}