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

#define NBUCKETS 7
struct bcache{
  struct spinlock lock;
  struct buf buf[NBUF];

  // Linked list of all buffers, through prev/next.
  // Sorted by how recently the buffer was used.
  // head.next is most recent, head.prev is least.
 
}bcach[NBUCKETS];

uint getindex(uint blockno)
{
   return blockno % NBUCKETS;
}

void
binit(void)
{
  struct buf *b;
  // Create linked list of buffers
  /*bcache.head.prev = &bcache.head;
  bcache.head.next = &bcache.head;*/
  for(int i=0;i<NBUCKETS;i++){
  	for(b = &bcach[i].buf[0]; b < &bcach[i].buf[0]+NBUF; b++){
  	//int j=0;j<NBUF;j++
  		initsleeplock(&b->lock, "buffer");// init every sleeplock
  	  b->time=ticks;
  	  
    /*b->next = bcache.head.next;
    b->prev = &bcache.head;
    
    bcache.head.next->prev = b;
    bcache.head.next = b;*/
  	}
  	initlock(&bcach[i].lock, "bcache.bucket");//init every lock
  }
}

// Look through buffer cache for block on device dev.
// If not found, allocate a buffer.
// In either case, return locked buffer.
static struct buf*
bget(uint dev, uint blockno)
{
	uint index=getindex(blockno);
	
  acquire(&bcach[index].lock);
	struct buf *b;
  // Is the block already cached?
  for(b = &bcach[index].buf[0]; b < &bcach[index].buf[0]+NBUF; b++){
    if(b->dev == dev && b->blockno == blockno){
      b->refcnt++;
      b->time=ticks;
      release(&bcach[index].lock);
      acquiresleep(&b->lock);
      return b;//find it in bcache[index]
    }
  }
  // Not cached.
  // Recycle the least recently used (LRU) unused buffer.
  uint min_tick=0xffffffff;
  struct buf * lru_buf=0;
  for(b = &bcach[index].buf[0]; b < &bcach[index].buf[0]+NBUF; b++){
    if(b->refcnt==0&&b->time<min_tick) {
     		min_tick=b->time;
     		lru_buf=b;
    } 
  } 
  if(lru_buf!=0){
      lru_buf->dev = dev;
      lru_buf->blockno = blockno;
      lru_buf->valid = 0;
      lru_buf->refcnt = 1;
      lru_buf->time=ticks;
      release(&bcach[index].lock);
      acquiresleep(&lru_buf->lock);
      return lru_buf;
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
    virtio_disk_rw(b, 0);//从磁盘读入数据到缓冲区b
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
  virtio_disk_rw(b, 1);//有睡眠锁才能往缓冲区写入数据
}

// Release a locked buffer.
// Move to the head of the most-recently-used list.
void
brelse(struct buf *b)
{
  if(!holdingsleep(&b->lock))
    panic("brelse");
    
	uint index=getindex(b->blockno);
  releasesleep(&b->lock);
  acquire(&bcach[index].lock);
  
  b->refcnt--;
  
  if (b->refcnt == 0) {
    // no one is waiting for it.
   /* b->next->prev = b->prev;
    b->prev->next = b->next;
    b->next = bcache.head.next;
    b->prev = &bcache.head;
    bcache.head.next->prev = b;
    bcache.head.next = b;*/
    b->time=ticks;
  }
  
 	release(&bcach[index].lock);
}

void
bpin(struct buf *b) {
  uint index=getindex(b->blockno);
  acquire(&bcach[index].lock);
  b->refcnt++;
  release(&bcach[index].lock);
}

void
bunpin(struct buf *b) {
	uint index=getindex(b->blockno);
  acquire(&bcach[index].lock);
  b->refcnt--;
  release(&bcach[index].lock);
}


