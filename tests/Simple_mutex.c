#include <stdio.h>
#include <stdint.h>
#include <pthread.h>
#include <stdlib.h>
//#include "../threads.c"
/* How many threads (aside from main) to create */
#define THREAD_CNT 3
/* pthread_join is not implemented in homework 2 */
#define HAVE_PTHREAD_JOIN 0

 void* trial(void *arg) {
  unsigned long int i = (unsigned long int)arg;
  printf("current thread: %ld\n", pthread_self());
  printf("Num inputted * 2: %ld\n", i*2);
  return NULL;
 }

pthread_mutex_t mutex;
int main(int argc, char **argv) {
 pthread_mutex_init(&mutex, NULL);
 pthread_t threads[THREAD_CNT];
 for (int i = 0; i < THREAD_CNT; i++) {
  pthread_mutex_lock(&mutex);
  pthread_create(&threads[i], NULL, &trial, (void*)(intptr_t)i);
  if (i % 2 == 0) pthread_mutex_unlock(&mutex);
 }
 for (int i = 0; i < 100000000; i++)
 {}
 // for (int i = 0; i < THREAD_CNT; i++) {
 //  pthread_join(threads[i], NULL);
 // }
 return 0;
}