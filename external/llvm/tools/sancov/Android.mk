LOCAL_PATH := $(call my-dir)

LLVM_ROOT_PATH := $(LOCAL_PATH)/../..


#===---------------------------------------------------------------===
# sancov command line tool
#===---------------------------------------------------------------===

llvm_sancov_SRC_FILES := \
  sancov.cc \

llvm_sancov_STATIC_LIBRARIES := \
  libLLVMIRReader \
  libLLVMARMCodeGen \
  libLLVMARMInfo \
  libLLVMARMDesc \
  libLLVMARMAsmPrinter \
  libLLVMARMDisassembler \
  libLLVMAArch64CodeGen \
  libLLVMAArch64Info \
  libLLVMAArch64Desc \
  libLLVMAArch64AsmPrinter \
  libLLVMAArch64Utils \
  libLLVMAArch64Disassembler \
  libLLVMMipsCodeGen \
  libLLVMMipsInfo \
  libLLVMMipsDesc \
  libLLVMMipsAsmPrinter \
  libLLVMMipsDisassembler \
  libLLVMX86CodeGen \
  libLLVMX86Info \
  libLLVMX86Desc \
  libLLVMX86AsmPrinter \
  libLLVMX86Utils \
  libLLVMX86Disassembler \
  libLLVMSymbolize \
  libLLVMDebugInfoDWARF \
  libLLVMDebugInfoPDB \
  libLLVMAsmPrinter \
  libLLVMSelectionDAG \
  libLLVMCodeGen \
  libLLVMTransformObjCARC \
  libLLVMVectorize \
  libLLVMScalarOpts \
  libLLVMPasses \
  libLLVMipo \
  libLLVMLinker \
  libLLVMInstCombine \
  libLLVMInstrumentation \
  libLLVMTransformUtils \
  libLLVMAnalysis \
  libLLVMTarget \
  libLLVMObject \
  libLLVMBitReader \
  libLLVMBitWriter \
  libLLVMMC \
  libLLVMMCParser \
  libLLVMProfileData \
  libLLVMCore \
  libLLVMAsmParser \
  libLLVMOption \
  libLLVMSupport \
  libLLVMMCDisassembler \

include $(CLEAR_VARS)

LOCAL_MODULE := sancov

LOCAL_CPP_EXTENSION := .cc
LOCAL_SRC_FILES := $(llvm_sancov_SRC_FILES)
LOCAL_STATIC_LIBRARIES := $(llvm_sancov_STATIC_LIBRARIES)

include $(LLVM_ROOT_PATH)/llvm.mk
include $(LLVM_HOST_BUILD_MK)
include $(BUILD_HOST_EXECUTABLE)
