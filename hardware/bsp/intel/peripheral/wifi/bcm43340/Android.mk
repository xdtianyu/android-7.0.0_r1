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

ifeq ($(WIFI_DRIVER_HAL_PERIPHERAL),bcm43340)

LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_CPPFLAGS += \
    -DWIFI_DRIVER_NVRAM_PATH_PARAM=\"$(WIFI_DRIVER_NVRAM_PATH_PARAM)\" \
    -DWIFI_DRIVER_FW_PATH_PARAM=\"$(WIFI_DRIVER_FW_PATH_PARAM)\" \
    -DWIFI_DRIVER_NVRAM_PATH=\"$(WIFI_DRIVER_NVRAM_PATH)\" \
    -DWIFI_DRIVER_FW_PATH_STA=\"$(WIFI_DRIVER_FW_PATH_STA)\" \
    -DWIFI_DRIVER_FW_PATH_AP=\"$(WIFI_DRIVER_FW_PATH_AP)\" \
    -DWIFI_DRIVER_FW_PATH_P2P=\"$(WIFI_DRIVER_FW_PATH_P2P)\"
LOCAL_CFLAGS += -Wno-unused-parameter
LOCAL_C_INCLUDES += device/generic/brillo/wifi_driver_hal/include
LOCAL_SHARED_LIBRARIES := libcutils
LOCAL_SRC_FILES := bcm43340_hal.cpp
LOCAL_MODULE := $(WIFI_DRIVER_HAL_MODULE)
LOCAL_MODULE_RELATIVE_PATH := hw
LOCAL_MODULE_TAGS := optional

include $(BUILD_SHARED_LIBRARY)

endif
