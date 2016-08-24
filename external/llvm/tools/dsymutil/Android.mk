LOCAL_PATH := $(call my-dir)

LLVM_ROOT_PATH := $(LOCAL_PATH)/../..
include $(LLVM_ROOT_PATH)/llvm.mk

llvm_dsymutil_SRC_FILES := \
  BinaryHolder.cpp \
  DebugMap.cpp \
  dsymutil.cpp \
  DwarfLinker.cpp \
  MachODebugMapParser.cpp \
  MachOUtils.cpp \

llvm_dsymutil_STATIC_LIBRARIES := \
  libLLVMARMCodeGen \
  libLLVMARMAsmParser \
  libLLVMARMInfo \
  libLLVMARMDesc \
  libLLVMARMAsmPrinter \
  libLLVMARMDisassembler \
  libLLVMAArch64CodeGen \
  libLLVMAArch64Info \
  libLLVMAArch64AsmParser \
  libLLVMAArch64Desc \
  libLLVMAArch64AsmPrinter \
  libLLVMAArch64Utils \
  libLLVMAArch64Disassembler \
  libLLVMMipsCodeGen \
  libLLVMMipsInfo \
  libLLVMMipsAsmParser \
  libLLVMMipsDesc \
  libLLVMMipsAsmPrinter \
  libLLVMMipsDisassembler \
  libLLVMX86CodeGen \
  libLLVMX86Info \
  libLLVMX86Desc \
  libLLVMX86AsmParser \
  libLLVMX86AsmPrinter \
  libLLVMX86Utils \
  libLLVMX86Disassembler \
  libLLVMX86CodeGen \
  libLLVMAsmPrinter \
  libLLVMSelectionDAG \
  libLLVMCodeGen \
  libLLVMDebugInfoDWARF \
  libLLVMInstrumentation \
  libLLVMMCParser \
  libLLVMMCDisassembler \
  libLLVMObject \
  libLLVMBitReader \
  libLLVMScalarOpts \
  libLLVMTransformUtils \
  libLLVMAnalysis \
  libLLVMTarget \
  libLLVMCore \
  libLLVMMC \
  libLLVMSupport \

include $(CLEAR_VARS)

LOCAL_MODULE := llvm-dsymutil
LOCAL_MODULE_CLASS := EXECUTABLES
LOCAL_MODULE_TAGS := optional
LOCAL_SRC_FILES := $(llvm_dsymutil_SRC_FILES)
LOCAL_LDLIBS += -lpthread -lm -ldl

LOCAL_STATIC_LIBRARIES := $(llvm_dsymutil_STATIC_LIBRARIES)

include $(LLVM_HOST_BUILD_MK)
include $(LLVM_GEN_INTRINSICS_MK)
include $(BUILD_HOST_EXECUTABLE)
