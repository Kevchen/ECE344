#include <types.h>
#include <kern/errno.h>
#include <lib.h>
#include <machine/pcb.h>
#include <machine/spl.h>
#include <machine/trapframe.h>
#include <kern/callno.h>
#include <syscall.h>
#include <thread.h>
#include <write.h>
#include <synch.h>
#include <kern/unistd.h>
#include <curthread.h>
#include <addrspace.h>
#include <vnode.h>
#include <machine/specialreg.h>

/*
 * System call handler.
 *
 * A pointer to the trapframe created during exception entry (in
 * exception.S) is passed in.
 *
 * The calling conventions for syscalls are as follows: Like ordinary
 * function calls, the first 4 32-bit arguments are passed in the 4
 * argument registers a0-a3. In addition, the system call number is
 * passed in the v0 register.
 *
 * On successful return, the return value is passed back in the v0
 * register, like an ordinary function call, and the a3 register is
 * also set to 0 to indicate success.
 *
 * On an error return, the error code is passed back in the v0
 * register, and the a3 register is set to 1 to indicate failure.
 * (Userlevel code takes care of storing the error code in errno and
 * returning the value -1 from the actual userlevel syscall function.
 * See src/lib/libc/syscalls.S and related files.)
 *
 * Upon syscall return the program counter stored in the trapframe
 * must be incremented by one instruction; otherwise the exception
 * return code will restart the "syscall" instruction and the system
 * call will repeat forever.
 *
 * Since none of the OS/161 system calls have more than 4 arguments,
 * there should be no need to fetch additional arguments from the
 * user-level stack.
 *
 * Watch out: if you make system calls that have 64-bit quantities as
 * arguments, they will get passed in pairs of registers, and not
 * necessarily in the way you expect. We recommend you don't do it.
 * (In fact, we recommend you don't use 64-bit quantities at all. See
 * arch/mips/include/types.h.)
 */
int sleep = 0;
int currentpid = -1;

void
mips_syscall(struct trapframe *tf)
{
	int callno;
	int32_t retval;
	int err;

	assert(curspl==0);

	callno = tf->tf_v0;

	/*
	 * Initialize retval to 0. Many of the system calls don't
	 * really return a value, just 0 for success and -1 on
	 * error. Since retval is the value returned on success,
	 * initialize it to 0 by default; thus it's not necessary to
	 * deal with it except for calls that return other values, 
	 * like write.
	 */

	retval = 0;

	switch (callno) {
	    case SYS_reboot:
		err = sys_reboot(tf->tf_a0);
		break;

	    /* Add stuff here */
	    case SYS_write:
		 err = sys_write (tf->tf_a0,(void*)tf->tf_a1,tf->tf_a2);
		break;

	    case SYS_read:
	    	//kprintf("\nin SYS_read\n");
	    	err = sys_read(tf->tf_a0,(char*)tf->tf_a1, tf->tf_a2, &retval);

		break;

	    case SYS_fork:
		err= sys_fork(tf, &retval);
		break;

		case SYS_waitpid:
		err = sys_waitpid((int)tf->tf_a0, (int *)tf->tf_a1, tf->tf_a2, &retval);
		break;

		case SYS__exit:
			sys_exit(tf->tf_a0);
			err = 0;
		break;

		case SYS_getpid:
			retval = sys_getpid();
			err = 0;
		break;

		case SYS_execv:
			err = sys_exeve((char*)tf->tf_a0, (char**)tf->tf_a1,(int*) &retval);

		break;


		/*
		case SYS_read:
		err = sys_read(tf->tf_a0, (void *)tf->tf_a1, tf->tf_a2);
		break;
*/

	    default:
		kprintf("Unknown syscall %d\n", callno);
		err = ENOSYS;
		break;
	}


	if (err) {
		/*
		 * Return the error code. This gets converted at
		 * userlevel to a return value of -1 and the error
		 * code in errno.
		 */
		tf->tf_v0 = err;
		tf->tf_a3 = 1;      /* signal an error */
	}
	else {
		/* Success. */
		tf->tf_v0 = retval;
		tf->tf_a3 = 0;      /* signal no error */
	}
	
	/*
	 * Now, advance the program counter, to avoid restarting
	 * the syscall over and over again.
	 */
	
	tf->tf_epc += 4;

	/* Make sure the syscall code didn't forget to lower spl */
	assert(curspl==0);
}

void
md_forkentry(void *data1, unsigned long data2)
{
	//int s = splhigh();

	struct trapframe tf;
	memcpy(&tf,(struct trapframe*)data1,sizeof(struct trapframe));
	kfree(data1);

	//copy addrspace from previousely stored tf
	curthread->t_vmspace = (struct addrspace *)data2;
	as_activate(curthread->t_vmspace);

	////////////map all the frames//////////////
	int i,j;
	for(i=0;i<PTEnum;i++){  //to walk through 1st lvl
		//map(pid_t pid,int page_nr, paddr_t addr)

		paddr_t framenum;
		pid_t pagenum;

		struct SecondTableEntry* secondLevel = curthread->t_vmspace->first_table[i].entry;
		if(secondLevel == NULL){
			//it doesnt have a 2nd level page table
			//do nothing move to next 1st level entry
			continue;
		}
		else{
			//walk through 2nd level
			for(j=0;j<PTEnum;j++){ //to walk through 2nd lvl
					//map only if this 2nd lvl entry is VALID!
					if(curthread->t_vmspace->first_table[i].entry[j].valid != 0){
						framenum = curthread->t_vmspace->first_table[i].entry[j].frame; //walk the 2nd table, ie. 0xfffff
						framenum = framenum << 12;
						pagenum = (i<<10)|j; //i is first lvl, j is 2nd lvl, total 20bits
						pagenum = pagenum << 12; //including the 12 offset bits, all 0, now pagenum is 32bits

						int curpid = curthread->pid;
						map(curpid,pagenum,framenum);
					}
			}
		}
	}

	////////////////////////////////////////////

	//Set the return value to 0 for the child process and advance the program counter to avoid restarting the syscall
	tf.tf_v0 = 0;
	tf.tf_epc += 4;
	tf.tf_a3 = 0;

	/*
	tf.tf_status = CST_IRQMASK | CST_IEp | CST_KUp;
	tf.tf_a0 = 0;
	tf.tf_a1 = 0;
*/
	//splx(s);

	mips_usermode(&tf);
}

int sys_fork(struct trapframe *tf, int* ret){
	int s = splhigh();

	int result;
	struct addrspace *addrcopy;
	struct thread *child_thread = NULL;
	//Make a copy of the parent's trap frame on the kernel heap
	struct trapframe *tf_copy = kmalloc(sizeof(struct trapframe));

	if (tf_copy == NULL) {
		splx(s);
		return ENOMEM;
	}

	//copy mem
	memcpy(tf_copy,tf,sizeof(struct trapframe));

	result = as_copy(curthread->t_vmspace, &addrcopy);
	if (result) {
		kfree(tf_copy);
		splx(s);
		return result;
	}

	as_activate(curthread->t_vmspace);

	result=thread_fork(curthread->t_name, (void*)tf_copy,(unsigned long)addrcopy, md_forkentry, &child_thread);
	if(result)
	{
		kfree(tf_copy);
		as_destroy(addrcopy);
		splx(s);
		return result;
	}



	//Return the child's process id
	*ret = child_thread->pid;
	splx(s);
    return 0;
}
int sys_read(int fd, char *buf, size_t buflen, int* ret){

	(void)fd;

	if(buf == NULL){
		*ret = EFAULT;
		return -1;
	}


	int err = 0;
	int count = 0;
	char *msg = (char *)kmalloc((buflen+1u) * sizeof(char));

	int i=0;
	for(i=0;i<buflen;i++){
		msg[i]=getch();
		//kprintf("msg[%d]= %c\n",i,buf[i]);
		count++;

		if(msg[i]=='\r')
			break;
	}

	err = copyout((const void*)msg, (userptr_t)buf, (sizeof(char))*count+1u);


	//kprintf("\nbuf = %c\n",*buf);
	//kprintf("\nbuflen = %d\n", buflen);


	if(err != 0){
		*ret = err;
		return -1;
	}

	*ret = count+1u;
	return 0;
}



int sys_getpid(void){
    return curthread->pid;
}

int sys_waitpid(pid_t pid, int *status, int options, int* ret){
	P(lock);
	//kprintf("\nthread pid %d is waiting for child %d\n",curthread->pid, pid);

    struct thread* child;
    int childpid;
    int childIndex;



    //check if this process is your child
	if(checkChild(curthread->pid, pid)==0){
	 kprintf("This is not your child.\n");
	 V(lock);
	 return EINVAL;
	}

//	//it is your child, check if you have waited for them before
//	int temp = findChildpid(pid, curthread);
//	if(temp == -1){
//		//kprintf("This is not your child.\n");
//		V(lock);
//		return EINVAL;
//	}

	if(checkWaited(pid)==1){
		 V(lock);
		 return EINVAL;
	}

    setwaited(pid);

    if(findexited(pid)==1){
    	//already exited
    	V(lock);
    	*status = findexcode(pid);
    	*ret = pid;
    	return 0;
    }
    else if(findexited(pid)==0){
    	//need to find child

        child = findThread(pid);;
        childpid = pid;

	    ///////////////found child//////////////////////////
	   // need to wait for them
	    V(lock);
	    P(child->wait);

	    *status = findexcode(childpid);
	    *ret = pid;
	    return 0;

    }
    else{
    	 V(lock);
    	 *status = findexcode(pid);
    	 *ret = pid;
    	 return EINVAL;
    }

}

int sys_exit(int exitcode){

	int s = splhigh();

	//kprintf("\ncurthread pid %d, in sys_exit\n", curthread->pid);


	setexcode(curthread->pid, exitcode);
	setexited(curthread->pid);

	//thread_wakeup(curthread);
	//thread_detach(curthread);

	splx(s);

	int i;
	for(i=0;i<sleep;i++){
		V(curthread->wait);
	}
	//s=splhigh();
	//kprintf("after v pid %d\n",curthread->pid);
	//splx(s);

	thread_exit();
	return 0;
}
///////////////////////////////////////////////////////////////////////////////////////////////////

int sys_exeve(const char *program, char **args, int* errno)
{

	// Copy the parameters of the function
	struct vnode *v;
	char* progpath;
	struct addresspace* as;
	vaddr_t entrypoint,stackptr,temp;
	int i,j,result;
	i=0;
	char *argv;
	if(program==NULL){
		return ENOENT;

	}
	void* dest = kmalloc(256);
	if(dest)
		return ENOMEM;

	result = copyinstr(program, dest, strlen(program)+1,NULL);
	if(result){
		return result;

	}
	while(args[i] != NULL)
	    i++;

	int argc = i+1;
	for(i = 0; i < argc; i++) {
		int len = strlen(args[i]);
	    len++;
	    argv[i]=(char*)kmalloc(len*sizeof(char));
	    if(argv[i])
	    	return ENOMEM;

	    result = copyinstr((userptr_t)args[i], argv[i], len+1, NULL);
	    if(result){
	    	return result;

	    }
	}
	argv[argc] = NULL;


	//Destroy the addrspace of the current thread
	as_destroy(curthread->t_vmspace);
	result=copyoutstr(dest,progpath,strlen(program)+1,NULL);
	result = vfs_open(progpath, O_RDONLY, &v);
		if (result) {

			return result;

		}
	as = curthread->t_vmspace;
	curthread->t_vmspace=as_create();
	if (curthread->t_vmspace==NULL){
		vfs_close(v);
		return ENOMEM;

	}
	//* Activate it.
		as_activate(curthread->t_vmspace);

		// Load the executable.
		result = load_elf(v, &entrypoint);
		if (result) {
			curthread->t_vmspace = as;
			as_activate(as);
			vfs_close(v);
			return result;

		}


		// Done with the file now.
		vfs_close(v);
		// Define the user stack in the address space
			result = as_define_stack(curthread->t_vmspace, &stackptr);
			if (result) {
				//* thread_exit destroys curthread->t_vmspace
				return result;

			}
		//Load arguments into the stack
			char* nullptr = '\0';
			userptr_t ptr[argc];
				for(i =argc-1; i>=0;i--){
					int length = strlen(args[i]);
						int pad;
						pad = 4-(length%4);
						length += pad;
						stackptr -= length;
						ptr[i]=stackptr;
					result=copyoutstr(args[i],stackptr,length+1,NULL);
					//kprintf("address is %x and value is %s\n" , stackptr,stackptr);
					if(result)
						return result;
				}
				//char* nullptr = '\0';
					stackptr -= 4;
					copyout(nullptr, stackptr, 4);
					//ptr[nargs-1]=NULL;
					if(result)
						return result;
					//kprintf("stackptr= %s",stackptr);
					for(i=argc-1;i>=0;i--){
						stackptr = stackptr-4;
						copyout(ptr+i,stackptr,4);
						//kprintf("points to %x, holding %s \n",((int) ptr[i]), ptr[i]);
					}
					userptr_t address;
					address=(userptr_t)stackptr;
					//int argv = stackptr;
					stackptr = stackptr - 4;
					copyout(&argc, stackptr, 4);

					md_usermode(argc, (userptr_t )address,
						    stackptr, entrypoint);
					panic("md_usermode returned\n");
							return EINVAL;

}
