#include "types.h"
#include "stat.h"
#include "fcntl.h"
#include "mmu.h"
#include "user.h"
#include "x86.h"

int thread_create(void (*start_routine)(void *, void *), void *arg1, void *arg2)
{
  const uint HEADER_SZ = sizeof(uint);
  uint mem = (uint)malloc(PGSIZE*2 + HEADER_SZ);
  if(mem == 0)
    return -1;
  uint stack_start = mem + HEADER_SZ;
  uint stack = PGROUNDUP(stack_start);
  uint *header = (uint *) (stack - HEADER_SZ);
  *header = mem;
  int rc = clone(start_routine, arg1, arg2, (void*)stack);
  if (rc == -1)
    free((void*)mem);
  return rc;
}

int thread_join()
{
  void* stack;
  int rc = join(&stack);
  if(rc < 0)
    return rc;
  uint *header = ((uint *) stack) - 1;
  void *malloc_block_start = (void *) *header;
  free(malloc_block_start);
  return rc;
}

static inline int fetch_and_add(int* variable, int value)
{
    __asm__ volatile("lock; xaddl %0, %1"
      : "+r" (value), "+m" (*variable) // input + output
      : // No input-only
      : "memory"
    );
    return value;
}

void lock_acquire(lock_t* lock)
{
  int myturn = fetch_and_add(&lock->ticket, 1);
  while(lock->turn != myturn)
  {
    sleep(1);
  }
}

void lock_release(lock_t* lock)
{
  lock->turn = lock->turn + 1;
}

void lock_init(lock_t* lock)
{
  lock->ticket = 0;
  lock->turn = 0;
}

char*
strcpy(char *s, const char *t)
{
  char *os;

  os = s;
  while((*s++ = *t++) != 0)
    ;
  return os;
}

int
strcmp(const char *p, const char *q)
{
  while(*p && *p == *q)
    p++, q++;
  return (uchar)*p - (uchar)*q;
}

uint
strlen(const char *s)
{
  int n;

  for(n = 0; s[n]; n++)
    ;
  return n;
}

void*
memset(void *dst, int c, uint n)
{
  stosb(dst, c, n);
  return dst;
}

char*
strchr(const char *s, char c)
{
  for(; *s; s++)
    if(*s == c)
      return (char*)s;
  return 0;
}

char*
gets(char *buf, int max)
{
  int i, cc;
  char c;

  for(i=0; i+1 < max; ){
    cc = read(0, &c, 1);
    if(cc < 1)
      break;
    buf[i++] = c;
    if(c == '\n' || c == '\r')
      break;
  }
  buf[i] = '\0';
  return buf;
}

int
stat(const char *n, struct stat *st)
{
  int fd;
  int r;

  fd = open(n, O_RDONLY);
  if(fd < 0)
    return -1;
  r = fstat(fd, st);
  close(fd);
  return r;
}

int
atoi(const char *s)
{
  int n;

  n = 0;
  while('0' <= *s && *s <= '9')
    n = n*10 + *s++ - '0';
  return n;
}

void*
memmove(void *vdst, const void *vsrc, int n)
{
  char *dst;
  const char *src;

  dst = vdst;
  src = vsrc;
  while(n-- > 0)
    *dst++ = *src++;
  return vdst;
}
