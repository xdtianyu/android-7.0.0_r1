#
#  Copyright (C) 2015 Google, Inc.
#
#  Licensed under the Apache License, Version 2.0 (the "License");
#  you may not use this file except in compliance with the License.
#  You may obtain a copy of the License at:
#
#  http://www.apache.org/licenses/LICENSE-2.0
#
#  Unless required by applicable law or agreed to in writing, software
#  distributed under the License is distributed on an "AS IS" BASIS,
#  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#  See the License for the specific language governing permissions and
#  limitations under the License.
#

LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE_TAGS := optional
LOCAL_MODULE := sl4n

LOCAL_C_INCLUDES += \
  frameworks/opt/net/wifi/service/jni \
  hardware/libhardware_legacy/include/hardware_legacy \
  system/bt \
  $(LOCAL_PATH)/rapidjson/include \
  $(LOCAL_PATH)/facades

LOCAL_SRC_FILES := \
  facades/bluetooth/bluetooth_binder_facade.cpp \
  facades/wifi/wifi_facade.cpp \
  main.cpp \
  utils/command_receiver.cpp \
  utils/common_utils.cpp

LOCAL_SHARED_LIBRARIES += \
  libbinder \
  libchrome \
  libcutils \
  libutils \
  libhardware \
  libhardware_legacy \
  liblog

LOCAL_STATIC_LIBRARIES += \
  libbtcore \
  libosi \
  libbluetooth-client

# set correct Wi-Fi HAL library path and add Wi-Fi related libraries
# ============================================================
LIB_WIFI_HAL := libwifi-hal

ifeq ($(BOARD_WLAN_DEVICE), bcmdhd)
  LIB_WIFI_HAL := libwifi-hal-bcm
else ifeq ($(BOARD_WLAN_DEVICE), qcwcn)
  LIB_WIFI_HAL := libwifi-hal-qcom
else ifeq ($(BOARD_WLAN_DEVICE), mrvl)
  # this is commented because none of the nexus devices
  # that sport Marvell's wifi have support for HAL
  # LIB_WIFI_HAL := libwifi-hal-mrvl
else ifeq ($(BOARD_WLAN_DEVICE), MediaTek)
  # support MTK WIFI HAL
  LIB_WIFI_HAL := libwifi-hal-mt66xx
endif

LOCAL_STATIC_LIBRARIES += \
  $(LIB_WIFI_HAL) \
  libnl \
  libwifi-hal-stub


LOCAL_CFLAGS += -std=c++11 -Wall -Wno-unused-parameter -Wno-missing-field-initializers

include $(BUILD_EXECUTABLE)
