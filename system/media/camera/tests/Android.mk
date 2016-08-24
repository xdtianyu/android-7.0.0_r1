# Build the unit tests.
LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)
LOCAL_ADDITIONAL_DEPENDENCIES := $(LOCAL_PATH)/Android.mk

LOCAL_SHARED_LIBRARIES := \
	libutils \
	libcamera_metadata

LOCAL_C_INCLUDES := \
	system/media/camera/include \
	system/media/private/camera/include

LOCAL_SRC_FILES := \
	camera_metadata_tests.cpp

LOCAL_CFLAGS += -Wall -Wextra -Werror

LOCAL_MODULE := camera_metadata_tests
LOCAL_MODULE_TAGS := tests
LOCAL_MODULE_STEM_32 := camera_metadata_tests
LOCAL_MODULE_STEM_64 := camera_metadata_tests64
LOCAL_MULTILIB := both

include $(BUILD_NATIVE_TEST)
