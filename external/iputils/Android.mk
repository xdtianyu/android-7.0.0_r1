LOCAL_PATH:= $(call my-dir)

iputils_cflags := \
  -Wno-missing-field-initializers \
  -Wno-sign-compare \
  -Wno-unused-parameter \

include $(CLEAR_VARS)
LOCAL_CFLAGS := $(iputils_cflags)
LOCAL_MODULE := ping
LOCAL_SRC_FILES := ping.c ping_common.c
include $(BUILD_EXECUTABLE)

include $(CLEAR_VARS)
LOCAL_CFLAGS := $(iputils_cflags)
LOCAL_MODULE := ping6
LOCAL_SHARED_LIBRARIES := libcrypto
LOCAL_SRC_FILES := ping6.c ping_common.c
include $(BUILD_EXECUTABLE)

include $(CLEAR_VARS)
LOCAL_CFLAGS := $(iputils_cflags)
LOCAL_MODULE := tracepath
LOCAL_MODULE_TAGS := debug
LOCAL_SRC_FILES := tracepath.c
include $(BUILD_EXECUTABLE)

include $(CLEAR_VARS)
LOCAL_CFLAGS := $(iputils_cflags)
LOCAL_MODULE := tracepath6
LOCAL_MODULE_TAGS := debug
LOCAL_SRC_FILES := tracepath6.c
include $(BUILD_EXECUTABLE)

include $(CLEAR_VARS)
LOCAL_CFLAGS := $(iputils_cflags)
LOCAL_MODULE := traceroute6
LOCAL_MODULE_TAGS := debug
LOCAL_SRC_FILES := traceroute6.c
include $(BUILD_EXECUTABLE)
