# Copyright (C) 2015 The Android Open Source Project
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

#
# Nanohub sensor HAL usage instructions:
#
# Add the following to your device.mk file.
#
# # Enable the nanohub sensor HAL
# TARGET_USES_NANOHUB_SENSORHAL := true
#
# # Nanohub sensor list source file
# NANOHUB_SENSORHAL_SENSORLIST := $(LOCAL_PATH)/sensorhal/sensorlist.cpp
#
# # Sensor HAL name override (optional)
# NANOHUB_SENSORHAL_NAME_OVERRIDE := sensors.nanohub
#
# # Enable lid-state reporting (optional)
# NANOHUB_SENSORHAL_LID_STATE_ENABLED := true
#
# # Enable mag-bias reporting (optional)
# NANOHUB_SENSORHAL_USB_MAG_BIAS_ENABLED := true
#

LOCAL_PATH := $(call my-dir)

ifeq ($(TARGET_USES_NANOHUB_SENSORHAL), true)

COMMON_CFLAGS := -Wall -Werror -Wextra

################################################################################

include $(CLEAR_VARS)

ifeq ($(NANOHUB_SENSORHAL_NAME_OVERRIDE),)
LOCAL_MODULE := sensors.$(TARGET_DEVICE)
else
LOCAL_MODULE := $(NANOHUB_SENSORHAL_NAME_OVERRIDE)
endif

LOCAL_MODULE_RELATIVE_PATH := hw
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_OWNER := google

LOCAL_CFLAGS += $(COMMON_CFLAGS)

LOCAL_C_INCLUDES += \
	device/google/contexthub/firmware/inc \
	device/google/contexthub/util/common

LOCAL_SRC_FILES := \
	sensors.cpp \
	../../../../$(NANOHUB_SENSORHAL_SENSORLIST)

LOCAL_SHARED_LIBRARIES := \
	libcutils \
	libhubconnection \
	libstagefright_foundation \
	libutils

include $(BUILD_SHARED_LIBRARY)

################################################################################

include $(CLEAR_VARS)

LOCAL_MODULE := activity_recognition.$(TARGET_DEVICE)
LOCAL_MODULE_RELATIVE_PATH := hw
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_OWNER := google

LOCAL_CFLAGS += $(COMMON_CFLAGS)

LOCAL_C_INCLUDES += \
	device/google/contexthub/firmware/inc \
	device/google/contexthub/util/common

LOCAL_SRC_FILES := \
	activity.cpp

LOCAL_SHARED_LIBRARIES := \
	libcutils \
	libhubconnection \
	liblog \
	libstagefright_foundation \
	libutils

include $(BUILD_SHARED_LIBRARY)

################################################################################

include $(CLEAR_VARS)

LOCAL_MODULE := libhubconnection
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_OWNER := google

LOCAL_CFLAGS += $(COMMON_CFLAGS)

ifeq ($(NANOHUB_SENSORHAL_LID_STATE_ENABLED), true)
LOCAL_CFLAGS += -DLID_STATE_REPORTING_ENABLED
endif

ifeq ($(NANOHUB_SENSORHAL_USB_MAG_BIAS_ENABLED), true)
LOCAL_CFLAGS += -DUSB_MAG_BIAS_REPORTING_ENABLED
endif

LOCAL_C_INCLUDES += \
	device/google/contexthub/firmware/inc \
	device/google/contexthub/util/common

LOCAL_SRC_FILES := \
	hubconnection.cpp \
	../util/common/file.cpp \
	../util/common/JSONObject.cpp \
	../util/common/ring.cpp

LOCAL_SHARED_LIBRARIES := \
	libcutils \
	liblog \
	libstagefright_foundation \
	libutils

include $(BUILD_SHARED_LIBRARY)

################################################################################

endif
