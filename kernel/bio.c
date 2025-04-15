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

#define BUCKETSIZE 13     // hash bucket数量
#define BuFFERSIZE 5      // 每个bucket的缓存数

extern uint ticks;

struct {
  struct spinlock lock;
  struct buf buf[BuFFERSIZE];
} bcachebucket[BUCKETSIZE];

int 
hash(uint blockno){
  return blockno % BUCKETSIZE;
}

void
binit(void)             // 初始化缓冲区缓存，构建双向链表
{
  for (int i = 0; i < BUCKETSIZE; i++)
  {
    initlock(&bcachebucket[i].lock, "bcachebucket");  // 自旋锁
    for (int j = 0; j < BuFFERSIZE; j++)
    {
      initsleeplock(&bcachebucket[i].buf[j].lock, "buffer");  // 睡眠锁
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
  int bucket = hash(blockno);   // 根据设磁盘块号计算hash
  acquire(&bcachebucket[bucket].lock);  // 上锁

  // Is the block already cached?
  // 遍历链表查找已缓存的块
  for (int i = 0; i < BuFFERSIZE; i++)
  {
    b = &bcachebucket[bucket].buf[i];
    if(b->dev == dev && b->blockno == blockno){ // 找到块
      b->refcnt++;  // 引用次数增加
      b->lastuse = ticks;
      release(&bcachebucket[bucket].lock);
      acquiresleep(&b->lock);
      return b;
    }
  }

  // Not cached.
  // Recycle the least recently used (LRU) unused buffer.
  // 未找到，寻找可替换的缓冲区（从最久未使用开始）
  uint least = 0xffffffff;  // 初始化时间戳
  int least_idx = -1;
  for (int i = 0; i < BuFFERSIZE; i++)
  {
    b = &bcachebucket[bucket].buf[i];
    if(b->refcnt == 0 && b->lastuse < least)  // 未被借用且时间戳最小的
    {
      least = b->lastuse;
      least_idx = i;
    }
  }
  
  if(least_idx == -1){  // 没有缓冲区实际可以去其他区偷一块
    panic("bget: no buffers");  // 无可用缓冲区，触发panic
  }
  b = &bcachebucket[bucket].buf[least_idx];
  b->dev = dev;
  b->blockno = blockno;
  b->lastuse = ticks;
  b->valid = 0;
  b->refcnt = 1;  // 标记借用一次
  release(&bcachebucket[bucket].lock);
  acquiresleep(&b->lock);
  return b;
}

// Return a locked buf with the contents of the indicated block.
struct buf*
bread(uint dev, uint blockno) // 读取块到缓冲区，必要时从磁盘加载
{
  struct buf *b;

  b = bget(dev, blockno); // 获取缓冲区
  if(!b->valid) {         // 数据无效则从磁盘获取
    virtio_disk_rw(b, 0); // 0表示读操作
    b->valid = 1;         // 标记数据有效
  }
  return b;
}

// Write b's contents to disk.  Must be locked.
void
bwrite(struct buf *b)     // 将缓冲区数据写入磁盘
{
  if(!holdingsleep(&b->lock))
    panic("bwrite");
  virtio_disk_rw(b, 1);
}

// Release a locked buffer.
// Move to the head of the most-recently-used list.
void
brelse(struct buf *b)   // 释放缓冲区，调整至MRU位置
{
  if(!holdingsleep(&b->lock)) // 确认持有锁
    panic("brelse");
  int bucket = hash(b->blockno);
  acquire(&bcachebucket[bucket].lock);
  b->refcnt--;
  release(&bcachebucket[bucket].lock);
  releasesleep(&b->lock);
}

void
bpin(struct buf *b) {
  int bucket = hash(b->blockno);
  acquire(&bcachebucket[bucket].lock);
  b->refcnt++;            // 增加引用计数
  release(&bcachebucket[bucket].lock);
}

void
bunpin(struct buf *b) {
  int bucket = hash(b->blockno);
  acquire(&bcachebucket[bucket].lock);
  b->refcnt--;
  release(&bcachebucket[bucket].lock);
}


