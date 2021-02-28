// Buffer cache.
//
// The buffer cache is a linked list of buf structures holding
// cached copies of disk block contents.  Caching disk blocks
// in memory reduces the number of disk reads and also provides
// a synchronization point for disk blocks used by multiple processes.
//
// Interface:
// * To get a buffer for a particular disk block, call bread.
// * After changing buffer data, call bwrite to write it to disk.
// * When done with the buffer, call brelse.
// * Do not use the buffer after calling brelse.
// * Only one process at a time can use a buffer,
//     so do not keep them longer than necessary.


#include "types.h"
#include "param.h"
#include "spinlock.h"
#include "sleeplock.h"
#include "riscv.h"
#include "defs.h"
#include "fs.h"
#include "buf.h"

#define NBUCKET 11 

struct {
  struct spinlock lock;
  struct buf buf[NBUCKET][NBUF];

  // Linked list of all buffers, through prev/next.
  // Sorted by how recently the buffer was used.
  // head.next is most recent, head.prev is least.
  struct buf head[NBUCKET];
} bcache;

//struct buf * map[13];
struct spinlock lock[NBUCKET];

void
binit(void)
{
  struct buf *b;

  initlock(&bcache.lock, "bcache");
  for (int i = 0; i < NBUCKET; ++i) {
      initlock(lock + i, "bucket");
      printf("%d\n",i);
  }

  // Create linked list of buffers
  for (int i = 0; i < NBUCKET; ++i) {
  struct buf * cur_head = &bcache.head[i];
  cur_head->prev = cur_head;
  cur_head->next = cur_head;
  for(b = bcache.buf[i]; b < bcache.buf[i] + NBUF; b++){
    b->next = cur_head->next;
    b->prev = cur_head;
    initsleeplock(&b->lock, "buffer");
    cur_head->next->prev = b;
    cur_head->next = b;
  }
  }
}

// Look through buffer cache for block on device dev.
// If not found, allocate a buffer.
// In either case, return locked buffer.
static struct buf*
bget(uint dev, uint blockno)
{
  struct buf *b;
  uint idx = blockno % NBUCKET;
  //struct buf * bucket = map[idx];
  struct buf * cur_head = &bcache.head[idx];
  // Is the block already cached?
  acquire(lock + idx);
  for(b = cur_head->next; b != cur_head; b = b->next){
    if(b->dev == dev && b->blockno == blockno){
      b->refcnt++;
      release(lock + idx);
      acquiresleep(&b->lock);
      return b;
    }
  }
  
  // Not cached.
  // Recycle the least recently used (LRU) unused buffer.
  for(b = cur_head->prev; b != cur_head; b = b->prev){
    if(b->refcnt == 0) {
      b->dev = dev;
      b->blockno = blockno;
      b->valid = 0;
      b->refcnt = 1;
      release(lock + idx);
      acquiresleep(&b->lock);
      return b;
    }
  }

  panic("bget: no buffers");
}

// Return a locked buf with the contents of the indicated block.
struct buf*
bread(uint dev, uint blockno)
{
  struct buf *b;

  b = bget(dev, blockno);
  if(!b->valid) {
    virtio_disk_rw(b, 0);
    b->valid = 1;
  }
  return b;
}

// Write b's contents to disk.  Must be locked.
void
bwrite(struct buf *b)
{
  if(!holdingsleep(&b->lock))
    panic("bwrite");
  virtio_disk_rw(b, 1);
}

// Release a locked buffer.
// Move to the head of the most-recently-used list.
void
brelse(struct buf *b)
{
  if(!holdingsleep(&b->lock))
    panic("brelse");

  uint idx = b->blockno % NBUCKET;
  struct buf * cur_head = &bcache.head[idx];
  
  releasesleep(&b->lock);

  acquire(lock + idx);
  b->refcnt--;
  if (b->refcnt == 0) {
    b->next->prev = b->prev;
    b->prev->next = b->next;
    b->next = cur_head->next;
    b->prev = cur_head;
    cur_head->next->prev = b;
    cur_head->next = b;
  }
  release(lock + idx);
}

void
bpin(struct buf *b) {
  acquire(&bcache.lock);
  b->refcnt++;
  release(&bcache.lock);
}

void
bunpin(struct buf *b) {
  acquire(&bcache.lock);
  b->refcnt--;
  release(&bcache.lock);
}


