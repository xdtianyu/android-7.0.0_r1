LOCAL_PATH:= $(call my-dir)

llvm_split_SRC_FILES := \
  llvm-split.cpp

include $(CLEAR_VARS)

LOCAL_MODULE := llvm-split
LOCAL_MODULE_CLASS := EXECUTABLES
LOCAL_MODULE_TAGS := optional
LOCAL_SRC_FILES := $(llvm_split_SRC_FILES)
LOCAL_LDLIBS += -lm
LOCAL_LDLIBS_windows := -limagehlp
LOCAL_LDLIBS_darwin := -lpthread -ldl
LOCAL_LDLIBS_linux := -lpthread -ldl

LOCAL_STATIC_LIBRARIES := \
  libLLVMIRReader \
  libLLVMBitReader \
  libLLVMBitWriter \
  libLLVMTransformUtils \
  libLLVMAnalysis \
  libLLVMCore \
  libLLVMAsmParser \
  libLLVMSupport

include $(LLVM_HOST_BUILD_MK)
include $(LLVM_GEN_ATTRIBUTES_MK)
include $(BUILD_HOST_EXECUTABLE)
