LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

LOCAL_NDK_STL_VARIANT := stlport_static

LOCAL_SRC_FILES:= \
	compute.cpp

LOCAL_STATIC_LIBRARIES := \
	libRScpp_static

LOCAL_CFLAGS := -std=c++11
LOCAL_LDFLAGS += -llog -ldl

LOCAL_MODULE:= rstest-cppf16

LOCAL_MODULE_TAGS := tests

intermediates := $(call intermediates-dir-for,STATIC_LIBRARIES,libRS,TARGET,)

LOCAL_C_INCLUDES += frameworks/rs/cpp
LOCAL_C_INCLUDES += frameworks/rs
LOCAL_C_INCLUDES += $(intermediates)

LOCAL_CLANG := true

include $(BUILD_EXECUTABLE)

