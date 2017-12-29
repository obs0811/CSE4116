#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

#include <errno.h>
#include <fcntl.h>
#include <dirent.h>
#include <linux/input.h>
#include <sys/stat.h>
#include <sys/select.h>
#include <sys/time.h>
#include <termios.h>
#include <signal.h>

#include <sys/ipc.h>
#include <sys/shm.h>

#include <fcntl.h>
#include <time.h>	// time(), struct tm
#include <sys/ioctl.h>

#include <signal.h>

#define BUFF_SIZE 64
#define KEY_RELEASE 0
#define KEY_PRESS 1

#define END_KEY 158
#define PROG_KEY 116
#define NEXT_KEY 115
#define PREV_KEY 114

#define MODE_CLOCK 0
#define MODE_COUNTER 1
#define MODE_EDITOR 2
#define MODE_BOARD 3
#define MODE_TEST 4

#define MAX_DIGIT 4
#define FND_DEVICE "/dev/fpga_fnd"
#define LED_DEVICE "/dev/fpga_led"
#define SW_DEVICE "/dev/fpga_push_switch"
#define DOT_DEVICE "/dev/fpga_dot"
#define TEXT_LCD_DEVICE "/dev/fpga_text_lcd"
#define BUZZER_DEVICE "/dev/fpga_buzzer"

#define MAX_BUTTON 9

#define NUM_DEC 0
#define NUM_OCT 1
#define NUM_FOUR 2
#define NUM_BIN 3

#define MAX_TEXTBUF 32
#define TEXT_ALPH 0
#define TEXT_NUM 1

/*************************** Functions ***************************/
/* Function to initialze */
void initDevices(int mode);

/* Functions that are called in main process */
void initClock();
void initCounter();
void initTextEditor();
void initDrawBoard();
void initTest();

/* Functions that are called in output process */
void clockMode();
void counterMode();
void textEditorMode();
void drawBoardMode();
void testMode();

int convertNumSystem(int base);	// convert decimal to base system
void init(int initMode);

/*********************** Global variables ************************/
/* Global variables */
int mode;
int shmid;
int *shmaddr;
int devSW, buff_size;
int devKey;
int devFND, devLED;
int devDOT, devTEXTLCD;

/* Clock variables */
int isModifying;
time_t second;

/* Counter variables */
int numSystem;
int counter;

/* Text editor variables */
char keyboard[11][4] = {"!!!", ".QZ", "ABC", "DEF", "GHI", "JKL", "MNO", "PRS", "TUV", "WXY"};
int curSwitch;
int prevSwitch, curAlph;
int alphabetMode = 1;
unsigned char str[34];
int cursor;
int cntPressed;

int modeSW = 0, modeFND = 0, modeLED = 0;

unsigned char board[10][10];
int blink = 1;
int row, col;

int main() {
	pid_t pid1, pid2;
	int i, keyPressed;
	int input;	// input: key number
	unsigned char push_sw_buff[MAX_BUTTON];
	struct input_event ev[BUFF_SIZE];
	int size;

	// Initialize shared memory
	if((shmid = shmget(IPC_PRIVATE, 1024, IPC_CREAT|0666)) == -1) {
		puts("Failed to create a shared memory (shmget)");
		return -1;
	}
	pid1 = fork();
	if(pid1 < 0) {
		printf("Fork failed\n");
		return 0;
	}
	else if(pid1 == 0) {	// input process
		printf("[input process] pid: %d\n", getpid());
		printf("[input process] ppid: %d\n", getppid());
		/* Map shared memory segment to virtual address space */
		if((shmaddr = (int*)shmat(shmid, (int *)NULL, 0)) == (void*)-1) {
			puts("Failed to create a shared memory (shmat)");
			return -1;
		}
		//////
		//printf("shmaddr: %x\n", shmaddr);
		/////
		shmaddr[0] = 0;		// initialize the shared memory
		devSW = open("/dev/fpga_push_switch", O_RDWR);	// open device file of switch with "nonblocking" mode
		devKey = open("/dev/input/event0", O_RDONLY | O_NONBLOCK);	// open device file of key with "nonblocking" mode
		while(1) {
			//printf("[input process] Input a key\n");
			/* read from switches without blocking: ?? implemented as interrupt(NOT polling) */
			buff_size = sizeof(push_sw_buff);
			usleep(200000);
			read(devSW, &push_sw_buff, buff_size);
			
			shmaddr[1] = 0;		// initialize the number of switches pressed
			for(i = 0; i < MAX_BUTTON; i++) {
				if(push_sw_buff[i]) {
					shmaddr[i+2] = 1;
					shmaddr[1]++;	// increment the number of switches pressed
					//printf("[input process] switch %d is pressed\n", i+1);
				}
				else {
					shmaddr[i+2] = 0;
				}
			}
			// test
			for(i = 0; i < 9; i++) {
				//printf("%d ", push_sw_buff[i]);
			}
			//puts("");
			/* read from keys without blocking: ?? implemented as interrupt(NOT polling) */
			size = sizeof(struct input_event);
			int rd = read(devKey, ev, size * BUFF_SIZE);
			if(rd == -1)	continue;
			keyPressed = ev[0].value;
			input = ev[0].code;
			if(!keyPressed || input == PROG_KEY)	continue;
			if(input == END_KEY) {	// terminating condition
				shmaddr[0] = -1;
				//puts("[input process] Good bye!");
				usleep(400000);
				break;
			}
			//printf("<input: %d>\n", input);
			if(input == NEXT_KEY)
				mode = (mode+1) % 5;
			else if(input == PREV_KEY)
				mode = (mode+4) % 5;
			/* Using shared memory, store "mode" in shared memory */
			shmaddr[0] = mode;
			//printf("[input process] mode: %d\n", mode);
			usleep(400000);
		}
		close(devSW);
	}
	else { // divided into output and main process
		pid2 = fork();
		if(pid2 < 0) {
			printf("Fork failed\n");
			return 0;
		}
		else if(pid2 == 0) {	// output process
			//printf("[output process] pid: %d\n", getpid());
			//printf("[output process] ppid: %d\n", getppid());
			
			/* To read a data from shared memory, map the shared memory to virtual address space */
			if((shmaddr = (int*)shmat(shmid, (int *)NULL, 0)) == (void*)-1) {
				puts("Failed to create a shared memory (shmat)");
				return -1;
			}
			////
			//printf("shmaddr: %x\n", shmaddr);
			////
			shmaddr[11] = 0;
			/* open FND and LED device */
			if((devFND = open(FND_DEVICE, O_RDWR)) < 0) {
				printf("Device open error: %s\n", FND_DEVICE);
				exit(1);
			}
			if((devLED = open(LED_DEVICE, O_RDWR)) < 0) {
				printf("Device open error: %s\n", LED_DEVICE);
				exit(1);
			}
			if((devDOT = open(DOT_DEVICE, O_WRONLY)) < 0) {
				printf("Device open error: %s\n", DOT_DEVICE);
				exit(1);
			}
			if((devTEXTLCD = open(TEXT_LCD_DEVICE, O_WRONLY)) < 0) {
				printf("Device open error: %s\n", TEXT_LCD_DEVICE);
				exit(1);
			}
			blink = shmaddr[53] = 1;	// why here?
			while(1) {
				mode = shmaddr[11];
				//printf("[output process] mode: %d\n", mode);
				if(mode == -1) {
					//printf("[output process] Good bye!\n");
					initDevices(mode);
					//printf("[output - terminating] second: %d, isModifying: %d\n", second, isModifying);
					clockMode();
					break;
				}
				switch(mode) {
					case MODE_CLOCK: 	clockMode();		break;
					case MODE_COUNTER: 	counterMode();		break;
					case MODE_EDITOR: 	textEditorMode();	break;
					case MODE_BOARD: 	drawBoardMode();	break;
					case MODE_TEST:     testMode();         break;
				}
				usleep(400000);
			}
			close(devFND);
			close(devLED);
			close(devDOT);
			close(devTEXTLCD);
		}
		else {	// main process
			//printf("[main process] pid: %d\n", getpid());
			//printf("[main process] ppid: %d\n", getppid());
			
			pid_t pid_child;
			int status;
			int lenSwitch;
			int prevMode = -1;
			
			//pid_child = wait(&status);
			//printf("Dead child#1: %d\n", pid_child);
			//pid_child = wait(&status);
			//printf("Dead child#2: %d\n", pid_child);
			
			/* To read a data from shared memory, map the shared memory to virtual address space */
			if((shmaddr = (int*)shmat(shmid, (int *)NULL, 0)) == (void*)-1) {
				puts("Failed to create a shared memory (shmat)");
				return -1;
			}
			////
			//printf("shmaddr: %x\n", shmaddr);
			////
			while(1) {
				mode = shmaddr[0];	// read mode from shared memory
				//printf("[main process] mode: %d\n", mode);
				/* when the program starts or changes mode, initialize */
				if(prevMode == -1 || mode != prevMode) {
					//printf("*** change mode ***\n");
					initDevices(mode);
				}
				if(mode == -1) {	// terminating key is inserted
					shmaddr[11] = mode;
					//printf("[main process] Good bye!\n");
					pid_child = wait(&status);	// wait for the child process
					//printf("Dead child#1: %d\n", pid_child);
					pid_child = wait(&status);
					//printf("Dead child#2: %d\n", pid_child);
					break;
				}
				//if((lenSwitch = shmaddr[1])) {
					//for(i = 0; i < lenSwitch; i++) {
						//printf("[main process] switch %d is pressed\n", shmaddr[i+2]);
					//}
				//}
				switch(mode) {
					case MODE_CLOCK: 	initClock();		break;
					case MODE_COUNTER: 	initCounter();		break;
					case MODE_EDITOR: 	initTextEditor();	break;
					case MODE_BOARD: 	initDrawBoard();	break;
					case MODE_TEST:     initTest();         break;
				}
				prevMode = mode;
				shmaddr[11] = mode;
				usleep(200000);
			}
			//initDevices(mode);
		}
	}
	return 0;
}

/* function to initialize all devices */
void initDevices(int mode) {
	//printf("[initDevices] mode: %d\n", mode);
	//memset(shmaddr, 0, sizeof(shmaddr));

	if(mode == MODE_CLOCK) {
		int i;
		shmaddr[12] = 0;	// isModifying
		shmaddr[13] = time(NULL);	// second
		//puts("CLOCK mode initialized");
		shmaddr[15] = counter = 0;
		//write(devDOT, fpga_set_blank, sizeof(fpga_set_blank));
		shmaddr[128] = shmaddr[129] = 0;
		for(i = 18; i < 50; i++)	shmaddr[i] = 0;
		modeSW = modeFND = modeLED = 0;
	}
	else if(mode == MODE_COUNTER) {
		//counter = 0;
		//numSystem = NUM_DEC;
		shmaddr[14] = numSystem = NUM_DEC;
		shmaddr[15] = counter = 0;
		//printf("counter\n");
		//puts("** counter mode initialized");
	}
	else if(mode == MODE_EDITOR) {
		cntPressed = 0;
		shmaddr[16] = 0;
		shmaddr[15] = counter = 0;
		init(1);
		//puts("** text editor mode initialized");
	}
	else if(mode == MODE_BOARD) {
		int i;
		blink = 1;
		row = col = 0;
		memset(board, 0, sizeof(board));
		shmaddr[124] = shmaddr[125] = shmaddr[126] = 0;
		for(i = 54; i <= 123; i++) {
			shmaddr[i] = 0;
		}
		//puts("** board mode initialized");
		shmaddr[127] = shmaddr[128] = shmaddr[129] = 0;
		for(i = 18; i < 50; i++)	shmaddr[i] = 0;
		modeSW = modeFND = modeLED = 0;
	}
	else if(mode == MODE_TEST) {
		int i;
		// initialize dot matrix
		for(i = 54; i <= 123; i++)
			shmaddr[i] = 0;
		shmaddr[124] = shmaddr[125] = shmaddr[126] = 0;
		modeSW = modeFND = modeLED = 0;
	}
	else if(mode == -1) {
		//puts("Quit!");
		shmaddr[12] = -1;
		shmaddr[13] = 0;
	}
}

/* functions for main process */
void initClock() {
	//static int isModifying = 0;
	//time_t second = time(NULL);
	int switchPressed = (shmaddr[1]) ? 1 : 0;
	second = shmaddr[13];
	
	if(!isModifying) {	// switch(1) is not pressed
		//printf("[initClock] shmaddr[1]: %d,  shmaddr[2]: %d\n", shmaddr[1], shmaddr[2]);
		if(switchPressed && shmaddr[2] == 1) {	// when switch(1) is pressed for the first time
			shmaddr[12] = isModifying = 1;
			//printf("[initClock] isModifying: %d, shmaddr[12]: %d\n", isModifying, shmaddr[12]);
		}
		second++;
	}
	else {	// switch(1) is once pressed. i.e. modifying mode is on
		if(switchPressed) {
			if(shmaddr[3] == 1)		// reset time to the board's current time
				second = time(NULL);
			else if(shmaddr[4] == 1)	// add 3600s(= 1 hour) to the current second
				second += 3600;
			else if(shmaddr[5] == 1)	// add 60s(= 1 min) to the current second
				second += 60;
			else if(shmaddr[2] == 1)	// modifying mode is over -> clear 'isModifying'
				shmaddr[12] = isModifying = 0;
		}
		else {
            // if user does not press any switch after he pressed switch(1),
            // the current time still needs to elapse
			second++;
		}
	}
	shmaddr[13] = second;	// store the calculated second in the assigned shared memory
}

void initCounter() {
	int sw2 = shmaddr[3], sw3 = shmaddr[4], sw4 = shmaddr[5];	// switch(2),(3),(4): number increment
	int sw1 = shmaddr[2];	// switch(1): converting number system
	int sumToAdd;
	int base;
	
	switch(numSystem) {
		case NUM_DEC:  base = 10;	break;
		case NUM_OCT:  base = 8;	break;
		case NUM_FOUR: base = 4;	break;
		case NUM_BIN:  base = 2;	break;
	}
	if(sw1) {	// convert the number system
		shmaddr[14] = numSystem = (numSystem+1) % 4;
	}
	sumToAdd = (base*base) * sw2 + base * sw3 + sw4;
	counter = (counter + sumToAdd) % 1000;
	shmaddr[15] = counter;
}

void initTextEditor() {
	int cntSwitch = shmaddr[1], i;
	int sw5 = shmaddr[6], sw6 = shmaddr[7];
	int sw2 = shmaddr[3], sw3 = shmaddr[4];
	int sw8 = shmaddr[9], sw9 = shmaddr[10];
	
	cntPressed += cntSwitch;
	shmaddr[16] = cntPressed;
	
	if(cntSwitch == 2) {
		if(sw2 && sw3) {
			//puts("23");
			init(0);
		}
		else if(sw5 && sw6) {
			//puts("56");
			if(shmaddr[cursor]) {
				cursor++;
				prevSwitch = curSwitch = -1;
				curAlph = 0;
			}
			alphabetMode ^= 1;
		}
		else if(sw8 && sw9) {
			//puts("89");
			if(shmaddr[cursor])	++cursor;
			shmaddr[cursor++] = ' ';
			prevSwitch = curSwitch = -1;
			curAlph = 0;
		}
	}
	else if(cntSwitch == 1) {
		// read a curSwitch
		for(i = 2; i <= 10; i++) {
			if(shmaddr[i]) {
				curSwitch = i-1;
				break;
			}
		}
		//puts("1 character is read");
		if(!alphabetMode) {
			shmaddr[cursor] = curSwitch + '0';
			cursor++;
		}
		else {
			if(prevSwitch != curSwitch && prevSwitch != -1) {
				++cursor;
				curAlph = 0;
			}
			shmaddr[cursor] = keyboard[curSwitch][curAlph];
			curAlph = (curAlph+1) % 3;
			prevSwitch = curSwitch;
		}
	}
	else {
		return;
	}
	if(cursor == 50 && !shmaddr[50])	return;
	if(!(sw5 && sw6) && cursor >= 50) {
		//puts(">= 32 characters!");
		for(i = 0; i < 32; i++) {
			shmaddr[i+18] = shmaddr[i+19];
		}
		shmaddr[50] = 0;
		cursor--;
	}
	shmaddr[52] = alphabetMode;
	//printf("alphabetMode: %d\n", alphabetMode);
}
	
void initDrawBoard() {
	int cntSwitch = shmaddr[1];
	int r, c;

	shmaddr[126] += cntSwitch;
	
	if(cntSwitch != 1)	return;
	if(shmaddr[3]) {	// sw2
		row = (row+9) % 10;
	}
	else if(shmaddr[5]) {	// sw4
		col = (col+6) % 7;
	}
	else if(shmaddr[7]) {	// sw6
		col = (col+1) % 7;
	}
	else if(shmaddr[9]) {	// sw8
		row = (row+1) % 10;
	}
	else if(shmaddr[6]) {	// sw5
		board[row][col] ^= 1;
	}
	else if(shmaddr[2]) {	// sw1
		memset(board, 0, sizeof(board));
		blink = 1;
		row = col = 0;
		shmaddr[126] = 0;
	}
	else if(shmaddr[8]) {	// sw7
		memset(board, 0, sizeof(board));
	}
	else if(shmaddr[4]) {	// sw3
		blink ^= 0x01;
	}
	else if(shmaddr[10]) {	// sw9
		for(r = 0; r < 10; r++) {
			for(c = 0; c < 7; c++)
				board[r][c] ^= 0x01;
		}
	}
	// write to shared memory
	int cnt = 54;
	shmaddr[53] = blink;
	//printf("[main process] blink: %d\n", blink);
	for(r = 0; r < 10; r++) {
		for(c = 0; c < 7; c++) {
			shmaddr[cnt++] = board[r][c];
		}
	}
	shmaddr[124] = row;
	shmaddr[125] = col;
}

void initTest() {
	int sw[10], i;
	//static int modeSW = 0, modeFND = 0, modeLED = 0;
	int cntSwitch = shmaddr[1];
	char initStr[33]  = "<Menu> 1.All 2.SW 3.FND 4.LED   ";
	char modeStr2[33] = "Current mode:   2.SW            ";
	char modeStr3[33] = "Current mode:   3.FND           ";
	char modeStr4[33] = "Current mode:   4.LED           ";
	int pressedSW;

	if(cntSwitch > 1)	return;	// only one-character input is allowed
	
	for(i = 1; i <= 9; i++)
		sw[i] = shmaddr[i+1];
	//printf("modeSW: %d\n", modeSW);
	if(cntSwitch == 0 && !modeSW && !modeFND && !modeLED) {
		//shmaddr[127] = shmaddr[128] = shmaddr[129] = 0;
		for(i = 18; i < 50; i++)
			shmaddr[i] = initStr[i-18];
	}
	else if(modeSW) {
		// Text LCD
		for(i = 18; i < 50; i++)
			shmaddr[i] = modeStr2[i-18];
		////////////////////////////////////
		for(i = 1; i <= 9; i++) {
			if(sw[i]) {
				pressedSW = i;
				break;
			}
		}
		//printf("sw[2]: %d, sw[3]: %d\n", sw[2], sw[3]);
		//printf("cntSwitch: %d\n", cntSwitch);
		if(cntSwitch == 0) {
			shmaddr[127] = shmaddr[128] = shmaddr[129] = 0;
			return;
		}
		else if(sw[9]) {
			//printf("Quit from modeSW\n");
			modeSW = 0;
			return;
		}
		// FND
		shmaddr[127] = pressedSW;
		// LED
		shmaddr[128] = 0;
		// Dot matrix: 0-9
		shmaddr[129] = 2;
		shmaddr[130] = pressedSW;
	}
	else if(modeFND) {
		// Text LCD
		for(i = 18; i < 50; i++)
			shmaddr[i] = modeStr3[i-18];
		////////////////////////////////////
		for(i = 1; i <= 9; i++) {
			if(sw[i]) {
				pressedSW = i;
				break;
			}
		}
		//printf("sw[2]: %d, sw[3]: %d\n", sw[2], sw[3]);
		//printf("cntSwitch: %d\n", cntSwitch);
		if(cntSwitch == 0) {
			shmaddr[127] = shmaddr[128] = shmaddr[129] = 0;
			return;
		}
		else if(sw[9]) {
			//printf("Quit from modeSW\n");
			modeFND = 0;
			return;
		}
		// FND
		if(sw[1])	shmaddr[127] = 8000;
		if(sw[2])	shmaddr[127] = 800;
		if(sw[3])	shmaddr[127] = 80;
		if(sw[4])	shmaddr[127] = 8;
		if(sw[5] || sw[6] || sw[7] || sw[8])	return;
		// LED
		shmaddr[128] = 0;
		// Dot matrix: 0-9
		shmaddr[129] = 2;
		shmaddr[130] = pressedSW;
	}
	else if(modeLED) {
		// Text LCD
		for(i = 18; i < 50; i++)
			shmaddr[i] = modeStr4[i-18];
		////////////////////////////////////
		for(i = 1; i <= 9; i++) {
			if(sw[i]) {
				pressedSW = i;
				break;
			}
		}
		//printf("sw[2]: %d, sw[3]: %d\n", sw[2], sw[3]);
		//printf("cntSwitch: %d\n", cntSwitch);
		if(cntSwitch == 0) {
			shmaddr[127] = shmaddr[128] = shmaddr[129] = 0;
			return;
		}
		else if(sw[9]) {
			//printf("Quit from modeSW\n");
			modeLED = 0;
			return;
		}
		// FND
		shmaddr[127] = pressedSW;
		// LED
		shmaddr[128] = (1 << (8-pressedSW));
		// Dot matrix: 0-9
		shmaddr[129] = 2;
		shmaddr[130] = pressedSW;
	}
	else {	// basic mode
		// sw -> specific test
		if(sw[1]) {
			// FND
			shmaddr[127] = 8888;
			// LED
			shmaddr[128] = 255;
			// DOT matrix
			shmaddr[129] = 1;
			// Text LCD
			for(i = 18; i < 50; i++)
				shmaddr[i] = initStr[i-18];
		}
		else if(sw[2]) {
			modeSW = 1;
		}
		else if(sw[3]) {
			modeFND = 1;
		}
		else if(sw[4]) {
			modeLED = 1;
		}
	}
	//usleep(200000);
}

void init(int initMode) {
	int i;
	curSwitch = prevSwitch = -1;
	curAlph = 0;
	//if(initMode)	alphabetMode = 1;
	if(initMode)	shmaddr[52] = alphabetMode = 1;
	for(i = 0; i < 34; i++)	shmaddr[i+18] = 0;
	cursor = 18;
}

/* functions for output process */
void clockMode() {
	int isModifying = shmaddr[12];
	time_t second = shmaddr[13];
	struct tm timeToDisplay = *localtime(&second);
	unsigned char dataFND[4], dataLED;
	//int devFND, devLED, ret;
	static int blinkOn = 0, alternateLED = 0;
	int ret;
	unsigned char fpga_set_blank[10] = {
		0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00
	};
	char strBlank[MAX_TEXTBUF] = {0, };
	int i;
	
	// initialize Dot matrix
	write(devDOT, fpga_set_blank, sizeof(fpga_set_blank));
	
	// initialize text LCD
	write(devTEXTLCD, strBlank, 32);

	//printf("[clockMode] second: %d\n", second);
	//printf("[clockMode] isModifying: %d\n", isModifying);
	/* decide what to write in FND */
	dataFND[0] = timeToDisplay.tm_hour / 10;
	dataFND[1] = timeToDisplay.tm_hour % 10;
	dataFND[2] = timeToDisplay.tm_min / 10;
	dataFND[3] = timeToDisplay.tm_min % 10;
	/* decide FND value */
	if((ret = write(devFND, &dataFND, 4)) < 0) {
		puts("Write error in clockMode()");
		return;
	}
	/* depending on whether SW(1) is pressed once, decide LED value */
	if(!isModifying) {
		dataLED = 128;
		blinkOn = 0;	// should be initialized to 0
	}
	else if(isModifying == 1) {
		if(!blinkOn) {	// right after SW(1) is pressed, turn both LED(3) and LED(4) on
			dataLED = 48;
			blinkOn = 1;
		}
		else {	// after a moment, alternate between LED(3) and LED(4)
			dataLED = (!alternateLED) ? 32 : 16;
			alternateLED = -1 * alternateLED + 1;	// 0 -> 1, 1 -> 0
		}
	}
	else if(isModifying == -1) {	// the entire program shuts down
		dataLED = 0;
		// clear dot matrix
		write(devDOT, fpga_set_blank, 10);
		// clear text LCD
		for(i = 0; i < MAX_TEXTBUF; i++) {
			strBlank[i] = '\0';
		}
		write(devTEXTLCD, strBlank, MAX_TEXTBUF);
	}
	write(devLED, &dataLED, 1);
	usleep(200000);
}

void counterMode() {
	unsigned char dataFND[4], dataLED;
	char strCounter[5];
	int base, convertedCounter;
	unsigned char fpga_set_blank[10] = {
		0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00
	};

	numSystem = shmaddr[14];
	counter = shmaddr[15];
	
	// initialize Dot matrix
	write(devDOT, fpga_set_blank, sizeof(fpga_set_blank));

	/* LED */
	switch(numSystem) {
		case NUM_DEC:  dataLED = 64;  base = 10;  break;
		case NUM_OCT:  dataLED = 32;  base = 8;   break;
		case NUM_FOUR: dataLED = 16;  base = 4;   break;
		case NUM_BIN:  dataLED = 128; base = 2;   break;
	}
	write(devLED, &dataLED, 1);
	
	/* FND */
	//printf("counter: %d (in decimal)\n", counter);
	convertedCounter = convertNumSystem(base);
	sprintf(strCounter, "%04d", convertedCounter);
	//printf("strCounter: %s\n", strCounter);
	dataFND[0] = strCounter[0]-'0';
	dataFND[1] = strCounter[1]-'0';
	dataFND[2] = strCounter[2]-'0';
	dataFND[3] = strCounter[3]-'0';
	write(devFND, &dataFND, 4);
}

/* convert decimal to base system */
int convertNumSystem(int base) {
	int ret = 0;
	int numBuf[20], cnt = 0;
	int tmp = counter, i;
	
	while(tmp) {
		numBuf[cnt++] = tmp % base;
		tmp = tmp / base;
	}
	for(i = cnt-1; i >= 0; i--) {
		ret = ret * 10 + numBuf[i];
	}
	return (ret % 10000);
}

void textEditorMode() {
	int cntPressed = shmaddr[16];
	//int idxTextBuf = shmaddr[17];
	char str[MAX_TEXTBUF];
	int alphabetMode = shmaddr[52], i;
	unsigned char fpga_dot[2][10] = {
		{0x1c, 0x36, 0x63, 0x63, 0x63, 0x7f, 0x7f, 0x63, 0x63, 0x63}, // A
		{0x0c, 0x1c, 0x1c, 0x0c, 0x0c, 0x0c, 0x0c, 0x0c, 0x0c, 0x1e}  // 1
	};
	int sizeDOT = sizeof(fpga_dot[0]);
	unsigned char dataFND[4], dataLED = 0;
	
	write(devLED, &dataLED, 1);

	// dot matrix
	if(alphabetMode)
		write(devDOT, fpga_dot[0], sizeDOT);
	else
		write(devDOT, fpga_dot[1], sizeDOT);
	
	// text LCD
	for(i = 0; i < 32; i++) {
		str[i] = shmaddr[18+i];
		//printf("str[%d]: %c\n", i, str[i]);
	}
	//printf("str: %s\n", str);
	write(devTEXTLCD, str, 32);
	
	// FND
	dataFND[0] = cntPressed / 1000;
	dataFND[1] = (cntPressed % 1000) / 100;
	dataFND[2] = (cntPressed % 100) / 10;
	dataFND[3] = cntPressed % 10;
	write(devFND, &dataFND, 4);
}

void drawBoardMode() {
	int r, c, cnt = 54;
	unsigned char tmp[10][10];
	unsigned char fpga_dot[10];
	int row = shmaddr[124], col = shmaddr[125];
	int hex1, hex2;
	static int alternate = 0;
	int cntPressed = shmaddr[126];
	unsigned char dataFND[4], dataLED = 0;
	char str[32] = {0, };

	blink = shmaddr[53];
	
	// initialize
	write(devLED, &dataLED, 1);
	write(devTEXTLCD, str, 32);

	//printf("<cursor> row: %d, col: %d\n", row, col);
	//puts("");
	for(r = 0; r < 10; r++) {
		for(c = 0; c < 7; c++) {
			tmp[r][c] = shmaddr[cnt++];
			//printf("%d", tmp[r][c]);
		}
		//puts("");
	}
	//printf("[output process] blink: %d\n", blink);
	if(blink && !shmaddr[54+row*7+col]) {
		//printf("Blink is ON!");
		tmp[row][col] = (alternate) ? 1 : 0;
		alternate = -1 * alternate + 1;
		//printf("Blink: tmp[%d][%d] = %d\n", row, col, tmp[row][col]);
	}
	for(r = 0; r < 10; r++) {
		/*for(c = 0; c < 7; c++) {
			printf("[%d][%d]: %d ", r, c, tmp[r][c]);
		}
		puts("");*/
		hex1 = 4*tmp[r][0]+2*tmp[r][1]+tmp[r][2];
		hex2 = 8*tmp[r][3]+4*tmp[r][4]+2*tmp[r][5]+tmp[r][6];
		
		//printf("[%d] hex1: %d, hex2: %d\n", r, hex1, hex2);
		//printf("[%d] hex1*16+hex2 = %d\n", r, hex1*16+hex2);
		//fpga_dot[r] = (4*tmp[r][0]+2*tmp[r][1]+tmp[r][2])*16
		//			  +(8*tmp[r][3]+4*tmp[r][4]+2*tmp[r][5]+tmp[r][6]);
		fpga_dot[r] = hex1*16+hex2;
	}
	write(devDOT, fpga_dot, sizeof(fpga_dot));
	
	// FND
	dataFND[0] = cntPressed / 1000;
	dataFND[1] = (cntPressed % 1000) / 100;
	dataFND[2] = (cntPressed % 100) / 10;
	dataFND[3] = cntPressed % 10;
	write(devFND, &dataFND, 4);

	usleep(200000);
}

void testMode() {
	unsigned char fpga_set_blank[10] = {
		0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00	// 0
	};
	unsigned char fpga_set_all[10] = {
		0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f  // 1
	};
	unsigned char fpga_set_number[10][10] = {
		{0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}, // 0
		{0x0c,0x1c,0x1c,0x0c,0x0c,0x0c,0x0c,0x0c,0x0c,0x1e}, // 1
		{0x7e,0x7f,0x03,0x03,0x3f,0x7e,0x60,0x60,0x7f,0x7f}, // 2
		{0xfe,0x7f,0x03,0x03,0x7f,0x7f,0x03,0x03,0x7f,0x7e}, // 3
		{0x66,0x66,0x66,0x66,0x66,0x66,0x7f,0x7f,0x06,0x06}, // 4
		{0x7f,0x7f,0x60,0x60,0x7e,0x7f,0x03,0x03,0x7f,0x7e}, // 5
		{0x60,0x60,0x60,0x60,0x7e,0x7f,0x63,0x63,0x7f,0x3e}, // 6
		{0x7f,0x7f,0x63,0x63,0x03,0x03,0x03,0x03,0x03,0x03}, // 7
		{0x3e,0x7f,0x63,0x63,0x7f,0x7f,0x63,0x63,0x7f,0x3e}, // 8
		{0x3e,0x7f,0x63,0x63,0x7f,0x3f,0x03,0x03,0x03,0x03} // 9
	};
	int i;
	int tmpFND = shmaddr[127];
	unsigned char dataFND[4], dataLED = shmaddr[128];
	unsigned char dataLCD[32];
	int dotMatrixMode = shmaddr[129];
	int numSwitch = shmaddr[130];

	//write(devDOT, fpga_set_blank, sizeof(fpga_set_blank));
	// FND
	//printf("tmpFND: %d\n", tmpFND);
	dataFND[0] = tmpFND / 1000;
	dataFND[1] = (tmpFND % 1000) / 100;
	dataFND[2] = (tmpFND % 100) / 10;
	dataFND[3] = tmpFND % 10;
	//printf("%d%d%d%d\n", dataFND[0], dataFND[1], dataFND[2], dataFND[3]);
	write(devFND, &dataFND, 4);

	// LED
	write(devLED, &dataLED, 1);

	// Text LCD
	for(i = 0; i < 32; i++)
		dataLCD[i] = shmaddr[18+i];
	write(devTEXTLCD, dataLCD, 32);

	// DOT matrix
	if(dotMatrixMode == 1) {
		write(devDOT, fpga_set_all, sizeof(fpga_set_all));
	}
	else if(dotMatrixMode == 0) {
		write(devDOT, fpga_set_blank, sizeof(fpga_set_blank));
	}
	else if(dotMatrixMode == 2) {
		write(devDOT, fpga_set_number[numSwitch], sizeof(fpga_set_number[numSwitch]));
	}
	//usleep(200000);
}
