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
 
signed char ref[32768];
struct spinlock rlock;
struct run {
  struct run *next;
};

struct {
  struct spinlock lock;
  struct run *freelist;
} kmem;

void
kinit()
{
  initlock(&kmem.lock, "kmem");
  memset(ref, 1, sizeof ref);
  freerange(end, (void*)PHYSTOP);
  initlock(&rlock, "ref");
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
  int idx = PA2IDX((uint64)pa);
  decrease(idx);
  if (get(idx)) return;
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
  if(r) {
    ref[PA2IDX((uint64)r)] = 1;
    kmem.freelist = r->next;
  }
  release(&kmem.lock);

  if(r)
    memset((char*)r, 5, PGSIZE); // fill with junk
  return (void*)r;
}

int count() {
    int ans = 0;
    struct run * p = kmem.freelist;
    while (p!= 0) {
        ++ans;
        p = p->next;
    }
    return ans;

}
void increase(int idx) {
    acquire(&kmem.lock);
    if (ref[idx] < 0) panic("increase error");
    ++ref[idx];
    release(&kmem.lock);
}

void decrease(int idx) {
    acquire(&kmem.lock);
    if (ref[idx] < 1) panic("decrease error");
    --ref[idx];
    release(&kmem.lock);
}

int get(int idx) {
    int c;
    acquire(&kmem.lock);
    c = ref[idx];
    release(&kmem.lock);
    if (c < 0) panic("get error");
    return c;
}
