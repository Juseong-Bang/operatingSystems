#include <proc/sched.h>
#include <proc/proc.h>
#include <device/device.h>
#include <interrupt.h>
#include <device/kbd.h>
#include <mem/paging.h>
#include <mem/palloc.h>
#include <mem/mm.h>
#include <mem/malloc.h>

pid_t do_fork(proc_func func, void* aux)
{
	pid_t pid;
	struct proc_option opt;

	opt.priority = cur_process-> priority;
	pid = proc_create(func, &opt, aux);

	return pid;
}

void do_exit(int status)
{
	cur_process->exit_status = status; 	
	proc_free();						
	do_sched_on_return();				
}

pid_t do_wait(int *status)
{
	while(cur_process->child_pid != -1)
		schedule();

	int pid = cur_process->child_pid;
	cur_process->child_pid = -1;

	extern struct process procs[];
	procs[pid].state = PROC_UNUSED;

	if(!status)
		*status = procs[pid].exit_status;

	return pid;
}

void * do_malloc(int size)
{
	void *result = NULL;
	printk("Allocating size... : %d\n", size);

	int blk_size = get_ceiling(size);	

	if((	result= block_checking( blk_size ))==NULL){
		printk("wrong!!\n");
		return NULL;
	}else{
		blk_list[blk_desc_cnt].start_addr=result;
		blk_list[blk_desc_cnt].end_addr=result+blk_size;
		blk_list[blk_desc_cnt].cut_size=blk_size;
		blk_list[blk_desc_cnt].block_size=size;
		blk_list[blk_desc_cnt].cur_using=1;
		blk_desc_cnt++;
	}//setting new block 

	printk("Memory Allocated Address : %x ~ %x\n", (int)blk_list[blk_desc_cnt-1].start_addr,(int)blk_list[blk_desc_cnt-1].end_addr);

	return result;
}

void do_free(void * st_addr)
{
	int target=0;
	for(target=0;target<blk_desc_cnt;target++)//find the block has start address same as st_addr
	{
		if(blk_list[target].cur_using==1)	
			if(blk_list[target].start_addr==st_addr)
				break;
	}

	if(target==blk_desc_cnt)//if there is no block has same
	{
		printk("nothing\n");
		return;
	}

	int one=0;
	int p=0;

	for( p=0;p<mem_desc_cnt;p++){
		if(desc_list[p].page_start<= blk_list[target].start_addr && blk_list[target].end_addr<=desc_list[p].page_end)
			for(int i=0;i<blk_desc_cnt;i++)//search all block
			{ 
				if(blk_list[i].cur_using==1)
				{ 
					void *str=blk_list[i].start_addr;
					if(desc_list[p].page_start <= str && str < desc_list[p].page_end)//if there is another block in the page 
						one++;				//count 
				}
			}
	}


	printk("Memory Deallocated size : %d(%d)\n", blk_list[target].cut_size,blk_list[target].block_size);
	printk("Memory Deallocated Address : %x\n", blk_list[target].start_addr);

	blk_desc_cnt--;//decremtent block count 
	blk_list[target].start_addr=blk_list[blk_desc_cnt].start_addr;		
	blk_list[target].end_addr=blk_list[blk_desc_cnt].end_addr;		
	blk_list[target].cut_size=blk_list[blk_desc_cnt].cut_size;		
	blk_list[target].block_size=blk_list[blk_desc_cnt].block_size;		
	blk_list[target].cur_using=blk_list[blk_desc_cnt].cur_using;
	blk_list[blk_desc_cnt].cur_using=0;
	//copy

	if(one==1){//the block is only on block of the page 
		mem_desc_cnt--;//decrememt count of using page 
		palloc_free_page (desc_list[p].page_start);	
		desc_list[p].page_start=desc_list[mem_desc_cnt].page_start;
		desc_list[p].page_end=desc_list[mem_desc_cnt].page_end;
		desc_list[p].cur_using=desc_list[mem_desc_cnt].cur_using;
		desc_list[mem_desc_cnt].cur_using=0;
	}//copy
}

void do_shutdown(void)
{
	dev_shutdown();
	return;
}

int do_ssuread(void)
{
	return kbd_read_char();
}

