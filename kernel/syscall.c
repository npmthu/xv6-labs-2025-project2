#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "riscv.h"
#include "spinlock.h"
#include "proc.h"
#include "syscall.h"
#include "defs.h"

// Fetch the uint64 at addr from the current process.
int
fetchaddr(uint64 addr, uint64 *ip)
{
  struct proc *p = myproc();
  if(addr >= p->sz || addr+sizeof(uint64) > p->sz) // both tests needed, in case of overflow
    return -1;
  if(copyin(p->pagetable, (char *)ip, addr, sizeof(*ip)) != 0)
    return -1;
  return 0;
}

// Fetch the nul-terminated string at addr from the current process.
// Returns length of string, not including nul, or -1 for error.
int
fetchstr(uint64 addr, char *buf, int max)
{
  struct proc *p = myproc();
  if(copyinstr(p->pagetable, buf, addr, max) < 0)
    return -1;
  return strlen(buf);
}

static uint64
argraw(int n)
{
  struct proc *p = myproc();
  switch (n) {
  case 0:
    return p->trapframe->a0;
  case 1:
    return p->trapframe->a1;
  case 2:
    return p->trapframe->a2;
  case 3:
    return p->trapframe->a3;
  case 4:
    return p->trapframe->a4;
  case 5:
    return p->trapframe->a5;
  }
  panic("argraw");
  return -1;
}

// Fetch the nth 32-bit system call argument.
void
argint(int n, int *ip)
{
  *ip = argraw(n);
}

// Retrieve an argument as a pointer.
// Doesn't check for legality, since
// copyin/copyout will do that.
void
argaddr(int n, uint64 *ip)
{
  *ip = argraw(n);
}

// Fetch the nth word-sized system call argument as a null-terminated string.
// Copies into buf, at most max.
// Returns string length if OK (including nul), -1 if error.
int
argstr(int n, char *buf, int max)
{
  uint64 addr;
  argaddr(n, &addr);
  return fetchstr(addr, buf, max);
}

// Prototypes for the functions that handle system calls.
extern uint64 sys_fork(void);
extern uint64 sys_exit(void);
extern uint64 sys_wait(void);
extern uint64 sys_pipe(void);
extern uint64 sys_read(void);
extern uint64 sys_kill(void);
extern uint64 sys_exec(void);
extern uint64 sys_fstat(void);
extern uint64 sys_chdir(void);
extern uint64 sys_dup(void);
extern uint64 sys_getpid(void);
extern uint64 sys_sbrk(void);
extern uint64 sys_sleep(void);
extern uint64 sys_uptime(void);
extern uint64 sys_open(void);
extern uint64 sys_write(void);
extern uint64 sys_mknod(void);
extern uint64 sys_unlink(void);
extern uint64 sys_link(void);
extern uint64 sys_mkdir(void);
extern uint64 sys_close(void);
extern uint64 sys_trace(void);
extern uint64 sys_sysinfo(void);

// An array mapping syscall numbers from syscall.h
// to the function that handles the system call.
static uint64 (*syscalls[])(void) = {
[SYS_fork]    sys_fork,
[SYS_exit]    sys_exit,
[SYS_wait]    sys_wait,
[SYS_pipe]    sys_pipe,
[SYS_read]    sys_read,
[SYS_kill]    sys_kill,
[SYS_exec]    sys_exec,
[SYS_fstat]   sys_fstat,
[SYS_chdir]   sys_chdir,
[SYS_dup]     sys_dup,
[SYS_getpid]  sys_getpid,
[SYS_sbrk]    sys_sbrk,
[SYS_sleep]   sys_sleep,
[SYS_uptime]  sys_uptime,
[SYS_open]    sys_open,
[SYS_write]   sys_write,
[SYS_mknod]   sys_mknod,
[SYS_unlink]  sys_unlink,
[SYS_link]    sys_link,
[SYS_mkdir]   sys_mkdir,
[SYS_close]   sys_close,
[SYS_trace]   sys_trace,
[SYS_sysinfo] sys_sysinfo,
};

// Array of syscall names for the trace output
static char *syscallnames[] = {
  [SYS_fork]    "fork",
  [SYS_exit]    "exit",
  [SYS_wait]    "wait",
  [SYS_pipe]    "pipe",
  [SYS_read]    "read",
  [SYS_kill]    "kill",
  [SYS_exec]    "exec",
  [SYS_fstat]   "fstat",
  [SYS_chdir]   "chdir",
  [SYS_dup]     "dup",
  [SYS_getpid]  "getpid",
  [SYS_sbrk]    "sbrk",
  [SYS_sleep]   "sleep",
  [SYS_uptime]  "uptime",
  [SYS_open]    "open",
  [SYS_write]   "write",
  [SYS_mknod]   "mknod",
  [SYS_unlink]  "unlink",
  [SYS_link]    "link",
  [SYS_mkdir]   "mkdir",
  [SYS_close]   "close",
  [SYS_trace]   "trace", // system call entry for tracing
  [SYS_sysinfo] "sysinfo",
};

/*
void
argfd(int n, int *fd)
{
    *fd = argraw(n); // Fetch the raw argument
}

static void
print_syscall_args(int num)
{
    uint64 arg0, arg1, arg2;

    switch (num) {
        case SYS_fork:
            break; // No arguments
        case SYS_exit:
            argint(0, (int*)&arg0); // int status
            printf(" status=%d", (int)arg0);
            break;
        case SYS_wait:
            argaddr(0, &arg0); // int *status
            printf(" status=%p", (void*)arg0);
            break;
        case SYS_pipe:
            argaddr(0, &arg0); // int *fd
            printf(" fd=%p", (void*)arg0);
            break;
        case SYS_read:
            argfd(0, (int*)&arg0);    // int fd
            argaddr(1, &arg1); // char *buf
            argint(2, (int*)&arg2);  // int n
            printf(" fd=%d buf=%p n=%d", (int)arg0, (void*)arg1, (int)arg2);
            break;
        case SYS_kill:
            argint(0, (int*)&arg0); // int pid
            printf(" pid=%d", (int)arg0);
            break;
        case SYS_exec:
            argaddr(0, &arg0); // char *path
            argaddr(1, &arg1); // char **argv
            printf(" path=\"%s\" argv=%p", (char*)arg0, (void*)arg1);
            break;
        case SYS_fstat:
            argfd(0, (int*)&arg0);    // int fd
            argaddr(1, &arg1); // struct stat *st
            printf(" fd=%d st=%p", (int)arg0, (void*)arg1);
            break;
        case SYS_chdir:
            argaddr(0, &arg0); // char *path
            printf(" path=\"%s\"", (char*)arg0);
            break;
        case SYS_dup:
            argfd(0, (int*)&arg0); // int fd
            printf(" fd=%d", (int)arg0);
            break;
        case SYS_getpid:
            break;  // No arguments
        case SYS_sbrk:
            argint(0, (int*)&arg0); // int incr
            printf(" incr=%d", (int)arg0);
            break;
        case SYS_sleep:
            argint(0, (int*)&arg0); // int ticks
            printf(" ticks=%d", (int)arg0);
            break;
        case SYS_uptime:
            break; // No arguments
        case SYS_open:
            argaddr(0, &arg0); // char *path
            argint(1, (int*)&arg1); // int mode
            printf(" path=\"%s\" mode=%d", (char*)arg0, (int)arg1);
            break;
        case SYS_write:
            argfd(0, (int*)&arg0);    // int fd
            argaddr(1, &arg1); // char *buf
            argint(2, (int*)&arg2);  // int n
            printf(" fd=%d buf=%p n=%d", (int)arg0, (void*)arg1, (int)arg2);
            break;
        case SYS_mknod:
            argaddr(0, &arg0); // char *path
            argint(1, (int*)&arg1); // int mode
            argint(2, (int*)&arg2); // int dev
            printf(" path=\"%s\" mode=%d dev=%d", (char*)arg0, (int)arg1, (int)arg2);
            break;
        case SYS_unlink:
            argaddr(0, &arg0); // char *path
            printf(" path=\"%s\"", (char*)arg0);
            break;
        case SYS_link:
            argaddr(0, &arg0); // char *old
            argaddr(1, &arg1); // char *new
            printf(" old=\"%s\" new=\"%s\"", (char*)arg0, (char*)arg1);
            break;
        case SYS_mkdir:
            argaddr(0, &arg0); // char *path
            printf(" path=\"%s\"", (char*)arg0);
            break;
        case SYS_close:
            argfd(0, (int*)&arg0); // int fd
            printf(" fd=%d", (int)arg0);
            break;
        case SYS_trace:
            argint(0, (int*)&arg0); // int mask
            printf(" mask=%d", (int)arg0);
            break;
        default:
            printf(" [unknown]");
            break;
    }
}
*/

void
syscall(void)
{
  int num;
  struct proc *p = myproc();

  num = p->trapframe->a7; // Get the system call number from the trapframe
  if (num > 0 && num < NELEM(syscalls) && syscalls[num]) {

    // Special case: Log the `trace` system call before updating the trace_mask
    // -> Ensure trace system call can trace itself
    if (num == SYS_trace) {
      if (num < NELEM(syscallnames) && syscallnames[num]) {
        printf("%d: syscall %s", p->pid, syscallnames[num]);
      }

      // Execute the `trace` system call and store its return value
      p->trapframe->a0 = syscalls[num]();

      // Print the return value for the `trace` system call
      printf(" -> %ld\n", p->trapframe->a0);
      return; // Exit early to avoid double logging
    }

    // Check if tracing is enabled for this system call
    if (p->trace_mask & (1 << num) && num < NELEM(syscallnames) && syscallnames[num]) {
      // Print the system call name and return value placeholder
      printf("%d: syscall %s", p->pid, syscallnames[num]);
    }

    // Execute the system call and store its return value
    p->trapframe->a0 = syscalls[num]();

    // Print the return value if tracing is enabled
    if (p->trace_mask & (1 << num)) {
      printf(" -> %ld\n", p->trapframe->a0); // Print return value on the same line
      //print_syscall_args(num);              // Print arguments on the next line
      printf("\n");                         // Add a newline for better readability
    }

  } else {
    // Handle unknown system calls
    printf("%d %s: unknown sys call %d\n", p->pid, p->name, num);
    p->trapframe->a0 = -1;
  }
}
