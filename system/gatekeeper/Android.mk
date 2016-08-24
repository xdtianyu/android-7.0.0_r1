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

LOCAL_PATH := $(call my-dir)

###
# libgatekeeper contains just the code necessary to communicate with a
# GoogleGateKeeper implementation, e.g. one running in TrustZone.
##
include $(CLEAR_VARS)
LOCAL_MODULE:= libgatekeeper
LOCAL_SRC_FILES := \
	gatekeeper_messages.cpp \
	gatekeeper.cpp
LOCAL_C_INCLUDES := \
	$(LOCAL_PATH)/include
LOCAL_CFLAGS = -Wall -Werror -g
LOCAL_MODULE_TAGS := optional
LOCAL_EXPORT_C_INCLUDE_DIRS := $(LOCAL_PATH)/include
LOCAL_ADDITIONAL_DEPENDENCIES := $(LOCAL_PATH)/Android.mk
# TODO(krasin): reenable coverage flags, when the new Clang toolchain is released.
# Currently, if enabled, these flags will cause an internal error in Clang.
# Bug: 25119481
LOCAL_CLANG_CFLAGS += -fno-sanitize-coverage=edge,indirect-calls,8bit-counters,trace-cmp

include $(BUILD_SHARED_LIBRARY)

###
# libgatekeeper_static is an empty static library that exports
# all of the files in gatekeeper as includes.
###
include $(CLEAR_VARS)
LOCAL_MODULE := libgatekeeper_static
LOCAL_EXPORT_C_INCLUDE_DIRS := \
        $(LOCAL_PATH) \
        $(LOCAL_PATH)/include
LOCAL_MODULE_TAGS := optional
LOCAL_ADDITIONAL_DEPENDENCIES := $(LOCAL_PATH)/Android.mk
include $(BUILD_STATIC_LIBRARY)

include $(call first-makefiles-under,$(LOCAL_PATH))
