# Copyright (c) 2011 The WebRTC project authors. All Rights Reserved.
#
# Use of this source code is governed by a BSD-style license
# that can be found in the LICENSE file in the root of the source
# tree. An additional intellectual property rights grant can be found
# in the file PATENTS.  All contributing project authors may
# be found in the AUTHORS file in the root of the source tree.

LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
include $(LOCAL_PATH)/android-webrtc.mk

LOCAL_ARM_MODE := arm
LOCAL_MODULE := libwebrtc_audio_preprocessing
LOCAL_MODULE_TAGS := optional

LOCAL_WHOLE_STATIC_LIBRARIES := \
    libwebrtc_aec \
    libwebrtc_aecm \
    libwebrtc_agc \
    libwebrtc_apm \
    libwebrtc_apm_utility \
    libwebrtc_apvad \
    libwebrtc_base \
    libwebrtc_beamformer \
    libwebrtc_common \
    libwebrtc_intell \
    libwebrtc_isac \
    libwebrtc_resampler \
    libwebrtc_ns \
    libwebrtc_spl \
    libwebrtc_system_wrappers \
    libwebrtc_transient \
    libwebrtc_vad \

# Add Neon libraries.
ifeq ($(WEBRTC_BUILD_NEON_LIBS),true)
LOCAL_WHOLE_STATIC_LIBRARIES_arm += \
    libwebrtc_aecm_neon \
    libwebrtc_ns_neon
endif

LOCAL_SHARED_LIBRARIES := \
    libcutils \
    libdl \
    libprotobuf-cpp-lite \

include $(BUILD_SHARED_LIBRARY)


include $(CLEAR_VARS)
include $(LOCAL_PATH)/android-webrtc.mk

LOCAL_ARM_MODE := arm
LOCAL_MODULE := libwebrtc_audio_coding
LOCAL_MODULE_TAGS := optional

LOCAL_WHOLE_STATIC_LIBRARIES := \
    libwebrtc_base \
    libwebrtc_common \
    libwebrtc_intell \
    libwebrtc_isac \
    libwebrtc_isacfix \
    libwebrtc_resampler \
    libwebrtc_spl \
    libwebrtc_system_wrappers \

ifeq ($(WEBRTC_BUILD_NEON_LIBS),true)
LOCAL_WHOLE_STATIC_LIBRARIES_arm += \
    libwebrtc_isacfix_neon
endif

LOCAL_SHARED_LIBRARIES := \
    libprotobuf-cpp-lite \
    libcutils \
    libdl \

include $(BUILD_SHARED_LIBRARY)


include $(CLEAR_VARS)
include $(LOCAL_PATH)/android-webrtc.mk

LOCAL_ARM_MODE := arm
LOCAL_MODULE := libwebrtc_audio_coding_gnustl_static
LOCAL_MODULE_TAGS := optional

LOCAL_WHOLE_STATIC_LIBRARIES := \
    libwebrtc_isac_gnustl_static \
    libwebrtc_isacfix_gnustl_static \
    libwebrtc_spl_gnustl_static \
    libwebrtc_system_wrappers_gnustl_static
ifeq ($(WEBRTC_BUILD_NEON_LIBS),true)
LOCAL_WHOLE_STATIC_LIBRARIES_arm += \
    libwebrtc_isacfix_neon_gnustl_static
endif

LOCAL_STATIC_LIBRARIES := \
    libprotobuf-cpp-lite

LOCAL_NDK_STL_VARIANT := gnustl_static
LOCAL_SDK_VERSION := 14

include $(BUILD_STATIC_LIBRARY)

webrtc_path := $(LOCAL_PATH)
# voice
include $(webrtc_path)/webrtc/base/Android.mk
include $(webrtc_path)/webrtc/common_audio/Android.mk
include $(webrtc_path)/webrtc/common_audio/resampler/Android.mk
include $(webrtc_path)/webrtc/common_audio/signal_processing/Android.mk
include $(webrtc_path)/webrtc/common_audio/vad/Android.mk
include $(webrtc_path)/webrtc/modules/audio_coding/codecs/isac/fix/source/Android.mk
include $(webrtc_path)/webrtc/modules/audio_coding/codecs/isac/main/source/Android.mk
include $(webrtc_path)/webrtc/modules/audio_processing/Android.mk
include $(webrtc_path)/webrtc/modules/audio_processing/aec/Android.mk
include $(webrtc_path)/webrtc/modules/audio_processing/aecm/Android.mk
include $(webrtc_path)/webrtc/modules/audio_processing/agc/Android.mk
include $(webrtc_path)/webrtc/modules/audio_processing/beamformer/Android.mk
include $(webrtc_path)/webrtc/modules/audio_processing/intelligibility/Android.mk
include $(webrtc_path)/webrtc/modules/audio_processing/ns/Android.mk
include $(webrtc_path)/webrtc/modules/audio_processing/transient/Android.mk
include $(webrtc_path)/webrtc/modules/audio_processing/utility/Android.mk
include $(webrtc_path)/webrtc/modules/audio_processing/vad/Android.mk
include $(webrtc_path)/webrtc/system_wrappers/source/Android.mk

# libwebrtc_audio_coding_gnustl_static dependencies
WEBRTC_STL := gnustl_static
include $(webrtc_path)/webrtc/system_wrappers/source/Android.mk
include $(webrtc_path)/webrtc/modules/audio_coding/codecs/isac/fix/source/Android.mk
include $(webrtc_path)/webrtc/modules/audio_coding/codecs/isac/main/source/Android.mk
include $(webrtc_path)/webrtc/common_audio/signal_processing/Android.mk
