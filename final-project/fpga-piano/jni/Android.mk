LOCAL_PATH:=$(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE:=ndk-prj
LOCAL_SRC_FILES:=first.c second.c metronome.c
LOCAL_LDLIBS := -llog

include $(BUILD_SHARED_LIBRARY)
