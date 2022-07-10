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
    int r = thread_create(threadfunc, (void*)314159, (void *)&i);
    printf(1, "Test: Creating a thread ret code %d\n", r);
    int x = thread_join();
    printf(1, "Test: Joining thread ret code %d\n", x);
 	exit();
}
