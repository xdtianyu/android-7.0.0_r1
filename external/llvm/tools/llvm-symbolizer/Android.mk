LOCAL_PATH := $(call my-dir)

LLVM_ROOT_PATH := $(LOCAL_PATH)/../..
include $(LLVM_ROOT_PATH)/llvm.mk

llvm_symbolizer_SRC_FILES := \
  llvm-symbolizer.cpp

include $(CLEAR_VARS)

LOCAL_MODULE := llvm-symbolizer
LOCAL_MODULE_CLASS := EXECUTABLES
LOCAL_MODULE_TAGS := optional
LOCAL_SRC_FILES := $(llvm_symbolizer_SRC_FILES)
LOCAL_LDLIBS += -lpthread -lm -ldl

LOCAL_STATIC_LIBRARIES := \
  libLLVMSymbolize \
  libLLVMDebugInfoDWARF \
  libLLVMDebugInfoPDB \
  libLLVMObject \
  libLLVMBitReader \
  libLLVMMC \
  libLLVMMCParser \
  libLLVMCore \
  libLLVMSupport

include $(LLVM_HOST_BUILD_MK)
include $(LLVM_GEN_INTRINSICS_MK)
include $(BUILD_HOST_EXECUTABLE)

include $(CLEAR_VARS)

ifneq (true,$(DISABLE_LLVM_DEVICE_BUILDS))
LOCAL_MODULE := llvm-symbolizer
LOCAL_MODULE_CLASS := EXECUTABLES
LOCAL_SRC_FILES := $(llvm_symbolizer_SRC_FILES)
LOCAL_SHARED_LIBRARIES := libLLVM

include $(LLVM_DEVICE_BUILD_MK)
include $(BUILD_EXECUTABLE)
endif
