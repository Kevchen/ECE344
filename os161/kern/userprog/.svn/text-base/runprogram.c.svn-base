/*
 * Sample/test code for running a user program.  You can use this for
 * reference when implementing the execv() system call. Remember though
 * that execv() needs to do more than this function does.
 */

#include <types.h>
#include <kern/unistd.h>
#include <kern/errno.h>
#include <lib.h>
#include <addrspace.h>
#include <thread.h>
#include <curthread.h>
#include <vm.h>
#include <vfs.h>
#include <test.h>

/*
 * Load program "progname" and start running it in usermode.
 * Does not return except on error.
 *
 * Calls vfs_open on progname and thus may destroy it.
 */
int
runprogram(char *progname, char **args,int nargs)
{

	//kprintf("\nin runprogram\n");

	//int s = splhigh();
	//kprintf("program name %s\n", progname);
	//kprintf("args[1] %s args2[2] %s\n", args[1], args[2]);
	//kprintf("nargs %d\n", nargs);
	//splx(s);

	//kprintf("after printing\n");

	struct vnode *v;
	vaddr_t entrypoint, stackptr;
	int result;
	int i=0;
	//char* path = kmalloc(128);
	//strcpy(progname,path);

		int agrc = nargs;
		char * nullptr = '\0';
		result = vfs_open(progname, O_RDONLY, &v);

		//kprintf("after vfs_open\n");
		if (result) {
			return result;
		}

	assert(curthread->t_vmspace==NULL);
	curthread->t_vmspace=as_create();
	if(curthread->t_vmspace==NULL){
		vfs_close(v);
		return ENOMEM;
	}

	as_activate(curthread->t_vmspace);

	result = load_elf(v, &entrypoint);
	if (result) {
		vfs_close(v);
		return result;
	}

	vfs_close(v);


	result = as_define_stack(curthread->t_vmspace, &stackptr);
	if (result) {

		return result;
	}

	userptr_t ptr[nargs];
	for(i =nargs-1; i>=0;i--){
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
	for(i=nargs-1;i>=0;i--){
		stackptr = stackptr-4;
		copyout(ptr+i,stackptr,4);
		//kprintf("points to %x, holding %s \n",((int) ptr[i]), ptr[i]);
	}
	userptr_t address;
	address=(userptr_t)stackptr;
	//int argv = stackptr;
	stackptr = stackptr - 4;
	copyout(&nargs, stackptr, 4);

	md_usermode(nargs, (userptr_t )address,
		    stackptr, entrypoint);


	//md_usermode(0, NULL,stackptr, entrypoint);
	panic("md_usermode returned\n");
	return EINVAL;
}
