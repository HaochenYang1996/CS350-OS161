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
  struct proc *p = curproc;
  struct addrspace *as;
#if OPT_A2
  if (curproc->parent != NULL) {
    lock_acquire(curproc->parent->wait_lock);
      for ( unsigned int i = 0; i < array_num(curproc->parent->childrenStatus); ++i) {
        struct procStatus *curProcInfo = array_get(curproc->parent->childrenStatus, i);
        if (curproc->pid ==curProcInfo->pid) {
          curProcInfo->exitCode = exitcode;
          break;
        }
      }
    lock_release(curproc->parent->wait_lock);
    // wake up a sleelping thread on wait channel
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
  panic("Return form sys_exit\n");
}


/* stub handler for getpid() system call                */
int
sys_getpid(pid_t *retval)
{
#if OPT_A2
    *retval=curproc->pid; 
#else
  *retval = 1;
#endif
  return(0);
}

int
sys_waitpid(pid_t pid, userptr_t status, int options, pid_t *retval) {
  // record whether child has already existed
  int exitStatus;
  int result;

  if (options != 0) {
    // "Invalid argument", 
    return(EINVAL);
  }
  
#if OPT_A2
  lock_acquire(curproc->wait_lock);
    bool childExists = false;
    for (unsigned int i = 0; i < array_num(curproc->childrenStatus); ++i) {
      struct procStatus * pinfo = array_get(curproc->childrenStatus,i);
      // check if current pid is eaqual to pid parameter in the signature
      if (pid == pinfo->pid) {
        struct procStatus *childStatus = array_get(curproc->childrenStatus, i);
        // wait until exit code is no longer -1
        while (childStatus->exitCode == -1) {
          cv_wait(childStatus->procAddr->wait_cv, curproc->wait_lock);
        }
        // generate exit status
        exitStatus = _MKWAIT_EXIT(childStatus->exitCode); 

        // indicate child do exit in curproc's children
        childExists =true;
      }
    }
    // if curproc does not contain the pid to terminate, return error ESRCH
    if (!childExists) {
      *retval= -1;
      lock_release(curproc->wait_lock);
      return (ESRCH);
    }
  lock_release(curproc->wait_lock);
#else
  // exit successfully
  exitStatus = 0;
#endif
  // copy exit status to exit Status
  result = copyout(( void *)&exitStatus, status, sizeof(int));

  if (result) {
    return( result);
  } else {
    *retval = pid;
  }
  return(0);
}

int
sys_fork( pid_t *retval, struct trapframe *tf)
{
  // create a new child process
  struct proc *child = proc_create_runprogram(curproc->p_name);  
  // check whether newly created process has valid pid and whether the process exist or not
  KASSERT(child->pid > 0); 
  KASSERT(child != NULL);   
  // set child's parent 
  child->parent = curproc; 

  // copy parent's address space to child
  int copyResult =as_copy(curproc_getas(), &(child->p_addrspace)); 
  if (copyResult != 0) {
    panic("ERROR: failed to copy parent's address to child");
  }

  // // craete a trapframe
  curproc->tf = kmalloc(sizeof(struct trapframe)); 
  KASSERT(curproc->tf != NULL); 
  memcpy(curproc->tf, tf, sizeof(struct trapframe)); // copy current process's trapframe to child

  // fork thread with current trapframe
  // create a new thread in "child" process, give current process's tf to it
  // execute it and enter user mode
  thread_fork(child->p_name,child, (void *)&enter_forked_process, curproc->tf,10);
  *retval =  child->pid;

  // add information of newly-created child's info to the infomation array(childStatus)
  struct procStatus *childStatus = kmalloc(sizeof(struct procStatus));
  childStatus->procAddr = child;
  childStatus->pid = child->pid; 
  childStatus->exitCode = -1;
  array_add(curproc->childrenStatus, childStatus, NULL);

  return(0); 
}


int sys_execv(const char *progname, char **args) 
{
  // (void) progname;
  // (void) args;
  int argsLen = 0;
  struct addrspace *as;
  struct vnode *v;
  int result;
  vaddr_t entrypoint, stackptr;
  struct addrspace *oldas;

  // allocate space to copy program name(null-terminated) 
  //   Remember that strlen does not count the NULL terminator. Make sure
  // to include room for the NULL terminator.
    size_t progLen = sizeof(char) * (strlen(progname)+1); 
    char * prognameKernel = kmalloc(progLen);
    KASSERT(prognameKernel!=NULL);
  // size_t *got = 0;
    int progCopyResult = copyinstr((userptr_t) progname, prognameKernel,progLen, &progLen);
    KASSERT(progCopyResult == 0);

    // kprintf("Program name after copy is :\n");
    // kprintf(prognameKernel);

    // find args length
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
        int copyArgResult = copyinstr((const_userptr_t) args[i], argsKernel[i], lenCurArg, &lenCurArg);
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



