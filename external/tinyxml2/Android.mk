LOG_TO_ANDROID_LOGCAT := true

LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

LOCAL_SRC_FILES:= tinyxml2.cpp

LOCAL_MODULE:=libtinyxml2
LOCAL_MODULE_TAGS := optional

ifeq ($(LOG_TO_ANDROID_LOGCAT),true)
LOCAL_CFLAGS+= -DDEBUG -DANDROID_NDK
endif

include $(BUILD_STATIC_LIBRARY)

include $(CLEAR_VARS)

LOCAL_SRC_FILES:= tinyxml2.cpp

LOCAL_MODULE:=libtinyxml2
LOCAL_MODULE_TAGS := optional

include $(BUILD_HOST_STATIC_LIBRARY)
