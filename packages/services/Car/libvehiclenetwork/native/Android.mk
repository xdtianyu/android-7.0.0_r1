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
#
LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)

LOCAL_SRC_FILES := $(patsubst ./%,%, $(shell cd $(LOCAL_PATH); \
    find . -name "*.cpp" -and -not -name ".*"))

LOCAL_C_INCLUDES += \
    libcore/include \
    frameworks/base/include \
    packages/services/Car/libvehiclenetwork/include

LOCAL_SHARED_LIBRARIES := \
    liblog \
    libutils \
    libhardware \
    libbinder \
    libprotobuf-cpp-lite

LOCAL_STATIC_LIBRARIES := \
    libvehiclenetworkproto-native

LOCAL_STRIP_MODULE := keep_symbols

LOCAL_MODULE := libvehiclenetwork-native
LOCAL_MODULE_TAGS := optional

include $(BUILD_SHARED_LIBRARY)
