#include <jni.h>
#include "android/log.h"

#include <unistd.h>
#include <sys/syscall.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>

#define DEV_MAJOR2 243
#define DEV_NAME2 "/dev/stopwatch"

#define DEV_MAJOR3 244
#define DEV_NAME3 "/dev/drum"

#define LOG_TAG "MyTag"
#define LOGV(...) __android_log_print(ANDROID_LOG_VERBOSE, LOG_TAG, __VA_ARGS__)

extern int first(int x, int y);
extern void beat();

jint JNICALL Java_com_embe_MainActivity_add(JNIEnv *env, jobject this, jint x, jint y) {
	LOGV("log test %d", 1234);
	return first(x, y);
}

void JNICALL Java_com_embe_MainActivity_testString(JNIEnv *env, jobject this, jstring string) {
	const char *str = (*env)->GetStringUTFChars(env, string, 0);
	jint len = (*env)->GetStringUTFLength(env, string);
	jint ret;

	LOGV("native testString len %d", len);
	LOGV("native testring %s", str);
	LOGV("test start!");

	test(str);
	LOGV("test completed!");

	(*env)->ReleaseStringUTFChars(env, string, str);
}

void JNICALL Java_com_embe_MainActivity_metronome(JNIEnv *env, jobject this) {
	LOGV("**** Metronome starts! ****");
	jint i;
	jint fd;
	const char *buf = {0,};
	jint cnt = 0;
	jint beat_rate = 1;
	jclass thisClass;
	jfieldID fidnumber;
	jint beaton;

	// open devices (fnd, led, dot matrix, and text lcd)
	fd = open(DEV_NAME2, O_RDWR);
	if(fd < 0) {
		printf("Can't open device file %s\n", DEV_NAME2);
		exit(-1);
	}
	// start stopwatch
	while(beat_rate > 0) {
		/*thisClass = (*env)->GetObjectClass(env, this);
		fidnumber = (*env)->GetFieldID(env, thisClass, "beatOn", "I");
		beaton = (*env)->GetIntField(env, this, fidnumber);
		beaton = 0;
		(*env)->SetIntField(env, this, fidnumber, beaton);
		*/
		beat_rate = write(fd, buf, beat_rate);
		thisClass = (*env)->GetObjectClass(env, this);
		fidnumber = (*env)->GetFieldID(env, thisClass, "beatOn", "I");
		beaton = (*env)->GetIntField(env, this, fidnumber);
		beaton = 1;
		(*env)->SetIntField(env, this, fidnumber, beaton);
		/*
		fidnumber = (*env)->GetFieldID(env, thisClass, "delay", "Z");
		beaton = (*env)->GetIntField(env, this, fidnumber);
		beaton = 1;
		(*env)->SetIntField(env, this, fidnumber, beaton);
		*/
		//LOGV("timer %d", cnt++);
		//LOGV("beat_rate: %d", beat_rate);
	}
	beaton = -1;
	(*env)->SetIntField(env, this, fidnumber, beaton);
	// close devices
	close(fd);
}

void JNICALL Java_com_embe_MainActivity_drum(JNIEnv *env, jobject this) {
	jint i;
	jint fd;
	const char *buf = {0,};
	jint cnt = 0;
	jclass thisClass;
	jfieldID fidnumber;
	jint drumPlayed = 0;
	jint beat_rate = 1;

	LOGV("drum starts!");

	// open devices (fnd, led, dot matrix, and text lcd)
	fd = open(DEV_NAME3, O_RDWR);
	if(fd < 0) {
		printf("Can't open device file %s\n", DEV_NAME3);
		exit(-1);
	}
	// start stopwatch
	//while(maxBound++ < 50) {
	// check if there exists an interrupt from FPGA device
	while(beat_rate > 0) {
		drumPlayed = write(fd, buf, 1);
		//usleep(10000);
		// if there exists an interrupt, transfer it to Java code
		thisClass = (*env)->GetObjectClass(env, this);
		fidnumber = (*env)->GetFieldID(env, thisClass, "drumPlayed", "I");
		drumPlayed = (*env)->GetIntField(env, this, fidnumber);
		if(drumPlayed == 1)
			drumPlayed = 1;
		else
			drumPlayed = 0;
		(*env)->SetIntField(env, this, fidnumber, drumPlayed);
	}
	drumPlayed = -1;
	(*env)->SetIntField(env, this, fidnumber, drumPlayed);
	// close devices
	close(fd);
}
