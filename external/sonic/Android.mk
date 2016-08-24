LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

LOCAL_SRC_FILES := sonic.c

LOCAL_CFLAGS += -Wno-sequence-point -Wno-extra

LOCAL_CPPFLAGS += -std=c++98

LOCAL_MODULE:= libsonic

include $(BUILD_SHARED_LIBRARY)

