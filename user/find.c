#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fs.h"
// 功能：从给定的文件路径提取文件名，并将其格式化为固定长度
char*
fmtname(char *path)
{
  static char buf[DIRSIZ+1];  // 定义一个静态字符数组，用来存储最终格式化的文件名
  char *p;    // 字符指针，遍历和操作输入的路径字符串

  // 从路径字符串的末尾向前遍历
  for(p=path+strlen(path); p >= path && *p != '/'; p--)
    ;
  p++; // 指针移动到斜杠后第一个位置

  // 文件名的长度大于DIRSIZ，则直接返回原始文件名
  if(strlen(p) >= DIRSIZ)
    return p;
  memmove(buf, p, strlen(p)); // 使用memmove将文件名复制到静态数组buf中
  memset(buf+strlen(p), 0, DIRSIZ-strlen(p)); //对buf中文件名之后的位置填充空格，使整个名称的长度达到DIRSIZ
  return buf; // 返回格式化后的文件
}

int norecures(char *path){
    char* buf = fmtname(path);
    if(buf[0] == '.' && buf[1] == 0) {return 1;}
    if(buf[0] == '.' && buf[1] == '.'&& buf[2] == 0) {return 1;}
    return 0;
}

void find(char *path, char *target){
    char buf[512], *p; // 定义一个字符数组buf（用于存储完整的路径）和一个字符指针p
    int fd;   // 文件描述符fd
    struct dirent de;   // dirent结构体变量de，用于存储从目录中读取的条目信息
    struct stat st;   // 文件状态信息

    if((fd = open(path, 0)) < 0){ // 打开目录
        fprintf(2, "find: cannot open %s\n", path);
        return;
    }

    if(fstat(fd, &st) < 0){   // 检查目录状态
        fprintf(2, "find: cannot stat %s\n", path);
        close(fd);
        return;
    }


    if(strcmp(fmtname(path), target) == 0){
        printf("%s \n",path);
    }

    switch(st.type){    // 根据文件类型进行不同的处理
      case T_FILE:        // 如果是普通文件
        break;

      case T_DIR:         // 如果是目录，则进一步处理
        if(strlen(path) + 1 + DIRSIZ + 1 > sizeof buf){   // 检查完整路径长度是否超出buf的容量，若超出则打印错误信息并跳出switch语句
          printf("ls: path too long\n");
          break;
        }
        strcpy(buf, path);    // 将原始路径复制到buf中
        p = buf+strlen(buf);  // 设置指针p指向buf中路径字符串的末尾
        *p++ = '/';   // 在路径末尾添加斜杠'/'，并将指针p移动到下一个位置
        while(read(fd, &de, sizeof(de)) == sizeof(de)){   // 循环读取目录内容，每次读取一个dirent结构体
          if(de.inum == 0)  // 如果inode编号为0，表示无效条目，跳过
            continue;
          memmove(p, de.name, DIRSIZ); // 将当前目录项的名字复制到buf的末尾
          p[DIRSIZ] = 0;  // 确保名字以null终止
          if(stat(buf, &st) < 0){ // 获取新构造路径的状态信息，如果失败，打印错误信息并跳过该条目
            printf("find: cannot stat %s\n", buf);
            continue;
          }
          
          if(norecures(buf) == 0){
            find(buf, target);
          }
        }
        break; 
      }
    close(fd); // 关闭文件描述符
} 

int main(int argc, char* argv[]){
  if(argc == 1){
    printf("error \n");
    exit(0);
  }
  if(argc == 2){
    find(".", argv[1]);
    exit(0);
  }
  if(argc == 3){
    find(argv[1], argv[2]);
    exit(0);
  }
  exit(0);
}