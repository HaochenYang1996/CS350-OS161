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
#include <synch.h>
#include <kern/fcntl.h>
#include <vfs.h>

void sys__exit(int exitcode) {

  struct addrspace *as;
  struct proc *p = curproc;
  /* for now, just include this to keep the compiler from complaining about
     an unused variable */
#if OPT_A2
  if (curproc->parent != NULL) {
    lock_acquire(curproc->parent->wait_lock);
      for (unsigned int i = 0; i < array_num(curproc->parent->children_info); i++) {
        struct proc_info *p_info = array_get(curproc->parent->children_info, i);
        if (curproc->pid == p_info->pid) {
          p_info->exit_code = exitcode;
          break;
        }
      }
    lock_release(curproc->parent->wait_lock);
    cv_signal(curproc->wait_cv, curproc->wait_lock);
  }
#else
  (void)exitcode;
#endif

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
#if OPT_A2
	*retval = curproc->pid;
#else
  *retval = 1;
#endif //OPT_A2
  return(0);
}

/* stub handler for waitpid() system call                */

// int
// sys_waitpid(pid_t pid,
//       userptr_t status,
//       int options,
//       pid_t *retval)
// {
//   int exitstatus;
//   int result;
//     int waitcurPid = curproc->pid;
//     int waitchildPid = pid;
//     if (waitcurPid == waitchildPid) {
//       panic("ERROR: curPid != childPID");
//     }

//   /* this is just a stub implementation that always reports an
//      exit status of 0, regardless of the actual exit status of
//      the specified process.   
//      In fact, this will return 0 even if the specified process
//      is still running, and even if it never existed in the first place.

//      Fix this!
//   */
//   if (options != 0) {
//     return(EINVAL);
//   }

// #if OPT_A2
//   if (pid<0) return ESRCH;
//   struct proc *process_to_wait = NULL;
//   int arrayLength = array_num(curproc->children);
//   for (int i = 0; i < arrayLength; i++) {
//     process_to_wait = array_get(curproc->children, i);
//     if (pid == process_to_wait->pid) {
//       break;
//     }
//   }

//   if (process_to_wait->parent != curproc) return ECHILD;

//   lock_acquire(process_to_wait->wait_lock);
//   while(!process_to_wait->can_exit) {
//   	cv_wait(process_to_wait->wait_cv, process_to_wait->wait_lock);
//   }
//   lock_release(process_to_wait->wait_lock);
//   exitstatus=process_to_wait->exit_code;
// #else
//   /* for now, just pretend the exitstatus is 0 */
//   exitstatus = 0;
// #endif
//   result = copyout((void *)&exitstatus,status,sizeof(int));
//   if (result) {
//     return(result);
//   }
//   *retval = pid;
//   return(0);
// }
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
#if OPT_A2
  lock_acquire(curproc->wait_lock);
    bool exists = false;
    for (unsigned int i = 0; i < array_num(curproc->children_info); i++) {
      struct proc_info * pinfo = array_get(curproc->children_info, i);
      if (pid == pinfo->pid) {
        exists = true;
        struct proc_info *child = array_get(curproc->children_info, i);
        while (child->exit_code == -1) {
          cv_wait(child->proc_addr->wait_cv, curproc->wait_lock);
        }
        exitstatus = _MKWAIT_EXIT(child->exit_code);
      }
    }
    if (!exists) {
      lock_release(curproc->wait_lock);
      *retval = -1;
      return (ESRCH);
    }
  lock_release(curproc->wait_lock);
#else
  exitstatus = 0;
#endif
  result = copyout((void *)&exitstatus,status,sizeof(int));
  if (result) {
    return(result);
  }
  *retval = pid;
  return(0);
}

// #if OPT_A2
//   pid_t sys_fork(pid_t *retval,struct trapframe *tf){
//     // create a new process
//     struct proc *child_process = proc_create_runprogram(curproc->p_name);
//     if (child_process == NULL) {
//       panic("ERROR: cannot create a child process by using a process name");
//       return ENOMEM; 
//     }

//     int forkcurPid = curproc->pid;
//     int forkchildPid = child_process->pid;
//     if (forkcurPid == forkchildPid) {
//       panic("ERROR: curPid != childPID");
//     }
//     // add to children_info array
//     struct proc_info *child_info = kmalloc(sizeof(struct proc_info));
//     child_info->proc_addr = child_process;
//     child_info->exit_code = -1;
//     child_info->pid = child_process->pid;
//     array_add(curproc->children_info, child_info, NULL);

//     // point child to parent???
//     // child_process->parent =curproc;

//     // create address space for child
//     as_copy(curproc_getas(), &(child_process->p_addrspace));
//     if (child_process->p_addrspace == NULL) {
//       proc_destroy(child_process);
//       panic("ERROR: copy address space encounters error");
//       return ENOMEM; // should we use this error?
//     }
     
//     // assign a new pid to the child process??
//     // char *child_pid = 

//     // assign parent trap frame to child
//     struct trapframe *child_tf = kmalloc(sizeof(struct trapframe));

//     if (child_tf == NULL) {
//       proc_destroy(child_process);
//       panic("ERROR: failed to create a trap frame for child");
//       return ENOMEM;
//     }

//     memcpy(child_tf, tf, sizeof(struct trapframe));
    

//     // create a new thread  as_copy(curproc_getas(), &(child->p_addrspace));
//     int create_thread_result = thread_fork(curthread->t_name, child_process, (void *) &enter_forked_process, child_tf, 0);
//     if (create_thread_result!=0) {
//       proc_destroy(child_process);
//       panic("ERROR: failed to create a new thread.");
//       return ENOMEM;
//     }

//     lock_acquire(curproc->wait_lock);
//     // array_add(curproc->children, child_process, NULL);
//     child_process->parent = curproc;
//     lock_release(curproc->wait_lock);
//     *retval = child_process->pid;
//     return 0;
//   }
// #else
// // old (pre-A2) version of the code goes here,
// // and is ignored by the compiler when you compile ASST2 // the ‘‘else’’ part is optional and can be left
// // out if you are just inserting new code for ASST2
// #endif /* OPT_A2 */



int
sys_fork( pid_t *retval, struct trapframe *tf)
{
  // create child proc
  struct proc *child = proc_create_runprogram(curproc->p_name); // perhaps we want a diff name for child
  KASSERT(child != NULL);
  KASSERT(child->pid > 0);

  int curForkPid = curproc->pid;
  int childForkPid = child->pid;
  if (curForkPid == 1000000) {
    panic("wtf");
  }
  if (childForkPid == 1000000) {
    panic("wtf");
  }


  // set parent pointer
  child->parent = curproc;

  // add to children_info array
  struct proc_info *child_info = kmalloc(sizeof(struct proc_info));
  child_info->proc_addr = child;
  child_info->exit_code = -1;
  child_info->pid = child->pid;
  array_add(curproc->children_info, child_info, NULL);

  // copy over address space
  int err = as_copy(curproc_getas(), &(child->p_addrspace));
  if (err != 0) panic("copy address space errored");
  // probably don't need this function since we are directly copying address space into child
  // address space
  /* proc_setas(child->p_addrspace, child); */

  // create temp trapframe
  curproc->tf = kmalloc(sizeof(struct trapframe));
  KASSERT(curproc->tf != NULL);
  memcpy(curproc->tf, tf, sizeof(struct trapframe));

  // fork thread with temp trapframe
  thread_fork(child->p_name, child, (void *)&enter_forked_process, curproc->tf, 10);

  *retval =  child->pid;

  return(0);
}


int sys_execv(const char *progname, char **args)
{
  // (void) progname;
  // (void) args;
	struct addrspace *as;
	struct vnode *v;
	vaddr_t entrypoint, stackptr;
	int result;
	int argsLen = 0;
  struct addrspace *oldas;

	size_t progLen = sizeof(char) * (strlen(progname)+1);
	char * prognameKernel = kmalloc(progLen);
	KASSERT(prognameKernel!=NULL);
	int progCopyResult = copyin((userptr_t) progname, prognameKernel,progLen);
	KASSERT(progCopyResult == 0);
	// kprintf("Program name after copy is :\n");
	// kprintf(prognameKernel);

	// find arg Len
	for (int i=0; args[i] != NULL; i++) {
		argsLen++;
	}

	// copy args in user space to kernel
	char **argsKernel = kmalloc(sizeof(char *) * (argsLen+1)); 
	KASSERT(argsKernel!=NULL);
	for (int i = 0; i<=argsLen; i++) {
		if (i == argsLen) {
			argsKernel[i] =  NULL;
			break;
		}
		size_t lenCurArg = sizeof(char) * (strlen(args[i])+1); 
		argsKernel[i] = kmalloc(lenCurArg);
		KASSERT(argsKernel[i]);
		int copyArgResult = copyin((const_userptr_t) args[i], argsKernel[i], lenCurArg);
		KASSERT(copyArgResult == 0);
	}


	/* Open the file. */
	result = vfs_open(prognameKernel, O_RDONLY, 0, &v);
	if (result) {
		return result;
	}

	/* We should be a new process. */
	// KASSERT(curproc_getas() == NULL);

	/* Create a new address space. */
	as = as_create();
	if (as ==NULL) {
		vfs_close(v);
		return ENOMEM;
	}

	/* Switch to it and activate it. */
	oldas = curproc_setas(as);
	as_activate();

	/* Load the executable. */
	result = load_elf(v, &entrypoint);
	if (result) {
		/* p_addrspace will go away when curproc is destroyed */
		vfs_close(v);
		return result;
	}

	/* Done with the file now. */
	vfs_close(v);

	/* Define the user stack in the address space */
	result = as_define_stack(as, &stackptr);
	if (result) {
		/* p_addrspace will go away when curproc is destroyed */
		return result;
	}

  // copy args to user stack
  vaddr_t curStackPtr = stackptr;
  vaddr_t * stackArgs = kmalloc(sizeof(vaddr_t) * (argsLen+1));

  for (int i = argsLen; i>=0; i--) {
    if (i == argsLen) {
      stackArgs[argsLen] =(vaddr_t)  NULL;
      continue;
    } else {
      int curArgLen = ROUNDUP(strlen(argsKernel[i]) + 1, 4);
      curStackPtr-=curArgLen*sizeof(char);
      // stackArgs[i] = malloc(sizeof(vaddr_t));
      // KASSERT(curVaddr);
      int copyResult = copyout((const_userptr_t) argsKernel[i], (userptr_t) curStackPtr, curArgLen);
      KASSERT(copyResult == 0);
      stackArgs[i] = curStackPtr;
    }
  }

  for (int i = argsLen; i>=0; i--) {
    curStackPtr-=sizeof(vaddr_t);
    int copyResult = copyout((const_userptr_t) &stackArgs[i], (userptr_t) curStackPtr, sizeof(vaddr_t));
    KASSERT(copyResult == 0);
  }

  as_destroy(oldas);
  // KASSERT(argsKernel==NULL);
  int pos =0;
  while (pos<=argsLen) {
    kfree(argsKernel[pos]);
    pos++;
  }
  kfree(argsKernel);
  kfree(prognameKernel);

  enter_new_process(argsLen, (userptr_t) curStackPtr, ROUNDUP(curStackPtr, 8), entrypoint);

	return EINVAL;
}



