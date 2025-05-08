#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <assert.h>
#include <pthread.h>

static int nthread = 1;   // 线程数
static int round = 0;     // 当前轮次

struct barrier {
  pthread_mutex_t barrier_mutex;  // 互斥锁
  pthread_cond_t barrier_cond;  // 条件变量
  int nthread;      // 已到达屏障的线程数
  int round;     // 当前屏障轮次
} bstate;

static void
barrier_init(void)
{
  assert(pthread_mutex_init(&bstate.barrier_mutex, NULL) == 0); // 初始化互斥锁
  assert(pthread_cond_init(&bstate.barrier_cond, NULL) == 0); // 初始化条件变量
  bstate.nthread = 0;   // 初始时无线程到达
}

static void 
barrier()
{
  pthread_mutex_lock(&bstate.barrier_mutex);  // 进入临界区
  int n = ++bstate.nthread; // 当前到达进程数+1
  if(n == nthread){
    ++bstate.round;   // 全局轮次+1
    bstate.nthread = 0; // 重置到达的线程计数器
    pthread_cond_broadcast(&bstate.barrier_cond); // 都到达了，广播唤醒所有等待进程
  }else{
    pthread_cond_wait(&bstate.barrier_cond, &bstate.barrier_mutex); // 等待其他线程
  }
  pthread_mutex_unlock(&bstate.barrier_mutex);  // 退出临界区
}

static void *
thread(void *xa)
{
  long n = (long) xa;
  long delay;
  int i;

  for (i = 0; i < 20000; i++) {
    int t = bstate.round; // 读取当前屏障轮次
    assert (i == t);      // 确保线程执行轮次与屏障同步
    barrier();            // 等待所有线程到达
    usleep(random() % 100); // 随机延迟模拟工作负载
  }

  return 0;
}

int
main(int argc, char *argv[])
{
  pthread_t *tha;
  void *value;
  long i;
  double t1, t0;

  if (argc < 2) {
    fprintf(stderr, "%s: %s nthread\n", argv[0], argv[0]);
    exit(-1);
  }
  nthread = atoi(argv[1]);
  tha = malloc(sizeof(pthread_t) * nthread);
  srandom(0);

  barrier_init();

  for(i = 0; i < nthread; i++) {
    assert(pthread_create(&tha[i], NULL, thread, (void *) i) == 0);
  }
  for(i = 0; i < nthread; i++) {
    assert(pthread_join(tha[i], &value) == 0);
  }
  printf("OK; passed\n");
}
