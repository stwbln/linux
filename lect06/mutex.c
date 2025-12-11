??? from here until ???END lines may have been inserted/deleted
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

int acc = 0;

pthread_mutex_t mtx;

void *TaskCode(void *argument) {
   int tid;
   tid = *((int*) argument);
   pthread_mutex_lock(&mtx);
   for (int i = 0; i < 1000000; i++) {
      acc = acc + 1;
   }
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

int acc = 0;

void *TaskCode(void *argument) {
   int tid;
   tid = *((int*) argument);
   for (int i = 0; i < 1000000; i++) {
      acc = acc + 1;
   }

   return NULL;
}

int main(int argc, char *argv[]) {
   pthread_t threads[4];
   int args[4];
   int i;

   /* create all threads */
   for (i = 0; i < 4; i++) {
      args[i] = i;
      pthread_create(&threads[i], NULL, TaskCode, (void *)&args[i]);
   }

   /* wait for all threads to complete */
   for (i = 0; i < 4; i++)
      pthread_join(threads[i], NULL);
   return NULL;
}

int main(int argc, char *argv[]) {
   pthread_t threads[4];
   int args[4];
   int i;

   /* create all threads */
   for (i = 0; i < 4; i++) {
      args[i] = i;
      pthread_create(&threads[i], NULL, TaskCode, (void *)&args[i]);
   }

   /* wait for all threads to complete */
   for (i = 0; i < 4; i++)
      pthread_join(threads[i], NULL);

   return 0;
}
???END
