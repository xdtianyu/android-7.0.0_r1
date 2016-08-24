# Copyright (c) 2015 The WebRTC project authors. All Rights Reserved.
#
# Use of this source code is governed by a BSD-style license
# that can be found in the LICENSE file in the root of the source
# tree. An additional intellectual property rights grant can be found
# in the file PATENTS.  All contributing project authors may
# be found in the AUTHORS file in the root of the source tree.

LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

include $(LOCAL_PATH)/../../android-webrtc.mk

LOCAL_ARM_MODE := arm
LOCAL_MODULE_CLASS := STATIC_LIBRARIES
LOCAL_MODULE := libwebrtc_common
LOCAL_MODULE_TAGS := optional
LOCAL_CPP_EXTENSION := .cc
LOCAL_SRC_FILES := \
    audio_converter.cc \
    audio_util.cc \
    blocker.cc \
    channel_buffer.cc \
    fft4g.c \
    fir_filter.cc \
    lapped_transform.cc \
    real_fourier_ooura.cc \
    real_fourier.cc \
    ring_buffer.c \
    audio_ring_buffer.cc \
    sparse_fir_filter.cc \
    window_generator.cc \

ifeq ($(TARGET_ARCH), $(filter $(TARGET_ARCH),x86 x86_64))
LOCAL_SRC_FILES += fir_filter_sse.cc
endif

# Flags passed to both C and C++ files.
LOCAL_CFLAGS := \
    $(MY_WEBRTC_COMMON_DEFS)

LOCAL_CFLAGS_arm := $(MY_WEBRTC_COMMON_DEFS_arm)
LOCAL_CFLAGS_x86 := $(MY_WEBRTC_COMMON_DEFS_x86)
LOCAL_CFLAGS_mips := $(MY_WEBRTC_COMMON_DEFS_mips)
LOCAL_CFLAGS_arm64 := $(MY_WEBRTC_COMMON_DEFS_arm64)
LOCAL_CFLAGS_x86_64 := $(MY_WEBRTC_COMMON_DEFS_x86_64)
LOCAL_CFLAGS_mips64 := $(MY_WEBRTC_COMMON_DEFS_mips64)

# Include paths placed before CFLAGS/CPPFLAGS
LOCAL_C_INCLUDES := \
    $(LOCAL_PATH) \
    $(LOCAL_PATH)/../.. \

ifdef WEBRTC_STL
LOCAL_NDK_STL_VARIANT := $(WEBRTC_STL)
LOCAL_SDK_VERSION := 14
LOCAL_MODULE := $(LOCAL_MODULE)_$(WEBRTC_STL)
endif

include $(BUILD_STATIC_LIBRARY)
