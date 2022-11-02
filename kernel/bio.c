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

#define CACHENUM 13
struct bcache
{
  struct spinlock lock;
  struct buf buf[NBUF];

  // Linked list of all buffers, through prev/next.
  // Sorted by how recently the buffer was used.
  // head.next is most recent, head.prev is least.
  struct buf head;
};
/**
 * use arry to devide bcaches for different group
 * get group order by blockno%CAHENUM
 */
struct bcache bcaches[CACHENUM];
/**use a big lock to solve dead lock
 * when get free buff from other bucket
 */
struct spinlock unitCacheLock;

void
binit(void)
{
  struct buf *b;
  initlock(&unitCacheLock, "bcacheUnit");
  for (int i = 0; i < CACHENUM; i++)
  {
    initlock(&bcaches[i].lock, "bcache");
    // Create linked list of buffers
    bcaches[i].head.prev = &bcaches[i].head;
    bcaches[i].head.next = &bcaches[i].head;
    for (b = bcaches[i].buf; b < bcaches[i].buf + (NBUF + CACHENUM - 1) / CACHENUM; b++)
    {
      b->next = bcaches[i].head.next;
      b->prev = &bcaches[i].head;
      initsleeplock(&b->lock, "buffer");
      bcaches[i].head.next->prev = b;
      bcaches[i].head.next = b;
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
  int cacheID = blockno % CACHENUM;
  acquire(&bcaches[cacheID].lock);

  // Is the block already cached?
  for (b = bcaches[cacheID].head.next; b != &bcaches[cacheID].head; b = b->next)
  {
    if (b->dev == dev && b->blockno == blockno)
    {
      b->refcnt++;
      release(&bcaches[cacheID].lock);
      acquiresleep(&b->lock);
      return b;
    }
  }
  release(&bcaches[cacheID].lock);
  // Not cached.
  acquire(&unitCacheLock);
  acquire(&bcaches[cacheID].lock);
  // Find own bcache for avoiding double load
  for (b = bcaches[cacheID].head.next; b != &bcaches[cacheID].head; b = b->next)
  {
    if (b->dev == dev && b->blockno == blockno)
    {
      b->refcnt++;
      release(&bcaches[cacheID].lock);
      release(&unitCacheLock);
      acquiresleep(&b->lock);
      return b;
    }
  }
  // Recycle the least recently used (LRU) unused buffer.
  // From ***this*** bucket
  for (b = bcaches[cacheID].head.prev; b != &bcaches[cacheID].head; b = b->prev)
  {
    if (b->refcnt == 0)
    {
      b->dev = dev;
      b->blockno = blockno;
      b->valid = 0;
      b->refcnt = 1;
      release(&bcaches[cacheID].lock);
      release(&unitCacheLock);
      acquiresleep(&b->lock);
      return b;
    }
  }
  release(&bcaches[cacheID].lock);

  // Not cached.
  // Recycle the least recently used (LRU) unused buffer.
  // From ***other*** bucket
  // printf("arrive get from other bucket");
  for (int i = 0; i < CACHENUM; i++)
  {
    if (i == cacheID)
      continue;
    acquire(&bcaches[i].lock);
    for (b = bcaches[i].head.prev; b != &bcaches[i].head; b = b->prev)
    {
      if (b->refcnt == 0)
      {
        acquire(&bcaches[cacheID].lock);
        b->dev = dev;
        b->blockno = blockno;
        b->valid = 0;
        b->refcnt = 1;

        b->prev->next = b->next;
        b->next->prev = b->prev;
        b->next = bcaches[cacheID].head.next;
        b->prev = &bcaches[cacheID].head;

        bcaches[cacheID].head.next->prev = b;
        bcaches[cacheID].head.next = b;

        release(&bcaches[cacheID].lock);
        release(&bcaches[i].lock);
        release(&unitCacheLock);

        acquiresleep(&b->lock);
        return b;
      }
    }
    release(&bcaches[i].lock);
  }
  release(&unitCacheLock);
  panic("bget: no buffers");
}

// Return a locked buf with the contents of the indicated block.
struct buf*
bread(uint dev, uint blockno)
{
  struct buf *b;

  b = bget(dev, blockno);
  if (!b->valid)
  {
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

  releasesleep(&b->lock);

  int cacheID = b->blockno % CACHENUM;
  acquire(&bcaches[cacheID].lock);
  b->refcnt--;
  if (b->refcnt == 0)
  {
    // no one is waiting for it.
    b->next->prev = b->prev;
    b->prev->next = b->next;
    b->next = bcaches[cacheID].head.next;
    b->prev = &bcaches[cacheID].head;
    bcaches[cacheID].head.next->prev = b;
    bcaches[cacheID].head.next = b;
  }
  release(&bcaches[cacheID].lock);
}

void
bpin(struct buf *b)
{
  int cacheID = b->blockno % CACHENUM;
  acquire(&bcaches[cacheID].lock);
  b->refcnt++;
  release(&bcaches[cacheID].lock);
}

void
bunpin(struct buf *b)
{
  int cacheID = b->blockno % CACHENUM;
  acquire(&bcaches[cacheID].lock);
  b->refcnt--;
  release(&bcaches[cacheID].lock);
}
