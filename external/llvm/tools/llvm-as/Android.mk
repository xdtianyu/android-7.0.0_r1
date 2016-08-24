LOCAL_PATH:= $(call my-dir)

llvm_as_SRC_FILES := \
  llvm-as.cpp

include $(CLEAR_VARS)

LOCAL_MODULE := llvm-as
LOCAL_MODULE_CLASS := EXECUTABLES
LOCAL_MODULE_TAGS := optional
LOCAL_SRC_FILES := $(llvm_as_SRC_FILES)
LOCAL_LDLIBS += -lm
LOCAL_LDLIBS_windows := -limagehlp
LOCAL_LDLIBS_darwin := -lpthread -ldl
LOCAL_LDLIBS_linux := -lpthread -ldl

LOCAL_STATIC_LIBRARIES := \
  libLLVMAsmParser \
  libLLVMBitWriter \
  libLLVMCore \
  libLLVMSupport

include $(LLVM_HOST_BUILD_MK)
include $(LLVM_GEN_ATTRIBUTES_MK)
include $(BUILD_HOST_EXECUTABLE)
