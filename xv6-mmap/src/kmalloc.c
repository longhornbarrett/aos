#include "types.h"
#include "stat.h"
#include "defs.h"
#include "mmu.h"
#include "param.h"

// From umalloc
typedef long Align;

union kmalloc_node {
  struct {
    union kmalloc_node *ptr;
    uint size;
  } s;
  Align x;
};

typedef union kmalloc_node KNode;

static KNode base;
static KNode *freep;

void
kmfree(void *addr)
{
  KNode *bp, *p;

  bp = (KNode*)addr - 1;
  for(p = freep; !(bp > p && bp < p->s.ptr); p = p->s.ptr)
    if(p >= p->s.ptr && (bp > p || bp < p->s.ptr))
      break;
  if(bp + bp->s.size == p->s.ptr){
    bp->s.size += p->s.ptr->s.size;
    bp->s.ptr = p->s.ptr->s.ptr;
  } else
    bp->s.ptr = p->s.ptr;
  if(p + p->s.size == bp){
    p->s.size += bp->s.size;
    p->s.ptr = bp->s.ptr;
  } else
    p->s.ptr = bp;
  freep = p;
}

static KNode*
kmorecore()
{
  char *p;
  KNode *hp;
  hp = (KNode*)kalloc();
  if(hp == 0)
    return 0;
  hp->s.size = PGSIZE / sizeof(KNode);
  kmfree((void*)(hp + 1));
  return freep;
}

void*
kmalloc(uint nbytes)
{
  if(nbytes > PGSIZE)
  {
    panic("kmalloc: more than 4096 requested");
  }

  KNode *p, *prevp;
  uint nunits;

  nunits = (nbytes + sizeof(KNode) - 1)/sizeof(KNode) + 1;
  if((prevp = freep) == 0){
    base.s.ptr = freep = prevp = &base;
    base.s.size = 0;
  }
  for(p = prevp->s.ptr; ; prevp = p, p = p->s.ptr){
    if(p->s.size >= nunits){
      if(p->s.size == nunits)
        prevp->s.ptr = p->s.ptr;
      else {
        p->s.size -= nunits;
        p += p->s.size;
        p->s.size = nunits;
      }
      freep = prevp;
      return (void*)(p + 1);
    }
    if(p == freep)
      if((p = kmorecore()) == 0)
        return 0;
  }
}