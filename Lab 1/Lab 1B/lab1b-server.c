//NAME: Rahul Sheth
//EMAIL: rahulssheth@g.ucla.edu
//ID: 304779669

#include <stdio.h>
#include <errno.h>
#include <getopt.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <poll.h>
#include <termios.h>
#include <fcntl.h> 
#include <sys/wait.h>
#include <signal.h>
#include <unistd.h>
#include <mcrypt.h>
#include <signal.h>
pid_t pid = -1;

 struct termios start;
int newSock; 
MCRYPT td;
MCRYPT tdDecrypt; 
char* keyFile; 
    
void prepareEncryption() {

   int i;
  char *key;
  char *IV;
  int keysize = 16;
  key = calloc(1, keysize);
  int keyFD = open(keyFile, O_RDONLY);
  if (keyFD < 0) {
  fprintf(stderr, "Error taking in key");
  char* errorString = strerror(errno);
  fprintf(stderr,"%s",  errorString);
  exit(1);
  }
  read(keyFD, key, keysize);

  td = mcrypt_module_open("twofish", NULL, "cfb", NULL);
  if (td==MCRYPT_FAILED) {
    fprintf(stderr, "Error");
    char* errorString = strerror(errno);
    fprintf(stderr,"%s", errorString);
    exit(1);
  }
IV = malloc(mcrypt_enc_get_iv_size(td));
for (i = 0; i < mcrypt_enc_get_iv_size(td); i++) {
  IV[i] = 'A';

 }

i = mcrypt_generic_init(td, key, keysize, IV);
if (i < 0) {
  fprintf(stderr, "error generating encryption");
      char* errorString = strerror(errno);
      fprintf(stderr,"%s", errorString);

  exit(1);
 }
 free(key);
}


void prepareDecryption() {

  int i;
  char *key;
  char *IV;
  int keysize = 16;
  key = calloc(1, keysize);
  int keyFD = open(keyFile, O_RDONLY);
  if (keyFD < 0) {
  fprintf(stderr, "Error taking in key");
    char* errorString = strerror(errno);
      fprintf(stderr,"%s", errorString);
  exit(1);
  }
  read(keyFD, key, keysize);
   
  tdDecrypt = mcrypt_module_open("twofish", NULL, "cfb", NULL);
  if (tdDecrypt==MCRYPT_FAILED) {
    fprintf(stderr, "Error setting decryption");
      char* errorString = strerror(errno);
      fprintf(stderr,"%s", errorString);
    exit(1);
  }
  IV = malloc(mcrypt_enc_get_iv_size(tdDecrypt));
  for (i = 0; i < mcrypt_enc_get_iv_size(tdDecrypt); i++) {
    IV[i] = 'A';

  }

  i = mcrypt_generic_init(tdDecrypt, key, keysize, IV);
  if (i < 0) {
    fprintf(stderr, "error generating encryption");
      char* errorString = strerror(errno);
      fprintf(stderr,"%s", errorString);
    exit(1);
  }

free(key);


}

void sig_Handler() {

  exit(0);
}
int isEncrypted = 0;
 
void exitAndReset() {
  if (isEncrypted) {
  mcrypt_generic_end(td);
  mcrypt_generic_end(tdDecrypt);
  }
    close(newSock);
 
  if (pid != -1) {
     
    int waitStatus; 
    waitpid(pid, &waitStatus, 0);
     
    fprintf(stderr, "SHELL EXIT SIGNAL=%d STATUS=%d", WTERMSIG(waitStatus), WEXITSTATUS(waitStatus)); 
    
    }
  
}



int main(int argc, char* argv[]) {
    
   
  errno = 0;
  

  static struct option const long_options[] =   {

    {"port", required_argument, NULL, 'p'},
    {"encrypt", required_argument, NULL, 'e'},
    {0, 0, 0, 0}
  };
 int opt, index, sock;
 int port; 
  while ((opt = getopt_long(argc, argv, "p:e:", long_options, &index)) != \
         -1) {

    switch (opt) {
    case 'p':
      port = atoi(optarg);
      break;
    case 'e':
      keyFile = optarg;
      isEncrypted = 1;
      prepareEncryption();
      prepareDecryption();
      break;
    case '?':
    default:
      fprintf(stderr, "Unrecognized argument");
      char* errorString = strerror(errno);
      fprintf(stderr, "%s", errorString);     
      exit(1);
      break;
    }
  }
 

    signal(SIGPIPE, sig_Handler); 
      atexit(exitAndReset);



  //Initialize the socket and bind

  sock = socket(AF_INET, SOCK_STREAM, 0);
  if (sock < 0) {
    fprintf(stderr, "Error Creating the socket");
   char* errorString = strerror(errno);
    fprintf(stderr, "%s", errorString); 
    exit(1); 
  }

  static struct sockaddr_in server, client; 
  bzero((char *) &server, sizeof(server));
  server.sin_family = AF_INET;
  server.sin_addr.s_addr = INADDR_ANY;
  server.sin_port = htons(port);
  int bindValue = bind(sock, (struct sockaddr*)&server, sizeof(server));
  if (bindValue < 0) { 
    fprintf(stderr, "Error with binding to the server");
    char* errorString = strerror(errno);
    fprintf(stderr, "%s", errorString);
    exit(1); 
  }
 
  listen(sock, 10);
  u_int clientLength = sizeof(client); 
  newSock = accept(sock, (struct sockaddr *)&client, &clientLength); 
  if (newSock < 0) {
    fprintf(stderr, "Socket could not be accepted");
    char* errorString = strerror(errno);
    fprintf(stderr, "%s", errorString);
    exit(1); 
  }




  //Initialize the pipes to and from the shell                                                                               
  int pipeToShell[2];
  int pipeFromShell[2];
  int checkPipes;

  checkPipes = pipe(pipeToShell);
  if (checkPipes < 0) {
    fprintf(stderr, "Error. Couldn't open pipeToShell");
    char* errorString = strerror(errno);
    fprintf(stderr,"%s", errorString);
    exit(1);
  }
  checkPipes = pipe(pipeFromShell);
  if (checkPipes < 0) {
    fprintf(stderr, "Error. Couldn't open pipeFromShell");
    char* errorString = strerror(errno);
    fprintf(stderr,"%s", errorString);
    exit(1);
  }

  pid = fork();
  
  if (pid < 0) {
    fprintf(stderr, "Error creating the fork");
    char* errorString = strerror(errno);
    fprintf(stderr, "%s", errorString); 
    exit(1);
  } else if (pid == 0) {
    //Child Process
      close(pipeToShell[1]);
      close(pipeFromShell[0]);
      
      
      close(0);
      dup(pipeToShell[0]);
      close(pipeToShell[0]);
      
      close(1);
      dup(pipeFromShell[1]);
      close(2);
      dup(pipeFromShell[1]);
      close(pipeFromShell[1]);
      char** val = NULL;
	execvp("/bin/bash", val);
      return -1;
      
    
  } else {
    //Parent Process. Since we are in the server, we are going to have to poll from the pipeFromShell[0] and from the socket
      char buf[255];
       
      close(pipeToShell[0]);
      close(pipeFromShell[1]);
      
      
      struct pollfd polls[2];
      polls[0].fd = newSock;
      polls[0].events = POLLIN + POLLHUP + POLLERR;
      polls[1].fd = pipeFromShell[0];
      polls[1].events = POLLIN + POLLHUP + POLLERR;
      
      
      
      while (1) {
	int retval = poll(polls, 2, 0);
          if (retval < 0) {
              fprintf(stderr, "Error polling!");
              char* errorString = strerror(errno);
              fprintf(stderr,"%s", errorString);
              exit(1);
          }
          if (polls[0].revents & POLLIN) {
              int readBytes = read(polls[0].fd, buf, 255);
              if (readBytes < 0) {
                  fprintf(stderr, "Error reading from the socket");
                  char* errorString = strerror(errno);
                  fprintf(stderr,"%s", errorString);
                  exit(1);
              }
	      for (int i = 0; i < readBytes; i++) {
		if (isEncrypted) {
		    
		  mdecrypt_generic(tdDecrypt, &buf[i], 1);
		  
		  
		}
		switch(buf[i]) {
		case '\004':
		  close(pipeToShell[1]);
		  close(pipeFromShell[0]); 
		  kill(pid, SIGHUP); 
		  exit(0);  

                  
		  
		   
		   
		  break;
		case '\003':
		  close(pipeToShell[1]);
                  close(pipeFromShell[0]);
		  kill(pid, SIGINT);
		  if (isEncrypted) {
		    mcrypt_generic(td, &buf[i], 1); 
		  }
		  write(newSock, &buf[i], 1);
		  exit(0);
		  break;
		default:
		   
		  write(pipeToShell[1], &buf[i], 1);
		  break; 
	      } 
	      } 
          } else if (polls[0].revents & (POLLHUP + POLLERR)) {
	    exit(0); 
          } else if (polls[1].revents & POLLIN) {
              int readBytes = read(polls[1].fd, buf, 255);
              if (readBytes < 0) {
                  fprintf(stderr, "Error Writing to output");
                  char* errorString = strerror(errno);
                  fprintf(stderr,"%s", errorString);
                  exit(1);
                  
                  
              }
	      
	      
	      for (int i = 0; i < readBytes; i++) {
		
		switch(buf[i]) {
		case '\r':
		case '\n': ;
		  char crNL[3] = "\r\n"; 
		   
		  if (isEncrypted) {
		    mcrypt_generic(td, crNL, 3);
		    
		  }
		  write(newSock, crNL, 3);
		   
		  break;
		case '\004':
		  close(pipeToShell[1]);
                  close(pipeFromShell[0]);
                  kill(pid, SIGKILL);
                  exit(0);                                               
		        
		  break; 
		default:
		  if (isEncrypted) {
		    mcrypt_generic(td, &buf[i], 1);
		  }
		  write(newSock, &buf[i], 1);
		  break;
		}
	      }
              
              
          } else if (polls[1].revents & (POLLHUP + POLLERR)) {
	    exit(0); 
	  }
          
          
      }
      
      
      
      
      
      
      
  }
  
  
  
  
  
  






}
