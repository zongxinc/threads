#include "ec440threads.h"
#include <pthread.h>
#include <stdlib.h>
#include <stdbool.h>
#include <setjmp.h>

/* You can support more threads. At least support this many. */
#define MAX_THREADS 128

/* Your stack should be this many bytes in size */
#define THREAD_STACK_SIZE 32767

/* Number of microseconds between scheduling events */
#define SCHEDULER_INTERVAL_USECS (50 * 1000)

/* Extracted from private libc headers. These are not part of the public
 * interface for jmp_buf.
 */
#define JB_RBX 0
#define JB_RBP 1
#define JB_R12 2
#define JB_R13 3
#define JB_R14 4
#define JB_R15 5
#define JB_RSP 6 //exit
#define JB_PC 7

/* thread_status identifies the current state of a thread. You can add, rename,
 * or delete these values. This is only a suggestion. */
enum thread_status
{
	TS_EXITED,
	TS_RUNNING,
	TS_READY
};

/* The thread control block stores information about a thread. You will
 * need one of this per thread.
 */
struct thread_control_block {
	pthread_t threadID;
	jmp_buf current_buf;
	unnsigned long* stack; //look up pointer arithemitic
	enum thread_status status;
};
//
struct control_threads {
	int current;
	int active;
	struct thread_control_block mythreads[MAX_THREADS];
	int t_num;
}

struct control_threads mycontrol;

static void schedule(int signal)
{
	setjmp(mycontrol->mythreads[mycontrol->current % MAX_THREADS]->current_buf);
	mycontrol->mythreads[mycontrol->current % MAX_THREADS]->status = TS_READY;
	mycontrol->current++;
	while(mycontrol->mythread[mycontrol->current % MAX_THREADS]->status != TS_READY)
	{
		mycontrol->current++;
	}
	mycontrol->mythreads[mycontrol->current % MAX_THREADS]->status = TS_RUNNING;
	longjmp(mycontrol->mythreads[mycontrol->current % MAX_THREADS]->current_buf, 1);
	/* TODO: implement your round-robin scheduler 
	 * 1. Use setjmp() to update your currently-active thread's jmp_buf
	 *    You DON'T need to manually modify registers here.
	 * 2. Determine which is the next thread that should run
	 * 3. Switch to the next thread (use longjmp on that thread's jmp_buf)
	 */
}

static void scheduler_init()
{
	mycontrol -> current = 0;
	mycontrol -> active = 0;
	mycontrol -> mythreads = malloc(MAX_THREADS * sizeof(struct thread_control_block));
	mycontrol -> t_num = 0; 

	struct thread_control_block *tcb = malloc(sizeof(struct thread_control_block));

	tcb -> stack = malloc(THREAD_STACK_SIZE/* sizeof(unsigned long)*/);
	tcb -> status = TS_READY;
	// unsigned long *exit_ptr = (unsigned long*)(tcb -> stack + (THREAD_STACK_SIZE/sizeof(unsigned long) - 1));
	// *exit_ptr = (unsigned long) pthread_exit
	tcb -> threadID = pthread_self();
	setjmp(tcb->current_buf);

	struct sigaction act;
	act.sa_handler = schedule;
	act.sa_flags = SA_NODEFER;
	sigaction(SIGALRM, &act, NULL)
	ualarm(50, 50)
	/* TODO: do everything that is needed to initialize your scheduler. For example:
	 * - Allocate/initialize global threading data structures
	 * - Create a TCB for the main thread. Note: This is less complicated
	 *   than the TCBs you create for all other threads. In this case, your
	 *   current stack and registers are already exactly what they need to be!
	 *   Just make sure they are correctly referenced in your TCB.
	 * - Set up your timers to call schedule() at a 50 ms interval (SCHEDULER_INTERVAL_USECS)
	 */
}

int thread_count = 0
int pthread_create(
	pthread_t *thread, const pthread_attr_t *attr,
	void *(*start_routine) (void *), void *arg)
{
	// Create the timer and handler for the scheduler. Create thread 0.
	static bool is_first_call = true;
	if (is_first_call)
	{
		is_first_call = false;
		scheduler_init();
	}
	if (thread_count > MAX_THREADS)
	{
		printf("too many threads");
		return 1;
	}
	struct thread_control_block *tcb = malloc(sizeof(struct thread_control_block));

	tcb -> stack = (unsigned long*)malloc(THREAD_STACK_SIZE/* sizeof(unsigned long)*/);
	tcb -> status = TS_READY;
	unsigned long *exit_ptr = (unsigned long*)(tcb -> stack + (THREAD_STACK_SIZE/sizeof(unsigned long) - 1));
	*exit_ptr = (unsigned long) pthread_exit
	tcb -> threadID = pthread_self();
	tcb -> current_buf._jmpbuf[JB_PC] = ptr_mangle((unsigned long)start_thunk);
	tcb -> current_buf._jmpbuf[JB_RSP] = ptr_mangle((unsigned long)exit);
	tcb -> current_buf._jmpbuf[JB_R12] = (unsigned long)start_routine;
	tcb -> current_buf._jmpbuf[JB_R13] = (unsigned long)arg;
	setjmp(tcb->current_buf);

	mycontrol -> mythreads[mycontrol -> t_num] = tcb; //assigned correct?
	mycontrol -> t_num += 1;





	/* TODO: Return 0 on successful thread creation, non-zero for an error.
	 *       Be sure to set *thread on success.
	 * Hints:
	 * The general purpose is to create a TCB:
	 * - Create a stack.
	 * - Assign the stack pointer in the thread's registers. Important: where
	 *   within the stack should the stack pointer be? It may help to draw
	 *   an empty stack diagram to answer that question.
	 * - Assign the program counter in the thread's registers.
	 * - Wait... HOW can you assign registers of that new stack? 
	 *   1. call setjmp() to initialize a jmp_buf with your current thread
	 *   2. modify the internal data in that jmp_buf to create a new thread environment
	 *      env->__jmpbuf[JB_...] = ...
	 *      See the additional note about registers below
	 *   3. Later, when your scheduler runs, it will longjmp using your
	 *      modified thread environment, which will apply all the changes
	 *      you made here.
	 * - Remember to set your new thread as TS_READY, but only  after you
	 *   have initialized everything for the new thread.
	 * - Optionally: run your scheduler immediately (can also wait for the
	 *   next scheduling event).
	 */
	/*
	 * Setting registers for a new thread:
	 * When creating a new thread that will begin in start_routine, we
	 * also need to ensure that `arg` is passed to the start_routine.
	 * We cannot simply store `arg` in a register and set PC=start_routine.
	 * This is because the AMD64 calling convention keeps the first arg in
	 * the EDI register, which is not a register we control in jmp_buf.
	 * We provide a start_thunk function that copies R13 to RDI then jumps
	 * to R12, effectively calling function_at_R12(value_in_R13). So
	 * you can call your start routine with the given argument by setting
	 * your new thread's PC to be ptr_mangle(start_thunk), and properly
	 * assigning R12 and R13.
	 *
	 * Don't forget to assign RSP too! Functions know where to
	 * return after they finish based on the calling convention (AMD64 in
	 * our case). The address to return to after finishing start_routine
	 * should be the first thing you push on your stack.
	 */
	return -1;
}

void pthread_exit(void *value_ptr)
{
	free(mycontrol->mythreads[mycontrol->current])
	// ？？？？？
		/* TODO: Exit the current thread instead of exiting the entire process.
	 * Hints:
	 * - Release all resources for the current thread. CAREFUL though.
	 *   If you free() the currently-in-use stack then do something like
	 *   call a function or add/remove variables from the stack, bad things
	 *   can happen.
	 * - Update the thread's status to indicate that it has exited
	 */
	exit(1);
}

pthread_t pthread_self(void)
{

	/* TODO: Return the current thread instead of -1
	 * Hint: this function can be implemented in one line, by returning
	 * a specific variable instead of -1.
	 */
	return -1;
}

/* Don't implement main in this file!
 * This is a library of functions, not an executable program. If you
 * want to run the functions in this file, create separate test programs
 * that have their own main functions.
 */
