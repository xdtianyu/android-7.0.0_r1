LOCAL_PATH := $(call my-dir)

LLVM_ROOT_PATH := $(LOCAL_PATH)/../..
include $(LLVM_ROOT_PATH)/llvm.mk

verify_uselistorder_SRC_FILES := \
  verify-uselistorder.cpp

include $(CLEAR_VARS)

LOCAL_MODULE := verify-uselistorder
LOCAL_MODULE_CLASS := EXECUTABLES
LOCAL_MODULE_TAGS := optional
LOCAL_SRC_FILES := $(verify_uselistorder_SRC_FILES)
LOCAL_LDLIBS += -lpthread -lm -ldl

LOCAL_STATIC_LIBRARIES := \
  libLLVMAsmParser \
  libLLVMBitReader \
  libLLVMBitWriter \
  libLLVMCore \
  libLLVMIRReader \
  libLLVMSupport

include $(LLVM_HOST_BUILD_MK)
include $(LLVM_GEN_ATTRIBUTES_MK)
include $(LLVM_GEN_INTRINSICS_MK)
include $(BUILD_HOST_EXECUTABLE)
