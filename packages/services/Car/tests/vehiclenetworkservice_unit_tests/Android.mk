#
#  Copyright (C) 2016 The Android Open Source Project
#
#  Licensed under the Apache License, Version 2.0 (the "License");
#  you may not use this file except in compliance with the License.
#  You may obtain a copy of the License at
#
#       http://www.apache.org/licenses/LICENSE-2.0
#
#  Unless required by applicable law or agreed to in writing, software
#  distributed under the License is distributed on an "AS IS" BASIS,
#  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#  See the License for the specific language governing permissions and
#  limitations under the License.

LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)

LOCAL_SRC_FILES := $(patsubst ./%,%, $(shell cd $(LOCAL_PATH); \
    find . -name "*.cpp" -and -not -name ".*")) \

LOCAL_C_INCLUDES += \
    packages/services/Car/libvehiclenetwork/include \
    packages/services/Car/libvehiclenetwork/libvehiclenetwork-audio-helper/include\
    packages/services/Car/vehicle_network_service \
    external/libxml2/include \
    external/icu/icu4c/source/common

LOCAL_SHARED_LIBRARIES := \
    libbinder \
    liblog \
    libutils \
    libvehiclenetwork-native \
    libcutils \
    libxml2

LOCAL_STATIC_LIBRARIES := \
    libvehiclenetwork-audio-helper \
    libvehiclenetworkservice

LOCAL_CFLAGS += -Wall -Wextra

LOCAL_STRIP_MODULE := keep_symbols

LOCAL_MODULE:= vehiclenetworkservice_unit_tests
LOCAL_MODULE_TAGS := tests

include $(BUILD_NATIVE_TEST)
