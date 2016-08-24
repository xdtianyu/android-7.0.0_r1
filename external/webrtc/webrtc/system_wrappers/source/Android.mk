# Copyright (c) 2012 The WebRTC project authors. All Rights Reserved.
#
# Use of this source code is governed by a BSD-style license
# that can be found in the LICENSE file in the root of the source
# tree. An additional intellectual property rights grant can be found
# in the file PATENTS.  All contributing project authors may
# be found in the AUTHORS file in the root of the source tree.

LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

include $(LOCAL_PATH)/../../../android-webrtc.mk

LOCAL_ARM_MODE := arm
LOCAL_MODULE := libwebrtc_system_wrappers
LOCAL_MODULE_TAGS := optional
LOCAL_CPP_EXTENSION := .cc
LOCAL_SRC_FILES := \
    android/cpu-features.c \
    cpu_features_android.c \
    sort.cc \
    aligned_malloc.cc \
    atomic32_posix.cc \
    condition_variable.cc \
    cpu_features.cc \
    cpu_info.cc \
    critical_section.cc \
    event.cc \
    file_impl.cc \
    logging.cc \
    metrics_default.cc \
    rw_lock.cc \
    trace_impl.cc \
    condition_variable_posix.cc \
    critical_section_posix.cc \
    sleep.cc \
    trace_posix.cc \
    rw_lock_posix.cc \

LOCAL_CFLAGS := $(MY_WEBRTC_COMMON_DEFS)
LOCAL_CPPFLAGS := -std=c++0x

LOCAL_CFLAGS_arm := $(MY_WEBRTC_COMMON_DEFS_arm)
LOCAL_CFLAGS_x86 := $(MY_WEBRTC_COMMON_DEFS_x86)
LOCAL_CFLAGS_mips := $(MY_WEBRTC_COMMON_DEFS_mips)
LOCAL_CFLAGS_arm64 := $(MY_WEBRTC_COMMON_DEFS_arm64)
LOCAL_CFLAGS_x86_64 := $(MY_WEBRTC_COMMON_DEFS_x86_64)
LOCAL_CFLAGS_mips64 := $(MY_WEBRTC_COMMON_DEFS_mips64)

LOCAL_C_INCLUDES := \
    $(LOCAL_PATH)/android \
    $(LOCAL_PATH)/../../.. \
    $(LOCAL_PATH)/../interface \
    $(LOCAL_PATH)/spreadsortlib

ifdef WEBRTC_STL
LOCAL_NDK_STL_VARIANT := $(WEBRTC_STL)
LOCAL_SDK_VERSION := 14
LOCAL_MODULE := $(LOCAL_MODULE)_$(WEBRTC_STL)
endif

include $(BUILD_STATIC_LIBRARY)
