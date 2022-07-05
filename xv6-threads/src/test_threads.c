#include "types.h"
#include "user.h"
int *p;

void threadfunc(void *arg1, void *arg2) {
  *(int *)arg2 = (int)arg1;
  exit();
}

int main(void)
{
    int i = 0;
    printf(1, "Test: Creating a thread %d\n", thread_create(threadfunc, (void*)314159, (void *)&i));
    printf(1, "Test: Joining thread %d\n", thread_join());
 	exit();
}
