struct file {
  enum { FD_NONE, FD_PIPE, FD_INODE, FD_DEVICE } type;    // 文件类型
  int ref;        //  引用次数（多进程共享）
  char readable;  //  文件是否可读
  char writable;  //  文件是否可写
  struct pipe *pipe; // 管道类型FD_PIPE
  struct inode *ip;  // 文件或设备对应的inodeFD_INODE and FD_DEVICE
  uint off;          // 文件读写偏移量FD_INODE
  short major;       // 设备主编号FD_DEVICE
};

#define major(dev)  ((dev) >> 16 & 0xFFFF)  // 从设备号提取主编号
#define minor(dev)  ((dev) & 0xFFFF)        // 从设备号提取次编号
#define	mkdev(m,n)  ((uint)((m)<<16| (n)))  // 组合主次编号伪设备号

// 内存中的inode副本
struct inode {
  uint dev;           // 设备号
  uint inum;          // inode在磁盘上的编号
  int ref;            // 引用计数
  struct sleeplock lock; // 睡眠锁
  int valid;          // 是否已从磁盘加载有效数据

  short type;         // 文件类型
  short major;
  short minor;
  short nlink;
  uint size;
  uint addrs[NDIRECT+2];
};

// 设备驱动函数表
struct devsw {
  int (*read)(int, uint64, int);    // 设备读函数
  int (*write)(int, uint64, int);   // 设备写函数
};

extern struct devsw devsw[];  // 全局设备驱动表

#define CONSOLE 1 // 控制台设备的主设备号
