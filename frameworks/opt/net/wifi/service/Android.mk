# Copyright (C) 2011 The Android Open Source Project
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

ifneq ($(TARGET_BUILD_PDK), true)

# Make HAL stub library 1
# ============================================================

include $(CLEAR_VARS)

LOCAL_REQUIRED_MODULES :=

LOCAL_CFLAGS += -Wall -Werror -Wextra -Wno-unused-parameter -Wno-unused-function \
                -Wunused-variable -Winit-self -Wwrite-strings -Wshadow

LOCAL_C_INCLUDES += \
	external/libnl-headers \
	$(call include-path-for, libhardware_legacy)/hardware_legacy

LOCAL_SRC_FILES := \
	lib/wifi_hal.cpp

LOCAL_MODULE := libwifi-hal

include $(BUILD_STATIC_LIBRARY)

# Make HAL stub library 2
# ============================================================

include $(CLEAR_VARS)

LOCAL_REQUIRED_MODULES :=

LOCAL_CFLAGS += -Wall -Werror -Wextra -Wno-unused-parameter -Wno-unused-function \
                -Wunused-variable -Winit-self -Wwrite-strings -Wshadow

LOCAL_C_INCLUDES += \
	$(LOCAL_PATH)/jni \
	external/libnl-headers \
	$(call include-path-for, libhardware_legacy)/hardware_legacy

LOCAL_SRC_FILES := \
	lib/wifi_hal_stub.cpp

LOCAL_MODULE := libwifi-hal-stub

include $(BUILD_STATIC_LIBRARY)

# set correct hal library path
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

# Make the JNI part
# ============================================================
include $(CLEAR_VARS)

LOCAL_REQUIRED_MODULES := libhardware_legacy

LOCAL_CFLAGS += -Wall -Werror -Wextra -Wno-unused-parameter -Wno-unused-function \
                -Wunused-variable -Winit-self -Wwrite-strings -Wshadow

LOCAL_C_INCLUDES += \
	$(JNI_H_INCLUDE) \
	$(call include-path-for, libhardware)/hardware \
	$(call include-path-for, libhardware_legacy)/hardware_legacy \
	libcore/include

LOCAL_SHARED_LIBRARIES += \
	libnativehelper \
	libcutils \
	libutils \
	libhardware \
	libhardware_legacy \
	libnl \
	libdl

LOCAL_STATIC_LIBRARIES += libwifi-hal-stub
LOCAL_STATIC_LIBRARIES += $(LIB_WIFI_HAL)

LOCAL_SRC_FILES := \
	jni/com_android_server_wifi_WifiNative.cpp \
	jni/jni_helper.cpp

ifdef INCLUDE_NAN_FEATURE
LOCAL_SRC_FILES += \
	jni/com_android_server_wifi_nan_WifiNanNative.cpp
endif

LOCAL_MODULE := libwifi-service
# b/22172328
LOCAL_CLANG := false

include $(BUILD_SHARED_LIBRARY)

# Build the java code
# ============================================================

include $(CLEAR_VARS)

LOCAL_AIDL_INCLUDES := $(LOCAL_PATH)/java
LOCAL_SRC_FILES := $(call all-java-files-under, java) \
	$(call all-Iaidl-files-under, java) \
	$(call all-logtags-files-under, java) \
	$(call all-proto-files-under, proto)

ifndef INCLUDE_NAN_FEATURE
LOCAL_SRC_FILES := $(filter-out $(call all-java-files-under, \
          java/com/android/server/wifi/nan),$(LOCAL_SRC_FILES))
endif

LOCAL_JAVA_LIBRARIES := bouncycastle conscrypt services
LOCAL_REQUIRED_MODULES := services
LOCAL_MODULE_TAGS :=
LOCAL_MODULE := wifi-service
LOCAL_PROTOC_OPTIMIZE_TYPE := nano

include $(BUILD_JAVA_LIBRARY)

endif
