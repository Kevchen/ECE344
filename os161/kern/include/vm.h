#ifndef _VM_H_
#define _VM_H_

#include <machine/vm.h>

/*
 * VM system-related definitions.
 *
 * You'll probably want to add stuff here.
 */
struct page {
	int pid;
	int pageNum;

	struct page *next;
};


struct frame {
	int allocated; //0 not allocated, 1 allocated
	int numpages; //how many pages are mapped to this frame
	int kernel; //0 if its user, 1 if kernel, -1 not allocated
	int size; //store the size of the contagious frame allocation, so when we free we know how many to free

	struct page *pagehead; //link list of the pages that are mapped to this frame
};



struct frame *coremap; //the start of the coremap array





/* Fault-type arguments to vm_fault() */
#define VM_FAULT_READ        0    /* A read was attempted */
#define VM_FAULT_WRITE       1    /* A write was attempted */
#define VM_FAULT_READONLY    2    /* A write to a readonly page was attempted*/


/* Initialization function */
void vm_bootstrap(void);

/* Fault handling function called by trap code */
int vm_fault(int faulttype, vaddr_t faultaddress);

/* Allocate/free kernel heap pages (called by kmalloc/kfree) */
vaddr_t alloc_kpages(unsigned long npages);
void free_kpages(vaddr_t addr);
struct PTE;


paddr_t getppages(unsigned long npages);
paddr_t allocate_frame(unsigned long npages);
paddr_t findContiguous(unsigned long npages, int index);
int findCoremapIndex(paddr_t addr);
void free_frame(vaddr_t addr);
void insertPage(int index, struct page *page);
void map(pid_t pid,int page_nr, paddr_t addr);
void unmap(pid_t pid,int page_nr);





#endif /* _VM_H_ */
