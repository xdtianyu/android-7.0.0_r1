# Build the unit tests for audio_utils

LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

LOCAL_SHARED_LIBRARIES := \
	liblog \
	libcutils \
	libaudioutils

LOCAL_C_INCLUDES := \
	$(call include-path-for, audio-utils)

LOCAL_SRC_FILES := \
	primitives_tests.cpp

LOCAL_MODULE := primitives_tests
LOCAL_MODULE_TAGS := tests

LOCAL_CFLAGS := -Werror -Wall
include $(BUILD_NATIVE_TEST)

include $(CLEAR_VARS)
LOCAL_SRC_FILES := fifo_tests.cpp
LOCAL_MODULE := fifo_tests
LOCAL_C_INCLUDES := $(call include-path-for, audio-utils)
LOCAL_SHARED_LIBRARIES := libaudioutils
# libmedia libbinder libcutils libutils
LOCAL_STATIC_LIBRARIES := libsndfile
LOCAL_CFLAGS := -Werror -Wall
include $(BUILD_EXECUTABLE)

include $(CLEAR_VARS)
LOCAL_SRC_FILES := fifo_tests.cpp
LOCAL_MODULE := fifo_tests
LOCAL_C_INCLUDES := $(call include-path-for, audio-utils)
# libmedia libbinder libcutils libutils
LOCAL_STATIC_LIBRARIES := libsndfile libaudioutils liblog
LOCAL_CFLAGS := -Werror -Wall
include $(BUILD_HOST_EXECUTABLE)

include $(CLEAR_VARS)
LOCAL_SRC_FILES := limiter_tests.c
LOCAL_MODULE := limiter_tests
LOCAL_C_INCLUDES := $(call include-path-for, audio-utils)
LOCAL_STATIC_LIBRARIES := libaudioutils
LOCAL_CFLAGS := -Werror -Wall
include $(BUILD_HOST_EXECUTABLE)
