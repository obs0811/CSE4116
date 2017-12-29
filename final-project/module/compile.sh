#!/bin/bash

argc=$#
argv0=$0
argv1=$1
argv2=$2

source /root/.bashrc
arm-none-linux-gnueabi-gcc -static -o $argv1 $argv2
adb push $argv1 /sdcard/
