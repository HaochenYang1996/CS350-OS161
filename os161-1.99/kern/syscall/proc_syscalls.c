#include <types.h>
#include <kern/errno.h>
#include <kern/unistd.h>
#include <kern/wait.h>
#include <lib.h>
#include <syscall.h>
#include <current.h>
#include <proc.h>
#include <thread.h>
#include <addrspace.h>
#include <copyinout.h>
#include "opt-A2.h"
#include <mips/trapframe.h>

  /* this implementation of sys__exit does not do anything with the exit code */
  /* this needs to be fixed to get exit() and waitpid() working properly */

void sys__exit(int exitcode) {

  struct addrspace *as;
  struct proc *p = curproc;
  /* for now, just include this to keep the compiler from complaining about
     an unused variable */
  (void)exitcode;

  DEBUG(DB_SYSCALL,"Syscall: _exit(%d)\n",exitcode);

  KASSERT(curproc->p_addrspace != NULL);
  as_deactivate();
  /*
   * clear p_addrspace before calling as_destroy. Otherwise if
   * as_destroy sleeps (which is quite possible) when we
   * come back we'll be calling as_activate on a
   * half-destroyed address space. This tends to be
   * messily fatal.
   */
  as = curproc_setas(NULL);
  as_destroy(as);

  /* detach this thread from its process */
  /* note: curproc cannot be used after this call */
  proc_remthread(curthread);

  /* if this is the last user process in the system, proc_destroy()
     will wake up the kernel menu thread */
  proc_destroy(p);
  
  thread_exit();
  /* thread_exit() does not return, so we should never get here */
  panic("return from thread_exit in sys_exit\n");
}


/* stub handler for getpid() system call                */
int
sys_getpid(pid_t *retval)
{
  /* for now, this is just a stub that always returns a PID of 1 */
  /* you need to fix this to make it work properly */
  *retval = 1;
  return(0);
}

/* stub handler for waitpid() system call                */

int
sys_waitpid(pid_t pid,
	    userptr_t status,
	    int options,
	    pid_t *retval)
{
  int exitstatus;
  int result;

  /* this is just a stub implementation that always reports an
     exit status of 0, regardless of the actual exit status of
     the specified process.   
     In fact, this will return 0 even if the specified process
     is still running, and even if it never existed in the first place.

     Fix this!
  */

  if (options != 0) {
    return(EINVAL);
  }
  /* for now, just pretend the exitstatus is 0 */
  exitstatus = 0;
  result = copyout((void *)&exitstatus,status,sizeof(int));
  if (result) {
    return(result);
  }
  *retval = pid;
  return(0);
}

#if OPT_A2
  pid_t sys_fork(pid_t *retval,struct trapframe *tf){
    // create a new process
    struct proc *child_process = proc_create_runprogram(curproc->p_name);
    if (child_process == NULL) {
      panic("ERROR: cannot create a child process by using a process name");
      return ENOMEM; 
    }

    // point child to parent???
    // child_process->parent =curproc;

    // create address space for child
    as_copy(curproc_getas(), &(child_process->p_addrspace));
    if (child_process->p_addrspace == NULL) {
      proc_destroy(child_process);
      panic("ERROR: copy address space encounters error");
      return ENOMEM; // should we use this error?
    }
     
    // assign a new pid to the child process??
    // char *child_pid = 

    // assign parent trap frame to child
    struct trapframe *child_tf = kmalloc(sizeof(struct trapframe));

    if (child_tf == NULL) {
      proc_destroy(child_process);
      panic("ERROR: failed to create a trap frame for child");
      return ENOMEM;
    }

    memcpy(child_tf, tf, sizeof(struct trapframe));
    

    // create a new thread  as_copy(curproc_getas(), &(child->p_addrspace));
    int create_thread_result = thread_fork(curthread->t_name, child_process, (void *) &enter_forked_process, child_tf, 0);
    if (create_thread_result!=0) {
      proc_destroy(child_process);
      panic("ERROR: failed to create a new thread.");
      return ENOMEM;
    }

    array_add(curproc->children, child_process, NULL);
    child_process->parent = curproc;

    *retval = child_process->pid;
    return 0;
  }
#else
// old (pre-A2) version of the code goes here,
// and is ignored by the compiler when you compile ASST2 // the ‘‘else’’ part is optional and can be left
// out if you are just inserting new code for ASST2
#endif /* OPT_A2 */
