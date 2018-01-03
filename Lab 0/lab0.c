#include <ctype.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <getopt.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <signal.h>


void errorSubRoutine();
void handler(); 
int main(int argc, char **argv) {
  //Set up for the different cases like input, output, segfault, catch 
  int option; 
  char* file1;
  char* file2;
  int case1 = 0; 
  int case2 = 0; 
  int case3 = 0; 
  int case4 = 0; 
  static struct option const long_options[] = {

    {"input", required_argument, NULL, 'i'}, 
    {"output", required_argument, NULL, 'o'}, 
    {"segfault", no_argument, NULL, 's'}, 
    {"catch", no_argument, NULL, 'c'},
    {NULL, 0, NULL, 0}
  };

  int index;
  char* errorArg = "Error: Unrecognized argument: ";
  //Grab all of the args from command line
  while ((option = getopt_long(argc, argv, "i:sco:", long_options, &index)) != -1) {
     
    switch (option) {
    case 'i':
      file1 = optarg;
      case1 = 1;
      break;
    case 'o':
      file2 = optarg; 
      case2 = 1; 
      break;
    case 's': 
      case3 = 1; 
      break;
    case 'c':
      
      case4 = 1; 
      break;
    case '?': 
      fprintf(stderr, "%s", errorArg);
      fprintf(stderr, "%c", '\n');

      exit(1); 
      break;
    default:
      fprintf(stderr, "%s", errorArg);
      fprintf(stderr, "%c", '\n');

      exit(1);
      break;

     
      }
  } 

  //Set up error codes that are general for all error messages 
  char* errorString1 = "Argument that caused error: "; 
  char* errorString2 = "File name: "; 
  char* errorString3 = "This is the error: "; 
  int fd0;
  errno = 0;
  //Check for need of input
  if (case1) {
  fd0 = open(file1, O_RDONLY);
    
  //if failed then error output
  if (fd0 < 0) {
    char* errorString4 = "--input  "; 
    char* errorString5 = strerror(errno); 
    fprintf(stderr, "%s", errorString1);
     
    fprintf(stderr, "%s", errorString4);
    fprintf(stderr, "%c", '\n');  
    fprintf(stderr, "%s", errorString2);
    
    fprintf(stderr, "%s", file1);
    fprintf(stderr, "%c", '\n');
 
    fprintf(stderr, "%s", errorString3);
     
    fprintf(stderr, "%s", errorString5); 
    fprintf(stderr, "%c", '\n');


   
    exit(2); 
     
  } else {
    //Else file redirection 
    close(0); 
    dup(fd0);
    close(fd0); 

  }
  }
  //Check for the second file
  if (case2) {
    
    int fd1 = creat(file2, 0666); 
  if (fd1 >= 0) {
    //File redirection  
    close(1);
    dup(fd1);
    close(fd1);
  } else {
    //Error Message to output
    char* errorString4 = "--output";
      char* errorString5 = strerror(errno); 
      fprintf(stderr, "%s", errorString1);
    fprintf(stderr, "%s", errorString4);
    fprintf(stderr, "%c", '\n');

    fprintf(stderr, "%s", errorString2);
    fprintf(stderr, "%s", file2);
    fprintf(stderr, "%c", '\n');
 
    fprintf(stderr, "%s", errorString3);
    fprintf(stderr, "%s", errorString5);
    fprintf(stderr, "%c", '\n');
 
    
     
    exit(3); 
  }
  } 

  if (case4) {
    //Call the signal handler 
    signal(SIGSEGV, handler);
  } 
  if (case3) {
    //Run the segfault
    errorSubRoutine(); 
  }
  char t;
  //Copy from 0 -> 1
  while (read(0, &t, 1) > 0) {
     

    write(1, &t, 1);
     
  }




  
  exit(0);


}


void handler() {
  //Handle if segfault called 
  char* errorHandler = "Segmentation Fault caught. Caused by storing into a null pointer"; 
  
  fprintf(stderr, "%s", errorHandler);
  fprintf(stderr, "%c", '\n');
  
  exit(4); 
}


void errorSubRoutine() {
  //Cause a segfault 
  char* val = NULL; 
  *val = 'a'; 
 
}
