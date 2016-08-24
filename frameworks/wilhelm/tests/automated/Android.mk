# Build the unit tests.
LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)
LOCAL_ADDITIONAL_DEPENDENCIES := $(LOCAL_PATH)/Android.mk

LOCAL_MODULE_TAGS := tests

LOCAL_C_INCLUDES:= \
    $(call include-path-for, wilhelm) \
    $(call include-path-for, wilhelm-ut)

LOCAL_SRC_FILES:= \
    BufferQueue_test.cpp

LOCAL_SHARED_LIBRARIES := \
	libOpenSLES \

LOCAL_STATIC_LIBRARIES := \
    libOpenSLESUT \

LOCAL_CFLAGS := -Werror -Wall
ifeq ($(TARGET_OS),linux)
	LOCAL_CFLAGS += -DXP_UNIX
endif

LOCAL_MODULE:= BufferQueue_test

include $(BUILD_NATIVE_TEST)

# Build the manual test programs.
include $(call all-makefiles-under,$(LOCAL_PATH))
