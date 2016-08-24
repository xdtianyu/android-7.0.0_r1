LOCAL_PATH:= $(call my-dir)

# For the host
# =====================================================
include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
  ExecutionUtils.cpp \
  IndirectionUtils.cpp \
  OrcMCJITReplacement.cpp \
  OrcTargetSupport.cpp

LOCAL_MODULE:= libLLVMOrcJIT

LOCAL_MODULE_HOST_OS := darwin linux windows

include $(LLVM_HOST_BUILD_MK)
include $(LLVM_GEN_ATTRIBUTES_MK)
include $(LLVM_GEN_INTRINSICS_MK)
include $(BUILD_HOST_STATIC_LIBRARY)
