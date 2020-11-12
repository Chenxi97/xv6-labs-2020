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

struct {
  struct spinlock lock[HTSIZE];
  struct buf buf[NBUF];

  // Linked list of all buffers, through prev/next.
  // Sorted by how recently the buffer was used.
  // head.next is most recent, head.prev is least.
  struct buf head[HTSIZE];
} bcache;

void
binit(void)
{
  struct buf *b;

  for(int i=0;i<HTSIZE;i++){
    initlock(&bcache.lock[i], "bcache.bucket");
    // Create linked list of buffers
    bcache.head[i].prev = &bcache.head[i];
    bcache.head[i].next = &bcache.head[i];
  }
  
  for(b = bcache.buf; b < bcache.buf+NBUF; b++){
    b->next = bcache.head[0].next;
    b->prev = &bcache.head[0];
    initsleeplock(&b->lock, "buffer");
    bcache.head[0].next->prev = b;
    bcache.head[0].next = b;
  }
}

// Look through buffer cache for block on device dev.
// If not found, allocate a buffer.
// In either case, return locked buffer.
static struct buf*
bget(uint dev, uint blockno)
{
  struct buf *b;
  int buck=blockno%HTSIZE;

  acquire(&bcache.lock[buck]);

  // Is the block already cached?
  for(b = bcache.head[buck].next; b != &bcache.head[buck]; b = b->next){
    if(b->dev == dev && b->blockno == blockno){
      b->refcnt++;
      release(&bcache.lock[buck]);
      acquiresleep(&b->lock);
      return b;
    }
  }

  // Not cached.
  // Recycle the least recently used (LRU) unused buffer.
  // search form current bucket.
  int newbuck=buck;
  do{
    if(newbuck!=buck)
      acquire(&bcache.lock[newbuck]);
    for(b = bcache.head[newbuck].prev; b != &bcache.head[newbuck]; b = b->prev){
      if(b->refcnt == 0) {
        // delete from original bucket
        b->next->prev=b->prev;
        b->prev->next=b->next;
        if(newbuck!=buck)
          release(&bcache.lock[newbuck]);
        // insert into current bucket
        b->next = bcache.head[buck].next;
        b->prev = &bcache.head[buck];
        bcache.head[buck].next->prev = b;
        bcache.head[buck].next = b;
        // initailize b
        b->dev = dev;
        b->blockno = blockno;
        b->valid = 0;
        b->refcnt = 1;
        release(&bcache.lock[buck]);
        acquiresleep(&b->lock);
        return b;
      }
    }
    if(newbuck!=buck)
      release(&bcache.lock[newbuck]);
    newbuck=(newbuck+1)%HTSIZE;
  } while(newbuck!=buck);
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

  releasesleep(&b->lock);

  int buck=b->blockno%HTSIZE;
  acquire(&bcache.lock[buck]);
  b->refcnt--;
  if (b->refcnt == 0) {
    // no one is waiting for it.
    b->next->prev = b->prev;
    b->prev->next = b->next;
    b->next = bcache.head[buck].next;
    b->prev = &bcache.head[buck];
    bcache.head[buck].next->prev = b;
    bcache.head[buck].next = b;
  }
  
  release(&bcache.lock[buck]);
}

void
bpin(struct buf *b) {
  int buck=b->blockno%HTSIZE;
  acquire(&bcache.lock[buck]);
  b->refcnt++;
  release(&bcache.lock[buck]);
}

void
bunpin(struct buf *b) {
  int buck=b->blockno%HTSIZE;
  acquire(&bcache.lock[buck]);
  b->refcnt--;
  release(&bcache.lock[buck]);
}


