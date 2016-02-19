#include <types.h>
#include <kern/errno.h>
#include <lib.h>
#include <thread.h>
#include <curthread.h>
#include <addrspace.h>
#include <vm.h>
#include <machine/spl.h>
#include <machine/tlb.h>

#define DUMBVM_STACKPAGES    1



/*
 * Note! If OPT_DUMBVM is set, as is the case until you start the VM
 * assignment, this file is not compiled or linked or in any way
 * used. The cheesy hack versions in dumbvm.c are used instead.
 */




/////////////////////////////////////////////////////////////////////////////////////////////////////

int CreatePT(struct addrspace *as){
	int i;
	as->first_table=kmalloc(sizeof(struct PTE)*PTEnum);
	if(as->first_table==NULL){
		kprintf("OUT OF MEMORY ALLOCATING PAGE TABLE");
		return ENOMEM;
	}
	for(i=0;i<PTEnum;i++){
		as->first_table[i].entry=NULL;
	}
	return 0;
}
struct SecondTableEntry* CreateSecondTable(void){
	struct SecondTableEntry* base;
	int i;
	base = kmalloc(sizeof(struct SecondTableEntry)*PTEnum);
	if(base==NULL){
			kprintf("OUT OF MEMORY ALLOCATING PAGE TABLE");
			return NULL;

	}
	for(i=0;i<PTEnum;i++){
		base[i].valid = 0;
		base[i].readOnly = 0;
	}
	return base;
}

struct addrspace *
as_create(void)
{
	int result;
	struct addrspace *as = kmalloc(sizeof(struct addrspace));
	if (as==NULL) {
		return NULL;
	}
//	as->as_vbase1 = 0;
//	as->as_pbase1 = 0;
//	as->as_npages1 = 0;
//	as->offset1=0;
//	as->v1=NULL;
//	as->as_vbase2 = 0;
//	as->as_pbase2 = 0;
//	as->as_npages2 = 0;
//	as->offset2=0;
//	as->v2=NULL;
//	as->as_stackpbase = 0;
//	struct region* stack;
//	struct region* heap;
//	heap = NULL;
//	stack = kmalloc(sizeof(struct region));
//	stack->vbase = USERSTACK;
//	stack->npages = 0;
//	as->regs = stack;
//	stack->next = heap;
	as->first_table=NULL;
	as->regs = NULL;

	result=CreatePT(as);
	if(result){
		return NULL;
	}
	return as;
}

void
as_destroy(struct addrspace *as)
{
	int i,j;
	for(i=0;i<PTEnum;i++){

		pid_t pagenum;
		struct SecondTableEntry* secondLevel = as->first_table[i].entry;

		if(secondLevel == NULL){
			//it doesnt have a 2nd level page table
			//do nothing move to next 1st level entry
			continue;
		}
		else{
			//walk and free 2nd level
			for(j=0;j<PTEnum;j++){ //to walk through 2nd lvl
					//unmap if it is VALID
					if(as->first_table[i].entry[j].valid != 0){
						pagenum = (i<<10)|j; //i is first lvl, j is 2nd lvl, total 20bits
						pagenum = pagenum << 12; //including the 12 offset bits, all 0, now pagenum is 32bits

						int curpid = curthread->pid;
						unmap(curpid,pagenum);

					}
			}

			//after unmap of entire 2nd lvl table, free the entire table from this 1st lvl PTE
			kfree(as->first_table[i].entry);
		}
	}

	//at this point no 2nd lvl page table exist, now free first level
	kfree(as->first_table);

	kfree(as);
}

void
as_activate(struct addrspace *as)
{
	int i, spl;

	(void)as;

	spl = splhigh();

	for (i=0; i<NUM_TLB; i++) {
		TLB_Write(TLBHI_INVALID(i), TLBLO_INVALID(), i);
	}

	splx(spl);
}

int
as_define_region(struct vnode* v,off_t offset,struct addrspace *as, vaddr_t vaddr, size_t sz,
		 int readable, int writeable, int executable)
{
	size_t npages;

	/* Align the region. First, the base... */
	sz += vaddr & ~(vaddr_t)PAGE_FRAME;		//size = size + offset
	vaddr &= PAGE_FRAME;					// vaddr = page num

	/* ...and now the length. */
	sz = (sz + PAGE_SIZE - 1) & PAGE_FRAME;

	npages = sz / PAGE_SIZE;
	kprintf("NUMBER OF PAGES IS %d \n", npages);



	struct region* temp;
	struct region* previous;
	if(as->regs==NULL){
		as->regs=kmalloc(sizeof(struct region));
		as->regs->vbase=vaddr;
		as->regs->npages=npages;
		as->regs->v=v;
		as->regs->offset=offset;
		as->regs->readable=readable;
		as->regs->writable=writeable;
		as->regs->executable=executable;
		as->regs->filesize=sz;
		return 0;
	}

	else{
		temp = as->regs;

		while(temp!=NULL){
				if(temp->vbase == 0){
					temp->vbase=vaddr;
					temp->npages=npages;
					temp->v=v;
					temp->offset=offset;
					temp->readable=readable;
					temp->writable=writeable;
					temp->executable=executable;
					as->regs->filesize=sz;
					return 0;
				}
				previous->next = temp;
				temp = temp->next;
			}
		kprintf("OUT OF THE LOOP\n");

		//Traversed to the the end of the linked list without finding empty region

		temp = kmalloc(sizeof(struct region));
		temp->vbase = vaddr;
		temp->npages = npages;
		temp->v=v;
		temp->offset=offset;
		temp->readable=readable;
		temp->writable=writeable;
		temp->executable=executable;
		as->regs->filesize=sz;
		previous->next = temp;
		return 0;
	}
}

int
as_prepare_load(struct addrspace *as)
{
//	assert(as->as_pbase1 == 0);
//	assert(as->as_pbase2 == 0);
//	assert(as->as_stackpbase == 0);
//
//	as->as_pbase1 = getppages(as->as_npages1);
//	if (as->as_pbase1 == 0) {
//		return ENOMEM;
//	}
//
//	as->as_pbase2 = getppages(as->as_npages2);
//	if (as->as_pbase2 == 0) {
//		return ENOMEM;
//	}
//
//	kprintf("gonna allocated dumbvm_stackpages\n");
//	as->as_stackpbase = getppages(DUMBVM_STACKPAGES);
//	if (as->as_stackpbase == 0) {
//		return ENOMEM;
//	}

	return 0;
}

int
as_complete_load(struct addrspace *as)
{
	(void)as;
	return 0;
}

int
as_define_stack(struct addrspace *as, vaddr_t vaddr){
	struct region* temp;
	struct region* previous;
	if(as->regs == NULL){
		temp = kmalloc(sizeof(struct region));
		if(temp == NULL){
			return ENOMEM;
		}
		temp->executable = 0;
		temp->filesize = 0;	/*Don't need*/
		temp->npages = 0;
		temp->next=NULL;
		temp->offset=0;		/*Don't need*/
		temp->readable=1;
		temp->v=NULL;		/*Don't need*/
		temp->vbase=vaddr;
		temp->writable=1;
		temp->offset=345;
		as->regs =temp;
	}
	else{
		temp = as->regs;
		while(temp!=NULL){
			previous = temp;
			temp = temp->next;
		}
		/*temp is at the end of the region list*/
			temp = kmalloc(sizeof(struct region));
			temp->executable = 0;
			temp->filesize = 0;	/*Don't need*/
			temp->npages = 0;
			temp->next=NULL;
			temp->offset=0;		/*Don't need*/
			temp->readable=1;
			temp->v=NULL;		/*Don't need*/
			temp->vbase=vaddr;
			temp->writable=1;
			temp->offset=345;
			previous->next = temp;

	}
	return 0;
}

int
as_copy(struct addrspace *old, struct addrspace **ret)
{

	struct addrspace *new;
	new = as_create();
	if (new==NULL) {
		return ENOMEM;
	}

	//deep copy first level
	struct PTE *copy = kmalloc(sizeof(struct PTE)*PTEnum);
	new->first_table = copy;

	//copy all 1st lvl PTE
	int i,j;
	for(i=0;i<PTEnum;i++){
		//struct PTE *newguy = kmalloc(sizeof(struct PTE));
		//newguy = old->first_table[i];
		//new->first_table[i] = newguy;

		copy[i].entry = old->first_table[i].entry;

	}

	//deep copy 2nd level
	for(i=0;i<PTEnum;i++){
		struct SecondTableEntry* oldcopy = new->first_table[i].entry;
		if(oldcopy!=NULL){  //this PTE points to 2nd level page table, copy that
			struct SecondTableEntry* secondnew = CreateSecondTable();
			new->first_table[i].entry = secondnew;


			for(j=0;j<PTEnum;j++){
			paddr_t *newptr = &(new->first_table[i].entry[j].frame);
			paddr_t *oldptr = &(old->first_table[i].entry[j].frame);
			*newptr = *oldptr;

			int *newvalid = &(new->first_table[i].entry[j].valid);
			int *oldvalid = &(old->first_table[i].entry[j].valid);
			*newvalid = *oldvalid;

			//set all PTE to read only
			new->first_table[i].entry[j].readOnly = 1;
			old->first_table[i].entry[j].readOnly = 1;
			}
		}
	}

	*ret = new;
	return 0;
	/*struct addrspace *new;

	new = as_create();
	if (new==NULL) {
		return ENOMEM;
	}

	new->as_vbase1 = old->as_vbase1;
	new->as_npages1 = old->as_npages1;
	new->as_vbase2 = old->as_vbase2;
	new->as_npages2 = old->as_npages2;

	if (as_prepare_load(new)) {
		as_destroy(new);
		return ENOMEM;
	}

	assert(new->as_pbase1 != 0);
	assert(new->as_pbase2 != 0);
	assert(new->as_stackpbase != 0);

	memmove((void *)PADDR_TO_KVADDR(new->as_pbase1),
		(const void *)PADDR_TO_KVADDR(old->as_pbase1),
		old->as_npages1*PAGE_SIZE);

	memmove((void *)PADDR_TO_KVADDR(new->as_pbase2),
		(const void *)PADDR_TO_KVADDR(old->as_pbase2),
		old->as_npages2*PAGE_SIZE);

	memmove((void *)PADDR_TO_KVADDR(new->as_stackpbase),
		(const void *)PADDR_TO_KVADDR(old->as_stackpbase),
		DUMBVM_STACKPAGES*PAGE_SIZE);

	*ret = new;

	return 0;*/
}


