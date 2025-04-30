#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/param.h"

#define MEGSIZE 16  // 缓存区大小

int
main(int argc, char *argv[])
{
    sleep(10);
    char buf[MEGSIZE];
    read(0, buf, MEGSIZE);  // 从标准输入读入
    
    char* xargv[MAXARG];
    int xargc = 0;
    for (int i = 1; i < argc; i++)  // 读取第2个及其之后的参数
    {
        xargv[xargc] = argv[i];
        xargc++;
    }
    char* p = buf;  // p指向缓冲区的地址
    for (int i = 0; i < MEGSIZE; i++)
    {
        if(buf[i] == '\n'){ // 检查到换行符
            int pid = fork();
            if(pid > 0){    // 父进程分支移动到下一行的起始位置
                p = &buf[i + 1];
                wait(0);
            }else{
                buf[i] = 0; // 将换行符替换为字符串终止符
                xargv[xargc] = p;   // 将缓冲区添加到参数列末尾
                xargc++;
                xargv[xargc] = 0;   // 添加0表示参数结束
                xargc++;
                exec(xargv[0], xargv);  // 执行目标程序
                exit(0);
            }
        }
    }
    exit(0);
}