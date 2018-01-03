#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <stdio.h>
#include <poll.h>
#include <signal.h>
#include <string.h>
#include <mraa.h>
#include <mraa/aio.h>
#include <sys/types.h>
#include <sys/errno.h>
#include <time.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <openssl/ssl.h>
#include <netdb.h>
#include <math.h>
#include <errno.h>
#include <getopt.h>
extern int errno;

int period = 1;
char scale = 'F';
int isLog = 0;
int fd;
char* host_name;
char* id;
int port_num;
int sockFD;
struct sockaddr_in serv;
struct hostent *server_host;
const int TCP_PORT = 18000;
const int TLS_PORT = 19000;


mraa_aio_context rotary;

int logReceipts = 1;
const int B = 4275;
const float R0 = 100000;
//mraa_gpio_context button;
time_t raw;
struct tm *formattedTime;
int noSleep = 0;

void getValueAndProcess();
void exitRoutine();
float convertToFarenheit(float celsiusTemp);


void processPolls(char* buffer, int readBytes) {
  /*
    int lastCommandEnd = 0;
    int x = 0;
    char curBuf[100];
    while (x < readBytes) {
        if (buffer[x] == '\n') {
            
            strncpy(curBuf, buffer+lastCommandEnd, x-lastCommandEnd + 1);
	     
            if (isLog) {
                write(fd, curBuf, x-lastCommandEnd + 1);
                
                
            }
            
            if (strstr(curBuf, "OFF")) {
                
                exitRoutine();
                
            } else if (strstr(curBuf, "START")) {
                logReceipts = 1;
                
            } else if (strstr(curBuf, "STOP")) {
                logReceipts = 0;
                
            } else if (strstr(curBuf, "PERIOD=")) {
                int numLength = x-lastCommandEnd + 1 - 7;
                char numBuf[numLength];
                strncpy(numBuf, curBuf+7, numLength);
                period = atoi(numBuf);
                
                
            } else if (strstr(curBuf, "SCALE=")) {
                int i = 0;
                while (i < 100) {
                    if (curBuf[i] == '=') {
                        scale = curBuf[i+1];
                    }
                    i++;
                }
                
            }
            memset(curBuf, 0, sizeof(curBuf));
            lastCommandEnd = x;
            
        }
        x++;
        
    }
  */

  int lastCommandEnd = 0;
    int x = 0;

    char curBuf[100];
    while (x < readBytes) {
        if (buffer[x] == '\n') {

            strncpy(curBuf, buffer+lastCommandEnd, x-lastCommandEnd + 1);

            if (isLog) {
                write(fd, curBuf, x-lastCommandEnd + 1);


            }

            if (strstr(curBuf, "OFF")) {

                exitRoutine();

            } else if (strstr(curBuf, "START")) {
                logReceipts = 1;

            } else if (strstr(curBuf, "STOP")) {
              
                logReceipts = 0;

            } else if (strstr(curBuf, "PERIOD=")) {
                int numLength = x-lastCommandEnd + 1 - 7;
                char numBuf[numLength];
                strncpy(numBuf, curBuf+7, numLength);
                period = atoi(numBuf);


            } else if (strstr(curBuf, "SCALE=")) {
                int i = 0;
                while (i < 100) {
                    if (curBuf[i] == '=') {
                        scale = curBuf[i+1];
                    }
                    i++;
                }

            } else if (strstr(curBuf, "LOG")) {
	      for (unsigned long i = 3; i < strlen(curBuf); i++) {
                    write(fd, &curBuf[i], 1);
                }
            }
            memset(curBuf, 0, sizeof(curBuf));
            lastCommandEnd = x;

        }
        x++;

    }

}
void pollForCommands() {
    
    
    
    struct pollfd pollFD;
    
    pollFD.fd = sockFD;
    pollFD.events = POLLIN;
    
    char buf[100];

    while (1) {
    int retVal = poll(&pollFD, 1, 0);
     
    if (retVal < 0) {
        
        fprintf(stderr, "Error Polling");
        char* errorString = strerror(errno);
        fprintf(stderr, "%s", errorString);
        exit(1);
    }
    
    if (pollFD.revents & POLLIN) {
        
        int readBytes = read(pollFD.fd, buf, 100);
        if (readBytes < 0) {
            fprintf(stderr, "Error reading from Poll");
            char* errorString = strerror(errno);
            fprintf(stderr, "%s", errorString);
            exit(1);
        }
        write(1, buf, readBytes); 
        processPolls(buf, readBytes);
        
        
    }
    if (noSleep && logReceipts) {
        getValueAndProcess();
    }
    noSleep = 1;
    sleep(period);
    }  
}



void exitRoutine() {
    
    time(&raw);
    
    formattedTime = localtime(&raw);
    
    char buffer[100];
    
    
    
    char *hour, *minute, *second;
    hour = (char*)malloc(2*sizeof(char));
    minute = (char*)malloc(2*sizeof(char));
    second = (char*)malloc(2*sizeof(char));
    
    
    if (formattedTime->tm_hour < 10) {
        sprintf(hour, "0");
        sprintf(hour+1, "%d", formattedTime->tm_hour);
        
    } else {
        sprintf(hour, "%d", formattedTime->tm_hour);
        
    }
    
    if (formattedTime->tm_min < 10) {
        sprintf(minute, "0");
        sprintf(minute+1, "%d", formattedTime->tm_min);
    } else {
        sprintf(minute, "%d", formattedTime->tm_min);
    }
    if (formattedTime->tm_sec < 10) {
        sprintf(second, "0");
        sprintf(second+1, "%d", formattedTime->tm_sec);
    } else {
        sprintf(second, "%d", formattedTime->tm_sec);
    }
    int bytesWritten = sprintf(buffer, "%s:%s:%s SHUTDOWN\n", hour, minute, second);
    
    dprintf(sockFD, "%s", buffer);
    if (isLog) {
        write(fd, buffer, bytesWritten);
    }
    free(hour);
    free(minute);
    free(second);
    exit(0);
    
}

float getTemp(int passedThroughSensor) {
    float R = 1023.0 / passedThroughSensor - 1.0;
    R = R0*R;
    
    float temperature = 1.0/(log(R/R0)/B + 1/298.15)-273.15;
    temperature = convertToFarenheit(temperature);
    
    return temperature;
}
float convertToCelsius(float farenheitTemp) {
    return (farenheitTemp-32) / 1.8;
}



float convertToFarenheit(float celsiusTemp) {
    return celsiusTemp * 1.8 + 32;
    
}



void getValueAndProcess() {
    time(&raw);
    
    formattedTime = localtime(&raw);
    
    uint16_t value = mraa_aio_read(rotary);
    float temp = getTemp(value);
     
    if (!strcmp(&scale, "C")) {
        temp = convertToCelsius(temp);
        
	}
   char buffer[100];
    
    char *hour, *minute, *second;
    hour = (char*)malloc(2*sizeof(char));
    minute = (char*)malloc(2*sizeof(char));
    second = (char*)malloc(2*sizeof(char));
    
    
    if (formattedTime->tm_hour < 10) {
        sprintf(hour, "0");
        sprintf(hour+1, "%d", formattedTime->tm_hour);
        
    } else { 
        sprintf(hour, "%d", formattedTime->tm_hour);
        
    }
    
    if (formattedTime->tm_min < 10) {
        sprintf(minute, "0");
        sprintf(minute+1, "%d", formattedTime->tm_min);
    } else {
        sprintf(minute, "%d", formattedTime->tm_min);  
    } 
    if (formattedTime->tm_sec < 10) {
        sprintf(second, "0");
        sprintf(second+1, "%d", formattedTime->tm_sec); 
    } else {
        sprintf(second, "%d", formattedTime->tm_sec); 
    }  
    int bytesWritten = sprintf(buffer, "%s:%s:%s %0.1f\n", hour, minute, second, temp);
    
    dprintf(sockFD, "%s", buffer);
    if (isLog) {
        write(fd, buffer, bytesWritten);
    }
    
    free(hour);
    free(minute);
    free(second); 
    
    
}




void reportError(char* errorString) {
    fprintf(stderr, "%s",errorString);
    fprintf(stderr, "%s",strerror(errno));
    
}
void extractArgument(int argc, char** argv) {
    
    static struct option const long_options[] = {
        {"log", required_argument, NULL, 'l'},
        {"period", required_argument, NULL, 'p'},
        {"host", required_argument, NULL, 'h'},
        {"id", required_argument, NULL, 'i'},
        {"scale", required_argument, NULL, 's'},
        {0, 0, 0, 0},
        
    };
    
    int opt, index;
    
    while ((opt = getopt_long(argc, argv, "l:p:s:i:h:", long_options, &index)) != -1) {
        switch(opt) {
            case 'l':
                isLog = 1;
                if ((fd = open(optarg, O_WRONLY | O_CREAT, 0666)) < 0) {
                    fprintf(stderr, "Error opening logfile");
                    exit(1);
                }
                break;
            case 'h':
                host_name = optarg;
                break;
            case 'i':
                id = optarg;
		break; 
            case 'p':
                period = atoi(optarg);
                break;
            case 's':
                scale = optarg[0];
                break;
            default:
                fprintf(stderr, "Unrecognized options");
                exit(1);
                
                
                
        }
    }
    
    if (optind < argc) {
        if (strstr(argv[optind], "tls") != NULL) {
            port_num = TLS_PORT;
        } else if (strstr(argv[optind], "tcp") != NULL) {
            port_num = TCP_PORT;
        } else {
            port_num = atoi(argv[optind]);
        }
    } else {
        port_num = TCP_PORT;
    }

}


void establishTCPConnection() {
    
    sockFD = socket(AF_INET, SOCK_STREAM, 0);
    if (sockFD < 0) {
        reportError("Error Opening Socket");
        exit(1);
    }
    
    if ((server_host = gethostbyname(host_name)) == NULL) {
        reportError("Couldnt' find the host from the given name");
        exit(1);
    }
    
    serv.sin_addr.s_addr = *(long*) (server_host->h_addr);
    serv.sin_family = AF_INET;
    serv.sin_port = htons(port_num);
    
    
    
    if (connect(sockFD, (struct sockaddr *)&serv, sizeof(serv)) < 0) {
        reportError("Couldn't Connect");
        exit(1); 
        
    }
    char buf[200];

    sprintf(buf, "ID=%s\n", id);
    dprintf(sockFD, "ID=%s\n", id); 
    if (isLog) {
      write(fd, buf, 15);
    }
    
    
    
    
}


int main(int argc, char** argv) {
    extractArgument(argc, argv);
    
    establishTCPConnection();
    
    
    
    
    rotary = mraa_aio_init(1);
    button = mraa_gpio_init(62);
    
    mraa_gpio_isr(button, MRAA_GPIO_EDGE_RISING, &exitRoutine, NULL);
    
       if (logReceipts) {
        getValueAndProcess();
    }
    
    
        pollForCommands();
    
    mraa_aio_close(rotary);
    mraa_gpio_close(button);
    return 0; 

    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
}
