#include <types.h>
#include <kern/errno.h>
#include <kern/unistd.h>
#include <write.h>
#include <lib.h>
#include <machine/spl.h>
#include <test.h>
#include <synch.h>
#include <thread.h>
#include <scheduler.h>
#include <dev.h>
#include <vfs.h>
#include <vm.h>
#include <syscall.h>
#include <version.h>
#include <machine/trapframe.h>

int sys_write(int fd, const void *buf,size_t nbytes){
	(void)fd;
	if (buf == NULL)
		return EINVAL;
/*	char *Msg = (char *) kmalloc(nbytes);
	if(Msg == NULL)
		return ENOMEM;
	strcpy(Msg, buf);



	kprintf("%s",Msg);
	return nbytes;
*/
	int i;
	char* ptr = kmalloc((nbytes+1)*sizeof(char));
	int check = copyin(buf, ptr, nbytes); //int to check error fcn
	if (check == 0){
		ptr[nbytes]= '\0';


		kprintf ("%s",ptr);

		kfree(ptr);
		return nbytes;
	}
	else{
		kfree(ptr);
		return EFAULT;
	}


}
