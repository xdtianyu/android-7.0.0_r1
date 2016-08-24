#
# Copyright (C) 2013 The Android Open Source Project
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

LOCAL_PATH := $(call my-dir)

# Executable for host
# ========================================================
include $(CLEAR_VARS)

LOCAL_MODULE := bcc_strip_attr
LOCAL_MODULE_CLASS := EXECUTABLES

LOCAL_SHARED_LIBRARIES := libLLVM

LOCAL_C_INCLUDES := \
  $(LOCAL_PATH)/../../include

LOCAL_LDLIBS += -lm
LOCAL_LDLIBS_darwin += -lpthread -ldl
LOCAL_LDLIBS_linux += -lpthread -ldl
LOCAL_SRC_FILES := bcc_strip_attr.cpp

include $(LIBBCC_HOST_BUILD_MK)
include $(LLVM_HOST_BUILD_MK)
include $(LLVM_GEN_ATTRIBUTES_MK)
include $(BUILD_HOST_EXECUTABLE)
