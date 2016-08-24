LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_MODULE := rmidevice
LOCAL_SRC_FILES := rmifunction.cpp rmidevice.cpp hiddevice.cpp
LOCAL_CPPFLAGS := -Wall

include $(BUILD_STATIC_LIBRARY)