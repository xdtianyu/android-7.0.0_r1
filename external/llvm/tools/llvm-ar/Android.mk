LOCAL_PATH := $(call my-dir)

LLVM_ROOT_PATH := $(LOCAL_PATH)/../..


#===---------------------------------------------------------------===
# llvm-ar command line tool
#===---------------------------------------------------------------===

llvm_ar_SRC_FILES := \
  llvm-ar.cpp

include $(CLEAR_VARS)

LOCAL_MODULE := llvm-ar
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := EXECUTABLES
LOCAL_IS_HOST_MODULE := true

LOCAL_SRC_FILES := $(llvm_ar_SRC_FILES)

LOCAL_SHARED_LIBRARIES := libLLVM

LOCAL_LDLIBS += -lpthread -lm -ldl

# Create symlink llvm-lib and llvm-ranlib pointing to llvm-ar.
# Use "=" (instead of ":=") to defer the evaluation.
LOCAL_POST_INSTALL_CMD = $(hide) ln -sf llvm-ar $(dir $(LOCAL_INSTALLED_MODULE))llvm-lib \
						 && ln -sf llvm-ar $(dir $(LOCAL_INSTALLED_MODULE))llvm-ranlib

include $(LLVM_ROOT_PATH)/llvm.mk
include $(LLVM_HOST_BUILD_MK)
include $(LLVM_GEN_ATTRIBUTES_MK)
include $(BUILD_HOST_EXECUTABLE)
