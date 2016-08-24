# This is the Android makefile for google3/third_party/libsrtp so that we can
# build it with the Android NDK.

LOCAL_PATH := $(call my-dir)

common_SRC_FILES := \
    files/source/compare.cc \
    files/source/convert.cc \
    files/source/convert_argb.cc \
    files/source/convert_from.cc \
    files/source/cpu_id.cc \
    files/source/format_conversion.cc \
    files/source/mjpeg_decoder.cc \
    files/source/planar_functions.cc \
    files/source/rotate.cc \
    files/source/rotate_argb.cc \
    files/source/row_common.cc \
    files/source/row_posix.cc \
    files/source/scale.cc \
    files/source/scale_argb.cc \
    files/source/video_common.cc

common_CFLAGS := -Wall -fexceptions -DHAVE_JPEG

common_C_INCLUDES = $(LOCAL_PATH)/files/include

# For the device
# =====================================================
# Device static library

include $(CLEAR_VARS)

LOCAL_CPP_EXTENSION := .cc

LOCAL_SDK_VERSION := 9
LOCAL_NDK_STL_VARIANT := stlport_static

LOCAL_SRC_FILES := $(common_SRC_FILES)
LOCAL_CFLAGS += $(common_CFLAGS)
LOCAL_C_INCLUDES += $(common_C_INCLUDES)
LOCAL_SHARED_LIBRARIES := libjpeg

LOCAL_EXPORT_C_INCLUDE_DIRS := $(LOCAL_PATH)/files/include

ifeq ($(ARCH_ARM_HAVE_NEON),true)
    LOCAL_CFLAGS_arm += -DLIBYUV_NEON
    LOCAL_SRC_FILES_arm += \
        files/source/compare_neon.cc \
        files/source/rotate_neon.cc \
        files/source/row_neon.cc \
        files/source/scale_neon.cc
endif

LOCAL_MODULE := libyuv_static

include $(BUILD_STATIC_LIBRARY)
