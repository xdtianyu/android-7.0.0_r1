LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
	radio_metadata.c

LOCAL_C_INCLUDES:= \
	system/media/radio/include \
	system/media/private/radio/include

LOCAL_SHARED_LIBRARIES := \
	libcutils \
	liblog

LOCAL_MODULE := libradio_metadata
LOCAL_MODULE_TAGS := optional

LOCAL_CFLAGS := -Werror -Wall
LOCAL_CFLAGS += -fvisibility=hidden

LOCAL_EXPORT_C_INCLUDE_DIRS := $(LOCAL_PATH)/../include

include $(BUILD_SHARED_LIBRARY)
