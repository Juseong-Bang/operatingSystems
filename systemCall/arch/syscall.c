#include <syscall.h>
#include <do_syscall.h>
#include <proc/proc.h>


// system call number -> push to stack
// parameters -> assign to register 
// return value -> top of the stack
#define syscall0(SYS_NUM) ({				\
		int ret;							\
		__asm__ __volatile(					\
				"pushl %[num]\n\t"			\
				"int $0x30\n\t"				\
				"popl %[ret]"				\
				:							\
				: [num] "g" (SYS_NUM),		\
				[ret] "g" (ret)		\
				);							\
		ret;								\
		})

#define syscall1(SYS_NUM, ARG0) ({				\
		int ret;								\
		__asm__ __volatile(						\
				"movl  %[arg0], %%ebx \n\t"			\
				"pushl %[num] \n\t"				\
				"int $0x30\n\t"					\
				"popl %[ret]"				\
				:								\
				: [num] "g" (SYS_NUM),			\
				[arg0] "g" (ARG0)	,			\
				[ret] "g" (ret)		\
				);								\
		ret;									\
		})

#define syscall2(SYS_NUM, ARG0, ARG1) ({		\
		int ret;								\
		__asm__ __volatile(						\
				"movl  %[arg1], %%ecx \n\t"			\
				"movl  %[arg0], %%ebx\n\t"			\
				"pushl %[num] \n\t"				\
				"int $0x30\n\t"					\
				"popl %[ret]"				\
				:								\
				: [num] "g" (SYS_NUM),			\
				[arg0] "g" (ARG0),				\
				[arg1] "g" (ARG1)	,			\
				[ret] "g" (ret)		\
				);								\
		ret;									\
		})

#define syscall3(SYS_NUM, ARG0, ARG1, ARG2) ({	\
		int ret;								\
		__asm__ __volatile(						\
				"movl %[arg2], %%edx \n\t"			\
				"movl %[arg1], %%ecx \n\t"			\
				"movl %[arg0], %%ebx \n\t"			\
				"pushl %[num] \n\t"				\
				"int $0x30\n\t"					\
				"popl %[ret]"				\
				:								\
				: [num] "g" (SYS_NUM),			\
				[arg0] "g" (ARG0),				\
				[arg1] "g" (ARG1),				\
				[arg2] "g" (ARG2)	,			\
				[ret] "g" (ret)		\
				);								\
		ret;									\
		})
int syscall_tbl[SYS_NUM][2];

#define REGSYS(NUM, FUNC, ARG) \
	syscall_tbl[NUM][0] = (int)FUNC; \
syscall_tbl[NUM][1] = ARG; 

void init_syscall(void)
{
	REGSYS(SYS_FORK, do_fork, 2);
	REGSYS(SYS_EXIT, do_exit, 1);
	REGSYS(SYS_WAIT, do_wait, 1);
	REGSYS(SYS_SSUREAD, do_ssuread, 0);
	REGSYS(SYS_SHUTDOWN, do_shutdown, 0);

	REGSYS(SYS_MALLOC, do_malloc, 1);
	REGSYS(SYS_FREE, do_free, 1);
}

void exit(int status)
{
	syscall1(SYS_EXIT, status);
}

pid_t fork(proc_func func, void* aux)
{
	return syscall2(SYS_FORK, func, aux);
}

pid_t wait(int *status)
{
	return syscall1(SYS_WAIT, status);
}

void* malloc(int size)
{

	return syscall1(SYS_MALLOC, size);
}

void free(void * mem_addr)
{
	syscall1(SYS_FREE, mem_addr);
}

int ssuread()
{
	return syscall0(SYS_SSUREAD);
}

void shutdown(void)
{
	syscall0(SYS_SHUTDOWN);
}
