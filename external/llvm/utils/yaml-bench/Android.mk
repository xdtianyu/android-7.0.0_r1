LOCAL_PATH := $(call my-dir)

LLVM_ROOT_PATH := $(LOCAL_PATH)/../..


yaml_bench_SRC_FILES := \
  YAMLBench.cpp

yaml_bench_STATIC_LIBRARIES := \
  libLLVMSupport \

# yaml-bench executable for the host
# =====================================================

include $(CLEAR_VARS)

LOCAL_MODULE := yaml-bench
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := EXECUTABLES
LOCAL_IS_HOST_MODULE := true

LOCAL_SRC_FILES := $(yaml_bench_SRC_FILES)

LOCAL_STATIC_LIBRARIES := $(yaml_bench_STATIC_LIBRARIES)

include $(LLVM_ROOT_PATH)/llvm.mk
include $(LLVM_HOST_BUILD_MK)
include $(BUILD_HOST_EXECUTABLE)
