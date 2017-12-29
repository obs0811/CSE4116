extern int first(int x, int y);
extern void test(char *argv);

#include <jni.h>
#include "android/log.h"

#define LOG_TAG "MyTag2"
#define LOGV(...) __android_log_print(ANDROID_LOG_VERBOSE, LOG_TAG, __VA_ARGS__)

#include <unistd.h>
#include <sys/syscall.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <stdlib.h>

//*** ioctl//
#include <linux/ioctl.h>
#include <fcntl.h>
#include <sys/ioctl.h>

#define SYSCALL_NUM 377

#define DEV_MAJOR 242
#define DEV_NAME "/dev/dev_driver"

//#define KERNEL_DEVICE_MAJOR 242
//#define KERNEL_DEVICE_NAME "/dev/dev_driver"
//#define IOCTL_SET _IOR(KERNEL_DEVICE_MAJOR, 0, long)

#define IOCTL_PIANO _IOR(DEV_MAJOR, 0, long)

struct parastruct {
	//int interval;
	//int counter;
	char key_pressed[3];
	long incoding;
	//int res;
};

void test(char* argv){

	struct parastruct my_st;
	long num;
	int fd;
	int flag = 0;

	argv[0] -= 32;
	if(argv[1] == 's') {
		argv[1] = argv[2];
		flag = 1;
	}
	strcpy(my_st.key_pressed,argv);

	num=syscall(377,&my_st);

	printf("test result %s encoding result %d \n",my_st.key_pressed,my_st.incoding);

	fd = open(DEV_NAME, O_RDWR);

	if(fd < 0) {
		printf("Can't open device file %s\n", DEV_NAME);
		exit(-1);
	}
	// activate timer (and write to devices)
	if(flag == 1)
		ioctl(fd, IOCTL_PIANO, my_st.incoding+1000);
	else
		ioctl(fd, IOCTL_PIANO, my_st.incoding);
	// close devices
	close(fd);
}

int first(int x, int y) {
	return (x + y);
}
