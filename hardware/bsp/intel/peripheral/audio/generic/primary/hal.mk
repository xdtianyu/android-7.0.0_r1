# Copyright (C) 2012 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE_RELATIVE_PATH := hw
LOCAL_SRC_FILES := \
	../audio_hal.c
LOCAL_C_INCLUDES += \
	external/tinyalsa/include \
	$(call include-path-for, audio-utils) \
	$(call include-path-for, alsa-utils)
LOCAL_SHARED_LIBRARIES := liblog libcutils libtinyalsa libaudioutils libalsautils
LOCAL_MODULE_TAGS := optional

# setting to build for primary audio or usb audio
# set -DTARGET_AUDIO_PRIMARY to 1 for Primary (audio jack)
# set -DTARGET_AUDIO_PRIMARY to 0 for USB audio
LOCAL_CFLAGS := -Wno-unused-parameter -DTARGET_AUDIO_PRIMARY=1
LOCAL_MODULE := audio.primary.$(TARGET_DEVICE)

include $(BUILD_SHARED_LIBRARY)

