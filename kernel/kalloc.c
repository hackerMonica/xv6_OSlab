// Physical memory allocator, for user processes,
// kernel stacks, page-table pages,
// and pipe buffers. Allocates whole 4096-byte pages.

#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "riscv.h"
#include "defs.h"

void freerange(void *pa_start, void *pa_end);

extern char end[]; // first address after kernel.
                   // defined by kernel.ld.

struct run {
  struct run *next;
};

struct kmem{
  struct spinlock lock;
  struct run *freelist;
} ;
//use array to devide different kmem for every cpu
struct kmem kmems[NCPU];

void
kinit()
{
  //init every kmem 
  for (int i = 0; i < NCPU; i++)
  {
    initlock(&kmems[i].lock, "kmem");
  }
  
  freerange(end, (void*)PHYSTOP);
}

void
freerange(void *pa_start, void *pa_end)
{
  char *p;
  p = (char*)PGROUNDUP((uint64)pa_start);
  for(; p + PGSIZE <= (char*)pa_end; p += PGSIZE)
    kfree(p);
}

// Free the page of physical memory pointed at by v,
// which normally should have been returned by a
// call to kalloc().  (The exception is when
// initializing the allocator; see kinit above.)
void
kfree(void *pa)
{
  struct run *r;

  if(((uint64)pa % PGSIZE) != 0 || (char*)pa < end || (uint64)pa >= PHYSTOP)
    panic("kfree");

  // Fill with junk to catch dangling refs.
  memset(pa, 1, PGSIZE);

  //get which cpu running here
  push_off();
  int cpuID=cpuid();
  pop_off();
  
  r = (struct run*)pa;

  //use this cpu's lock
  acquire(&kmems[cpuID].lock);
  r->next = kmems[cpuID].freelist;
  kmems[cpuID].freelist = r;
  release(&kmems[cpuID].lock);
}

// Allocate one 4096-byte page of physical memory.
// Returns a pointer that the kernel can use.
// Returns 0 if the memory cannot be allocated.
void *
kalloc(void)
{
  struct run *r;

  //get which cpu running here
  push_off();
  int cpuID=cpuid();
  pop_off();
  //use this cpu's lock
  acquire(&kmems[cpuID].lock);
  r = kmems[cpuID].freelist;
  if(r)
    kmems[cpuID].freelist = r->next;
  release(&kmems[cpuID].lock);
  //when memery use up, steal some from other cpu
  if(!r)
    for (int i = 0; i < NCPU; i++)
    {
      if(i==cpuID)  continue;
      acquire(&kmems[i].lock);
      r = kmems[i].freelist;
      if(r)
        kmems[i].freelist = r->next;
      release(&kmems[i].lock);
      if(r) break;
    }
    
  if(r)
    memset((char*)r, 5, PGSIZE); // fill with junk
  return (void*)r;
}
