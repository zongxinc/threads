#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <time.h>
#include <assert.h>

#define THREAD_COUNT 4

pthread_barrier_t mybarrier;
int ret;
int waitVals[THREAD_COUNT+1];
int waitValsI=0;

void* threadFn(void *id_ptr) {
  int thread_id = *(int*)id_ptr;
  int wait_sec = 1 + rand() % 5;
  printf(">> thread %d: Wait for %d seconds.\n", thread_id, wait_sec);
  sleep(wait_sec);
  printf(">> thread %d: I'm ready...\n", thread_id);

  ret = pthread_barrier_wait(&mybarrier);
  waitVals[waitValsI++] = ret;
  printf(">> thread %d: ret=%d\n", thread_id, ret);

  printf(">> thread %d: going!\n", thread_id);
  return NULL;
}

/*
 * 
 */
int main(int argc, char** argv) {
    printf("NOTE: PTHREAD_BARRIER_SERIAL_THREAD=%d\n", PTHREAD_BARRIER_SERIAL_THREAD);

    int i;
    pthread_t ids[THREAD_COUNT];
    int short_ids[THREAD_COUNT];

    srand(time(NULL));
    ret = pthread_barrier_init(&mybarrier, NULL, THREAD_COUNT + 1);
    assert(ret == 0);

    for (i=0; i < THREAD_COUNT; i++) {
      short_ids[i] = i+1;
      pthread_create(&ids[i], NULL, threadFn, &short_ids[i]);
    }

    printf(">> main() is ready.\n");

    ret = pthread_barrier_wait(&mybarrier);
    printf(">> main(): ret=%d\n", ret);
    waitVals[waitValsI++] = ret;

    printf(">> main() is going!\n");

    // Add this sleep here so we can see which of the threads returns PTHREAD_BARRIER_SERIAL_THREAD at end of barrier
    sleep(3);

    ret = pthread_barrier_destroy(&mybarrier);
    assert(ret == 0);

    // Add the elements of waitVals. If the sum is PTHREAD_BARRIER_SERIAL_THREAD, return from barrier_wait() is a success.
    int sum = 0;
    for(int i=0; i<THREAD_COUNT+1; i++){
      sum += waitVals[i];
    }
    assert(sum == PTHREAD_BARRIER_SERIAL_THREAD);

    return (EXIT_SUCCESS);
}