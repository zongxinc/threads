#include "ec440threads.h"
#include <pthread.h>
#include <stdlib.h>
#include <stdbool.h>
#include <setjmp.h>
#include <sys/types.h>
#include <signal.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>

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
	TS_READY,
	TS_BLOCK
};

/* The thread control block stores information about a thread. You will
 * need one of this per thread.
 */
struct thread_control_block {
	int threadID;
	jmp_buf current_buf;
	unsigned long* stack; //look up pointer arithemitic
	enum thread_status status;
};
//
struct control_threads {
	int current;
	struct thread_control_block mythreads[MAX_THREADS];
	int t_num;
};

struct control_threads mycontrol;

static void schedule(int signal)
{
	// printf("%s\n", "mycontrol.current!!!!!!!!!!!!!!!!!!!!!!!!");
	if (setjmp(mycontrol.mythreads[mycontrol.current].current_buf) == 0)
	{
		// printf("%s, %d\n", "setjmp current thread", mycontrol.current);
		if ((mycontrol.mythreads[mycontrol.current]).status == TS_RUNNING)
			mycontrol.mythreads[mycontrol.current].status = TS_READY;
		mycontrol.current = (mycontrol.current + 1) % MAX_THREADS;
		// printf("%d\n", mycontrol.current);
		while(mycontrol.mythreads[mycontrol.current].status != TS_READY)
		{
			mycontrol.current = (mycontrol.current + 1) % MAX_THREADS;

		}
		// printf("%s, %d\n", "next one ready is ", mycontrol.current);
		mycontrol.mythreads[mycontrol.current].status = TS_RUNNING;
		longjmp(mycontrol.mythreads[mycontrol.current].current_buf, 1);
	}
	else {
		// printf("%s\n", "setjmp error");
		mycontrol.mythreads[mycontrol.current].status = TS_RUNNING;
	}
	
	/* TODO: implement your round-robin scheduler 
	 * 1. Use setjmp() to update your currently-active thread's jmp_buf
	 *    You DON'T need to manually modify registers here.
	 * 2. Determine which is the next thread that should run
	 * 3. Switch to the next thread (use longjmp on that thread's jmp_buf)
	 */
}

static void scheduler_init()
{
	mycontrol.current = 0;
	//struct thread_control_block* mythread = malloc(sizeof(struct thread_control_block));
	mycontrol.t_num = 0;
	
	for (int i = 0; i < MAX_THREADS; i++)
	{
		mycontrol.mythreads[i].status = TS_EXITED;
		mycontrol.mythreads[i].threadID = i;
	}

	mycontrol.mythreads[0].stack = NULL;//malloc(THREAD_STACK_SIZE/* sizeof(unsigned long)*/);
	mycontrol.mythreads[0].status = TS_RUNNING;
	// unsigned long *exit_ptr = (unsigned long*)(tcb -> stack + (THREAD_STACK_SIZE/sizeof(unsigned long) - 1));
	// *exit_ptr = (unsigned long) pthread_exit
	mycontrol.mythreads[0].threadID = 0;
	mycontrol.t_num++;
	setjmp(mycontrol.mythreads[0].current_buf);


	struct sigaction act = {{0}};
	act.sa_handler = schedule;
	sigemptyset(&act.sa_mask);
	act.sa_flags = SA_NODEFER;
	sigaction(SIGALRM, &act, NULL);
	ualarm(SCHEDULER_INTERVAL_USECS, SCHEDULER_INTERVAL_USECS);
	/* TODO: do everything that is needed to initialize your scheduler. For example:
	 * - Allocate/initialize global threading data structures
	 * - Create a TCB for the main thread. Note: This is less complicated
	 *   than the TCBs you create for all other threads. In this case, your
	 *   current stack and registers are already exactly what they need to be!
	 *   Just make sure they are correctly referenced in your TCB.
	 * - Set up your timers to call schedule() at a 50 ms interval (SCHEDULER_INTERVAL_USECS)
	 */
}

int thread_count = 0;
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
	// printf("%s, %d\n", "main done", mycontrol.t_num);
	struct thread_control_block new_thread;
	setjmp(new_thread.current_buf);

	new_thread.stack = (unsigned long*)malloc(THREAD_STACK_SIZE/* sizeof(unsigned long)*/);
	memset(new_thread.stack, 0, THREAD_STACK_SIZE);

	unsigned long *exit_ptr = (unsigned long*)(new_thread.stack + (THREAD_STACK_SIZE/sizeof(unsigned long) - 1));
	unsigned long exit_add = (uintptr_t) &pthread_exit;
	memcpy(exit_ptr, &exit_add, sizeof(exit_add));
	// printf("%p\n", pthread_exit);
	// printf("%lx\n", exit_add);
	// *exit_ptr = (unsigned long) pthread_exit;
	new_thread.current_buf[0].__jmpbuf[JB_PC] = ptr_mangle((unsigned long)start_thunk);
	new_thread.current_buf[0].__jmpbuf[JB_RSP] = ptr_mangle((unsigned long)exit_ptr);
	new_thread.current_buf[0].__jmpbuf[JB_R12] = (unsigned long)start_routine;
	// printf("%lx\n", *(unsigned long*) ptr_demangle(new_thread.current_buf[0].__jmpbuf[JB_RSP]));
	new_thread.current_buf[0].__jmpbuf[JB_R13] = (unsigned long)arg;
	new_thread.threadID = mycontrol.t_num;
	*thread = mycontrol.t_num;
	// printf("%s, %d\n", "new thread id assigned", new_thread.threadID);
	new_thread.status = TS_READY;
	// printf("demangle pc is 0x%081lx\n, %d", ptr_demangle(new_thread.current_buf[0].__jmpbuf[JB_R13]), mycontrol.t_num);
	mycontrol.mythreads[mycontrol.t_num] = new_thread;
	mycontrol.t_num += 1;


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
	return 0;
}

void pthread_exit(void *value_ptr)
{
	// free(mycontrol.mythreads[mycontrol.current].stack);
	mycontrol.mythreads[mycontrol.current].status = TS_EXITED;
	// printf("%s\n", "exited!!!!!!");
	// bool next = true;
	for (int i = 0; i < MAX_THREADS; i++)
	{
		if (mycontrol.mythreads[i].status != TS_EXITED)
		{
			// next = false;
			schedule(0);
			printf("%d, ", mycontrol.current);
			break;
		}
	}
	exit(0);
	// if (next != 1)
	// {
	// 	printf("%d%s\n", mycontrol.current, " done");
	// 	schedule(0);
	// }
	// else
	// {
	// 	printf("%d, %d\n", next, mycontrol.current);
	// 	exit(0);
	// }
	__builtin_unreachable();
	// printf("%s\n", "exited!!!!!!");
	// __builtin_unreachable();
	// printf("%s\n", "exited!!!!!!");
		/* TODO: Exit the current thread instead of exiting the entire process.
	 * Hints:
	 * - Release all resources for the current thread. CAREFUL though.
	 *   If you free() the currently-in-use stack then do something like
	 *   call a function or add/remove variables from the stack, bad things
	 *   can happen.
	 * - Update the thread's status to indicate that it has exited
	 */
	// for (int i = 0; i < MAX_THREADS; i++)
	// {
	// 	if (mycontrol.mythreads[i].status != TS_EXITED)
	// 		break;
	// 	if (i == MAX_THREADS - 1)
	// 		exit(0);
	// }
	// if (mycontrol.current == 0)
	// {
	// 	bool alldone = false;
	// 	while(!alldone)
	// 	{
	// 		for (int i = 0; i < MAX_THREADS; i++)
	// 		{
	// 			if (mycontrol.mythreads[i].status != TS_EXITED)
	// 				break;
	// 			if (i == MAX_THREADS - 1)
	// 				alldone = true;
	// 		}
	// 	}
	// exit(0);
	// }
}

pthread_t pthread_self(void)
{

	/* TODO: Return the current thread instead of -1
	 * Hint: this function can be implemented in one line, by returning
	 * a specific variable instead of -1.
	 */
	// printf("%s, %d\n", "my id is ", mycontrol.mythreads[mycontrol.current].threadID);
	return mycontrol.mythreads[mycontrol.current].threadID;
}

/* Don't implement main in this file!
 * This is a library of functions, not an executable program. If you
 * want to run the functions in this file, create separate test programs
 * that have their own main functions.
 */
enum mutex_status
{
	LOCK,
	UNLOCK
};
// Your lock function should disable the timer that calls your schedule routine, and unlock should re-enable the timer. You can use the sigprocmask function to this end (one function using SIG_BLOCK, the other using SIG_UNBLOCK, with a mask on your alarm signal). Use these functions to prevent your scheduler from running when your threading library is internally in a critical section (users of your library will use barriers and mutexes for critical sections that are external to your library).
// prevent race cases in threading 
static void lock()
{
	sigset_t block; 
	sigemptyset(&block);
    sigaddset(&block, SIGALRM);
    sigprocmask(SIG_BLOCK, &block, NULL);
}

static void unlock()
{
	sigset_t block; 
	sigemptyset(&block);
    sigaddset(&block, SIGALRM);
    sigprocmask(SIG_UNBLOCK, &block, NULL);
}
struct mymutex
{
	enum mutex_status state;
	int tblock[128];
	int holder;
};


int pthread_mutex_init(
	pthread_mutex_t *restrict mutex,
	const pthread_mutexattr_t *restrict attr)
{
	// The pthread_mutex_init() function initializes a given mutex. The attr argument is unused in this assignment (we will always test it with NULL). Behavior is undefined when an already-initialized mutex is re-initialized. Always return 0.
	// mutex = PTHREAD_MUTEX_INITIALIZER;
	struct mymutex* myMutex = (struct mymutex*) malloc(sizeof(struct mymutex));
	myMutex->state = UNLOCK;
	for (int i = 0; i < 128; i++)
	{
		myMutex->tblock[i] = 130;
	}
	mutex->__align = (long) myMutex;

	return 0;
}

int pthread_mutex_destroy(
	pthread_mutex_t *mutex)
{
	// struct mymutex* myMutex = (struct mymutex*) (mutex->__align);

	free((void *) mutex->__align);
	return 0;
	// The pthread_mutex_destroy() function destroys the referenced mutex. Behavior is undefined when a mutex is destroyed while a thread is currently blocked on, or when destroying a mutex that has not been initialized. Behavior is undefined when locking or unlocking a destroyed mutex, unless it has been re-initialized by pthread_mutex_init. Return 0 on success.
}

int pthread_mutex_lock(pthread_mutex_t *mutex)
{
	struct mymutex* myMutex = (struct mymutex*) (mutex->__align);
	if (myMutex->state == UNLOCK)
	{
		myMutex->holder = mycontrol.current;
		myMutex->state = LOCK;
		printf("%d is getting the mutex\n", mycontrol.current);
	}	
	else
	{
		lock();
		mycontrol.mythreads[mycontrol.current].status = TS_BLOCK;
		printf("blocking %d\n", mycontrol.current);
		unlock();
		int empty = 0;
		bool find = 0;
		for (int i = 0; i < 128; i++)
		{
			if (myMutex->tblock[i] == 130)
			{
				empty = i;
				find = 1;
				break;
			}

		}			
		if (find == 0)
		{
			printf("%s\n", "tblock too small");
		}
		else
		{
			printf("storing mutex: %d at %d\n", mycontrol.current, empty);
			myMutex->tblock[empty] = mycontrol.current;
		}
		schedule(0);


	}
	// The pthread_mutex_lock() function locks a referenced mutex. If the mutex is not already locked, the current thread acquires the lock and proceeds. If the mutex is already locked, the thread blocks until the mutex is available. If multiple threads are waiting on a mutex, the order that they are awoken is undefined. Return 0 on success, or an error code otherwise.
	// 
	return 0;
}

int pthread_mutex_unlock(pthread_mutex_t *mutex)
{
	struct mymutex* myMutex = (struct mymutex*) (mutex->__align);

	for (int i = 0; i < 128; i++)
	{
		printf("now at %d for %d\n", i, mycontrol.current);
		if (myMutex->tblock[i] != 130)
		{
			lock();
			printf("unblocking %d\n", myMutex->tblock[i]);
			mycontrol.mythreads[myMutex->tblock[i]].status = TS_READY;
			unlock();
			myMutex->tblock[i] = 130;
			return 0;


		}
	}
	myMutex->state = UNLOCK;
	printf("all freed\n");
	return 0;
	// The pthread_mutex_unlock() function unlocks a referenced mutex. If another thread is waiting on this mutex, it will be woken up so that it can continue running. Note that when that happens, the woken thread will finish acquiring the lock. Return 0 on success, or an error code otherwise.
}

struct mybarrier
{
	unsigned count;
	unsigned now;
	int thread_list[128];
};

int pthread_barrier_init(
pthread_barrier_t *restrict barrier,
const pthread_barrierattr_t *restrict attr,
unsigned count)
{
	if (count == 0)
		return -1;
	struct mybarrier* myBarrier = (struct mybarrier*) malloc(sizeof(struct mybarrier));
	myBarrier->count = count;
	myBarrier->now = 1;
	for (int i = 0; i < 128; i++)
	{
		myBarrier->thread_list[i] = 130;
	}
	// memcpy(barrier, myBarrier, sizeof(mybarrier));
	barrier->__align = (long) myBarrier;

	return 0;
	// The pthread_barrier_init() function initializes a given barrier. The attr argument is unused in this assignment (we will always test it with NULL). The count argument specifies how many threads must enter the barrier before any threads can exit the barrier. Return 0 on success. It is an error if count is equal to zero (return EINVAL). Behavior is undefined when an already-initialized barrier is re-initialized.
}

int pthread_barrier_destroy(pthread_barrier_t *barrier)
{
	// struct mybarrier* myBarrier = (struct mybarrier*) (barrier->__align);
	free((void *) barrier->__align);
	return 0;
	// The pthread_barrier_destroy() function destroys the referenced barrier. Behavior is undefined when a barrier is destroyed while a thread is waiting in the barrier or when destroying a barrier that has not been initialized. Behavior is undefined when attempting to wait in a destroyed barrier, unless it has been re-initialized by pthread_barrier_init. Return 0 on success.
}

int pthread_barrier_wait(pthread_barrier_t *barrier)
{
	struct mybarrier* myBarrier = (struct mybarrier*) (barrier->__align);
	printf("now have %d, count is %d\n", myBarrier->now, myBarrier->count);
	if (myBarrier->now < myBarrier->count)
	{
		lock();
		mycontrol.mythreads[mycontrol.current].status = TS_BLOCK;
		unlock();
		myBarrier->thread_list[myBarrier->now] = mycontrol.current;
		printf("storing %d at %d\n", mycontrol.current, myBarrier->now);
		myBarrier->now++;
		schedule(0);
	}
	else
	{
		for (int i = 0; i < 128; i++)
		{
			if (myBarrier->thread_list[i] != 130)
			{
				printf("free %d at %d\n", myBarrier->thread_list[i], i);
				lock();
				mycontrol.mythreads[myBarrier->thread_list[i]].status = TS_READY;
				unlock();
				myBarrier->thread_list[i] = 130;
				myBarrier->now--;
			}
		}
	}
	if (mycontrol.current == 0)
		return PTHREAD_BARRIER_SERIAL_THREAD;
	return 0;
	// The pthread_barrier_wait() function enters the referenced barrier. The calling thread shall not proceed until the required number of threads (from count in pthread_barrier_init) have already entered the barrier. Other threads shall be allowed to proceed while this thread is in a barrier (unless they are also blocked for other reasons). Upon exiting a barrier, the order that the threads are awoken is undefined. Exactly one of the returned threads shall return PTHREAD_BARRIER_SERIAL_THREAD (it does not matter which one). The rest of the threads shall return 0.
}



