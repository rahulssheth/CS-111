//NAME: Rahul Sheth
//EMAIL: rahulssheth@g.ucla.edu
//ID: 304779669

#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <poll.h>
#include <getopt.h>
#include <errno.h>
#include <termios.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <mcrypt.h>
#include <signal.h>

//Client End of the Code

MCRYPT td;
MCRYPT tdDecrypt; 
int isLog = 0;
int isEncrypted = 0; 
char* keyFile; 
struct termios start;

errno; 
void sig_Handler() {
  exit(0); 
}
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
  fprintf(stderr, errorString); 
  exit(1);
  }
read(keyFD, key, keysize);

td = mcrypt_module_open("twofish", NULL, "cfb", NULL);
if (td==MCRYPT_FAILED) {
  fprintf(stderr, "Error setting up encryption");
  char* errorString = strerror(errno);
  fprintf(stderr, errorString);
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
  fprintf(stderr, errorString);
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
    fprintf(stderr, "%s", errorString);
    exit(1);
  }
  read(keyFD, key, keysize);
   
  tdDecrypt = mcrypt_module_open("twofish", NULL, "cfb", NULL);
  if (tdDecrypt==MCRYPT_FAILED) {
    fprintf(stderr, "Error setting up decryption");
    char* errorString = strerror(errno);
    fprintf(stderr, "%s",  errorString);
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
    fprintf(stderr, "%s", errorString);
    exit(1);
  }

free(key);

}



 
void exitAndReset() {
  if (isEncrypted) {
    mcrypt_generic_end(td);
    mcrypt_generic_end(tdDecrypt);
  }
  tcsetattr(0, TCSANOW, &start);
}

int main(int argc, char* argv[]) {
  
  int logFD; 
  static struct option const long_options[] =   {
    
    {"port", required_argument, NULL, 'p'},
    {"log", required_argument, NULL, 'l'},
    {"encrypt", required_argument, NULL, 'e'},
    {0, 0, 0, 0}
  };
  
  int opt, index;
  int port;
  char* logFile; 
  while ((opt = getopt_long(argc, argv, "p:l:e:", long_options, &index)) != \
         -1) {

    switch (opt) {
    case 'p':
      port = atoi(optarg); 
      break;
    case 'l':
      logFile = optarg;
            isLog = 1;
	    chmod(logFile, 0777);
	    logFD = creat(logFile, 0666);
	    
        if (logFD < 0) {
            fprintf(stderr, "Error opening file");
            char* errorString = strerror(errno);
            fprintf(stderr, "%s", errorString);

            exit(1);

        }
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
      fprintf(stderr, "%s", errorString);      exit(1);
      break;
    }
  }
  signal(SIGPIPE, sig_Handler); 

  atexit(exitAndReset);
  tcgetattr(0, &start);
    struct termios new;
    
    new = start;
    new.c_iflag = ISTRIP;
    new.c_oflag = 0;
    new.c_lflag = 0;

    tcsetattr(0, TCSANOW, &new);
  
  int sock; 
  

  struct sockaddr_in serv;

  sock = socket(AF_INET, SOCK_STREAM, 0);
  if (sock < 0) {
    fprintf(stderr, "Error Opening Socket");
    char* errorString = strerror(errno);
  fprintf(stderr, errorString);
    exit(1); 
  }



  serv.sin_addr.s_addr = inet_addr("127.0.0.1");
  serv.sin_family = AF_INET; 
  serv.sin_port = htons(port);
    
    
  
  if (connect(sock, (struct sockaddr *)&serv, sizeof(serv)) < 0) {
    fprintf(stderr, "Couldn't connect");
    char* errorString = strerror(errno);
    fprintf(stderr, "%s", errorString);
    exit(1); 

  }
    char buf[255];
    struct pollfd polls[2];
    polls[0].fd = 0;
    polls[0].events = POLLIN + POLLHUP + POLLERR;
    polls[1].fd = sock; 
    polls[1].events = POLLIN + POLLHUP + POLLERR;

    while(1) {
        int retval = poll(polls, 2, 0);
        if (retval < 0) {
            fprintf(stderr, "Error Polling");
            char* errorString = strerror(errno);
            fprintf(stderr, "%s", errorString);
            exit(1);
            
        }



      if (polls[0].revents & POLLIN) {
          int readBytes = read(polls[0].fd, buf, 255);
          if (readBytes < 0) {
              fprintf(stderr, "Error reading from the socket");
              char* errorString = strerror(errno);
              fprintf(stderr, "%s", errorString);
              exit(1);
          }
	  char* encryptedVal; 
          for (int i = 0; i < readBytes; i++) {
              switch (buf[i]) {
                  case '\r':
                  case '\n': ;
                    write(1, "\r\n", 3);
                    char val = '\n';
		    if (isEncrypted) {
		    mcrypt_generic(td, &val, 1);
		    }
	           
		      write(sock, &val, 1);
		      if (isLog) {

			 char* writeString;
			 char newLine = '\n';
  
			 write(logFD, "SENT ", 5);
  

			 char value = 1 + '0';
			 write(logFD, &value, sizeof(value));
			 write(logFD, " bytes: ", 8);
			 write(logFD, &buf[i], 1);
			 write(logFD, &newLine, 1); 
		      }

		    
                    break;
                  case '\004':
                    write(1, "^D", 2);
		    if (isEncrypted) {
	            mcrypt_generic(td, &buf[i], 1);
                    }
		      write(sock, &buf[i], 1);
		      if (isLog) {

			char* writeString;
                         char newLine = '\n';

                         write(logFD, "SENT ", 5);


                         char value = 1 + '0';
                         write(logFD, &value, sizeof(value));
                         write(logFD, " bytes: ", 8);
                         write(logFD, &buf[i], 1);
                         write(logFD, &newLine, 1);
		      }
		    
		      
                    break;
                  case '\003':
                    write(1, "^C", 2);
		    if (isEncrypted) {
		      mcrypt_generic(td, &buf[i], 1);
		    }
		      write(sock, &buf[i], 1);
		      if (isLog) {
			char* writeString;
                         char newLine = '\n';

                         write(logFD, "SENT ", 5);


                         char value = 1 + '0';
                         write(logFD, &value, sizeof(value));
                         write(logFD, " bytes: ", 8);
                         write(logFD, &buf[i], 1);
                         write(logFD, &newLine, 1); 
		      }
		    
                      break;
                  default:
                    write(1, &buf[i], 1);
		    if (isEncrypted) {
		    mcrypt_generic(td, &buf[i], 1);

                    } 
                      write(sock, &buf[i], 1);
                      if (isLog) {
                          char* writeString;
                         char newLine = '\n';

                         write(logFD, "SENT ", 5);


                         char value = 1 + '0';
                         write(logFD, &value, sizeof(value));
                         write(logFD, " bytes: ", 8);
                         write(logFD, &buf[i], 1);
                         write(logFD, &newLine, 1);
                      }
		    
                    break;
              }
              
          }
          
          
          
      } else if (polls[1].revents & (POLLHUP + POLLERR)) {
	              exit(0);

	}else if (polls[1].revents & POLLIN) {
          int readBytes = read(polls[1].fd, buf, 255);
          if (readBytes < 0) {
	    char* errorString = strerror(errno);
  fprintf(stderr, errorString);
              exit(1);
          } else if (readBytes == 0) {
	    exit(0);
	  }
	  if (isLog) {
	    {

                         char* writeString;
                         char newLine = '\n';

                         write(logFD, "RECEIVED ", 9);

			 char value[5];
			 int count = sprintf(value, "%d", readBytes); 
			  
                         
                         write(logFD, &value, count);
                         write(logFD, " bytes: ", 8);
                         write(logFD, buf, readBytes);
                         write(logFD, &newLine, 1);
                      }
	    }
	  if (isEncrypted) {
          mdecrypt_generic(tdDecrypt, buf, readBytes);
	  }
	  for (int i = 0; i < readBytes; i++) {
	    switch(buf[i]) {

	    case '\004':
	      
	      exit(0);
	      break;
	    default:
	      
	  write(1, &buf[i], 1);
	  break;
	   
	  }
	  }
	  } else if (polls[1].revents & (POLLHUP + POLLERR)) {
	    exit(0);
	    
	  }
    }


  

  
}

