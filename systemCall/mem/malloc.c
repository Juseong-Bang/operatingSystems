#include <mem/malloc.h>
#include <debug.h>
#include <list.h>
#include <round.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <mem/palloc.h>
#include <mem/paging.h>
#include <synch.h>

extern struct mem_desc desc_list[MAX_NUM_DESC];
extern struct block_desc blk_list[MAX_BLOCK_DESC];
extern int mem_desc_cnt;
extern int blk_desc_cnt;

void *block_getaddr(void *, int);

void init_malloc(void)
{
	int i;
	void * init_addr;

	for (i = 0; i < MAX_NUM_DESC; i++) 
		desc_list[i].cur_using = 0;

	for (i = 0; i < MAX_BLOCK_DESC; i++) 
		blk_list[i].cur_using = 0;

	mem_desc_cnt = 0;
	blk_desc_cnt = 0;

	init_addr = (void *) palloc_get_page();
	desc_list[mem_desc_cnt].page_start = init_addr;
	desc_list[mem_desc_cnt].page_end = init_addr + PAGE_SIZE;
	desc_list[mem_desc_cnt].cur_using = 1;

	printk("Mem addr start : %X\n", desc_list[mem_desc_cnt].page_start);
	printk("Mem addr end : %X\n", desc_list[mem_desc_cnt].page_end);
	mem_desc_cnt++;
}

// Find 2^n < req_size <= 2<n+1, and returns 2^n+1.
// Parameters : req_size = requested memory size by process.
// Return value : 

int get_ceiling(int req_size) 
{
	int ruler = 64;
	while ((ruler < req_size) && (ruler < PAGE_SIZE))	ruler *= 2;
	return ruler;
}

// Browse all memory descriptor and find appropriate address of block which size is 'blk_size'. 
// Parameters : blk_size = the size of block that was calculated by 'get_ceiling()'.
// Return value : returns available address in memory descryptor on success, returns NULL on failure.
void* block_checking(int blk_size)
{
	int cnt=0;
	void * result=NULL;	
	if(mem_desc_cnt!=0)//if there are used pages 
	{
		for(cnt=0;cnt<mem_desc_cnt;cnt++)//searching used pages
		{
			if (desc_list[cnt].cur_using==1)//if this page is current used
			{
				if((result=block_getaddr(desc_list[cnt].page_start,blk_size))!=NULL)
				{
					return result;// return appropriate address of this page's block 
				}
			}
		}
		if(cnt==mem_desc_cnt)//there is no enough space
		{
			void * init_addr = (void *) palloc_get_page();//allocate a new page 
			desc_list[mem_desc_cnt].page_start = init_addr;//put the new page`s address into mem_desc list
			desc_list[mem_desc_cnt].page_end = init_addr + PAGE_SIZE;
			desc_list[mem_desc_cnt].cur_using = 1;

			printk("New page addr : %X\n", desc_list[mem_desc_cnt].page_start);
			result =desc_list[mem_desc_cnt].page_start;

			mem_desc_cnt++;//incrememt count
			return result;// return 
		}
	}
	else// there is no used page it means init error
		return result;
}	
// Find appropriate location of block in page.
// Parameters : page_addr = start address of page, blk_size = the size of block.
// Return value : returns available address in memory descryptor on success, returns NULL on failure.
void* block_getaddr(void *page_addr, int blk_size)
{
	void * result =NULL;
	int num=4096/blk_size;
	bool check[64];//check all state of this page`s blocks
	
for(int a=0;a<64;a++)
		check[a]=false;

	for(int i=0;i<blk_desc_cnt;i++)//search all block
	{ 
		if(blk_list[i].cur_using==1)
		{
			void *str=blk_list[i].start_addr;
			if(page_addr <= str && str < page_addr+PAGE_SIZE) // if the block is in the page
				for(int j=0 ;j< blk_list[i].cut_size/64 ;j++)// size of the block 
				{
					check[(str-page_addr)/64+j]=true;// check true  
				}
		}
	}

	for(int i=0;i<num;i++)//num is mean the number of location in this page could be start address of this block.  
	{
		int location=i*blk_size/64;

		if(!check[location]){// if the location is not used

			int size=location+blk_size/64;

			for(location;location<size;location++){
				if(check[location])//check the state of block size
				{	
					break;
				}
			}
			if(location==size)//state of the size is not current used 
			{
				result=page_addr+ blk_size*i;//return the start address 
				return result;
			}			
		}
	}
	return result;// if this page dosen`t have enought space then return null.
}
