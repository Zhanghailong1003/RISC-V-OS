// On-disk file system format.
// Both the kernel and user programs use this header file.


#define ROOTINO  1   // 根目录的inode编号
#define BSIZE 1024  // 磁盘块大小

// Disk layout:
// [ boot block | super block | log | inode blocks |
//                                          free bit map | data blocks]
//
// mkfs computes the super block and builds an initial file system. The
// 超级块结构:
struct superblock {
  uint magic;        // FSMAGIC 验证文件系统合法性
  uint size;         // 文件系统总块数
  uint nblocks;      // 数据块的总数
  uint ninodes;      // inode总数
  uint nlog;         // 日志块数量
  uint logstart;     // 日志区起始块号
  uint inodestart;   // inode区起始块号
  uint bmapstart;    // 空闲位图起始块号
};

#define FSMAGIC 0x10203040  // 固定值

#define NDIRECT 11    // 直接数据块数量
#define NINDIRECT (BSIZE / sizeof(uint))  // 间接块数量256
#define NININDIRECT (NINDIRECT * NINDIRECT)
#define MAXFILE (NDIRECT + NINDIRECT + NININDIRECT)   // 最大文件块数量 12 + 256 = 268

// On-disk inode structure
struct dinode {
  short type;           // 文件类型
  short major;          // 主设备号 (T_DEVICE only)
  short minor;          // 次设备号 (T_DEVICE only)
  short nlink;          // 硬链接计数（删除文件时归0）
  uint size;            // Size of file (bytes)
  uint addrs[NDIRECT+2];   // 数据块地址（12直接 + 1间接 + 1双重间接）
};

// 每块存放的inode数 1024/64 = 16
#define IPB           (BSIZE / sizeof(struct dinode))

// inode编号i所在的物理块号
#define IBLOCK(i, sb)     ((i) / IPB + sb.inodestart)

// 每块位图管理的数据块数 1024 * 8
#define BPB           (BSIZE*8)

// 数据块b的位图信息
#define BBLOCK(b, sb) ((b)/BPB + sb.bmapstart)

// 文件名的最大长度
#define DIRSIZ 14 

// 目录结构
struct dirent {
  ushort inum;        // 目录项对应的inode编号
  char name[DIRSIZ];  // 文件名
};

