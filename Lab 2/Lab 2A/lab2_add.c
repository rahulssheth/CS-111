#include <pthread.h>
#include <stdio.h>
#include <getopt.h>
#include <errno.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>



int opt_yield = 0; 
long num_iterations = 1;
int isMutex = 0;
int isSpinLock = 0;
int isCAS = 0; 
pthread_mutex_t globalMutex;
 
//Basic add routine.  
void add(long long *pointer, long long value) {
  
  long long sum = *pointer + value;
  if (opt_yield) {
    sched_yield(); 
  }
  *pointer = sum; 

}

void addMutex(long long *pointer, long long value){
  pthread_mutex_lock(&globalMutex);
  long long sum = *pointer + value;
  if (opt_yield) {
    sched_yield();
  }
  *pointer = sum;
  pthread_mutex_unlock(&globalMutex); 

}

void addSpinlock(long long *pointer, long long value) {
  long long temp;
  do {
    temp = *pointer;
  } while (__sync_lock_test_and_set(pointer, temp, temp+value) != temp);
  __sync_lock_release(pointer);
  

}

void addCompAndSwap(long long *pointer, long long value) {
  long long temp;
  do {
    temp = *pointer;
  } while (__sync_val_compare_and_swap(pointer, temp, temp+value) != temp);
  


}
//Thread Func. Calls Add routine
//Needs the counter variable passed in from the threadFunc. 
void* threadFunc(void* arg) {
  for (int i = 0; i < num_iterations; i++) {
    if (isMutex) {
      addMutex(arg, 1);
    } else if (isSpinLock) {
      addSpinlock(arg, 1);
    } else if (isCAS) {
      addCompAndSwap(arg, 1); 
    } else {
    add(arg, 1);
    } 

  }
  for (int j = 0; j < num_iterations; j++) {

    if (isMutex) {
      addMutex(arg, -1);
    } else if (isSpinLock) {
      addSpinlock(arg, -1);
    } else if (isCAS) {
      addCompAndSwap(arg, -1);
    } else {
      add(arg, -1);
    }
     
  }
  return NULL;

}



int main(int argc, char* argv[]) {

  //Variable Declaration. Options struct read from command line, counter, variables for options, num_threads and iterations, and start/endTime Specs 
  errno = 0;
  static struct option const long_options[] = {
    {"threads", required_argument , NULL, 't'}, 
    {"iterations", required_argument, NULL, 'i'},
    {"yield", no_argument, NULL, 'y'},
    {"sync", required_argument, NULL, 's'}, 
    {0, 0, 0, 0}
  }; 
  long long counter = 0; 
  int opt, index; 
  long num_threads = 1;
   
  struct timespec start, end; 
  
  



  //Extract Options
  
  while ((opt = getopt_long(argc, argv, "t:i:", long_options, &index)) != -1) {

    switch (opt) {
    case 't':
      num_threads = atoi(optarg); 
      break;
    case 'i':
      num_iterations = atoi(optarg); 
      break; 
    case 'y':
      opt_yield = 1;
      break;
     
    case 's':
      switch (*optarg) {
      case 'm':
	pthread_mutex_init(&globalMutex, NULL);  
	isMutex = 1; 
	break; 
      case 's':
	isSpinLock = 1;
	break;
      case 'c':
	isCAS = 1; 
	break;
      default:
	break; 
      } 
      break;
    default: 
      fprintf(stderr, "Error. Unrecognized option");
      char* errorString = strerror(errno);
      fprintf(stderr, "%s", errorString);
      exit(1);



  }

  }
  //Start time Uncomment on linux server 
  clock_gettime(CLOCK_MONOTONIC, &start); 
     
  pthread_t threads[num_threads]; 
  void* status;
  for (int i = 0; i < num_threads; i++) {

    int retVal = pthread_create(&threads[i], NULL, threadFunc, &counter);
    if (retVal < 0) {
      fprintf(stderr, "Error with creation of threads");
      char* errorString = strerror(errno);
      fprintf(stderr, "%s", errorString); 
    }
  }
  for (int i = 0; i < num_threads; i++) {

    int retVal = pthread_join(threads[i], &status);
    if (retVal < 0) {
      fprintf(stderr, "Error joining the threads");
      char* errorString= strerror(errno);
      fprintf(stderr, "%s", errorString);

    }
    
			 
  }
  clock_gettime(CLOCK_MONOTONIC, &end);

  char* addString;
  //add-none: standard setup.
  if (isMutex) {
    if (opt_yield) {
      addString = "add-yield-m";
    } else {
      addString = "add-m";
    }
  } else if (isCAS) {

    if (opt_yield) {
      addString = "add-yield-c";
    } else {
      addString = "add-c";
    }
  } else if (isSpinLock) {
    if (opt_yield) {
      addString = "add-yield-s";
    } else {
      addString = "add-s";
    }
  } else {
  if (opt_yield) {
    addString = "add-yield-none";
  } else {
    addString = "add-none"; 
  }
  }
  long timeSlice = end.tv_nsec - start.tv_nsec; 
  long num_operations = num_threads * num_iterations * 2;
  long avgTimeOperations = timeSlice / num_operations;
  
  //name of test, number of threads, number of iterations, number of operations, total run time, average time per operation, final counter

  //Open a file using a because this will create it if doesn't exist and simply append to it if it does. 
  
  printf("%s",addString);
  printf(",%ld,%ld,%ld, %ld,%ld,%lld", num_threads, num_iterations, num_operations, timeSlice, avgTimeOperations, counter);
  printf("\n");   
      pthread_exit(NULL);
}
