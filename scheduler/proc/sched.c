#include <list.h>
#include <proc/sched.h>
#include <mem/malloc.h>
#include <proc/proc.h>
#include <proc/switch.h>
#include <interrupt.h>

extern struct list plist;
extern struct process procs[PROC_NUM_MAX];
extern struct list *active;
extern struct list *expired;
bool more_prio(const struct list_elem *a, const struct list_elem *b,void *aux);
struct process* get_next_proc(struct list *rlist_target);
int scheduling;									// interrupt.c

// Browse the 'active' array
struct process* sched_find_set(void) {//find unempty list  
	struct process * result = NULL;
	for(int i=0;i<PLIST_ROW * PLIST_COL;i++){	//search all lists in active array
		if(!list_empty(&(active[i]))){	//if unempty list exist
			result=get_next_proc(&(active[i]));	//pass a list to get_next_proc
			if(result!=NULL)	//if the list dosen`t have runnable process		
				return result;
		}
	}

	return NULL;
}

struct process* get_next_proc(struct list *rlist_target) {//find runnable process
	struct list_elem *e;

	for(e = list_begin (rlist_target); e != list_end (rlist_target);e = list_next (e))//search all process in the list  
	{
		struct process* p = list_entry(e, struct process, elem_stat);//get a process in the list 
		if(p->state == PROC_RUN )//if a process is runnable
		{	
			return p;// return a runnale process
		}
	}
	return NULL;
}
void schedule(void)
{
	struct process *cur;
	struct process *next;

	if(cur_process->time_slice>=TIMER_MAX){//if the process`s time slice is run out.
		if( cur_process != idle_process){// if not idle process
			list_remove(&cur_process->elem_stat);//remove to active array 
			list_push_back(&(expired[cur_process->priority]),&cur_process->elem_stat);//push to expired array
		}
	}
	if(cur_process != idle_process){// if not idle process 
		enum intr_level old_level = intr_disable();// disable interrupt 
		cur=cur_process;//put current process into cur
		cur_process=idle_process;//put idle process into cur_process

		next = idle_process;
		next->time_slice =0;

		switch_process(cur, next);//switch from normal process to idle process
		intr_set_level (old_level);//recover to old interrupt set 
		return ;
	}

	if((next=sched_find_set())==NULL){//if there is no process in active array
		struct list *temp;
		temp=active;
		active=expired;
		expired=temp;
		//exchange active with expired
		if((next=sched_find_set())==NULL){//if there is no process again it mean every normal processes are done 
			return ;
		}
	}
	BOOL first=true;
	for(struct list_elem *e = list_begin (&plist); e != list_end (&plist);	e = list_next (e))
	{
		struct process* p = list_entry(e, struct process, elem_all);
		if(p->state == PROC_RUN )//print all runnable processes
		{
			if(!first)
				printk(", ");

			printk("#=%2d p=%2d c=%2d u=%2d",p->pid,p->priority,p->time_slice,p->time_used);	
			first=false;
		}
	}
	printk("\nSelect:%d\n",next->pid);//print a selected process

	enum intr_level old_level = intr_disable();//disable interrupt 
	cur=cur_process;//put current process into cur
	cur_process = next;//put next process into cur_process
	cur_process -> time_slice = 0;//next process time slice init 
	proc_wake();//sleep process wake 
	switch_process(cur, next);//switch processes 
	intr_set_level (old_level);//enable interrupt 
}
