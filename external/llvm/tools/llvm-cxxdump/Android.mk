LOCAL_PATH := $(call my-dir)

LLVM_ROOT_PATH := $(LOCAL_PATH)/../..
include $(LLVM_ROOT_PATH)/llvm.mk

llvm_cxxdump_SRC_FILES := \
  Error.cpp \
  llvm-cxxdump.cpp

llvm_cxxdump_STATIC_LIBRARIES := \
  libLLVMARMAsmParser \
  libLLVMARMInfo \
  libLLVMARMDesc \
  libLLVMARMAsmPrinter \
  libLLVMARMDisassembler \
  libLLVMAArch64Info \
  libLLVMAArch64AsmParser \
  libLLVMAArch64Desc \
  libLLVMAArch64AsmPrinter \
  libLLVMAArch64Utils \
  libLLVMAArch64Disassembler \
  libLLVMMipsInfo \
  libLLVMMipsAsmParser \
  libLLVMMipsDesc \
  libLLVMMipsAsmPrinter \
  libLLVMMipsDisassembler \
  libLLVMX86Info \
  libLLVMX86Desc \
  libLLVMX86AsmParser \
  libLLVMX86CodeGen \
  libLLVMX86AsmPrinter \
  libLLVMX86Utils \
  libLLVMX86Disassembler \
  libLLVMAsmPrinter \
  libLLVMCodeGen \
  libLLVMAnalysis \
  libLLVMTarget \
  libLLVMObject \
  libLLVMMCParser \
  libLLVMMC \
  libLLVMMCDisassembler \
  libLLVMBitReader \
  libLLVMCore \
  libLLVMAsmParser \
  libLLVMSupport \


include $(CLEAR_VARS)

LOCAL_MODULE := llvm-cxxdump
LOCAL_MODULE_CLASS := EXECUTABLES
LOCAL_MODULE_TAGS := optional
LOCAL_SRC_FILES := $(llvm_cxxdump_SRC_FILES)
LOCAL_LDLIBS += -lpthread -lm -ldl

LOCAL_STATIC_LIBRARIES := $(llvm_cxxdump_STATIC_LIBRARIES)

include $(LLVM_HOST_BUILD_MK)
include $(LLVM_GEN_INTRINSICS_MK)
include $(BUILD_HOST_EXECUTABLE)
