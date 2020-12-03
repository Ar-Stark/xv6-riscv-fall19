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
};

struct kmem kmem[NCPU];//将kalloc的共享freelist改为每个CPU独立的freelist

void
kinit()
{
  for(int i = 0; i < NCPU; i++)
    initlock(&kmem[i].lock, "kmem");//初始化
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

  r = (struct run*)pa;
  push_off();//关闭中断
  int num = cpuid();//获取当前CPU的id号
  acquire(&kmem[num].lock);//获取锁
  r->next = kmem[num].freelist;
  kmem[num].freelist = r;//将空闲页加到链表头
  release(&kmem[num].lock);//释放锁
  pop_off();//打开中断
}

// Allocate one 4096-byte page of physical memory.
// Returns a pointer that the kernel can use.
// Returns 0 if the memory cannot be allocated.
void *
kalloc(void)
{
  struct run *r;

  struct run *temp; 
  push_off();
  int num = cpuid();//获取当前CPU的id号
  acquire(&kmem[num].lock);//每次操作kmem.freelist时都需要先申请kmem.lock，此后再进行内存页面的操作
  r = kmem[num].freelist;//获取空闲链表的第一个空闲页
  if(r)//如果当前CPU的freelist有空闲内存块
    kmem[num].freelist = r->next;//移除空闲链表的第一个空闲页（82行已经获取）
  release(&kmem[num].lock);

  if(!r){//如果当前CPU的freelist没有空闲内存块，从其他CPU的freelist中窃取内存块
    for(int i = 0; i < NCPU; i++){
      acquire(&kmem[i].lock);
      temp = kmem[i].freelist;
      if(temp){
        r = temp;
        kmem[i].freelist = r->next;
        release(&kmem[i].lock);
        break;
      }
      release(&kmem[i].lock);
    }
  }
  pop_off();

  if(r)
    memset((char*)r, 5, PGSIZE); // fill with junk
  return (void*)r;
}
