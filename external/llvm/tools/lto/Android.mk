#
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

LOCAL_PATH:= $(call my-dir)

LLVM_ROOT_PATH := $(LOCAL_PATH)/../../
include $(LLVM_ROOT_PATH)/llvm.mk

# For the host only
# =====================================================
include $(CLEAR_VARS)
include $(CLEAR_TBLGEN_VARS)

LOCAL_MODULE := libLTO

LOCAL_MODULE_CLASS := SHARED_LIBRARIES

llvm_lto_SRC_FILES := \
  lto.cpp \
  LTODisassembler.cpp

LOCAL_SRC_FILES := $(llvm_lto_SRC_FILES)

llvm_lto_STATIC_LIBRARIES := \
  libLLVMLinker \
  libLLVMipo \
  libLLVMDebugInfoDWARF \
  libLLVMDebugInfoPDB \
  libLLVMIRReader \
  libLLVMBitWriter \
  libLLVMBitReader \
  libLLVMARMCodeGen \
  libLLVMARMAsmParser \
  libLLVMARMAsmPrinter \
  libLLVMARMInfo \
  libLLVMARMDesc \
  libLLVMARMDisassembler \
  libLLVMMipsCodeGen \
  libLLVMMipsInfo \
  libLLVMMipsDesc \
  libLLVMMipsAsmParser \
  libLLVMMipsAsmPrinter \
  libLLVMMipsDisassembler \
  libLLVMX86CodeGen \
  libLLVMX86Info \
  libLLVMX86Desc \
  libLLVMX86AsmParser \
  libLLVMX86AsmPrinter \
  libLLVMX86Utils \
  libLLVMX86Disassembler \
  libLLVMAArch64CodeGen \
  libLLVMAArch64Info \
  libLLVMAArch64Desc \
  libLLVMAArch64AsmParser \
  libLLVMAArch64AsmPrinter \
  libLLVMAArch64Utils \
  libLLVMAArch64Disassembler \
  libLLVMExecutionEngine \
  libLLVMRuntimeDyld \
  libLLVMMCJIT \
  libLLVMOrcJIT \
  libLLVMAsmPrinter \
  libLLVMSelectionDAG \
  libLLVMCodeGen \
  libLLVMObject \
  libLLVMScalarOpts \
  libLLVMInstCombine \
  libLLVMInstrumentation \
  libLLVMTransformObjCARC \
  libLLVMTransformUtils \
  libLLVMVectorize \
  libLLVMAnalysis \
  libLLVMTarget \
  libLLVMMCDisassembler \
  libLLVMMC \
  libLLVMMCParser \
  libLLVMCore \
  libLLVMAsmParser \
  libLLVMOption \
  libLLVMLTO \
  libLLVMSupport \
  libLLVMProfileData

LOCAL_LDLIBS_darwin := -lpthread -ldl
LOCAL_LDLIBS_linux := -lpthread -ldl

LOCAL_STATIC_LIBRARIES := $(llvm_lto_STATIC_LIBRARIES) $(llvm_lto_STATIC_LIBRARIES)

include $(LLVM_HOST_BUILD_MK)
include $(LLVM_GEN_ATTRIBUTES_MK)
include $(LLVM_GEN_INTRINSICS_MK)
include $(BUILD_HOST_SHARED_LIBRARY)
