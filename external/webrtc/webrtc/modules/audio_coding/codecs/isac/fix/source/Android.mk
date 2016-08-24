# Copyright (c) 2012 The WebRTC project authors. All Rights Reserved.
#
# Use of this source code is governed by a BSD-style license
# that can be found in the LICENSE file in the root of the source
# tree. An additional intellectual property rights grant can be found
# in the file PATENTS.  All contributing project authors may
# be found in the AUTHORS file in the root of the source tree.

#############################
# Build the non-neon library.

LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

include $(LOCAL_PATH)/../../../../../../../android-webrtc.mk

LOCAL_ARM_MODE := arm
LOCAL_MODULE_CLASS := STATIC_LIBRARIES
LOCAL_MODULE := libwebrtc_isacfix
LOCAL_MODULE_TAGS := optional
LOCAL_SRC_FILES := \
    arith_routines.c \
    arith_routines_hist.c \
    arith_routines_logist.c \
    bandwidth_estimator.c \
    decode.c \
    decode_bwe.c \
    decode_plc.c \
    encode.c \
    entropy_coding.c \
    fft.c \
    filterbank_tables.c \
    filterbanks.c \
    filters.c \
    initialize.c \
    isacfix.c \
    lattice.c \
    lpc_masking_model.c \
    lpc_tables.c \
    pitch_estimator.c \
    pitch_estimator_c.c \
    pitch_filter.c \
    pitch_gain_tables.c \
    pitch_lag_tables.c \
    spectrum_ar_model_tables.c \
    transform_tables.c \
    transform.c

# TODO: Use pitch_estimator_mips.c for mips, pitch_estimator_c.c

# Using .S (instead of .s) extention is to include a C header file in assembly.
my_as_src := \
    lattice_armv7.S \
    pitch_filter_armv6.S
my_c_src := \
    lattice_c.c \
    pitch_filter_c.c
LOCAL_SRC_FILES_arm += $(my_as_src)
LOCAL_SRC_FILES_x86 += $(my_c_src)
LOCAL_SRC_FILES_mips += $(my_c_src)
LOCAL_SRC_FILES_arm64 += $(my_c_src)
LOCAL_SRC_FILES_x86_64 += $(my_c_src)
LOCAL_SRC_FILES_mips64 += $(my_c_src)

# Flags passed to both C and C++ files.
LOCAL_CFLAGS := \
    $(MY_WEBRTC_COMMON_DEFS)

LOCAL_CFLAGS_arm := $(MY_WEBRTC_COMMON_DEFS_arm)
LOCAL_CFLAGS_x86 := $(MY_WEBRTC_COMMON_DEFS_x86)
LOCAL_CFLAGS_mips := $(MY_WEBRTC_COMMON_DEFS_mips)
LOCAL_CFLAGS_arm64 := $(MY_WEBRTC_COMMON_DEFS_arm64)
LOCAL_CFLAGS_x86_64 := $(MY_WEBRTC_COMMON_DEFS_x86_64)
LOCAL_CFLAGS_mips64 := $(MY_WEBRTC_COMMON_DEFS_mips64)

LOCAL_C_INCLUDES := \
    $(LOCAL_PATH)/../interface \
    $(LOCAL_PATH)/../../../../../../.. \
    $(LOCAL_PATH)/../../../../../../common_audio/signal_processing/include

ifdef WEBRTC_STL
LOCAL_NDK_STL_VARIANT := $(WEBRTC_STL)
LOCAL_SDK_VERSION := 14
LOCAL_MODULE := $(LOCAL_MODULE)_$(WEBRTC_STL)
endif

include $(BUILD_STATIC_LIBRARY)

#########################
# Build the neon library.
ifeq ($(WEBRTC_BUILD_NEON_LIBS),true)

include $(CLEAR_VARS)

LOCAL_ARM_MODE := arm
LOCAL_MODULE_CLASS := STATIC_LIBRARIES
LOCAL_MODULE := libwebrtc_isacfix_neon
LOCAL_MODULE_TAGS := optional
LOCAL_SRC_FILES := \
    filters_neon.c \
    lattice_neon.S \
    lpc_masking_model_neon.S

# Flags passed to both C and C++ files.
LOCAL_CFLAGS := \
    $(MY_WEBRTC_COMMON_DEFS) \
    -mfpu=neon \
    -mfloat-abi=softfp \
    -flax-vector-conversions

LOCAL_MODULE_TARGET_ARCH := arm
LOCAL_CFLAGS_arm := $(MY_WEBRTC_COMMON_DEFS_arm)

LOCAL_C_INCLUDES := \
    $(LOCAL_PATH)/../interface \
    $(LOCAL_PATH)/../../../../../../.. \
    $(LOCAL_PATH)/../../../../../../common_audio/signal_processing/include

ifdef WEBRTC_STL
LOCAL_NDK_STL_VARIANT := $(WEBRTC_STL)
LOCAL_SDK_VERSION := 14
LOCAL_MODULE := $(LOCAL_MODULE)_$(WEBRTC_STL)
endif

include $(BUILD_STATIC_LIBRARY)

endif # ifeq ($(WEBRTC_BUILD_NEON_LIBS),true)
