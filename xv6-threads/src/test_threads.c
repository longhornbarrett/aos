#include "types.h"
#include "user.h"
int *p;

void threadfunc3(void *arg1, void *arg2) {
  *(int *)arg2 = (int)arg1;
  //lock_acquire(&thread_lock);
  printf(1, "inside thread %d\n", (*(int*)arg2));
  //lock_release(&thread_lock);
  exit();
}

lock_t mylock;
int global;

void threadfunc(void *arg1, void *arg2) {
  for (int i = 0; i < 100; i++) {
    lock_acquire(&mylock);
    for (int j = 0; j < 10; j++)
    {
      printf(1, "THREAD 1 %d\n", i);
      global++;
    }
    lock_release(&mylock);
  }
  exit();
}

void threadfunc2(void *arg1, void *arg2) {
  for (int i = 0; i < 100; i++) {
    lock_acquire(&mylock);
    for (int j = 0; j < 10; j++)
    {
      printf(1, "THREAD 2 %d\n", i);
      global--;     
    }
    lock_release(&mylock);
  }
  exit();
}

int main(void)
{
  // int i = 0;
  // lock_init(&thread_lock);
  // for(int j = 0; j < 100; j++)
  // {
  //   if(thread_create(threadfunc, (void*)(314159+j), (void *)&i) != 0)
  //     printf(2, "ERROR CREATING THREAD\n");

  // }
  // if(thread_join() != 0)
  //   printf(2, "ERROR JOINING THREAD\n");
  // printf(1, "MAIN EXITING\n");
  //
  //lock_acquire(&thread_lock);
  //lock_release(&thread_lock);

  lock_init(&mylock);
  thread_create(threadfunc, (void*)(1), (void *)0x456);
  thread_create(threadfunc2, (void*)(2), (void *)0xbbb);
  thread_join();
  thread_join();

  printf(1, "XV6_TEST_OUTPUT %d\n", global);

 	exit();
}
