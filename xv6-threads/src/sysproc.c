#include "types.h"
#include "x86.h"
#include "defs.h"
#include "date.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "proc.h"
#include "spinlock.h"

struct spinlock memlock;

int
sys_fork(void)
{
  return fork();
}

int
sys_exit(void)
{
  exit();
  return 0;  // not reached
}

int
sys_wait(void)
{
  return wait();
}

int
sys_kill(void)
{
  int pid;

  if(argint(0, &pid) < 0)
    return -1;
  return kill(pid);
}

int
sys_getpid(void)
{
  return myproc()->pid;
}

int sys_clone(void)
{
  int fcn;
  int arg1;
  int arg2;
  int stack;
  if(argint(0, &fcn) <0)
    return -1;
  if(argint(1, &arg1) <0)
    return -1;
  if(argint(2, &arg2) <0)
    return -1;
  if(argint(3, &stack) <0)
    return -1;
  return clone((void*)fcn, (void*)arg1, (void*)arg2, (void*)stack);
}

int sys_join(void)
{
  int stack;
  if(argint(0, &stack) <0)
    return -1;
  return join((void**)stack);
}


int sys_mprotect(void)
{  
  int addr,len;

  if(argint(0, &addr) <0)
    return -1;
  if(argint(1,&len) <0)
    return -1;

  return memoryprotect((void*)addr,len, PROT_READ, myproc());
}

int sys_munprotect(void)
{
  int addr,len;

  if(argint(0, &addr) <0)
    return -1;
  if(argint(1,&len) <0)
    return -1;

  return memoryprotect((void*)addr,len, PROT_WRITE, myproc());
}

int
sys_sbrk(void)
{
  int addr;
  int n;

  if(argint(0, &n) < 0)
    return -1;
  acquire(&memlock);
  addr = myproc()->sz;
  int grow_rc = growproc(n);
  release(&memlock);
  if(grow_rc < 0)
    return -1;
  return addr;
}

int
sys_sleep(void)
{
  int n;
  uint ticks0;

  if(argint(0, &n) < 0)
    return -1;
  acquire(&tickslock);
  ticks0 = ticks;
  while(ticks - ticks0 < n){
    if(myproc()->killed){
      release(&tickslock);
      return -1;
    }
    sleep(&ticks, &tickslock);
  }
  release(&tickslock);
  return 0;
}

// return how many clock tick interrupts have occurred
// since start.
int
sys_uptime(void)
{
  uint xticks;

  acquire(&tickslock);
  xticks = ticks;
  release(&tickslock);
  return xticks;
}
