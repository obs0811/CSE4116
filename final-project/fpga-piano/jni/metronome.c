extern void beat();

#include <jni.h>
#include "android/log.h"

#define LOG_TAG "MyTag"
#define LOGV(...) __android_log_print(ANDROID_LOG_VERBOSE, LOG_TAG, __VA_ARGS__)

#include <unistd.h>
#include <sys/syscall.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>

#define DEV_MAJOR2 243
#define DEV_NAME2 "/dev/stopwatch"

void beat() {
	int i;
	int fd;
	char buf[2] = {0,};
	int cnt = 0;
	int beat_rate = 3;

	// open devices (fnd, led, dot matrix, and text lcd)
	fd = open(DEV_NAME2, O_RDWR);
	if(fd < 0) {
		printf("Can't open device file %s\n", DEV_NAME2);
		exit(-1);
	}
	// start stopwatch
	while(beat_rate > 0) {
		beat_rate = write(fd, buf, beat_rate);
		/*jclass thisClass = (*env)->GetObjectClass(env, this);
		jfieldID fidnumber = (*env)->GetFieldID(env, thisClass, "beatOn", "Z");
		jboolean beaton = (*env)->GetBooleanField(env, this, fidnumber);
		beaton = true;
		(*env)->SetBooleanField(env, this, number);*/
		LOGV("timer %d", cnt++);
		LOGV("beat_rate: %d", beat_rate);
	}
	// close devices
	close(fd);
}
