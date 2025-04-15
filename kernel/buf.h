struct buf {
  int valid;   // 标记缓冲区数据是否有效
  int disk;    // 磁盘是否拥有该缓冲区
  uint dev;    // 标识缓冲区所属的磁盘设备号
  uint blockno; // 标识缓冲区对应的磁盘块号
  struct sleeplock lock;  // 保护缓冲区的睡眠锁
  uint refcnt;    // 引用计数，记录当前使用该缓冲区的进程数
  struct buf *prev; // LRU cache list
  struct buf *next;
  uchar data[BSIZE];  // 存储磁盘块数据的字节数组
  uint lastuse; // 当前使用时间
};

