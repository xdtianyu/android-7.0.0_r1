# Copyright (c) 2012 The WebRTC project authors. All Rights Reserved.
#
# Use of this source code is governed by a BSD-style license
# that can be found in the LICENSE file in the root of the source
# tree. An additional intellectual property rights grant can be found
# in the file PATENTS.  All contributing project authors may
# be found in the AUTHORS file in the root of the source tree.

###########################
# isac test app

LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE_TAGS := tests
LOCAL_CPP_EXTENSION := .cc
LOCAL_SRC_FILES:= kenny.cc

# Flags passed to both C and C++ files.
LOCAL_CFLAGS := $(MY_WEBRTC_COMMON_DEFS)

LOCAL_CFLAGS_arm := $(MY_WEBRTC_COMMON_DEFS_arm)
LOCAL_CFLAGS_x86 := $(MY_WEBRTC_COMMON_DEFS_x86)
LOCAL_CFLAGS_mips := $(MY_WEBRTC_COMMON_DEFS_mips)
LOCAL_CFLAGS_arm64 := $(MY_WEBRTC_COMMON_DEFS_arm64)
LOCAL_CFLAGS_x86_64 := $(MY_WEBRTC_COMMON_DEFS_x86_64)
LOCAL_CFLAGS_mips64 := $(MY_WEBRTC_COMMON_DEFS_mips64)

LOCAL_C_INCLUDES := \
    $(LOCAL_PATH)/../include \
    $(LOCAL_PATH)/../../../../../../..

MY_LIB_SUFFIX :=
ifdef WEBRTC_STL
MY_LIB_SUFFIX := _$(WEBRTC_STL)
endif

LOCAL_STATIC_LIBRARIES := \
    libwebrtc_isacfix$(MY_LIB_SUFFIX) \
    libwebrtc_spl$(MY_LIB_SUFFIX) \
    libwebrtc_system_wrappers$(MY_LIB_SUFFIX)

ifeq ($(WEBRTC_BUILD_NEON_LIBS),true)
# We need to dup libwebrtc_isacfix$(MY_LIB_SUFFIX) because ibwebrtc_isacfix_neon$(MY_LIB_SUFFIX)
# has dependency on it.
LOCAL_STATIC_LIBRARIES_arm += \
    libwebrtc_isacfix$(MY_LIB_SUFFIX) \
    libwebrtc_isacfix_neon$(MY_LIB_SUFFIX)
endif

LOCAL_SHARED_LIBRARIES := \
    libutils

LOCAL_MODULE := webrtc_isac_test

ifdef WEBRTC_STL
LOCAL_NDK_STL_VARIANT := $(WEBRTC_STL)
LOCAL_SDK_VERSION := 14
LOCAL_MODULE := $(LOCAL_MODULE)_$(WEBRTC_STL)
LOCAL_SHARED_LIBRARIES :=
endif

include $(BUILD_NATIVE_TEST)
