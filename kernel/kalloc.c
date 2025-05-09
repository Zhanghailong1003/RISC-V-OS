// Physical memory allocator, for user processes,
// kernel stacks, page-table pages,
// and pipe buffers. Allocates whole 4096-byte pages.

#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "riscv.h"
#include "defs.h"

void freerange(void *pa_start, void *pa_end);

extern char end[]; // first address after kernel.
                   // defined by kernel.ld.

struct run {
  struct run *next;
};

struct {
  struct spinlock lock;   // 自旋锁
  struct run *freelist;   // 指向空闲内存块链表的指针
} kmem[NCPU];             // 数组大小为NCPU

void
kinit()
{
  for (int i = 0; i < NCPU; i++)  // 遍历cpu核心，初始化对应的自旋锁
  {
    char name[9] = {0};
    snprintf(name, 8, "kmem-%d", i);  // 格式化锁的名称
    initlock(&kmem[i].lock, "kmem");  // 初始化每个cpu的锁
  }
  freerange(end, (void*)PHYSTOP); // 将物理内存从内核结束地址到物理内存上限的区间加入空闲链表
}

void
freerange(void *pa_start, void *pa_end)
{
  char *p;
  p = (char*)PGROUNDUP((uint64)pa_start);
  for(; p + PGSIZE <= (char*)pa_end; p += PGSIZE)
    kfree(p);
}

// Free the page of physical memory pointed at by v,
// which normally should have been returned by a
// call to kalloc().  (The exception is when
// initializing the allocator; see kinit above.)
void
kfree(void *pa)
{
  struct run *r;

  if(((uint64)pa % PGSIZE) != 0 || (char*)pa < end || (uint64)pa >= PHYSTOP)    // 未按页对齐、地址低于内核代码结束地址或超过物理内存上限
    panic("kfree");

  // Fill with junk to catch dangling refs.
  push_off(); // 关中断，防止当前cpu被打扰
  int cpu = cpuid();
  memset(pa, 1, PGSIZE);  // 将释放的内存填充1

  r = (struct run*)pa;  // 将物理地址转换为空闲链表节点

  acquire(&kmem[cpu].lock); // 获取cpu空闲链表锁
  r->next = kmem[cpu].freelist; // 将新释放的页插入链表头部
  kmem[cpu].freelist = r;   // 更新链表头指针
  release(&kmem[cpu].lock); // 释放锁
  pop_off();  // 开中断
}

void *
ksteal(int cpu){  // 从其他cpu偷了一个内存页
  struct run *r;
  for (int i = 1; i < NCPU; i++)  // 遍历其他CPU
  {
    int next_cpu = (cpu + i) % NCPU;  // 计算目标cpu的索引
    acquire(&kmem[next_cpu].lock);  // 获取锁
    r = kmem[next_cpu].freelist;
    if(r){
      kmem[next_cpu].freelist = r->next;  // 从邻居偷一块内存
    }
    release(&kmem[next_cpu].lock);  // 释放目标cpu的锁
    if(r){
      break;
    }
  }
  return r;   // 邻居没空页则返回NULL；
}

// Allocate one 4096-byte page of physical memory.
// Returns a pointer that the kernel can use.
// Returns 0 if the memory cannot be allocated.
void *
kalloc(void)
{
  struct run *r;
  push_off(); // 关中断
  int cpu = cpuid();

  acquire(&kmem[cpu].lock); // 获取当前cpu的锁
  r = kmem[cpu].freelist; // 从当前cpu的空闲链表取页
  if(r)
    kmem[cpu].freelist = r->next; // 更新链表头
  release(&kmem[cpu].lock);

  if(r == 0){
    r = ksteal(cpu);  // 若当前cpu无空闲页则尝试从其他cpu窃取
  }

  if(r)
    memset((char*)r, 5, PGSIZE); // 若成功取页则填充5
  pop_off();  // 开中断
  return (void*)r;
}
