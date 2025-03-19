#include "types.h"
#include "riscv.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "proc.h"
#include "sysinfo.h"

uint64 collect_freemem(void);
uint64 collect_nproc(void);
uint64 calculate_loadavg(void);

uint64
sys_exit(void)
{
  int n;
  argint(0, &n);
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
  argaddr(0, &p);
  return wait(p);
}

uint64
sys_sbrk(void)
{
  uint64 addr;
  int n;

  argint(0, &n);
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

/**
 * System call tracing implementation (sys_trace)
 * 
 * Arguments:
 *   This function doesn't directly take arguments in its signature because
 *   xv6 system calls receive arguments through special helper functions.
 *   It retrieves one argument from user space:
 *     - mask: a bit mask determining which system calls to trace
 *
 * Return value:
 *   - 0 on success
 *
 * Description:
 *   This function enables tracing for specific system calls based on a bit mask.
 *   When a process calls trace(mask), this handler sets the process's trace mask.
 *   
 * Data Structure:
 *   - The mask is stored in the p->mask field of the proc structure
 *   - Each bit position in the mask corresponds to a system call number
 *   - The mask is a 32-bit integer where:
 *     * Bit N set to 1 means system call number N will be traced
 *     * Bit N set to 0 means system call number N will not be traced
 *   - Example: To trace fork (SYS_fork=1) and exec (SYS_exec=7):
 *     mask = (1<<1) | (1<<7) = 2 + 128 = 130 = 1000 0010
 *
 * Behavior:
 *   - When trace(mask) is called, this mask is stored in the current process
 *   - The mask is inherited by children during fork()
 *   - Every time a system call is made, the kernel checks if its bit is set
 *   - If set, details about the call are printed to the console
 */
uint64
sys_trace(void)
{
  int mask;
  
  // Get the mask argument from user space
  // void argint(int n, int *ip):
  //    defined in kernel/syscall.c, declared in kernel/defs.h
  //    n: Index of the argument to retrieve (0 = first arg)
  //    ip: Address where to store the retrieved value (bit mask argument will be stored at &mask)
  argint(0, &mask);
  
  // Get the current process structure
  // struc proc:
  //    defined in kernel/proc.h
  //    contains:
  //      - int mask: stores the system call trace mask
  // struct proc* myproc():
  //    defined in kernel/proc.c, declared in kernel/defs.h
  //    return value: Pointer to the current process's proc structure
  struct proc *p = myproc();
  
  // Store the mask in the process's mask field
  // This mask will be:
  // 1. Inherited by child processes during fork()
  // 2. Checked during every system call in syscall()
  p->trace_mask = mask;
  
  return 0; // 0 on success
}

uint64
sys_sysinfo(void)
{
  struct sysinfo si;
  uint64 addr;
  struct proc* p = myproc();
  argaddr(0, &addr);

  si.freemem = collect_freemem();
  si.nproc = collect_nproc();
  si.loadavg = calculate_loadavg();  // load avg

  if(copyout(p->pagetable, addr, (char*)&si, sizeof(si)) < 0)
    return -1;
  return 0;
}

