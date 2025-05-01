// 内核上下文结构
struct context {
  uint64 ra;
  uint64 sp;

  // callee-saved
  uint64 s0;
  uint64 s1;
  uint64 s2;
  uint64 s3;
  uint64 s4;
  uint64 s5;
  uint64 s6;
  uint64 s7;
  uint64 s8;
  uint64 s9;
  uint64 s10;
  uint64 s11;
};

// Per-CPU state.
struct cpu {
  struct proc *proc;          // 当前运行进程
  struct context context;     // 调度器上下文
  int noff;                   // push_off嵌套深度
  int intena;                 // push_off前的中断使能状态
};

extern struct cpu cpus[NCPU];

// per-process data for the trap handling code in trampoline.S.
// sits in a page by itself just under the trampoline page in the
// user page table. not specially mapped in the kernel page table.
// the sscratch register points here.
// uservec in trampoline.S saves user registers in the trapframe,
// then initializes registers from the trapframe's
// kernel_sp, kernel_hartid, kernel_satp, and jumps to kernel_trap.
// usertrapret() and userret in trampoline.S set up
// the trapframe's kernel_*, restore user registers from the
// trapframe, switch to the user page table, and enter user space.
// the trapframe includes callee-saved user registers like s0-s11 because the
// return-to-user path via usertrapret() doesn't return through
// the entire kernel call stack.
struct trapframe {    // 陷阱帧结构
  /*   0 */ uint64 kernel_satp;   // 内核页表地址
  /*   8 */ uint64 kernel_sp;     // 进程内核栈顶地址
  /*  16 */ uint64 kernel_trap;   // 陷阱处理函数指针
  /*  24 */ uint64 epc;           // 用户程序计数器
  /*  32 */ uint64 kernel_hartid; // 内核线程
  /*  40 */ uint64 ra;
  /*  48 */ uint64 sp;
  /*  56 */ uint64 gp;
  /*  64 */ uint64 tp;
  /*  72 */ uint64 t0;
  /*  80 */ uint64 t1;
  /*  88 */ uint64 t2;
  /*  96 */ uint64 s0;
  /* 104 */ uint64 s1;
  /* 112 */ uint64 a0;
  /* 120 */ uint64 a1;
  /* 128 */ uint64 a2;
  /* 136 */ uint64 a3;
  /* 144 */ uint64 a4;
  /* 152 */ uint64 a5;
  /* 160 */ uint64 a6;
  /* 168 */ uint64 a7;
  /* 176 */ uint64 s2;
  /* 184 */ uint64 s3;
  /* 192 */ uint64 s4;
  /* 200 */ uint64 s5;
  /* 208 */ uint64 s6;
  /* 216 */ uint64 s7;
  /* 224 */ uint64 s8;
  /* 232 */ uint64 s9;
  /* 240 */ uint64 s10;
  /* 248 */ uint64 s11;
  /* 256 */ uint64 t3;
  /* 264 */ uint64 t4;
  /* 272 */ uint64 t5;
  /* 280 */ uint64 t6;
};

enum procstate { UNUSED, SLEEPING, RUNNABLE, RUNNING, ZOMBIE };

// Per-process state
struct proc {
  struct spinlock lock;     // 进程锁

  // p->lock must be held when using these:
  enum procstate state;        // 进程状态
  struct proc *parent;         // 父进程指针
  void *chan;                  // 等待通道
  int killed;                  // 是否终止
  int xstate;                  // 退出状态码
  int pid;                     // 进程ID

  // these are private to the process, so p->lock need not be held.
  uint64 kstack;               // 内核栈虚拟地址
  uint64 sz;                   // 用户内存大小
  pagetable_t pagetable;       // 用户页表
  struct trapframe *trapframe; // 用户陷阱帧指针
  struct context context;      // 内核上下文转换
  struct file *ofile[NOFILE];  // 打开文件表
  struct inode *cwd;           // 当前工作目录
  char name[16];               // 进程名

  uint64 trace_mask;    // 系统调用跟踪掩码
};
