#
# Copyright (C) 2011-2012 The Android Open Source Project
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

local_cflags_for_libbcinfo := -Wall -Wno-unused-parameter -Werror
ifneq ($(TARGET_BUILD_VARIANT),eng)
local_cflags_for_libbcinfo += -D__DISABLE_ASSERTS
endif

LOCAL_PATH := $(call my-dir)

include frameworks/compile/slang/rs_version.mk
local_cflags_for_libbcinfo += $(RS_VERSION_DEFINE)

libbcinfo_SRC_FILES := \
  BitcodeTranslator.cpp \
  BitcodeWrapper.cpp \
  MetadataExtractor.cpp

libbcinfo_C_INCLUDES := \
  $(LOCAL_PATH)/../include \
  $(RS_ROOT_PATH) \
  $(LOCAL_PATH)/../../slang

libbcinfo_STATIC_LIBRARIES := \
  libLLVMWrap \
  libLLVMBitReader_2_7 \
  libLLVMBitReader_3_0 \
  libLLVMBitWriter_3_2

LLVM_ROOT_PATH := external/llvm

ifneq (true,$(DISABLE_LLVM_DEVICE_BUILDS))
include $(CLEAR_VARS)

LOCAL_MODULE := libbcinfo
LOCAL_MODULE_CLASS := SHARED_LIBRARIES
LOCAL_MODULE_TAGS := optional

LOCAL_SRC_FILES := $(libbcinfo_SRC_FILES)

LOCAL_CFLAGS += $(local_cflags_for_libbcinfo)

LOCAL_C_INCLUDES := $(libbcinfo_C_INCLUDES)

LOCAL_STATIC_LIBRARIES := $(libbcinfo_STATIC_LIBRARIES)
LOCAL_SHARED_LIBRARIES := libLLVM libcutils liblog

include $(LLVM_ROOT_PATH)/llvm-device-build.mk
include $(LLVM_GEN_ATTRIBUTES_MK)
include $(BUILD_SHARED_LIBRARY)
endif

# Don't build for unbundled branches
ifeq (,$(TARGET_BUILD_APPS))

include $(CLEAR_VARS)

LOCAL_MODULE := libbcinfo
LOCAL_MODULE_CLASS := SHARED_LIBRARIES
LOCAL_MODULE_HOST_OS := darwin linux windows
LOCAL_IS_HOST_MODULE := true

LOCAL_SRC_FILES := $(libbcinfo_SRC_FILES)

LOCAL_CFLAGS += $(local_cflags_for_libbcinfo)

LOCAL_C_INCLUDES := $(libbcinfo_C_INCLUDES)

LOCAL_STATIC_LIBRARIES += $(libbcinfo_STATIC_LIBRARIES)
LOCAL_STATIC_LIBRARIES += libcutils liblog

LOCAL_LDLIBS_darwin := -ldl -lpthread
LOCAL_LDLIBS_linux := -ldl -lpthread

include $(LOCAL_PATH)/../llvm-loadable-libbcc.mk

ifneq ($(CAN_BUILD_HOST_LLVM_LOADABLE_MODULE),true)
LOCAL_SHARED_LIBRARIES_linux += libLLVM
endif
LOCAL_SHARED_LIBRARIES_darwin += libLLVM
LOCAL_SHARED_LIBRARIES_windows += libLLVM

include $(LLVM_ROOT_PATH)/llvm-host-build.mk
include $(LLVM_GEN_ATTRIBUTES_MK)
include $(BUILD_HOST_SHARED_LIBRARY)

endif # don't build for unbundled branches

#=====================================================================
# Include Subdirectories
#=====================================================================
include $(call all-makefiles-under,$(LOCAL_PATH))
