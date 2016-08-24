LOCAL_PATH := $(call my-dir)

LLVM_ROOT_PATH := $(LOCAL_PATH)/../..
include $(LLVM_ROOT_PATH)/llvm.mk

llvm_pdbdump_SRC_FILES := \
  llvm-pdbdump.cpp \
  BuiltinDumper.cpp \
  ClassDefinitionDumper.cpp \
  CompilandDumper.cpp \
  EnumDumper.cpp \
  ExternalSymbolDumper.cpp \
  FunctionDumper.cpp \
  LinePrinter.cpp \
  TypedefDumper.cpp \
  TypeDumper.cpp \
  VariableDumper.cpp

llvm_pdbdump_STATIC_LIBRARIES := \
  libLLVMDebugInfoPDB \
  libLLVMSupport

include $(CLEAR_VARS)

LOCAL_MODULE := llvm-pdbdump
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := EXECUTABLES
LOCAL_IS_HOST_MODULE := true

LOCAL_SRC_FILES := $(llvm_pdbdump_SRC_FILES)

LOCAL_STATIC_LIBRARIES := $(llvm_pdbdump_STATIC_LIBRARIES)

LOCAL_LDLIBS += -lpthread -lm -ldl

include $(LLVM_HOST_BUILD_MK)
include $(LLVM_GEN_INTRINSICS_MK)
include $(BUILD_HOST_EXECUTABLE)
