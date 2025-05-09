// Buffer cache.
//
// The buffer cache is a linked list of buf structures holding
// cached copies of disk block contents.  Caching disk blocks
// in memory reduces the number of disk reads and also provides
// a synchronization point for disk blocks used by multiple processes.
//
// Interface:
// * To get a buffer for a particular disk block, call bread.
// * After changing buffer data, call bwrite to write it to disk.
// * When done with the buffer, call brelse.
// * Do not use the buffer after calling brelse.
// * Only one process at a time can use a buffer,
//     so do not keep them longer than necessary.


#include "types.h"
#include "param.h"
#include "spinlock.h"
#include "sleeplock.h"
#include "riscv.h"
#include "defs.h"
#include "fs.h"
#include "buf.h"

#define BUCKETSIZE 13     // 哈希桶的数量
#define BuFFERSIZE 5      // 每个桶的缓存数

extern uint ticks;      // 系统时钟计数器

struct {
  struct spinlock lock;         // 自旋锁
  struct buf buf[BuFFERSIZE];   // 当前桶内缓冲区数组
} bcachebucket[BUCKETSIZE];     // 哈希桶数组

int 
hash(uint blockno){
  return blockno % BUCKETSIZE;  // 确定块号所属的桶
}

void
binit(void)             // 初始化缓冲区缓存，构建双向链表
{
  for (int i = 0; i < BUCKETSIZE; i++)
  {
    initlock(&bcachebucket[i].lock, "bcachebucket");  // 为每个哈希桶的自旋锁初始化
    for (int j = 0; j < BuFFERSIZE; j++)
    {
      initsleeplock(&bcachebucket[i].buf[j].lock, "buffer");  // 为每个缓冲区的睡眠锁初始化
    }
  }
}

// Look through buffer cache for block on device dev.
// If not found, allocate a buffer.
// In either case, return locked buffer.
static struct buf*
bget(uint dev, uint blockno)    // 获取指定设备和块号的缓冲区，若未缓存则分配
{
  struct buf *b;
  int bucket = hash(blockno);   // 根据设磁盘块号计算hash所属桶
  acquire(&bcachebucket[bucket].lock);  // 上锁

  // Is the block already cached?
  // 遍历链表查找已缓存的块
  for (int i = 0; i < BuFFERSIZE; i++)  // 遍历桶内缓冲区
  {
    b = &bcachebucket[bucket].buf[i];
    if(b->dev == dev && b->blockno == blockno){ // 找到块
      b->refcnt++;  // 引用次数增加
      b->lastuse = ticks;   // 更新最后使用时间
      release(&bcachebucket[bucket].lock);  // 释放桶锁
      acquiresleep(&b->lock); // 获取睡眠锁
      return b;
    }
  }

  // Not cached.
  // Recycle the least recently used (LRU) unused buffer.
  // 未找到，寻找可替换的缓冲区（从最久未使用开始）
  uint least = 0xffffffff;  // 初始化时间戳
  int least_idx = -1;
  for (int i = 0; i < BuFFERSIZE; i++)  // 遍历桶内缓冲区
  {
    b = &bcachebucket[bucket].buf[i];
    if(b->refcnt == 0 && b->lastuse < least)  // 寻找未被引用且最久未使用的缓冲区
    {
      least = b->lastuse;
      least_idx = i;    // 记录候选缓冲区索引
    }
  }
  
  if(least_idx == -1){  // 没有缓冲区实际可以去其他区偷一块，未实现
    panic("bget: no buffers");  // 无可用缓冲区，触发panic
  }
  // 替换选中的缓冲区
  b = &bcachebucket[bucket].buf[least_idx]; // 更新设备号
  b->dev = dev; // 更新设备号
  b->blockno = blockno; // 更新块号
  b->lastuse = ticks; // 更新时间戳
  b->valid = 0; // 标记数据无效（需从磁盘加载），表示缓冲区的内容未与磁盘同步（新分配的缓冲区可能之前缓冲了其他块的数据）
  b->refcnt = 1;  // 标记引用一次
  release(&bcachebucket[bucket].lock);  // 释放桶锁
  acquiresleep(&b->lock); // 获取缓冲区睡眠锁
  return b;
}

// Return a locked buf with the contents of the indicated block.
struct buf*
bread(uint dev, uint blockno) // 读取块到缓冲区，必要时从磁盘加载
{
  struct buf *b;

  b = bget(dev, blockno); // 获取缓冲区
  if(!b->valid) {         // 数据无效则从磁盘获取，从磁盘读取数据到缓冲区
    virtio_disk_rw(b, 0); // 0表示读操作
    b->valid = 1;         // 标记数据有效
  }
  return b;
}

// Write b's contents to disk.  Must be locked.
void
bwrite(struct buf *b)     // 将缓冲区数据写入磁盘
{
  if(!holdingsleep(&b->lock))  // 检查是否持有缓冲区的睡眠锁
    panic("bwrite");
  virtio_disk_rw(b, 1);   // 执行磁盘写操作，将缓冲区数据写入磁盘
}

// Release a locked buffer.
// Move to the head of the most-recently-used list.
void
brelse(struct buf *b)   // 释放缓冲区，调整至MRU位置
{
  if(!holdingsleep(&b->lock)) // 确认持有锁
    panic("brelse");
  int bucket = hash(b->blockno);  // 计算桶
  acquire(&bcachebucket[bucket].lock);  // 获取哈希桶自旋锁
  b->refcnt--;  // 减少引用计数
  release(&bcachebucket[bucket].lock);  // 释放哈希桶锁
  releasesleep(&b->lock); // 释放缓冲区睡眠锁
}

void
bpin(struct buf *b) {   
  int bucket = hash(b->blockno);  // 计算缓冲区所属的哈希桶
  acquire(&bcachebucket[bucket].lock);
  b->refcnt++;            // 增加引用计数
  release(&bcachebucket[bucket].lock);
}

void
bunpin(struct buf *b) {
  int bucket = hash(b->blockno);
  acquire(&bcachebucket[bucket].lock);
  b->refcnt--;  // 减少引用计数
  release(&bcachebucket[bucket].lock);
}


