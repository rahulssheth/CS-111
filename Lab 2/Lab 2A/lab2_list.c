#include <pthread.h>
#include <stdio.h>
#include <getopt.h>
#include <errno.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <signal.h>


#include "SortedList.h"
#include <unistd.h> 
int curLength = 1;
int isMutex = 0;
int isSpin = 0; 
int yieldInsert = 0;
int yieldLookup = 0;
int yieldDelete = 0; 
long num_iterations = 1;
long num_threads = 1;
pthread_mutex_t globalMutex;
SortedList_t* globalList; 
volatile int lockVal = 0; 
 char* randomChars = "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789"; 
void catch_alarm() {
   
  char* errorString = "SIGSEGV Error";
  fprintf(stderr, "%s", errorString);
  exit(1);
}
 

void insertion(SortedListElement_t* element) {
   
  if (yieldInsert) {
    opt_yield = INSERT_YIELD; 
  }
   
  SortedList_insert(globalList, element);
  
   

}

void insertionMutex(SortedListElement_t* element) {
  pthread_mutex_lock(&globalMutex);
  if (yieldInsert) {
    opt_yield =	INSERT_YIELD;
  }
  
  SortedList_insert(globalList, element); 
  pthread_mutex_unlock(&globalMutex);
}

void insertionSpin(SortedListElement_t* element) {
  
  while (__sync_lock_test_and_set(&lockVal, 1)) {
  }
  if (yieldInsert) {
    opt_yield = INSERT_YIELD;
    }
  
   
    SortedList_insert(globalList, element);
     
  
  __sync_lock_release(&lockVal);
  
}

void deletion(SortedListElement_t* element) {
  if (yieldDelete) {
    opt_yield =	DELETE_YIELD;
  }
  int retVal = SortedList_delete(element);
  if (retVal == 1) {
    char* errorString = "Failed Deletion ";
    fprintf(stderr, "%s", errorString); 
    exit(1);
  }
}

void deletionMutex(SortedListElement_t* element) {
  pthread_mutex_lock(&globalMutex);
  if (yieldDelete) {
    opt_yield = DELETE_YIELD;
  }
  int retVal = SortedList_delete(element);
  if (retVal == 1) {
    char* errorString = "Error with deletion with Mutex"; 
      fprintf(stderr, "%s", errorString); 
    exit(1);
  }
  pthread_mutex_unlock(&globalMutex);

}
 
void deletionSpin(SortedListElement_t* element) {
      
  while (__sync_lock_test_and_set(&lockVal, 1)) {
  }
   if (yieldDelete) {
    opt_yield = DELETE_YIELD;
  }   
    
  int retVal = SortedList_delete(element);
  if (retVal == 1) {
    char* errorString = "Error Deletion with Spin"; 
    fprintf(stderr, "%s", errorString);  
    exit(1);
  }
    

  __sync_lock_release(&lockVal);

}


void lookup(SortedListElement_t* element) {
  if (yieldLookup) {
    opt_yield = LOOKUP_YIELD;
  }
  
   const char* curKey = element->key;
   SortedList_lookup(globalList, curKey); 
     

}

void lookupMutex(SortedListElement_t* element) {
  pthread_mutex_lock(&globalMutex);
  if (yieldLookup) {
    opt_yield = LOOKUP_YIELD;
  }
  
     const char* curKey = element->key;

  SortedList_lookup(globalList, curKey);
  pthread_mutex_unlock(&globalMutex);

}
void lookupSpin(SortedListElement_t* element) {
   
  while (__sync_lock_test_and_set(&lockVal, 1)) {
  }
    if  (yieldLookup) {
    opt_yield = LOOKUP_YIELD;
  } 
  
  
  const char* curKey =	element->key;
   SortedList_lookup(globalList, curKey);
    
  
      __sync_lock_release(&lockVal);

   
}

void length() {
  if (yieldLookup) {
    opt_yield = LOOKUP_YIELD;
  }
    curLength = SortedList_length(globalList);
    if (curLength == -1) {
      char* errorString = "List Corrupted! Found in SpinLength";
      fprintf(stderr, "%s", errorString);
      exit(1); 
    }
}
void mutexLength() {
  pthread_mutex_lock(&globalMutex);
  if (yieldLookup) {
    opt_yield = LOOKUP_YIELD;
  }
    curLength = SortedList_length(globalList);
    if (curLength == -1) {
      char* errorString = "List Corrupted! Found in SpinLength";
      fprintf(stderr, "%s", errorString);
      exit(1); 
    }
  pthread_mutex_unlock(&globalMutex);
}

void spinLength() {
  while (__sync_lock_test_and_set(&lockVal, 1)) {
  }
    if (yieldLookup) {
    opt_yield = LOOKUP_YIELD;
  }
    curLength = SortedList_length(globalList);
    if (curLength == -1) {
      char* errorString = "List Corrupted! Found in SpinLength";
      fprintf(stderr, "%s", errorString);
      exit(1); 
    }
  
      __sync_lock_release(&lockVal);
}
  
void* threadFunc(void* arg) {
  SortedListElement_t *cur = ((SortedListElement_t *)arg);
  //Insert all of them
  
  for (int i = 0; i < num_iterations; i++) {
     
    if (isMutex) {
      insertionMutex(&cur[i]); 
    } else if (isSpin) {
      insertionSpin(&cur[i]);
    } else {
      insertion(&cur[i]);
    }
  //Get the List Length
  }
   
  if (isMutex) { 
    mutexLength();
  } else if (isSpin) {
    spinLength();
  } else {
    length();
  } 
  
  
  //Delete them all 
  for (int i = 0; i < num_iterations; i++) {
    if (isMutex) {
      lookupMutex(&cur[i]);
      deletionMutex(&cur[i]);
    } else if (isSpin) {
      lookupSpin(&cur[i]);
      deletionSpin(&cur[i]);
    } else {
      lookup(&cur[i]); 
      deletion(&cur[i]);
    }


  }
  return 0;  
  

}
int main(int argc, char** argv) {
   
   
  //Variable Declaration. Options struct read from command line, counter, variables for options, num_threads and iterations, and start/endTime Specs           
  signal (SIGSEGV, catch_alarm);
  errno = 0;
  static struct option const long_options[] = {
    {"threads", required_argument , NULL, 't'},
    {"iterations", required_argument, NULL, 'i'},
    {"yield", required_argument, NULL, 'y'},
    {"sync", required_argument, NULL, 's'},
    {0, 0, 0, 0}
  };
  
  int opt, index;
   
  struct timespec start, end;
  
  char* syncOpts =  "none";
  char* yieldOpts = "none"; 



  //Extract Options                                                             

  while ((opt = getopt_long(argc, argv, "t:i:y:s:", long_options, &index)) != -1) {

    switch (opt) {
    case 't':
      num_threads = atoi(optarg);
      break;
    case 'i':
      num_iterations = atoi(optarg);
      break;
    case 'y':
      yieldOpts = optarg; 
      if (!strcmp(optarg, "i")) {
	yieldInsert = 1;
	
	} else if (!strcmp(optarg, "d")) {
	yieldDelete = 1; 
	} else if (!strcmp(optarg, "il")) {
	yieldInsert = 1;
	yieldLookup = 1;
	} else if (!strcmp(optarg, "dl")) {
	yieldDelete = 1;
	yieldLookup = 1;
      } else if (!strcmp(optarg, "idl")) {
	yieldInsert = 1;
	yieldDelete = 1;
	yieldLookup = 1; 
      } else {
	fprintf(stderr, "error unrecognized options for yield");
	exit(1);
      }
      break;
    case 's':
      syncOpts = optarg; 
      switch (*optarg) {
      case 'm':
        pthread_mutex_init(&globalMutex, NULL);
        isMutex = 1;
	break;
      case 's':
	isSpin = 1;
	break;
      default:
	fprintf(stderr, "Unrecognized option for sync");
	exit(1); 

      }
      break;
      default:
	fprintf(stderr, "error not found args"); 
	exit(1);
    }
   
  }


    
  
  SortedListElement_t arrayOfElements[num_threads][num_iterations]; 
  
  globalList = malloc(sizeof(SortedListElement_t)); 
  for (int i = 0; i < num_threads; i++) {
    for (int j = 0; j < num_iterations; j++) {
          
    arrayOfElements[i][j].key = malloc(sizeof(const char));
    const char A = (const char) randomChars[rand() % 36];    
    
	     arrayOfElements[i][j].key=&A;
	      
     
    } 
  }
  pthread_t threads[num_threads];
  void* status;
    clock_gettime(CLOCK_MONOTONIC, &start);

  for (int i = 0; i < num_threads; i++) {
     
    int retVal = pthread_create(&threads[i], NULL, threadFunc, arrayOfElements[i]);
     
    if (retVal < 0) {
      fprintf(stderr, "Error with creation of threads");
      char* errorString = strerror(errno);
      fprintf(stderr, "%s", errorString);
      exit(1); 
    }
  }
  for (int i = 0; i < num_threads; i++) {

    int retVal = pthread_join(threads[i], &status);
    if (retVal < 0) {
      fprintf(stderr, "Error joining the threads");
      char* errorString= strerror(errno);
      fprintf(stderr, "%s", errorString);
      exit(1); 
    }


  }

    clock_gettime(CLOCK_MONOTONIC, &end);
    
    
    if (isMutex) {
    mutexLength();
  } else if (isSpin) {
    spinLength();
  } else {
    length();
  }
    if (curLength != 0) {
      char* errorString = "Error! List is not empty";
      fprintf(stderr, "%s", errorString); 
      exit(2);
    } 
    free(globalList); 
    long  timeSlice = end.tv_nsec - start.tv_nsec;
    int  num_operations = num_threads * num_iterations * 3;
    int  avgTimeOperations = timeSlice / num_operations;
    
    char* addString = "list";
    printf(addString); printf("-"); printf(yieldOpts); printf("-"); printf(syncOpts); 
    printf(",%ld,%ld,1,%d, %ld, %d", num_threads, num_iterations,num_operations, timeSlice, avgTimeOperations);
    printf("\n"); 
    //yieldInsert, yieldDelete, yieldLookup
    //isMutex, isSpin
    

  

}
