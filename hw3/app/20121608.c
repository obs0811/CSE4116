#include <unistd.h>
#include <syscall.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>

#define DEV_MAJOR 242
#define DEV_NAME "/dev/stopwatch"

int main(int argc, char **argv) {
	int ret, i;
	int fd;
	char buf[2] = {0,};
		
	// open devices (fnd, led, dot matrix, and text lcd)
	fd = open(DEV_NAME, O_RDWR);
	if(fd < 0) {
		printf("Can't open device file %s\n", DEV_NAME);
		return 0;
	}
	// start stopwatch
	ret = write(fd, buf, 2);

	// close devices
	close(fd);
	return 0;
}
