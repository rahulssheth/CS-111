//NAME: Rahul Sheth
//EMAIL: rahulssheth@g.ucla.edu
//ID: 304779669

#include <stdio.h>
#include <termios.h>
#include <stdbool.h>
#include <unistd.h>
#include <getopt.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <stdlib.h>
#include <fcntl.h> 
#include <poll.h>
#include <signal.h>
#include <errno.h>
#include <string.h>



struct termios start;

pid_t pid = -1;

void exitAndReset() {
    
    tcsetattr(0, TCSANOW, &start);
    int waitStatus;
    if (pid != -1) {
      
    waitpid(pid, &waitStatus, 0);
    fprintf(stderr, "SHELL EXIT SIGNAL=%d STATUS=%d", WTERMSIG(waitStatus), WEXITSTATUS(waitStatus)); 
    
    }
    
    
    
}

void sig_Handler() {
  exit(0);
}

int main(int argc, char **argv) {

    errno = 0;
    atexit(exitAndReset);

  static struct option const long_options[] =   {

    {"shell", no_argument, NULL, 's'}, 
    {0, 0, 0, 0}
  }; 
  int shell = 0; 
  int opt, index;  
  while ((opt = getopt_long(argc, argv, "s", long_options, &index)) != \
	 -1) {

    switch (opt) {
    case 's':
      shell = 1; 
      break;
    case '?':
    default:
      fprintf(stderr, "Unrecognized argument");
            char* errorString = strerror(errno);
            fprintf(stderr, "%s", errorString);      exit(1);
      break;
    }
  }
   

  signal(SIGPIPE, sig_Handler); 
 struct termios new;
  tcgetattr(0, &start);



  new = start;
    new.c_iflag = ISTRIP;
  new.c_oflag = 0;
  new.c_lflag = 0;
  new.c_cc[VMIN] = 1;
  new.c_cc[VTIME] = 0;



  tcsetattr(0, TCSANOW, &new);
  
  if (shell) {
     
    
      
    
    
    int pipeFD[2], pipeFD2[2]; 
    //pipeFD represents the from Shell and pipeFD2 represents the to Shell. Therefore, inside the shell we should be able to read input going to the shell and write output going from the shell.   
    char bufff[255]; 
    int val = pipe(pipeFD);
    if (val < 0) {
      fprintf(stderr, "Error! Couldn't initialize first pipe");
        char* errorString = strerror(errno);
        fprintf(stderr, "%s", errorString);
      exit(1);
    }
    int val2 = pipe(pipeFD2);
    if (val2 < 0) {
      fprintf(stderr, "Error! Couldn't initialize the second pipe");
        char* errorString = strerror(errno);
        fprintf(stderr, "%s", errorString);
	exit(1);
    }

    int y;
    int status; 
    
      pid = fork(); 
      if (pid < 0) {
	fprintf(stderr, "Fork Failed!");
          char* errorString = strerror(errno);
          fprintf(stderr, "%s", errorString);
	exit(1);
      } else if (pid == 0) { 
       //Child Process
	
      close(pipeFD[0]);
      close(pipeFD2[1]); 
      //At this point, we will not be writing to the shell or reading from the shell
      
      close(0);
      dup(pipeFD2[0]);
      close(pipeFD2[0]); 


      
      close(1);
      dup(pipeFD[1]);
      close(2);
      dup(pipeFD[1]);
      close(pipeFD[1]);
       
     
      //At this point,stdin is from the input going to the shell and stdoutput and stderr is from what is writing from the shell. 
      
      char **val = NULL; 
	execvp("/bin/bash", val);	     
	return -1;
      } else {
	 
        close(pipeFD2[0]);
	close(pipeFD[1]);
	//At this point, we will not be reading input that should be going to the shell nor writing 
	
	
	struct pollfd polls[2]; 
	polls[0].fd = 0;
	polls[0].events = POLLIN + POLLHUP + POLLERR;
	  polls[1].fd = pipeFD[0]; 
	polls[1].events = POLLIN + POLLHUP + POLLERR;

	  
     	bool shouldILoop = true;
	
	while (shouldILoop) {
	  int retVal = poll(polls, 2, 0);
	   
	  if (retVal < 0) {
	    fprintf(stderr, "Error! Could not poll properly");
          char* errorString = strerror(errno);
          fprintf(stderr, "%s", errorString);

	    exit(1);
	  } else {
	  if (polls[0].revents && POLLIN) {
	    char buf[255];  
	    int count = read(polls[0].fd, buf, 255);
          if (count > 0) {
              
              for (int i = 0; i < count; i++) {
                  if (buf[i] == '\004') {
		     
                      close(pipeFD2[1]);
                      close(pipeFD[0]);
                      kill(pid, SIGHUP);
                      exit(0);
                      
                  } else if (buf[i] == '\003')  {
		    
		      
		     
                      kill(pid, SIGTERM);
		      
		     
                  } else if (buf[i] == '\r' || buf[i] == '\n') {
		     
		    
                      char val = '\n';
		    
                      int x = write(1, "\r\n", 2);
                      if (x < 0) {
                          fprintf(stderr, "Error writing to standard output");
                          char* errorString = strerror(errno);
                          fprintf(stderr, "%s", errorString);
                          exit(1);
                      }
                      x = write(pipeFD2[1], &val, 1);
                      if (x < 0) {
                          fprintf(stderr, "Error writing to the shell");
                          char* errorString = strerror(errno);
                          fprintf(stderr, "%s", errorString);
                          exit(1);
                          
                      }
 
                  } else {
                      int x = write(1, &buf[i], 1);
                      if (x < 0) {
                          fprintf(stderr, "Error writing to standard output");
                          char* errorString = strerror(errno);
                          fprintf(stderr, "%s", errorString);
                          exit(1);
                      }
                      x = write(pipeFD2[1], &buf[i], 1);
                      if (x < 0) {
                          fprintf(stderr, "Error writing to the shell");
                          char* errorString = strerror(errno);
                          fprintf(stderr, "%s", errorString);
                          exit(1);
                      }
		    
                    }
                }
            } else {
                fprintf(stderr, "Error with reading in from keyboard");
                char* errorString = strerror(errno);
                fprintf(stderr, "%s", errorString);
                exit(1);
            }
          
	    
    } else if (polls[0].revents & (POLLHUP + POLLERR)) {
            exit(0);
    } else if (polls[1].revents & POLLIN) {
        char buffer[255];
	      
        int count = read(polls[1].fd, buffer, 255);
        if (count > 0) {

            for (int i = 0; i < count; i++) {
                int x;
                if (buffer[i] == '\n') {
                    x = write(1, "\r\n", 3);
                    if (x < 0) {
                        fprintf(stderr, "Error writing newline and carriage return to standard output");
                        char* errorString = strerror(errno);
                        fprintf(stderr, "%s", errorString);
                        exit(1);
                    }
                } else {
                    x =  write(1, &buffer[i], 1);
                    if (x < 0) {
                        fprintf(stderr, "Error writing character to standard output");
                        char* errorString = strerror(errno);
                        fprintf(stderr, "%s", errorString);
                        exit(1);
                    }
                }
	     }
        }  else {
            fprintf(stderr, "Error reading from the shell");
            char* errorString = strerror(errno);
            fprintf(stderr, "%s", errorString);
            exit(1);
        }
    } else if (polls[1].revents & (POLLHUP + POLLERR)) {
            exit(0);
    }
	
        }
	}
      }
  } else {
   
  char buffer[255] = {0};  
  
 
   
  bool val = true;
  int option, i; 
  while ((option = read(0, &buffer, sizeof(buffer))) > 0) {
    for (i = 0; i < option; i++) {
      switch (buffer[i]) { 

	case '\004':
	  
	  exit(0);  
	  break;
    case '\n': ;
    case '\r': ;
	  int x = write(1, "\r\n", 3);
	  if (x < 0) {
	    fprintf(stderr, "Error writing newline carriage return to std out (No Shell)");
          char* errorString = strerror(errno);
          fprintf(stderr, "%s", errorString);
	    exit(1);
	  }
	  break;
    default: ;
	   
      x = write(1, &buffer[i], 1);
	  if (x < 0) {
	    fprintf(stderr, "Error writing character to std out");
          char* errorString = strerror(errno);
          fprintf(stderr, "%s", errorString);
	    exit(1);
	  break; 
      }
    }
    
  
    if (val == false) {
      break;
    }
  }


  }

}
}
