/***
 * Program: picmd
 * Usage: picmd [gpio] '[command]'
 * Description: run as daemon, listening on GPIO pin (wiringPi numbering)
 *              on high signal the command is executed, Ctrl+C to stop program
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/time.h>
#include <wiringPi.h>

#define PRESS_DELAY 1000
#define LOOP_DELAY  50

unsigned int gpio_pin = 0;
int sig=1;
char *cmd;
int flag=0;

void printUsage(void);
int init(void);
void exInt0_ISR(void);

void printUsage() {
	fprintf(stderr, "Usage: picmd GPIO 1|0 'CMD'\n"
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
	wiringPiISR(gpio_pin, INT_EDGE_FALLING, &exInt0_ISR);
	printf("Press_Delay=%dms. Loop_Delay=%dms\n",PRESS_DELAY,LOOP_DELAY); 
	printf("WiringPi initialization done...\n");
	flag=0;
	return 0;
}

void exInt0_ISR(void) {
	flag=1;
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
	printf("Start Listener on GPIO PIN %d for Signal %d\n",gpio_pin, sig);
	cmd = argv[3];
	printf("Command='%s'\n",cmd);

	init();

	while (1) {
		if (flag) {
			printf("High signal received on GPIO PIN '%d'.\n", gpio_pin);
			printf("Executing '%s'... start\n", cmd);	
			system(cmd);
			printf("Sleeping %d ms ...\n", PRESS_DELAY);
			fflush(stdout);
			delay(PRESS_DELAY);
			printf("Sleeping done\n");
			flag=0;
		}	
		delay(LOOP_DELAY);
	}
	exit(EXIT_SUCCESS);
	return 0;
}



