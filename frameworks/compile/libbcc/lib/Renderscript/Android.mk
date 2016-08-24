#
# Copyright (C) 2012 The Android Open Source Project
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

LOCAL_PATH := $(call my-dir)

#=====================================================================
# Common: libbccRenderscript
#=====================================================================

libbcc_renderscript_SRC_FILES := \
  RSAddDebugInfoPass.cpp \
  RSCompilerDriver.cpp \
  RSEmbedInfo.cpp \
  RSKernelExpand.cpp \
  RSGlobalInfoPass.cpp \
  RSInvariant.cpp \
  RSScript.cpp \
  RSInvokeHelperPass.cpp \
  RSIsThreadablePass.cpp \
  RSScreenFunctionsPass.cpp \
  RSStubsWhiteList.cpp \
  RSScriptGroupFusion.cpp \
  RSX86CallConvPass.cpp \
  RSX86TranslateGEPPass.cpp

#=====================================================================
# Device Static Library: libbccRenderscript
#=====================================================================
ifneq (true,$(DISABLE_LLVM_DEVICE_BUILDS))
include $(CLEAR_VARS)

LOCAL_MODULE := libbccRenderscript
LOCAL_MODULE_CLASS := STATIC_LIBRARIES

LOCAL_SRC_FILES := $(libbcc_renderscript_SRC_FILES)

include $(LIBBCC_DEVICE_BUILD_MK)
include $(LLVM_DEVICE_BUILD_MK)
include $(LLVM_GEN_ATTRIBUTES_MK)
include $(LLVM_GEN_INTRINSICS_MK)
include $(BUILD_STATIC_LIBRARY)
endif

#=====================================================================
# Host Static Library: libbccRenderscript
#=====================================================================

include $(CLEAR_VARS)

LOCAL_MODULE := libbccRenderscript
LOCAL_MODULE_HOST_OS := darwin linux windows
LOCAL_MODULE_CLASS := STATIC_LIBRARIES
LOCAL_IS_HOST_MODULE := true

LOCAL_SRC_FILES := $(libbcc_renderscript_SRC_FILES)

include $(LIBBCC_HOST_BUILD_MK)
include $(LLVM_HOST_BUILD_MK)
include $(LLVM_GEN_ATTRIBUTES_MK)
include $(LLVM_GEN_INTRINSICS_MK)
include $(BUILD_HOST_STATIC_LIBRARY)
