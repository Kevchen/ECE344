/*
 * vm.c
 *
 *  Created on: 2013-11-18
 *      Author: chiangd2
 */
#include <types.h>
#include <kern/errno.h>
#include <lib.h>
#include <thread.h>
#include <curthread.h>
#include <addrspace.h>
#include <vm.h>
#include <vnode.h>
#include <uio.h>
#include <machine/spl.h>
#include <machine/tlb.h>
#include <machine/vm.h>

#define DUMBVM_STACKPAGES    1
/*
 * Dumb MIPS-only "VM system" that is intended to only be just barely
 * enough to struggle off the ground. You should replace all of this
 * code while doing the VM assignment. In fact, starting in that
 * assignment, this file is not included in your kernel!
 */

/* under dumbvm, always have 48k of user stack */
//#define DUMBVM_STACKPAGES    12

paddr_t paddrStart; //the real start address for frames
int coremapReady = 0; //1 if coremap is set up
int frameNum; //total number of frames
int freeFrame = 0; //number of free frames


//////////////////////////////////////////////////////////////////////////
paddr_t    //allocate frame
allocate_frame(unsigned long npages)
{
	u_int32_t paddr;
	//kprintf("in allocat_frame npages=%d\n",npages);
	int i;
	for(i=0;i<frameNum;i++){
		if(coremap[i].allocated == 0){  //this frame is not allocated, search its contiguous frames
				//kprintf("coremap[%d].allocated ==0\n",i);
				if(npages > 1){
					paddr = findContiguous(npages, i);

					if(paddr != 0) //contiguous frame found!!!
						return paddr;

				}
				else if(npages==1){ //no contiguous frame required
					coremap[i].allocated = 1; //frame is now allocated
					freeFrame--;
					paddr = paddrStart+(i*PAGE_SIZE); //return the physical address
					//kprintf("alloc index=%d, freeFrame = %d\n",i,freeFrame);

					return paddr;
				}
		}
	}

	return 0; //no contiguous frame found
}
//find if contiguous frame available
paddr_t  //return addr or 0 if no space available
findContiguous(unsigned long npages, int index)
{
	//kprintf("inside findContiguous, npages=%d, index=%d\n",npages, index);
	int j,k;
	for(j=1;j<npages;j++){
		if(coremap[index+j].allocated == 1){ //frame has allocated, return 0
			return 0;
		}
	}
	//at this point, all frames needed are unallocated
	//set allocated for contiguous frames

	for(k=0;k<npages;k++){
		coremap[index+k].allocated=1; //set allocated
		coremap[index+k].size = npages; //set number of contiguous frames

	}

	freeFrame -= npages;
	///////found contiguous frames, return paddr of frame coremap[index]
	return paddrStart+(index*PAGE_SIZE);
}

//return index of frame with given addr, -1 if not found
int findCoremapIndex(paddr_t addr){

	return (addr-paddrStart)/PAGE_SIZE;
}

void free_frame(vaddr_t addr){
	int s = splhigh();
	//kprintf("free frame addr=%x\n",addr);

	addr = KVADDR_TO_PADDR(addr);//convert addr to physical

	int index = findCoremapIndex(addr);

	//////////at this point, index is found

	if(coremap[index].size==1){  //no contiguous frame
		freeFrame++;
		coremap[index].allocated = 0;
	}
	else if(coremap[index].size>1){
		int i;
		int size = coremap[index].size; //number of contiguous frames

		for(i=0;i<size;i++){
			coremap[index+i].allocated = 0;
			coremap[index+i].size = 1; //set to orginial

		}
		freeFrame += size;

	}

	//kprintf("free index=%d, freeFrame = %d\n",index,freeFrame);
	splx(s);
}
//insert page to the frame structure
void insertPage(int index, struct page *page){

	if(coremap[index].pagehead==NULL){
		coremap[index].pagehead = page;
	}
	else{
		struct page *temp;
		temp = coremap[index].pagehead;

		while(temp->next!=NULL){
			temp = temp->next;
		}
		// at the last node of the list
		temp->next = page;
	}
}

//page_nr is 32bits
//paddr is 32bits, paddr of the frame
void map(pid_t pid,int page_nr, paddr_t addr){
	addr = addr & 0xfffff000;
	int index = findCoremapIndex(addr);

	struct page *page = kmalloc(sizeof(struct page));
	page->pageNum = page_nr & 0xfffff000; //get rid of offsets
	page->pid = pid;
	page->next = NULL;

	//insert Page into the frame
	insertPage(index, page);

	//update coremap info
	coremap[index].numpages += 1;

}
void unmap(pid_t pid,int page_nr){
	//travel through coremap array to find matching pid
	page_nr = page_nr & 0xfffff000; //remove offset
	int i;
	int found = 0;
	int index = -1;
	for(i=0;i<frameNum && found == 0;i++){
		struct page* temp = coremap[i].pagehead;

		if(temp->next == NULL){ // only 1 page in the list
			if(temp->pageNum == page_nr && temp->pid == pid){ //found matching page
				//update coremap
				found = 1;
				coremap[i].numpages -= 1;
				index = i;

				coremap[i].pagehead = NULL;
				kfree(temp);
				break;
			}

		}
		else if(temp->pageNum == page_nr && temp->pid == pid){ //matched with 1st page in list
			//update coremap
			found = 1;
			coremap[i].numpages -= 1;
			index = i;

			//remove page from the linked list
			coremap[i].pagehead = temp->next;
			kfree(temp);
			break;


		}
		else { 	// in the middle of the list
				while(temp->next!=NULL){
					if(temp->next->pageNum == page_nr && temp->next->pid == pid){ //found matching page
						found = 1;
						coremap[i].numpages -= 1;
						index = i;

						struct page* delete = temp->next;
						temp->next = temp->next->next;
						temp->next->next = NULL;
						kfree(delete);
						break;
					}
					temp = temp->next;
				}
		}

	}
	//not found
	if(index==-1)
		return;

	//free the frame if no page is mapped to it
	if(coremap[index].numpages == 0){
		int addr = paddrStart+(index*PAGE_SIZE);
		addr = addr && 0xfffff000;
		addr = PADDR_TO_KVADDR(addr); //free_frame takes virtual addr
		free_frame(addr);
	}
}

void countFreeFrame(){
	int i;
	int total=0;
	for(i=0;i<frameNum;i++){
		if(coremap[i].allocated==0)
			total++;
	}

	kprintf("Total Free Frame: %d\n",total);
}

//COW function
void Copy_On_Write(vaddr_t faultaddress){
	//allocate a new frame

	struct addrspace* as = curthread->t_vmspace;
	paddr_t newFrame = getppages(1);

	//copy old stuff from old frame to new frame
	paddr_t oldframenum = KVADDR_TO_PADDR(faultaddress);
	int pid = curthread->pid;

	memcpy(PADDR_TO_KVADDR(newFrame), PADDR_TO_KVADDR(oldframenum), PAGE_SIZE);

	int pagenum = faultaddress;
	unmap(pid,pagenum);

	//void map(pid_t pid,int page_nr, paddr_t addr)
	map(pid,pagenum,newFrame);


}





//////////////////////////////////////////////////////////////////////////
void
vm_bootstrap(void)
{

	int ramstart;
	int ramend;

	ram_getsize(&ramstart, &ramend);

    frameNum = (ramend-ramstart) / (PAGE_SIZE+sizeof(struct frame));
    freeFrame = frameNum;

    kprintf("total %d of frames\n",frameNum);

    //total size of coremap
	int coremapSize = sizeof(struct frame)*frameNum;

	//calculate how many pages coremap needs
	coremapSize = ROUNDUP(coremapSize, PAGE_SIZE);

	paddrStart = ramstart + coremapSize;

	kprintf("ramstart = %x, ramend = %x\n",ramstart, ramend);

	//allocate frame array
	//coremap = (struct frame*)kmalloc(sizeof(struct frame)*frameNum);

	coremap = PADDR_TO_KVADDR(ramstart);

	kprintf("coremap[0] = %x\n",coremap);
	kprintf("paddrStart = %x\n",paddrStart);

	int i;
	for(i=0; i<frameNum; i++){
		//kprintf("before initialized\n");
		coremap[i].allocated = 0;
		coremap[i].kernel = -1;
		coremap[i].numpages = 0;
		coremap[i].size = 1;
		coremap[i].pagehead = NULL;
		//kprintf("after initialized\n");
	}

	coremapReady = 1; //coremap is set up

	kprintf("coremap initailized\n");
}

paddr_t
getppages(unsigned long npages)
{
	int spl = splhigh();
	paddr_t addr;


	//kprintf("in getppages npages=%d\n",npages);
	//kprintf("current freeFrame = %d\n",freeFrame);

	if(coremapReady == 0){  //run ram_stealmem if coremap is not set up
		//kprintf("coremapReady == 0\n");
		addr = ram_stealmem(npages);
	}
	else{
		//kprintf("coremapReady == 1\n");
		addr = allocate_frame(npages);
	}

	//kprintf("in getppages, addr = %d\n",addr);
	//kprintf("before return in getpppages\n");
	//kprintf("got freeFrame = %d\n",freeFrame);

	splx(spl);
	return addr;
}

// Allocate/free some kernel-space virtual pages
vaddr_t
alloc_kpages(unsigned long npages)
{
	//kprintf("in alloc_kpages npages=%d\n",npages);
	paddr_t pa;

	pa = getppages(npages);

	if (pa==0) {
		return 0;
	}



	return PADDR_TO_KVADDR(pa);
}

void
free_kpages(vaddr_t addr)  //addr is address to the beginning of frame
{
	//kprintf("free kpages addr=%x\n",addr);
	addr = addr & 0xfffff000;
	free_frame(addr);
}




//////////////////////////////////////////////////////////////////////////////////////////////


int
vm_fault(int faulttype, vaddr_t faultaddress)
{
	int entry, offset,page_num;
	int spl;
	struct addrspace * as;
	spl=splhigh();
	as = curthread->t_vmspace;
	if (as == NULL) {
			/*
			 * No address space set up. This is probably a kernel
			 * fault early in boot. Return EFAULT so as to panic
			 * instead of getting into an infinite faulting loop.
			 */
			return EFAULT;
		}
//	/* Assert that the address space has been set up properly. */
//		assert(as->as_vbase1 != 0);
//		assert(as->as_npages1 != 0);
//		assert(as->as_vbase2 != 0);
//		assert(as->as_npages2 != 0);
//		assert(as->as_stackpbase != 0);
//		assert((as->as_vbase1 & PAGE_FRAME) == as->as_vbase1);
//		assert((as->as_vbase2 & PAGE_FRAME) == as->as_vbase2);
//		assert((as->as_stackpbase & PAGE_FRAME) == as->as_stackpbase);

	//Check the faultaddress belongs to which region
		vaddr_t vbase,vtop;
		struct region* temp;
		struct region* ptr;
		temp = kmalloc(sizeof(struct region));		//Temp will hold the information about the region that it's in
		int exist=0;										//flag for if the vaddr exist in the region
		ptr=as->regs;
		while(ptr!=NULL){
			if(ptr->v == NULL && ptr->offset == 345){
				/*Belongs to the stack*/
				/*vtop is below the base for stack*/
				vbase=ptr->vbase;
				vtop = vbase - ((ptr->npages)*PAGE_SIZE);
				if(faultaddress <=vbase && faultaddress >= vtop){
					exist = 1;
					temp->executable = ptr->executable;
					temp->next = ptr->next;
					temp->npages = ptr->npages;
					temp->offset = ptr->offset;
					temp->readable = ptr->readable;
					temp->v = ptr->v;
					temp->vbase = ptr->vbase;
					temp->writable = ptr->writable;
					temp->filesize=ptr->filesize;
				}
				vtop -= PAGE_SIZE;
				if(faultaddress <=vbase && faultaddress >= vtop){
					exist = 2;
					temp->executable = ptr->executable;
					temp->next = ptr->next;
					temp->npages = ptr->npages;
					temp->offset = ptr->offset;
					temp->readable = ptr->readable;
					temp->v = ptr->v;
					temp->vbase = ptr->vbase;
					temp->writable = ptr->writable;
					temp->filesize=ptr->filesize;
				}


			}
			else{
				/*Belongs anything besides the stack*/
			vbase=ptr->vbase;
			vtop = vbase + ((ptr->npages)*PAGE_SIZE);
			if(faultaddress >= vbase && faultaddress < vtop){
				exist=1;
				temp->executable = ptr->executable;
				temp->next = ptr->next;
				temp->npages = ptr->npages;
				temp->offset = ptr->offset;
				temp->readable = ptr->readable;
				temp->v = ptr->v;
				temp->vbase = ptr->vbase;
				temp->writable = ptr->writable;
				temp->filesize=ptr->filesize;
				break;
			}
			else
				ptr = ptr->next;
			}
		}
	if(exist == 0){
		//When the vaddr doesn't fall in any of the defined regions
		splx(spl);
		return EFAULT;
	}
	vaddr_t first,second;
	u_int32_t ehi, elo;
	paddr_t fr_num,paddress;
	struct SecondTableEntry * frameptr;			//variable that holds the pointer to the frame
	int result;
	page_num = faultaddress & PAGE_FRAME;			//removes the offset
	second = page_num >>12;
	second &= 0x3ff;
	first = page_num>>22;
	switch (faulttype) {
		    case VM_FAULT_READONLY:
		    	/* We always create pages read-write, so we can't get this */
				panic("dumbvm: got VM_FAULT_READONLY\n");
				break;
		    case VM_FAULT_READ:

		    	if(temp->readable==0){
		    		/*Is not readable*/
		    		kprintf("Permission Denied");
		    		splx(spl);
		    		return EINVAL;
		    	}
		    		if(as->first_table[first].entry == NULL){
		    			as->first_table[first].entry = CreateSecondTable();
		    		}
		    		frameptr= (struct SecondTableEntry*) &as->first_table[first].entry[second];
		    		if((frameptr->valid=0)){
		    			paddress=getppages(1);					//Returns the physical address
		    			fr_num = paddress>>12;					//contains the frame number
		    			frameptr->frame = fr_num;
		    			frameptr->valid = 1;
		    			result = page_read(temp->v,temp->offset,temp->vbase,temp->filesize,faultaddress);
		    			if(result){
		    				splx(spl);
		    				return result;
		    			}
		    			map(curthread->pid,faultaddress,paddress);
		    		}
		    	break;
		    case VM_FAULT_WRITE:
		    	if(temp->writable ==0){
		    		/*No permission to write to page*/
		    		kprintf("Permission Denied");
		    		splx(spl);
		    		return EINVAL;
		    	}
				if(as->first_table[first].entry == NULL){
					/*Second table is not allocated yet*/
					as->first_table[first].entry = CreateSecondTable();
				}
				frameptr= (struct SecondTableEntry*) &as->first_table[first].entry[second];
				if((frameptr->valid=0)){
					paddress=getppages(1);					//Returns the physical address
					fr_num = paddress>>12;					//contains the frame number
					frameptr->frame = fr_num;
					frameptr->valid = 1;
					result = load_segment(temp->v,temp->offset,temp->vbase,PAGE_SIZE,temp->filesize,temp->executable);
					if(result){
						splx(spl);
						return result;
					}
					map(curthread->pid,faultaddress,paddress);
				}
				break;
		    default:
			splx(spl);
			return EINVAL;
		}
	/*Add the stuff into the TLB*/
	ehi = faultaddress;
	elo = paddress |TLBLO_DIRTY | TLBLO_VALID;
	TLB_Random(ehi,elo);
	return 0;
}
