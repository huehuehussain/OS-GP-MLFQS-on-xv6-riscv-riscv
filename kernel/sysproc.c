#include "types.h"
#include "riscv.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "proc.h"
#include "vm.h"

// External declarations for MLFQ scheduler
extern struct proc proc[NPROC];
extern struct mlfq_stats scheduler_stats;
extern struct spinlock stats_lock;

uint64
sys_exit(void)
{
  int n;
  argint(0, &n);
  kexit(n);
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
  return kfork();
}

uint64
sys_wait(void)
{
  uint64 p;
  argaddr(0, &p);
  return kwait(p);
}

uint64
sys_sbrk(void)
{
  uint64 addr;
  int t;
  int n;

  argint(0, &n);
  argint(1, &t);
  addr = myproc()->sz;

  if(t == SBRK_EAGER || n < 0) {
    if(growproc(n) < 0) {
      return -1;
    }
  } else {
    // Lazily allocate memory for this process: increase its memory
    // size but don't allocate memory. If the processes uses the
    // memory, vmfault() will allocate it.
    if(addr + n < addr)
      return -1;
    if(addr + n > TRAPFRAME)
      return -1;
    myproc()->sz += n;
  }
  return addr;
}

uint64
sys_pause(void)
{
  int n;
  uint ticks0;

  argint(0, &n);
  if(n < 0)
    n = 0;
  acquire(&tickslock);
  ticks0 = ticks;
  while(ticks - ticks0 < n){
    if(killed(myproc())){
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

  argint(0, &pid);
  return kkill(pid);
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

// Get process information for MLFQ scheduler
uint64
sys_getprocinfo(void)
{
  uint64 addr;
  struct proc *p = myproc();
  struct procinfo info;

  argaddr(0, &addr);

  // Fill the procinfo structure with current process information
  info.pid = p->pid;
  info.state = p->state;
  info.queue_level = p->queue_level;
  info.time_in_queue = p->time_in_queue;
  info.time_slices = p->time_slices;
  safestrcpy(info.name, p->name, sizeof(info.name));

  // Copy the structure to user space
  if(copyout(p->pagetable, addr, (char *)&info, sizeof(info)) < 0)
    return -1;

  return 0;
}

// Week 3: Manual priority boost syscall for testing
uint64
sys_boostproc(void)
{
  int pid;
  struct proc *p;
  
  argint(0, &pid);
  
  // If pid is 0, boost all processes
  // Otherwise, boost the process with given pid
  if(pid == 0) {
    // Boost all processes to level 0
    priority_boost();
    return 0;
  } else {
    // Boost specific process
    for(p = proc; p < &proc[NPROC]; p++) {
      acquire(&p->lock);
      if(p->pid == pid) {
        // Set to highest priority
        p->queue_level = 0;
        p->time_in_queue = 0;
        release(&p->lock);
        return 0;
      }
      release(&p->lock);
    }
    return -1;  // Process not found
  }
}

uint64
sys_getschedulerstats(void)
{
  uint64 stats_ptr;
  argaddr(0, &stats_ptr);
  struct mlfq_stats stats;
  
  // Acquire lock and copy statistics
  acquire(&stats_lock);
  stats = scheduler_stats;
  release(&stats_lock);
  
  // Copy to user space
  if(copyout(myproc()->pagetable, stats_ptr, (char *)&stats, sizeof(stats)) < 0)
    return -1;
  
  return 0;
}

