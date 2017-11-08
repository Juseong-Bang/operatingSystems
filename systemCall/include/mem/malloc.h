#ifndef		__MALLOC_H__
#define		__MALLOC_H__

#include <list.h>

#define MAX_NUM_DESC 10
#define MAX_BLOCK_DESC 64

// page descriptor.
struct mem_desc {
	void * page_start;			// start address of page.
	void * page_end;				// end address of page.
	int cur_using;				// check using or not.
};

// block descriptor.
struct block_desc {
	void * start_addr;		// start address of this block.
	void * end_addr;		// end address of this block.
	int cut_size;			// block size provided by Buddy algorithm.
	int block_size;			// size of memory requested by process.	
	int cur_using;			// check using or not.
};

struct mem_desc desc_list[MAX_NUM_DESC];	// page descriptor instance.
struct block_desc blk_list[MAX_BLOCK_DESC];	// block descriptor instance. 
int mem_desc_cnt;				// current allocated number of page.
int blk_desc_cnt;				// current allocated number of block.

void* malloc(unsigned size);
void free(void* buf);

#endif
