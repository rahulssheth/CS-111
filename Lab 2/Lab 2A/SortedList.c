#include "SortedList.h"
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
int opt_yield; 
void SortedList_insert(SortedList_t *list, SortedListElement_t *element) {
   
  SortedListElement_t* temp; 
  temp = list;
  while (temp->next != NULL) {
     
    if (opt_yield & INSERT_YIELD) {
      sched_yield();
    }
    if (element->key < temp->key) {
      //Insert Happens Here
      
      SortedListElement_t* previous = temp->prev;
      previous->next = element;
      temp->prev = element;
      element->next = temp;
      element->prev = previous;
      
      return; 
    } else {
      temp = temp->next;
    }
  }
   
  temp->next = element;
  element->prev = temp;
  element->next = NULL;
  


}


int SortedList_delete(SortedListElement_t *element) {
  
  SortedList_t* previous = element->prev;
  SortedList_t* nextOne = element->next;
  
  
  
  if ((previous != NULL && nextOne != NULL) && (previous->next != element || nextOne->prev != element)) {
    return 1;
  }
    if (opt_yield & DELETE_YIELD) {
    sched_yield();
  }
  if (previous != NULL) {
    if (nextOne != NULL) {  
  previous->next = nextOne;
    } else {
      previous->next = NULL;
    }
  }
  if (nextOne != NULL) {
    if (previous != NULL) {
  nextOne->prev = previous;
    } else {
      nextOne->prev = NULL;

    }
  }
  
   
  return 0;
 
  }
  


SortedListElement_t *SortedList_lookup(SortedList_t *list, const char *key) {

  SortedList_t* temp = list;
  while (temp->next != NULL) {
    if (opt_yield & LOOKUP_YIELD) {
      sched_yield();
    }
    if (temp->key == key) {
      return temp;
    }
    temp=temp->next;
  }
  return 0;

}

int SortedList_length(SortedList_t *list) {

  int counter = 0;
  SortedList_t* temp = list;
   
  while (temp->next != NULL) {
    if (opt_yield  & LOOKUP_YIELD) {
      sched_yield();
    }    
    
    SortedList_t* nextVal = temp->next;
    //Check Base Case with the List pointer
    if (temp == list) {
      if (temp->prev != NULL) {
	return -1;
      }
      return 0;  
    } else {
       
      if (nextVal->prev != temp->next) {
	return -1;
      }
      counter++;
      temp = nextVal; 

}
  }

  return counter; 
}

