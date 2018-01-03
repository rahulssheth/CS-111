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
int num_lists = 1;
int isMutex = 0;
int* perThreadTimes; 
SortedList_t** topHeader;
SortedListElement_t** AllElements;
char*** keys; 
pthread_mutex_t* topMutex;
volatile int* topLockVals; 
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
struct threadVals {

  SortedListElement_t* cur;
  int curThread;
}; 
void catch_alarm() {
   
  char* errorString = "SIGSEGV Error";
  fprintf(stderr, "%s", errorString);
  exit(1);
}


void multiList() {
   
  topHeader = (SortedList_t**)malloc(num_lists * sizeof(SortedList_t*));
  topMutex = (pthread_mutex_t*)malloc(num_lists * sizeof(pthread_mutex_t)); 
  topLockVals = (int*)malloc(num_lists * sizeof(int)); 
    
    for (int i = 0; i < num_lists; i++) {
      
      topHeader[i] = (SortedList_t*)malloc(sizeof(SortedList_t)); 
       
      topHeader[i][0].prev = NULL; 
      topHeader[i][0].next = NULL;
      
        const char c = (const char) randomChars[rand() % 35];
        topHeader[i][0].key = &c;
	pthread_mutex_init(&topMutex[i], NULL);
	topLockVals[i] = 0; 
    } 
    
      
}

 

void insertion(SortedListElement_t* element) {
        if (num_lists == 1) {
  
   
        SortedList_insert(globalList, element);
    } else {
      
	  int curKey = atoi(element->key);
        int list = (curKey + 128) % num_lists;
	
	SortedList_t* cur = topHeader[list]; 
	 SortedList_insert(cur, element);
    }
   

}

void insertionMutex(SortedListElement_t* element, int tid) {
  struct timespec start, end;  
  if (num_lists == 1) {
    clock_gettime(CLOCK_MONOTONIC, &start); 
  pthread_mutex_lock(&globalMutex);
    clock_gettime(CLOCK_MONOTONIC, &end);

  SortedList_insert(globalList, element); 
  pthread_mutex_unlock(&globalMutex);
  

  
  } else {
    
     
    int curKey = atoi(element->key);
    
    
    int list = (curKey + 128) % num_lists;
	 
	clock_gettime(CLOCK_MONOTONIC, &start);
        pthread_mutex_lock(&topMutex[list]);
	clock_gettime(CLOCK_MONOTONIC, &end);
	  
	  SortedList_insert(topHeader[list], element);
	  pthread_mutex_unlock(&topMutex[list]);
	   

  }
  perThreadTimes[tid] += ((end.tv_sec - start.tv_sec) * 1000000 + (end.tv_nsec - start.tv_nsec)); 
	
}

void insertionSpin(SortedListElement_t* element, int tid) {
  struct timespec start, end; 
  if (num_lists == 1) {
    clock_gettime(CLOCK_MONOTONIC, &start); 
  while (__sync_lock_test_and_set(&lockVal, 1)) {
  }
   clock_gettime(CLOCK_MONOTONIC, &end);

  
   
    SortedList_insert(globalList, element);
     
  
  __sync_lock_release(&lockVal);
  
  } else {
    int curKey = atoi(element->key);
    int list = (curKey + 128) % num_lists;
    clock_gettime(CLOCK_MONOTONIC, &start); 
    while (__sync_lock_test_and_set(&topLockVals[list], 1)) {
      }
      clock_gettime(CLOCK_MONOTONIC, &end);
     
      SortedList_insert(topHeader[list], element);
      __sync_lock_release(&topLockVals[list]);
       


      }
    perThreadTimes[tid] += ((end.tv_sec - start.tv_sec) * 1000000 + (end.tv_nsec - start.tv_nsec));

  
}

void deletion(SortedListElement_t* element) {
  
  
  int retVal = SortedList_delete(element);
  if (retVal == 1) {
    char* errorString = "Failed Deletion ";
    fprintf(stderr, "%s", errorString); 
    exit(2);
  }
}

void deletionMutex(SortedListElement_t* element, int tid) {
  struct timespec start, end;
  if (num_lists ==  1) {

    clock_gettime(CLOCK_MONOTONIC, &start);
    pthread_mutex_lock(&globalMutex);
    clock_gettime(CLOCK_MONOTONIC, &end);

    int retVal = SortedList_delete(element);
    if (retVal == 1) {
      char* errorString = "Error with Deletion with Mutex";
      fprintf(stderr, "%s", errorString);
      exit(2);
    }
    pthread_mutex_unlock(&globalMutex);
  } else {
  int curKey = atoi(element->key);
  curKey = (curKey + 128) % num_lists; 
  clock_gettime(CLOCK_MONOTONIC, &start); 
  pthread_mutex_lock(&topMutex[curKey]); 
    clock_gettime(CLOCK_MONOTONIC, &end);

 
  int retVal = SortedList_delete(element);
  if (retVal == 1) {
    char* errorString = "Error with deletion with Mutex"; 
      fprintf(stderr, "%s", errorString); 
    exit(2);
  }
  pthread_mutex_unlock(&topMutex[curKey]);
  }
    perThreadTimes[tid] += ((end.tv_sec - start.tv_sec) * 1000000 + (end.tv_nsec - start.tv_nsec));

}
 
void deletionSpin(SortedListElement_t* element, int tid) {

  
  struct timespec start, end;
  if (num_lists == 1) {
  clock_gettime(CLOCK_MONOTONIC, &start); 
  while (__sync_lock_test_and_set(&lockVal, 1)) {
  }
    clock_gettime(CLOCK_MONOTONIC, &end);

    
  int retVal = SortedList_delete(element);
  if (retVal == 1) {
    char* errorString = "Error Deletion with Spin"; 
    fprintf(stderr, "%s", errorString);  
    exit(2);
  }
    

  __sync_lock_release(&lockVal);
  } else {
    int curKey = atoi(element->key);
  curKey = (curKey + 128) % num_lists;
  clock_gettime(CLOCK_MONOTONIC, &start);
  while (__sync_lock_test_and_set(&topLockVals[curKey], 1)) {
  }
  clock_gettime(CLOCK_MONOTONIC, &end);

  int retVal = SortedList_delete(element);
  if (retVal == 1) {
    char* errorString = "Error Deletion with Spin";
    fprintf(stderr, "%s", errorString);
    exit(2);
  }
   __sync_lock_release(&topLockVals[curKey]);
  }
    perThreadTimes[tid] += ((end.tv_sec - start.tv_sec) * 1000000 + (end.tv_nsec - start.tv_nsec));

}


void lookup(SortedListElement_t* element) {
 
  if (num_lists == 1) {  
   const char* curKey = element->key;
   SortedList_lookup(globalList, curKey); 

  } else {
    int intKey = atoi(element->key);
        int list = (intKey + 128) % num_lists;
	const char* curKey = element->key; 
        SortedList_t* cur = topHeader[list];
         SortedList_lookup(cur, curKey);
  }

}

void lookupMutex(SortedListElement_t* element, int tid) {
  struct timespec start, end; 
  if (num_lists == 1) {
    clock_gettime(CLOCK_MONOTONIC, &start); 
  pthread_mutex_lock(&globalMutex);
    clock_gettime(CLOCK_MONOTONIC, &end);

  
     const char* curKey = element->key;

  SortedList_lookup(globalList, curKey);
  pthread_mutex_unlock(&globalMutex);
   
    } else {
        int intKey = atoi(element->key);
        int list = (intKey) % num_lists;
	const char* curKey = element->key; 
        clock_gettime(CLOCK_MONOTONIC, &start);
        pthread_mutex_lock(&topMutex[list]);
	clock_gettime(CLOCK_MONOTONIC, &end);
	
        SortedList_t* cur = topHeader[list];
        SortedList_lookup(cur, curKey);
        pthread_mutex_unlock(&topMutex[list]);
        
	
    }
    perThreadTimes[tid] += ((end.tv_sec - start.tv_sec) * 1000000 + (end.tv_nsec - start.tv_nsec));


}
void lookupSpin(SortedListElement_t* element, int tid) {
  struct timespec start, end;
  if (num_lists == 1) {
    clock_gettime(CLOCK_MONOTONIC, &start);
  while (__sync_lock_test_and_set(&lockVal, 1)) {
  }
       clock_gettime(CLOCK_MONOTONIC, &end);
  
  
    
  const char* curKey =	element->key;
   SortedList_lookup(globalList, curKey);
       __sync_lock_release(&lockVal);
        

    } else {
    int intKey = atoi(element->key); 
   const char* curKey = element->key;
        int list = (intKey + 128) % num_lists;
	clock_gettime(CLOCK_MONOTONIC, &start); 
        while (__sync_lock_test_and_set(&topLockVals[list], 1)) {
            
        }
	        clock_gettime(CLOCK_MONOTONIC, &end);

        SortedList_t* cur = topHeader[list];
         SortedList_lookup(cur, curKey);
        __sync_lock_release(&topLockVals[list]);
	
    }

    perThreadTimes[tid] += ((end.tv_sec - start.tv_sec) * 1000000 + (end.tv_nsec - start.tv_nsec));

  

   
}

void length() {
    
    if (num_lists == 1) {
  
    curLength = SortedList_length(globalList);
    if (curLength == -1) {
      char* errorString = "List Corrupted! Found in Length";
      fprintf(stderr, "%s", errorString);
        exit(2);
    }
    } else {
      int tempLength, fullLength = 0;  
        for (int i = 0; i < num_lists; i++) {
            tempLength = SortedList_length(topHeader[i]);
            if (tempLength == -1) {
                char* errorString = "List Corrupted. Found in Length";
                fprintf(stderr, "%s", errorString);
                exit(2);
            } else {
                fullLength += tempLength;
            }
            
            
        }
	curLength = fullLength; 
    }
}
void mutexLength(int tid) {
  struct timespec start, end;
  if (num_lists == 1) {
    clock_gettime(CLOCK_MONOTONIC, &start); 
  pthread_mutex_lock(&globalMutex);
   clock_gettime(CLOCK_MONOTONIC, &end);

    curLength = SortedList_length(globalList);
    if (curLength == -1) {
      char* errorString = "List Corrupted! Found in MutexLength";
      fprintf(stderr, "%s", errorString);
    }
  perThreadTimes[tid] += ((end.tv_sec - start.tv_sec) * 1000000 + (end.tv_nsec - start.tv_nsec));
  pthread_mutex_unlock(&globalMutex);
  
    } else {
        
    
    int tempLength, fullLength = 0;
            
            for (int i = 0; i < num_lists; i++) {
	      clock_gettime(CLOCK_MONOTONIC, &start);
              pthread_mutex_lock(&topMutex[i]);
	      clock_gettime(CLOCK_MONOTONIC, &end);
	        
                tempLength = SortedList_length(topHeader[i]);
                if (tempLength == -1) {
                    char* errorString = "List Corrupted. Found in MutexLength";
                    fprintf(stderr, "%s", errorString);
                    exit(2);
                } else {
                    fullLength += tempLength;
                }
                
		    
           perThreadTimes[tid] += ((end.tv_sec - start.tv_sec) * 1000000 + (end.tv_nsec - start.tv_nsec));   
           pthread_mutex_unlock(&topMutex[i]);  
        }
	    curLength = fullLength; 
	     


    }
    

}

void spinLength(int tid) {
  struct timespec start, end;
    if (num_lists == 1) {
      clock_gettime(CLOCK_MONOTONIC, &start); 
  while (__sync_lock_test_and_set(&lockVal, 1)) {
  }
    clock_gettime(CLOCK_MONOTONIC, &end);

    
    curLength = SortedList_length(globalList);
    if (curLength == -1) {
      char* errorString = "List Corrupted! Found in SpinLength";
      fprintf(stderr, "%s", errorString);
    }
      perThreadTimes[tid] += ((end.tv_sec - start.tv_sec) * 1000000 + (end.tv_nsec - start.tv_nsec));
      __sync_lock_release(&lockVal);
      
       
    } else {
        
            int tempLength, fullLength = 0;
        for (int i = 0; i < num_lists; i++) {
	  clock_gettime(CLOCK_MONOTONIC, &start); 
            while (__sync_lock_test_and_set(&topLockVals[i], 1)) {
                
            }
	   clock_gettime(CLOCK_MONOTONIC, &end);

            tempLength = SortedList_length(topHeader[i]);
            if (tempLength == -1) {
                char* errorString = "List Corrupted. Found in Length";
                fprintf(stderr, "%s", errorString);
                exit(2);
            } else {
                fullLength += tempLength;
            }
	                      perThreadTimes[tid] += ((end.tv_sec - start.tv_sec) * 1000000 + (end.tv_nsec - start.tv_nsec));

            __sync_lock_release(&topLockVals[i]);

	             
        }
        curLength = fullLength; 
        
    }
      

}

void* threadFunc(void* arg) {
  struct threadVals *curStruct = ((struct threadVals*)arg); 
  //SortedListElement_t *cur = curStruct->cur;
  
  int curThreadId = curStruct->curThread; 
  //Insert all of them
  
  for (int i = 0; i < num_iterations; i++) {
    //printf("%s", AllElements[curThreadId][i].key);  
    if (isMutex) {
      insertionMutex(&AllElements[curThreadId][i], curThreadId); 
    } else if (isSpin) {
      insertionSpin(&AllElements[curThreadId][i], curThreadId);
    } else {
      insertion(&AllElements[curThreadId][i]);
    }
  //Get the List Length
  }
   
  if (isMutex) { 
    mutexLength(curThreadId);
  } else if (isSpin) {
    spinLength(curThreadId);
  } else {
    length();
  } 
  
  
  //Delete them all 
  for (int i = 0; i < num_iterations; i++) {
    if (isMutex) {
      lookupMutex(&AllElements[curThreadId][i], curThreadId);
      deletionMutex(&AllElements[curThreadId][i], curThreadId);
    } else if (isSpin) {
      lookupSpin(&AllElements[curThreadId][i], curThreadId);
      deletionSpin(&AllElements[curThreadId][i], curThreadId);
    } else {
      lookup(&AllElements[curThreadId][i]); 
      deletion(&AllElements[curThreadId][i]);
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
    {"lists", required_argument, NULL, 'l'},
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
    case 'l':
      num_lists = atoi(optarg);
      break; 
    case 'y':
      yieldOpts = optarg;
      for (unsigned int i = 0; i < strlen(optarg); i++ ) {
	 
            if (yieldOpts[i] ==  'i') {
                opt_yield |= INSERT_YIELD;
            } else if (yieldOpts[i] == 'l') {
                opt_yield |= LOOKUP_YIELD;
            } else if (yieldOpts[i] == 'd') {
                opt_yield |= DELETE_YIELD;
            } else {
                fprintf(stderr, "error unrecognized options for yield");
                exit(1);
            }
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

  perThreadTimes = (int*)malloc(num_threads * sizeof(int));
  for (int i = 0; i < num_threads; i++) {
    perThreadTimes[i] = 0;
  } 
    
    if (num_lists == 1) {
        
        globalList = malloc(sizeof(SortedListElement_t));

    } else {
        multiList();
    }
    
    AllElements = (SortedListElement_t**)malloc(num_threads * sizeof(SortedListElement_t*));
    keys = (char***)malloc(num_threads * sizeof(char**)); 
  for (int i = 0; i < num_threads; i++) {
    AllElements[i] = (SortedListElement_t*)malloc(num_iterations * sizeof(SortedListElement_t));
    keys[i] = (char**)malloc(num_iterations * sizeof(char*)); 
    for (int j = 0; j < num_iterations; j++) {
       
      AllElements[i][j].key = malloc(sizeof(const char)); 
      keys[i][j] = (char*)malloc(5 * sizeof(char)); 
      for (int k = 0; k < 4; k++) {
      keys[i][j][k] = randomChars[rand() % 36];
      }
    AllElements[i][j].key = keys[i][j];
     
     
    } 
  }
  
   
  struct threadVals arrayOfThreads[num_threads];
  
  pthread_t threads[num_threads];
  void* status;
  clock_gettime(CLOCK_MONOTONIC, &start);

  for (int i = 0; i < num_threads; i++) {
    arrayOfThreads[i].curThread = i;
     
    int retVal = pthread_create(&threads[i], NULL, threadFunc, (void*) &arrayOfThreads[i]);
     
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
    
    
    
    length();
  
     
    if (curLength != 0) {
      char* errorString = "Error! List is not empty";
      fprintf(stderr, "%s", errorString); 
      exit(2);
    }
    if (num_lists == 1) {
    free(globalList);
    } else {
      for (int i = 0; i < num_lists; i++) {
	//free(topHeader[i]);
      }
      // free(topHeader); 
    }

    for (int i = 0; i < num_threads; i++) {
      free(AllElements[i]);
      free(keys[i]);
    }
    free(AllElements);
    free(keys);
    
    free(topMutex);
    
    free(topHeader); 
    long long totalLockTime = 0;
    
    for (int i = 0; i < num_threads; i++) {
      
      totalLockTime += abs(perThreadTimes[i]);
    }
     
    free(perThreadTimes);
     
    int  num_operations = num_threads * num_iterations * 3;
    
    totalLockTime /= num_operations;
    
     
    long long timeSlice =(end.tv_sec- start.tv_sec) * 1000000 + (end.tv_nsec - start.tv_nsec);
    if(timeSlice < 0) {
      timeSlice = abs(timeSlice);
    } 
    
    long long  avgTimeOperations = timeSlice / num_operations;
    
    char* addString = "list";
    printf("%s", addString); printf("%s", "-"); printf("%s", yieldOpts); printf("%s", "-"); printf("%s", syncOpts); 
    printf(",%ld,%ld,%d,%d, %lld, %lld, %lld", num_threads, num_iterations, num_lists, num_operations, timeSlice, avgTimeOperations, totalLockTime);
    printf("\n"); 
    //yieldInsert, yieldDelete, yieldLookup
    //isMutex, isSpin
    

  

}
