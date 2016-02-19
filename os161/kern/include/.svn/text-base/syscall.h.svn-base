#ifndef _SYSCALL_H_
#define _SYSCALL_H_

/*
 * Prototypes for IN-KERNEL entry points for system call implementations.
 */

int sys_reboot(int code);



int sys_fork(struct trapframe *tf, int* ret);
int sys_getpid(void);
int sys_waitpid(pid_t pid, int *status, int options, int* ret);
int sys_exit(int exitcode);
int sys_exeve(const char *program, char **args, int* errno);
int sys_read(int fd, char *buf, size_t buflen, int* ret);

#endif /* _SYSCALL_H_ */
