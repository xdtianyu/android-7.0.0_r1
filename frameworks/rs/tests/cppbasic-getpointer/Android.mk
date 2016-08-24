LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)
LOCAL_ADDITIONAL_DEPENDENCIES := $(LOCAL_PATH)/Android.mk

LOCAL_SRC_FILES:= \
	mono.rs \
	compute.cpp

LOCAL_SHARED_LIBRARIES := \
	libRScpp \

LOCAL_MODULE:= rstest-compute-getpointer

LOCAL_MODULE_TAGS := tests

intermediates := $(call intermediates-dir-for,STATIC_LIBRARIES,libRS,TARGET,)

LOCAL_CFLAGS := -std=c++11

LOCAL_C_INCLUDES += frameworks/rs/cpp
LOCAL_C_INCLUDES += frameworks/rs
LOCAL_C_INCLUDES += $(intermediates)

LOCAL_CLANG := true

include $(BUILD_EXECUTABLE)

