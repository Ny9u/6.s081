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

struct {
  struct spinlock lock;
  struct run *freelist;
  int pagenum;
  char *used;
  char *newend;
} kmem;

int pagecount(void *pa_start,void*pa_end)
{
  char *p;
  int ret=0;
  p = (char*)PGROUNDUP((uint64)pa_start);
  for(; p + PGSIZE <= (char*)pa_end; p += PGSIZE)
      	ret++;
  return ret;
}

void
kinit()
{
  initlock(&kmem.lock, "kmem");
  kmem.pagenum=pagecount(end,(void*)PHYSTOP);
  kmem.used=end;
  for(int i=0;i<kmem.pagenum;i++)
  {
	kmem.used[i]=0;	
  }
  kmem.newend=kmem.pagenum+kmem.used;
  freerange(kmem.newend, (void*)PHYSTOP);
}
void
freerange(void *pa_start, void *pa_end)
{
  char *p;
  p = (char*)PGROUNDUP((uint64)pa_start);
  for(; p + PGSIZE <= (char*)pa_end; p += PGSIZE)
    kfree(p);
}
int page_index(uint64 pa)
{
	pa=PGROUNDDOWN(pa);
	int res=(pa-(uint64)kmem.newend)/PGSIZE;
	if(res<0||res>=kmem.pagenum)
	{
		panic("illegal");
	}
	return res;
}
void increase(void*pa)
{
	int index=page_index((uint64)pa);
	acquire(&kmem.lock);
	kmem.used[index]++;
	release(&kmem.lock);
}
void decrease(void*pa)
{
	int index=page_index((uint64)pa);
	acquire(&kmem.lock);
	kmem.used[index]--;
	release(&kmem.lock);
}
// Free the page of physical memory pointed at by v,
// which normally should have been returned by a
// call to kalloc().  (The exception is when
// initializing the allocator; see kinit above.)
void
kfree(void *pa)
{
  struct run *r;
  int index=page_index((uint64)pa);
  if(kmem.used[index]>1)
  {
	decrease(pa);
	return;
  }
  if(kmem.used[index]==1)
  {
	decrease(pa);
  }
  
  if(((uint64)pa % PGSIZE) != 0 || (char*)pa < end || (uint64)pa >= PHYSTOP)
    panic("kfree");

  // Fill with junk to catch dangling refs.
  memset(pa, 1, PGSIZE);

  r = (struct run*)pa;

  acquire(&kmem.lock);
  r->next = kmem.freelist;
  kmem.freelist = r;
  release(&kmem.lock);
}
// Allocate one 4096-byte page of physical memory.
// Returns a pointer that the kernel can use.
// Returns 0 if the memory cannot be allocated.
void *
kalloc(void)
{
  struct run *r;

  acquire(&kmem.lock);
  r = kmem.freelist;
  if(r)
    kmem.freelist = r->next;
  release(&kmem.lock);

  if(r)
    { 
    	memset((char*)r, 5, PGSIZE);
    	increase(r);
    } // fill with junk
  return (void*)r;
}
