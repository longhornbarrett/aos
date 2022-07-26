#include "types.h"
#include "x86.h"
#include "defs.h"
#include "date.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "proc.h"

#define NULL (mmapped_region*)0

mmapped_region* create_region(uint pgaligned_addr, int length, int offset)
{
  // Allocate using kmalloc to not waste memory
    mmapped_region* region = (mmapped_region*)kmalloc(sizeof(mmapped_region));
    if (region == NULL){
      return (void*)-1;
    }

    region->addr = (void*)pgaligned_addr;
    region->length = length;
    region->offset = offset;
    region->region_type = ANONYMOUS;
    region->next = 0;
    return region;
}

mmapped_region* insert_into_list(mmapped_region* header, mmapped_region* region, int expand)
{
  //This function inserts into a linked list at the sorted location
  if(!header)
    header = region;
  else{
    mmapped_region* curr = header;
    mmapped_region* prev = NULL;
    while(curr)
    {
      if(curr->addr > region->addr)
      {
        //found location
        if(!prev)
        {
          header = region;
          region->next = curr;
        }
        else
        {
          if(expand && (uint)curr->addr == (uint)region->addr + region->length)
          {
            region->length += curr->length;
            region->next = curr->next;
            prev->next = curr->next;
            kmfree(curr);
          }
          if(expand && (uint)prev->addr + prev->length == (uint)region->addr)
          {
            prev->length += region->length;
            kmfree(region);
          }else
          {
            prev->next = region;
            region->next = curr;
          }
        }
        curr = NULL;
      }else if(!curr->next)
      {
        if(expand && (uint)curr->addr + curr->length == (uint)region->addr)
        {
            curr->length += region->length;
            kmfree(region);
        }else
          curr->next = region;
        curr = NULL;
      }else{
        prev = curr;
        curr = curr->next;
      }
    }
  }
  return header;
}

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

  curr = myproc()->unmapped_header;
  while (curr){
    next = curr->next;
    del_mmapped_node(curr, 0);
    curr = next;
  }
}
// Print all the memory mappings
void print_maps() {
  struct proc *p = myproc();
  cprintf("Total regions: %d\n", p->nmapped_regions);
  mmapped_region* curr = p->mmapped_header;
  cprintf("==============Mapped regions==================\n");
  while (curr) {
    cprintf("Addr: %p\tLength: %d\tProt: %d\n",
            curr->addr, curr->length,
            curr->prot);
    curr = curr->next;
  }
  cprintf("==================================================\n");
}

// Print all the memory mappings
void print_umaps() {
  struct proc *p = myproc();
  cprintf("Total regions: %d\n", p->nmapped_regions);
  mmapped_region* curr = p->unmapped_header;
  cprintf("==============Unmapped regions==================\n");
  while (curr) {
    cprintf("Addr: %p\tLength: %d\tProt: %d\n",
            curr->addr, curr->length,
            curr->prot);
    curr = curr->next;
  }
  cprintf("==================================================\n");
}

mmapped_region* create_and_expand(uint length, int offset)
{
  struct proc *p = myproc();
  uint orig_sz = p->sz;
  uint pgaligned_addr = PGROUNDUP(orig_sz);
  p->sz = allocuvm(p->pgdir, orig_sz,  pgaligned_addr + length);
  if (p->sz == 0)
    return (void*)-1;
  switchuvm(p);
  return create_region(pgaligned_addr, length, offset);
}

void *mmap(void *addr, uint length, int prot, int flags, int fd, int offset)
{
  if ((uint)addr >= KERNBASE || length <= 0)
    return (void*)-1;
  struct proc *p = myproc();

  if (!p->mmapped_header && !p->unmapped_header){
    p->mmapped_header = create_and_expand(length, offset);
    addr = p->mmapped_header->addr;
  }else {
    void* middleAddr = (void*)PGROUNDUP((uint)addr) - PGSIZE/2;
    if(addr < middleAddr)
      addr = (void*)PGROUNDDOWN((uint)addr);
    else
      addr = (void*)PGROUNDUP((uint)addr);
    mmapped_region* curr = p->unmapped_header;
    mmapped_region* prev = NULL;
    int found_region = 0;
    while (curr && !found_region){
      if((addr == curr->addr || (uint)addr  == 0) && curr->length > length)
      {
        curr->length = length;
        curr->offset = offset;
        curr->region_type = ANONYMOUS;
        // Found a region that can be broken up
        if(curr->length > PGROUNDUP(length))
        {
          void* new_addr = (void*)PGROUNDUP((uint)curr->addr + length);
          uint old_boundary = PGROUNDUP((uint)curr->addr + curr->length);
          mmapped_region* new_subregion = create_region((uint)new_addr, (old_boundary - (uint)new_addr), offset);
          if(!prev)
            p->unmapped_header = new_subregion;
          else{
            prev->next = new_subregion;
            new_subregion->next = curr->next;
          }
        }else if(!prev)
          p->unmapped_header = prev;
        else{
          prev->next = curr->next;
        }
        p->mmapped_header = insert_into_list(p->mmapped_header, curr, 0);
        addr = curr->addr;
        found_region = 1;
      }else if(addr > curr->addr && (uint)addr < (uint)curr->addr + curr->length)
      {
        //Found inside a segment
        uint new_end_addr = PGROUNDUP((uint)addr + length);
        uint new_end_length = (uint)curr->addr + curr->length - (uint)addr;
        mmapped_region* new_subregion_end = NULL;
        if(new_end_addr < (uint)curr->addr)
        {
          new_subregion_end = create_region(new_end_addr, new_end_length, offset);
          new_subregion_end->next = curr->next;
        }
        curr->length = (uint)addr - (uint)curr->addr;
        mmapped_region* new_subregion = create_region((uint)addr, length, offset);
        
        curr->next = new_subregion_end;
        p->mmapped_header = insert_into_list(p->mmapped_header, new_subregion, 0);
        addr = new_subregion->addr;
        found_region = 1;
      }
      prev = curr;
      curr = curr->next;
    }
    if(!found_region)
    {
      mmapped_region* new_region = create_and_expand(length, offset);
      p->mmapped_header = insert_into_list(p->mmapped_header, new_region, 0);
      addr = new_region->addr;
    }
  }
  p->nmapped_regions++;
  return addr;
}

int munmap(void *addr, uint length)
{
  struct proc *p = myproc();
  if (addr >= (void*)KERNBASE || length <= 0 || p->nmapped_regions == 0)
    return -1;

  mmapped_region *curr = p->mmapped_header;
  mmapped_region *prev = NULL;

  while(curr)
  {
    if (curr->addr == addr && curr->length == length)
    {
      curr->length = PGROUNDUP(curr->length);
      memset(addr, 0, curr->length);
      p->nmapped_regions--;
      if(!prev && !curr->next)
      {
        p->mmapped_header = NULL;
      }else if(!prev){
        p->mmapped_header = curr->next;
        curr->next = NULL;
      }else{
        prev->next = curr->next;
      }
      p->unmapped_header = insert_into_list(p->unmapped_header, curr, 1);
      switchuvm(p);
      return 0;
    }
    prev = curr;
    curr = curr->next;
  }
  return -1;
}

void expand_node(){
}

