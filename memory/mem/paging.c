#include <device/io.h>
#include <mem/mm.h>
#include <mem/paging.h>
#include <device/console.h>
#include <proc/proc.h>
#include <interrupt.h>
#include <mem/palloc.h>
#include <ssulib.h>

uint32_t *PID0_PAGE_DIR;

intr_handler_func pf_handler;

uint32_t scale_up(uint32_t base, uint32_t size)
{
	uint32_t mask = ~(size-1);
	if(base & mask != 0)
		base = base & mask + size;
	return base;
}

uint32_t scale_down(uint32_t base, uint32_t size)
{
	uint32_t mask = ~(size-1);
	if(base & mask != 0)
		base = base & mask - size;
	return base;
}

void init_paging()
{
	uint32_t *page_dir = palloc_get_page();
	uint32_t *page_tbl = palloc_get_page();
	page_dir = VH_TO_RH(page_dir);//0x200000
	page_tbl = VH_TO_RH(page_tbl);//0x201000
	PID0_PAGE_DIR = page_dir;

	int NUM_PT, NUM_PE;
	uint32_t cr0_paging_set;
	int i;

	NUM_PE = mem_size() / PAGE_SIZE; 
	NUM_PT = NUM_PE / 1024;	 

	printk("-PE=%d, PT=%d\n", NUM_PE, NUM_PT);
	printk("-page dir=%x page tbl=%x\n", page_dir, page_tbl);

	//페이지 디렉토리 구성
	page_dir[0] = (uint32_t)page_tbl | PAGE_FLAG_RW | PAGE_FLAG_PRESENT;

	NUM_PE = RKERNEL_HEAP_START / PAGE_SIZE;
	//페이지 테이블 구성
	for ( i = 0; i < NUM_PE; i++ ) {
		page_tbl[i] = (PAGE_SIZE * i)	| PAGE_FLAG_RW | PAGE_FLAG_PRESENT;
		//writable & present
	}


	//CR0레지스터 설정
	cr0_paging_set = read_cr0() | CR0_FLAG_PG;		// PG bit set

	//컨트롤 레지스터 저장
	write_cr3( (unsigned)page_dir );		// cr3 레지스터에 PDE 시작주소 저장
	write_cr0( cr0_paging_set );          // PG bit를 설정한 값을 cr0 레지스터에 저장

	reg_handler(14, pf_handler);
}

void memcpy_hard(void* from, void* to, uint32_t len)
{
	write_cr0( read_cr0() & ~CR0_FLAG_PG);
	memcpy(from, to, len);
	write_cr0( read_cr0() | CR0_FLAG_PG);
}

uint32_t* get_cur_pd()
{
	return (uint32_t*)read_cr3();
}

uint32_t pde_idx_addr(uint32_t* addr)
{
	uint32_t ret = ((uint32_t)addr & PAGE_MASK_PDE) >> PAGE_OFFSET_PDE;
	return ret;// return page directory index number 
}

uint32_t pte_idx_addr(uint32_t* addr)
{
	uint32_t ret = ((uint32_t)addr & PAGE_MASK_PTE) >> PAGE_OFFSET_PTE;
	return ret;// return page table index number 
}

uint32_t* pt_pde(uint32_t pde)
{
	uint32_t * ret = (uint32_t*)(pde & PAGE_MASK_BASE);
	return ret;//make base address make 12bit as 0  
}

uint32_t* pt_addr(uint32_t* addr)
{
	uint32_t idx = pde_idx_addr(addr);//idx is directory index 
	uint32_t* pd = get_cur_pd();//current page directory  
	return pt_pde(pd[idx]); // pd[idx] is page table's addr  
}

uint32_t* pg_addr(uint32_t* addr)
{
	uint32_t *pt = pt_addr(addr);//pt is page table
	uint32_t idx = pte_idx_addr(addr);//idx is page table entry 
	uint32_t *ret = (uint32_t*)(pt[idx] & PAGE_MASK_BASE); //pt[idx] is pointer of the page 
	return ret;// this pointer must have the page`s real address
}

void  pt_copy(uint32_t *pd, uint32_t *dest_pd, uint32_t idx)
{
	uint32_t *pt = RH_TO_VH(pt_pde(pd[idx]));//pt is page table  
	uint32_t i;
	uint32_t *temp=palloc_get_page();//allocate a page to use new table 

	dest_pd[idx]= (uint32_t)VH_TO_RH(temp) | PAGE_FLAG_RW | PAGE_FLAG_PRESENT;
	//mapping new table to dest directory 
	for(i = 0; i<1024; i++)
	{
		if(pt[i] & PAGE_FLAG_PRESENT)//search all pages in the table 
		{
			temp[i]=pt[i];//copy a page
		}
	}
}
void pd_copy(uint32_t* from, uint32_t* to)
{
	uint32_t i;
	for(i = 0; i < 1024; i++)
	{
		if(from[i] & PAGE_FLAG_PRESENT)
		{	
			pt_copy(from, to, i);
		}
	}
}
uint32_t* pd_create (pid_t pid)
{

	uint32_t *pd = palloc_get_page(); // allocate new page to copy page directory
	uint32_t *cur=get_cur_pd(); // current page directory

	pd_copy(RH_TO_VH(cur),pd);//send to current as virtual address and new one

	pd=VH_TO_RH(pd);//convert new directory to real address 

	return pd;//return 
}

void pf_handler(struct intr_frame *iframe)
{
	write_cr0( read_cr0() & ~CR0_FLAG_PG);
	void *fault_addr;
	asm ("movl %%cr2, %0" : "=r" (fault_addr));

	printk("page fault : %X\n",fault_addr);
#ifdef SCREEN_SCROLL
	refreshScreen();
#endif

	if(pt_addr(fault_addr)==0){

		uint32_t *new_page = palloc_get_page();//allocate a new page 
		uint32_t *dir=get_cur_pd(); //current page directory

		dir[pde_idx_addr(new_page)]= (uint32_t)VH_TO_RH(new_page)| PAGE_FLAG_RW | PAGE_FLAG_PRESENT;
		//mapping new page to directory as a page table 		

		uint32_t *cur_table=pt_addr(fault_addr);//take current page table

		cur_table[pte_idx_addr(new_page)]=(uint32_t)VH_TO_RH(new_page)|PAGE_FLAG_RW|PAGE_FLAG_PRESENT;
		//mapping new page to page table as a page 		

		dir[pde_idx_addr(fault_addr)]= (uint32_t)VH_TO_RH(new_page) | PAGE_FLAG_RW | PAGE_FLAG_PRESENT;
		//mapping new page table to directory as page table of fault address  	
	}

	uint32_t *table=pt_addr(fault_addr);//take page table 
	table[pte_idx_addr(fault_addr)]=(uint32_t)VH_TO_RH(fault_addr)|PAGE_FLAG_RW| PAGE_FLAG_PRESENT;
	//mapping fault page to page table as a page 		
	write_cr0( read_cr0() | CR0_FLAG_PG);
}
