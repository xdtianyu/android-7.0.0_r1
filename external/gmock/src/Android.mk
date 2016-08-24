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

# Gmock builds 2 libraries: libgmock and libgmock_main. libgmock
# contains most of the code (assertions...) and libgmock_main just
# provide a common main to run the test (ie if you link against
# libgmock_main you won't/should not provide a main() entry point.
#
# We build these 2 libraries for the target device and for the host if
# it is running linux and using ASTL.
#

# TODO: The targets below have some redundancy. Check if we cannot
# condense them using function(s) for the common code.

LOCAL_PATH := $(call my-dir)

libgmock_target_includes := \
  $(LOCAL_PATH)/.. \
  $(LOCAL_PATH)/../include \
  $(TOP)/external/gtest/include

libgmock_host_includes := \
  $(LOCAL_PATH)/.. \
  $(LOCAL_PATH)/../include \
  $(TOP)/external/gtest/include

libgmock_cflags := \
  -Wno-missing-field-initializers \

#######################################################################
# gmock lib for the NDK

include $(CLEAR_VARS)
LOCAL_ADDITIONAL_DEPENDENCIES := $(LOCAL_PATH)/Android.mk

LOCAL_SDK_VERSION := 9
LOCAL_NDK_STL_VARIANT := gnustl_static

LOCAL_CPP_EXTENSION := .cc
LOCAL_SRC_FILES := gmock-all.cc
LOCAL_C_INCLUDES := $(libgmock_target_includes)
LOCAL_EXPORT_C_INCLUDE_DIRS := $(LOCAL_PATH)/../include
LOCAL_CPPFLAGS := -std=c++11
LOCAL_C_FLAGS += $(libgmock_cflags)
LOCAL_MODULE := libgmock_ndk

include $(BUILD_STATIC_LIBRARY)

#######################################################################
# gmock_main for the NDK

include $(CLEAR_VARS)
LOCAL_ADDITIONAL_DEPENDENCIES := $(LOCAL_PATH)/Android.mk

LOCAL_SDK_VERSION := 9
LOCAL_NDK_STL_VARIANT := gnustl_static

LOCAL_CPP_EXTENSION := .cc
LOCAL_SRC_FILES := gmock_main.cc
LOCAL_C_INCLUDES := $(libgmock_target_includes)
LOCAL_EXPORT_C_INCLUDE_DIRS := $(LOCAL_PATH)/../include
LOCAL_CPPFLAGS := -std=c++11
LOCAL_C_FLAGS += $(libgmock_cflags)
LOCAL_MODULE := libgmock_main_ndk

include $(BUILD_STATIC_LIBRARY)

# Don't build for unbundled branches
ifeq (,$(TARGET_BUILD_APPS))
#######################################################################
# gmock lib host

include $(CLEAR_VARS)

LOCAL_CPP_EXTENSION := .cc
LOCAL_SRC_FILES := gmock-all.cc
LOCAL_C_INCLUDES := $(libgmock_host_includes)
LOCAL_C_FLAGS += $(libgmock_cflags)
LOCAL_EXPORT_C_INCLUDE_DIRS := $(LOCAL_PATH)/../include
LOCAL_MODULE := libgmock_host
LOCAL_MULTILIB := both
LOCAL_SANITIZE := never
LOCAL_RTTI_FLAG := -frtti

include $(BUILD_HOST_STATIC_LIBRARY)

#######################################################################
# gmock_main lib target

include $(CLEAR_VARS)

LOCAL_CLANG := true
LOCAL_CPP_EXTENSION := .cc
LOCAL_SRC_FILES := gmock_main.cc
LOCAL_C_INCLUDES := $(libgmock_host_includes)
LOCAL_EXPORT_C_INCLUDE_DIRS := $(LOCAL_PATH)/../include
LOCAL_C_FLAGS += $(libgmock_cflags)
LOCAL_MODULE := libgmock_main_host
LOCAL_MULTILIB := both
LOCAL_SANITIZE := never

include $(BUILD_HOST_STATIC_LIBRARY)

#######################################################################
# gmock lib target

include $(CLEAR_VARS)

LOCAL_CLANG := true
LOCAL_CPP_EXTENSION := .cc
LOCAL_SRC_FILES := gmock-all.cc
LOCAL_C_INCLUDES := $(libgmock_target_includes)
LOCAL_EXPORT_C_INCLUDE_DIRS := $(LOCAL_PATH)/../include
LOCAL_CFLAGS += $(libgmock_cflags)
LOCAL_MODULE := libgmock
LOCAL_SANITIZE := never
LOCAL_RTTI_FLAG := -frtti

include $(BUILD_STATIC_LIBRARY)

#######################################################################
# gmock_main lib target

include $(CLEAR_VARS)

LOCAL_CPP_EXTENSION := .cc
LOCAL_SRC_FILES := gmock_main.cc
LOCAL_C_INCLUDES := $(libgmock_target_includes)
LOCAL_EXPORT_C_INCLUDE_DIRS := $(LOCAL_PATH)/../include
LOCAL_CFLAGS += $(libgmock_cflags)
LOCAL_MODULE := libgmock_main
LOCAL_SANITIZE := never

include $(BUILD_STATIC_LIBRARY)
endif
