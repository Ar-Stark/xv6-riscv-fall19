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

#define NBUCKETS 13

struct {
  struct spinlock lock[NBUCKETS];
  struct buf buf[NBUF];

  // Linked list of all buffers, through prev/next.
  // head.next is most recently used.
  // struct buf head;
  struct buf hashbucket[NBUCKETS]; //每个哈希队列一个linked list及一个lock
} bcache; 

void
binit(void)
{
  struct buf *b;

  for(int i = 0; i < NBUCKETS; i++){
    initlock(&bcache.lock[i], "bcache.bucket");
    // Create linked list of buffers
    bcache.hashbucket[i].prev = &bcache.hashbucket[i];//创建链表，初始化，prev和next均指向头结点
    bcache.hashbucket[i].next = &bcache.hashbucket[i];
  }

  for(b = bcache.buf; b < bcache.buf+NBUF; b++){//循环遍历buf数组，采用头插法逐个链接到头结点bcache.hashbucket[0]后面。
    b->next = bcache.hashbucket[0].next;
    b->prev = &bcache.hashbucket[0];
    initsleeplock(&b->lock, "buffer");
    bcache.hashbucket[0].next->prev = b;
    bcache.hashbucket[0].next = b;
  }
}

// Look through buffer cache for block on device dev.
// If not found, allocate a buffer.
// In either case, return locked buffer.
static struct buf*
bget(uint dev, uint blockno)
{
  struct buf *b;
  int index = blockno % NBUCKETS;//获得blockno对应的哈希表地址
  acquire(&bcache.lock[index]);

  // Is the block already cached?检查请求的磁盘块是否在缓存中
  for(b = bcache.hashbucket[index].next; b != &bcache.hashbucket[index]; b = b->next){
    if(b->dev == dev && b->blockno == blockno){
      b->refcnt++;
      release(&bcache.lock[index]);
      acquiresleep(&b->lock);//bget()在获取到缓存块后，调用acquiresleep()获取睡眠锁。
      return b;
    }
  }

  // Not cached; recycle an unused buffer.
  int next_index = (index+1) % NBUCKETS;
  while(next_index != index){
    acquire(&bcache.lock[next_index]);
    for(b = bcache.hashbucket[next_index].prev; b != &bcache.hashbucket[next_index]; b = b->prev){
      if(b->refcnt == 0) {
        b->dev = dev;
        b->blockno = blockno;
        b->valid = 0;
        b->refcnt = 1;
        b->next->prev=b->prev;
        b->prev->next=b->next;
        release(&bcache.lock[next_index]);
        b->next=bcache.hashbucket[index].next;//头插法
        b->prev=&bcache.hashbucket[index];
        bcache.hashbucket[index].next->prev=b;
        bcache.hashbucket[index].next=b;
        release(&bcache.lock[index]);
        acquiresleep(&b->lock);
        return b;
      }
    }
    release(&bcache.lock[next_index]);
    next_index=(next_index+1) % NBUCKETS;
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
    virtio_disk_rw(b->dev, b, 0);
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
  virtio_disk_rw(b->dev, b, 1);
}

// Release a locked buffer.
// Move to the head of the MRU list.
void
brelse(struct buf *b)
{
  if(!holdingsleep(&b->lock))
    panic("brelse");

  releasesleep(&b->lock);
  int index = (b->blockno) % NBUCKETS;
  acquire(&bcache.lock[index]);
  b->refcnt--;
  if (b->refcnt == 0) {
    // no one is waiting for it.
    b->next->prev = b->prev;
    b->prev->next = b->next;
    b->next = bcache.hashbucket[index].next;
    b->prev = &bcache.hashbucket[index];
    bcache.hashbucket[index].next->prev = b;
    bcache.hashbucket[index].next = b;
  }
  
  release(&bcache.lock[index]);
}

void
bpin(struct buf *b) {
  int index = (b->blockno) % NBUCKETS;
  acquire(&bcache.lock[index]);
  b->refcnt++;
  release(&bcache.lock[index]);
}

void
bunpin(struct buf *b) {
  int index = (b->blockno) % NBUCKETS;
  acquire(&bcache.lock[index]);
  b->refcnt--;
  release(&bcache.lock[index]);
}


