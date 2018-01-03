#include <math.h>
#include <mraa.h>
#include <mraa/aio.h>
#include <unistd.h>
#include <stdio.h>
#include <time.h>
#include <getopt.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <mraa/gpio.h>
#include <poll.h>
#include <string.h>

mraa_aio_context rotary;
int period = 1;

char scale = 'F';
char* filename;
int isLog, fd;
int logReceipts = 1; 
extern int errno;
const int B = 4275;
const float R0 = 100000;
mraa_gpio_context button;
time_t raw;
struct tm *formattedTime;
int noSleep = 0; 
void getValueAndProcess();
void exitRoutine();
float convertToFarenheit(float celsiusTemp);

void processPolls(char* buffer, int readBytes) {
int lastCommandEnd = 0;
int x = 0;
char curBuf[100]; 
//write(1, buffer, readBytes);
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
}
void pollForCommands() {



struct pollfd pollFD;
 
pollFD.fd = 0;
pollFD.events = POLLIN;

char buf[100];

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
	 
        processPolls(buf, readBytes);


} 
if (noSleep && logReceipts) {
getValueAndProcess();
}
noSleep = 1; 
sleep(period);
  
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

write(1, buffer, bytesWritten); 
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
int bytesWritten = sprintf(buffer, "%s:%s:%s %0.1f \n", hour, minute, second, temp);

write(1, buffer, bytesWritten);
if (isLog) {
write(fd, buffer, bytesWritten);
}

free(hour);
free(minute);
free(second); 


}

int
main(int argc, char** argv)
{

static struct option const long_options[] = {
{"log", required_argument, NULL, 'l'},
{"period", required_argument, NULL, 'p'},
{"scale", required_argument, NULL, 's'},
{0, 0, 0, 0},

};

int opt, index;

while ((opt = getopt_long(argc, argv, "l:p:s:", long_options, &index)) != -1) {
switch(opt) {
        case 'l':
                isLog = 1;
                if ((fd = open(optarg, O_WRONLY | O_CREAT, 0666)) < 0) {
                 fprintf(stderr, "Error opening logfile");
                  exit(1);
                }
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

    rotary = mraa_aio_init(1);
    button = mraa_gpio_init(62);

        mraa_gpio_isr(button, MRAA_GPIO_EDGE_RISING, &exitRoutine, NULL);
  
if (logReceipts) {
getValueAndProcess();
}

while (1) {
pollForCommands();
}

mraa_aio_close(rotary);
mraa_gpio_close(button);
return 0; 
}
