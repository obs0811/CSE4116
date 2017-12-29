#include <unistd.h>
#include <syscall.h>
#include <stdio.h>
#include <string.h>
#include <linux/ioctl.h>
#include <sys/ioctl.h>
#define SYSCALL_NUM 377

#define DEV_MAJOR 242
#define DEV_NAME "/dev/dev_driver"

#define IOCTL_TIMER _IOR(DEV_MAJOR, 0, char *)

// structure to send data from user space to kernel space (and vice versa)
struct param_input {
	int interval;
	int cnt;
	char start[4];
	char result[4];
};

int main(int argc, char **argv) {
	int interval, cnt, start_pos, start_val;
	struct param_input inputs;
	int ret, i;
	long packed_arg = 0;
	int fd;

	if(argc != 4) {
		puts("argc should be 4");
		return 0;
	}
	// input: argv[1] = timer interval and argv[2] = the # of timer interrupts
	sscanf(argv[1], "%d", &interval);
	if(interval < 1 || interval > 100) {
		puts("Wrong arguments: argv[1] (time interval) should be  a value in [1,100]");
		return 0;
	}
	sscanf(argv[2], "%d", &cnt);
	if(cnt < 1 || cnt > 100) {
		puts("Wrong arguments: argv[2] (the # of timer interrupts) should be  a value in [1,100]");
		return 0;
	}
	inputs.interval = interval;	// store in structure
	inputs.cnt = cnt;	// store in structure

	// input: argv[3] - store in structure
	if(argv[3][0] == '0' && argv[3][1] == '0' && argv[3][2] == '0' && argv[3][3] == '0') {
		puts("Wrong arguments: argv[3][0..3] (start option) is 0000");
		return 0;
	}
	for(i = 0; i < 4; i++) {
		if(argv[3][i] < '0' || argv[3][i] > '8') {
			puts("Wrong arguments: argv[3][0..3] (start option) should be a value in ['0','8']");
			return 0;
		}
	}
	//printf("argv[3]: %s\n", argv[3]);
	strcpy(inputs.start, argv[3]);

	// system call
	ret = syscall(SYSCALL_NUM, &inputs);	
	//printf("ret: %d\n", ret);
	//printf("%d %d %d %d\n", inputs.result[0], inputs.result[1], inputs.result[2], inputs.result[3]);
	
	// convert the array given by the system call to long data ('char [4]' into 'long')
	packed_arg = (char)inputs.result[0];
	packed_arg = packed_arg << 8;
	packed_arg |= (char)inputs.result[1];
	packed_arg = packed_arg << 8;
	packed_arg |= (char)inputs.result[2];
	packed_arg = packed_arg << 8;
	packed_arg |= (char)inputs.result[3];
	
	// open devices (fnd, led, dot matrix, and text lcd)
	fd = open(DEV_NAME, 0);
	if(fd < 0) {
		printf("Can't open device file %s\n", DEV_NAME);
		return 0;
	}
	// activate timer (and write to devices)
	ioctl(fd, IOCTL_TIMER, packed_arg);
	// close devices
	close(fd);
	return 0;
}
