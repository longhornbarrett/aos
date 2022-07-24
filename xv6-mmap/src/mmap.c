#include "types.h"
#include "x86.h"
#include "defs.h"
#include "date.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "proc.h"

static void del_mmapped_node(mmapped_region *curr, mmapped_region *prev)
{
  if(!curr)
    return;
  // If you are removing the current mapped header replace it with next node
  if (curr == myproc()->mmapped_header)
    myproc()->mmapped_header = myproc()->mmapped_header->next;
  else
    prev->next = curr->next;
  kmfree(curr);
}


void mmap_free()
{
  mmapped_region* curr = myproc()->mmapped_header;
  mmapped_region* next;

  while (curr){
    next = curr->next;
    del_mmapped_node(curr, 0);
    curr = next;
  }
}

void *mmap(void *addr, uint length, int prot, int flags, int fd, int offset)
{
  if ((uint)addr >= KERNBASE || length <= 0)
    return (void*)-1;

  struct proc *p = myproc();

  uint orig_sz = p->sz;
  p->sz = allocuvm(p->pgdir, orig_sz,  orig_sz + length);
  if (p->sz == 0)
    return (void*)-1;
  switchuvm(p);

  // Allocate using kmalloc to not waste memory
  mmapped_region* region = (mmapped_region*)kmalloc(sizeof(mmapped_region*));
  if (region == (mmapped_region*)0){
    // If the region was not able to be kmalloc'ed then return process back to orig size
    deallocuvm(p->pgdir, p->sz, orig_sz);
    return (void*)-1;
  }

  region->addr = (void*)PGROUNDDOWN(orig_sz);
  region->length = length;
  region->offset = offset;
  region->next = 0;

  addr = region->addr;
  if (!p->nmapped_regions){
    p->mmapped_header = region;
  }else {
    mmapped_region* curr = p->mmapped_header;
    while (curr->next){
      if ((uint)addr >= KERNBASE){
        kmfree(region);
        deallocuvm(p->pgdir, p->sz, orig_sz);
        return (void*)-1;
      }
      if (addr == curr->addr){
        addr += PGROUNDDOWN(PGSIZE+length);
        curr = p->mmapped_header;
      }else 
        curr = curr->next;
    }
    if (addr == curr->addr){
      addr += PGROUNDDOWN(PGSIZE+length);
    }
    curr->next = region;
  }
  p->nmapped_regions++;
  region->addr = addr;
  memset(region->addr, 0, region->length);
  return region->addr;
}

int munmap(void *addr, uint length)
{
  struct proc *p = myproc();
  if (addr >= (void*)KERNBASE || length <= 0 || p->nmapped_regions == 0)
    return -1;

  // Travese our mmap dll to see if address and length are valid
  mmapped_region *curr = p->mmapped_header;
  mmapped_region *next = p->mmapped_header->next;
  int size = 0;

  // Check the head
  if (p->mmapped_header->addr == addr && p->mmapped_header->length)
  {
    /*deallocate the memory from the current process*/
    p->sz = deallocuvm(p->pgdir, p->sz, p->sz - length);
    switchuvm(p);
    p->nmapped_regions--;  

    if(p->mmapped_header->next != 0)
    {
      /* Calls to kmfree were changing the node->next's length value
       * in the linked-list. This is a hacky fix, but I don't know
       * what is really causing that problem... */
      size = p->mmapped_header->next->length;
      del_mmapped_node(p->mmapped_header, 0);
      p->mmapped_header->length = size;
    }
    else
    {
      del_mmapped_node(p->mmapped_header, 0);
    }

    /*return success*/
    return 0;
  }

  while(next != 0)
  {
    if (next->addr == addr && next->length == length)
    {
      /*deallocate the memory from the current process*/
      p->sz = deallocuvm(p->pgdir, p->sz, p->sz - length);
      switchuvm(p);
      p->nmapped_regions--;  
      
      /*remove the node from our ll*/
      size = next->next->length;
      del_mmapped_node(next, curr);
      curr->next->length = size;
      
      /*return success*/
      return 0;
    }
    curr = next;
    next = curr->next;
  }

  // if there was no match, return -1
  return -1;
}

// Print all the memory mappings
void print_maps() {
  struct proc *p = myproc();
  cprintf("Total regions: %d\n", p->nmapped_regions);
  mmapped_region* curr = p->mmapped_header;
  while (curr) {
    cprintf("Addr: %p\tLength: %d\tProt: %d\n",
            curr->addr, curr->length,
            curr->prot);
    curr = curr->next;
  }
}