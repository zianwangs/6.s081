#include "types.h"
#include "riscv.h"
#include "defs.h"
#include "date.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "sleeplock.h"
#include "proc.h"
#include "fs.h"
#include "file.h"
#include "my.h"
#include "fcntl.h"

struct vt vmatable;

uint64 sys_mmap(void) {
    uint64 addr, length, offset, ret;
    int fd, prot, flags;
    if (argaddr(0, &addr) || argaddr(1, &length) || argint(2, &prot) || argint(3, &flags) || argint(4, &fd) || argaddr(5, &offset))
        return -1;
    struct proc * p = myproc();
    if (!p->ofile[fd]->writable && (prot & PROT_WRITE) && (flags == MAP_SHARED))
        return -1;
    int i;
    acquire(&vmatable.lock);
    for (i = 0; i < 16; ++i) {
        if (!vmatable.vma[i].used) {
            vmatable.vma[i].used = 1;
            break;
        }
    }
    release(&vmatable.lock);
    if (addr != 0) {
        ret = vmatable.vma[i].addr = addr;
    } else {
        ret = vmatable.vma[i].addr = PGROUNDUP(p->sz);
    }
    p->sz += PGROUNDUP(length);
    vmatable.vma[i].pid = p->pid;
    vmatable.vma[i].length = length;
    vmatable.vma[i].prot = prot;
    vmatable.vma[i].flags = flags;
    vmatable.vma[i].fd = fd;
    vmatable.vma[i].f = p->ofile[fd];
    vmatable.vma[i].offset = offset;
    filedup(p->ofile[fd]);
    return ret;
}

uint64 sys_munmap(void) {
    uint64 addr, length;
    if (argaddr(0, &addr) || argaddr(1, &length))
        return -1;
    return sys_munmap_helper(addr, length);
}

uint64 sys_munmap_helper(uint64 addr, uint64 length) {
    struct proc * p = myproc();
    struct ventry * cur;
    int i;
    acquire(&vmatable.lock);
    for (i = 0; i < 16; ++i) {
        if (vmatable.vma[i].addr == addr && vmatable.vma[i].pid == p->pid && vmatable.vma[i].used) {
            cur = &vmatable.vma[i];
            break;
        }
    }
    release(&vmatable.lock);
    if (i == 16)
        return -1;
    
    if (length > cur->length)
        return -1;
    if (cur->flags == MAP_SHARED && (cur->prot & PROT_WRITE)) {
        struct inode * ip = cur->f->ip;
        begin_op();
        ilock(ip);
        writei(ip, 1, cur->addr, cur->offset, length);
        iunlock(ip);
        end_op();
    }
    int pages;
    if (length == cur->length)
        pages = (PGROUNDUP(addr + length) - PGROUNDDOWN(addr)) / PGSIZE;
    else
        pages = (PGROUNDDOWN(addr + length) - PGROUNDDOWN(addr)) / PGSIZE;
    uvmunmap(p->pagetable, PGROUNDDOWN(cur->addr), pages, 1);
    acquire(&vmatable.lock);
    cur->addr += length;
    cur->length -= length;
    if (cur->length == 0) {
        cur->used = 0;
        cur->addr = 0;
        cur->pid = 0;
        release(&vmatable.lock);
        filedec(cur->f);
    } else release(&vmatable.lock);
    return 0;

}


uint64
sys_exit(void)
{
  int n;
  if(argint(0, &n) < 0)
    return -1;
  exit(n);
  return 0;  // not reached
}

uint64
sys_getpid(void)
{
  return myproc()->pid;
}

uint64
sys_fork(void)
{
  return fork();
}

uint64
sys_wait(void)
{
  uint64 p;
  if(argaddr(0, &p) < 0)
    return -1;
  return wait(p);
}

uint64
sys_sbrk(void)
{
  int addr;
  int n;

  if(argint(0, &n) < 0)
    return -1;
  addr = myproc()->sz;
  if(growproc(n) < 0)
    return -1;
  return addr;
}

uint64
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

uint64
sys_kill(void)
{
  int pid;

  if(argint(0, &pid) < 0)
    return -1;
  return kill(pid);
}

// return how many clock tick interrupts have occurred
// since start.
uint64
sys_uptime(void)
{
  uint xticks;

  acquire(&tickslock);
  xticks = ticks;
  release(&tickslock);
  return xticks;
}
