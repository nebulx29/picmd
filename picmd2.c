/***
 * Program: picmd2
 * Usage: picmd2 [gpio] '[command]'
 * Description: run as daemon, listening on GPIO pin (wiringPi numbering)
 *              on high signal the command is executed, Ctrl+C to stop program
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/time.h>
#include <wiringPi.h>

#define PRESS_DELAY 500
#define LOOP_DELAY  50
#define DEBUG 0
#define TIMESTAMP_LENGTH 27
#define INTERRUPT_WAIT 99

unsigned int gpio_pin = 0;
int sig=1;
char *cmd;
int flag=0;
char timestamp[TIMESTAMP_LENGTH];


void printUsage(void);
int init(void);
void exInt0_ISR(void);

void printUsage() {
	fprintf(stderr, "Usage: picmd2 GPIO 1|0 'CMD'\n"
                "   Program listens on GPIO PIN [GPIO] for signal=[1|0] and executes [CMD] on high signal received.\n"
                "   Default GPIO is 0 (BCM 17).\n\n"
	        "   Examples: picmd 0 1 'sudo reboot'\n"
	        "             picmd 0 1 'sudo shutdown -h now'\n"
	        "             picmd 3 1 'date +\"%c button pressed\" >> /home/pi/btn.log '\n"
	        "             picmd 4 1 'echo gpio 4 rising... >> /home/pi/gpio.log'\n"
	        "             picmd 4 0 'echo gpio 4 falling... >> /home/pi/gpio.log'\n"
	        "             picmd 4 1 'pm2 restart magicmirror'\n"
		);
	exit(EXIT_FAILURE);
}

int init(void) {
	if (wiringPiSetup()<0) {
		fprintf(stderr, "Unable to setup wiringPi: %s\n", strerror(errno));
		exit(EXIT_FAILURE);
		return 1;
	}
	wiringPiISR(gpio_pin, (sig==1?INT_EDGE_RISING:INT_EDGE_FALLING), &exInt0_ISR);
	pullUpDnControl(gpio_pin, PUD_UP);

	createTimestamp(timestamp);
 	fprintf(stdout,"[%s] [picmd] init(GPIO=%d) sig=%d (%s). Command='%s'\n", timestamp, gpio_pin, sig, (sig==1?"INT_EDGE_RISING":"INT_EDGE_FALLING"), cmd);
	fprintf(stdout,"[%s] [picmd] init Press_Delay=%dms. Loop_Delay=%dms. Interrupt_Wait=%dms\n",timestamp, PRESS_DELAY,LOOP_DELAY,INTERRUPT_WAIT); 
	fflush(stdout);

	if (DEBUG) printf("WiringPi initialization done...\n");
	flag=0;
	return 0;
}

void exInt0_ISR(void) {
	if (!flag && digitalRead(gpio_pin)==sig) {
		createTimestamp(timestamp);
		fprintf(stdout, "[%s] exInt0_ISR(%d): ACCEPTED\n", timestamp, gpio_pin);
		fflush(stdout);
		flag=1;
		return;
	}
	createTimestamp(timestamp);	
	fprintf(stdout, "[%s] exInt0_ISR(%d): IGNORED\n", timestamp, gpio_pin);
	fflush(stdout);
}

/*void exInt0_ISR(void) {
	// only if flag is not already set
	if (!flag) {
		delay(INTERRUPT_WAIT/3);
		if (digitalRead(gpio_pin)==sig) {
			delay(INTERRUPT_WAIT/3);
			if (digitalRead(gpio_pin)==sig) {
				delay(INTERRUPT_WAIT/3);
				if (digitalRead(gpio_pin)==sig) { // only if signal still active after 3 positive WAIT period 
					createTimestamp(timestamp);	
					fprintf(stdout, "[%s] exInt0_ISR(%d): ACCEPTED\n", timestamp, gpio_pin);
					fflush(stdout);
					flag=1;
					return;
				}	
			}
		}
	}	
	createTimestamp(timestamp);	
	fprintf(stdout, "[%s] exInt0_ISR(%d): IGNORED\n", timestamp, gpio_pin);
	fflush(stdout);
}*/

int createTimestamp(char* buffer) {
  int millisec;
  struct tm* tm_info;
  struct timeval tv;
  char buffer2[TIMESTAMP_LENGTH];
  
  gettimeofday(&tv, NULL);

  millisec = lrint(tv.tv_usec/1000.0); 
  if (millisec>=1000) { 
    millisec -=1000;
    tv.tv_sec++;
  }

  tm_info = localtime(&tv.tv_sec);
  strftime(buffer, TIMESTAMP_LENGTH, "%Y-%m-%d %H:%M:%S", tm_info); 
  sprintf(buffer, "%s.%d", buffer, millisec);
}

int main(int argc, char* argv[]) {
	if (argc!=4) {
		printUsage();
		exit(1);
		return 1;
	}
	gpio_pin = atoi(argv[1]);
	sig = atoi(argv[2]);
	if (!gpio_pin && *argv[1]!='0') 
		printf("Warning: Invalid gpio argument '%s'. Defaulting to GPIO '0'.\n",argv[1]);
	if (sig!=0&&sig!=1) {
		printf("Warning: Invalid gpio signal type '%s'. Allowed are '1' or '0'. Defaulting to '1'.\n");
		sig=1;
	}
	cmd = argv[3];

	init();

	while (1) {
		if (flag) {
			createTimestamp(timestamp);
			fprintf(stdout,"[%s] [picmd] High signal received on GPIO PIN '%d'. Executing '%s'... start. Afterwards sleeping %dms. \n", timestamp, gpio_pin, cmd, PRESS_DELAY);
			fflush(stdout);
			system(cmd);
			delay(PRESS_DELAY);
			flag=0;
		}	
		delay(LOOP_DELAY);
	}
	exit(EXIT_SUCCESS);
	return 0;
}



