LOCAL_PATH := $(call my-dir)

LLVM_ROOT_PATH := $(LOCAL_PATH)/../..


#===---------------------------------------------------------------===
# llvm-dwp command line tool
#===---------------------------------------------------------------===

llvm_dwp_SRC_FILES := \
  llvm-dwp.cpp

llvm_dwp_STATIC_LIBRARIES := \
  libLLVMDebugInfoDWARF \
  libLLVMObject \
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

include $(CLEAR_VARS)

LOCAL_MODULE := llvm-dwp
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := EXECUTABLES
LOCAL_IS_HOST_MODULE := true

LOCAL_SRC_FILES := $(llvm_dwp_SRC_FILES)

LOCAL_STATIC_LIBRARIES := $(llvm_dwp_STATIC_LIBRARIES)

LOCAL_LDLIBS += -lpthread -lm -ldl

include $(LLVM_ROOT_PATH)/llvm.mk
include $(LLVM_HOST_BUILD_MK)
include $(LLVM_GEN_INTRINSICS_MK)
include $(BUILD_HOST_EXECUTABLE)
