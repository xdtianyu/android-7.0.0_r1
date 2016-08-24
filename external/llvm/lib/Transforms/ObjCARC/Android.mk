LOCAL_PATH:= $(call my-dir)

transforms_objcarc_SRC_FILES := \
  DependencyAnalysis.cpp \
  ObjCARCAPElim.cpp \
  ObjCARCContract.cpp \
  ObjCARC.cpp \
  ObjCARCExpand.cpp \
  ObjCARCOpts.cpp \
  PtrState.cpp \
  ProvenanceAnalysis.cpp \
  ProvenanceAnalysisEvaluator.cpp

# For the host
# =====================================================
include $(CLEAR_VARS)

LOCAL_SRC_FILES := $(transforms_objcarc_SRC_FILES)
LOCAL_MODULE:= libLLVMTransformObjCARC

LOCAL_MODULE_HOST_OS := darwin linux windows

include $(LLVM_HOST_BUILD_MK)
include $(LLVM_GEN_ATTRIBUTES_MK)
include $(LLVM_GEN_INTRINSICS_MK)
include $(BUILD_HOST_STATIC_LIBRARY)

# For the device
# =====================================================
ifneq (true,$(DISABLE_LLVM_DEVICE_BUILDS))
include $(CLEAR_VARS)

LOCAL_SRC_FILES := $(transforms_objcarc_SRC_FILES)
LOCAL_MODULE:= libLLVMTransformObjCARC

include $(LLVM_DEVICE_BUILD_MK)
include $(LLVM_GEN_ATTRIBUTES_MK)
include $(LLVM_GEN_INTRINSICS_MK)
include $(BUILD_STATIC_LIBRARY)
endif
