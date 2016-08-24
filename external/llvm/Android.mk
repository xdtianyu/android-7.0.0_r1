LOCAL_PATH := $(call my-dir)
LLVM_ROOT_PATH := $(LOCAL_PATH)

FORCE_BUILD_LLVM_DISABLE_NDEBUG ?= false
# Legality check: FORCE_BUILD_LLVM_DISABLE_NDEBUG should consist of one word -- either "true" or "false".
ifneq "$(words $(FORCE_BUILD_LLVM_DISABLE_NDEBUG))$(words $(filter-out true false,$(FORCE_BUILD_LLVM_DISABLE_NDEBUG)))" "10"
  $(error FORCE_BUILD_LLVM_DISABLE_NDEBUG may only be true, false, or unset)
endif

FORCE_BUILD_LLVM_DEBUG ?= false
# Legality check: FORCE_BUILD_LLVM_DEBUG should consist of one word -- either "true" or "false".
ifneq "$(words $(FORCE_BUILD_LLVM_DEBUG))$(words $(filter-out true false,$(FORCE_BUILD_LLVM_DEBUG)))" "10"
  $(error FORCE_BUILD_LLVM_DEBUG may only be true, false, or unset)
endif

include $(CLEAR_VARS)

# LLVM Libraries
subdirs := \
  lib/Analysis \
  lib/AsmParser \
  lib/Bitcode/Reader \
  lib/Bitcode/Writer \
  lib/ExecutionEngine \
  lib/ExecutionEngine/RuntimeDyld \
  lib/ExecutionEngine/MCJIT \
  lib/ExecutionEngine/Orc \
  lib/ExecutionEngine/Interpreter \
  lib/Fuzzer \
  lib/CodeGen \
  lib/CodeGen/AsmPrinter \
  lib/CodeGen/MIRParser \
  lib/CodeGen/SelectionDAG \
  lib/DebugInfo/DWARF \
  lib/DebugInfo/PDB \
  lib/DebugInfo/Symbolize \
  lib/IR \
  lib/IRReader \
  lib/LibDriver \
  lib/Linker \
  lib/LTO \
  lib/MC \
  lib/MC/MCDisassembler \
  lib/MC/MCParser \
  lib/Object \
  lib/Option \
  lib/Passes \
  lib/ProfileData \
  lib/Support \
  lib/TableGen \
  lib/Target \
  lib/Transforms/Hello \
  lib/Transforms/IPO \
  lib/Transforms/InstCombine \
  lib/Transforms/Instrumentation \
  lib/Transforms/ObjCARC \
  lib/Transforms/Scalar \
  lib/Transforms/Utils \
  lib/Transforms/Vectorize \

# ARM Code Generation Libraries
subdirs += \
  lib/Target/ARM \
  lib/Target/ARM/AsmParser \
  lib/Target/ARM/InstPrinter \
  lib/Target/ARM/Disassembler \
  lib/Target/ARM/MCTargetDesc \
  lib/Target/ARM/TargetInfo

# AArch64 Code Generation Libraries
subdirs += \
  lib/Target/AArch64  \
  lib/Target/AArch64/AsmParser \
  lib/Target/AArch64/InstPrinter \
  lib/Target/AArch64/Disassembler \
  lib/Target/AArch64/MCTargetDesc \
  lib/Target/AArch64/TargetInfo \
  lib/Target/AArch64/Utils

# MIPS Code Generation Libraries
subdirs += \
  lib/Target/Mips \
  lib/Target/Mips/AsmParser \
  lib/Target/Mips/InstPrinter \
  lib/Target/Mips/Disassembler \
  lib/Target/Mips/MCTargetDesc \
  lib/Target/Mips/TargetInfo

# X86 Code Generation Libraries
subdirs += \
  lib/Target/X86 \
  lib/Target/X86/AsmParser \
  lib/Target/X86/InstPrinter \
  lib/Target/X86/Disassembler \
  lib/Target/X86/MCTargetDesc \
  lib/Target/X86/TargetInfo \
  lib/Target/X86/Utils

# LLVM Command Line Tools
subdirs += \
  tools/bugpoint \
  tools/bugpoint-passes \
  tools/dsymutil \
  tools/llc \
  tools/lli \
  tools/lli/ChildTarget \
  tools/llvm-ar \
  tools/llvm-as \
  tools/llvm-bcanalyzer \
  tools/llvm-c-test \
  tools/llvm-config \
  tools/llvm-cov \
  tools/llvm-cxxdump \
  tools/llvm-dis \
  tools/llvm-diff \
  tools/llvm-dwarfdump \
  tools/llvm-dwp \
  tools/llvm-extract \
  tools/llvm-link \
  tools/llvm-lto \
  tools/llvm-mc \
  tools/llvm-mcmarkup \
  tools/llvm-nm \
  tools/llvm-objdump \
  tools/llvm-pdbdump \
  tools/llvm-profdata \
  tools/llvm-readobj \
  tools/llvm-rtdyld \
  tools/llvm-size \
  tools/llvm-split \
  tools/llvm-symbolizer \
  tools/lto \
  tools/gold \
  tools/obj2yaml \
  tools/opt \
  tools/sancov \
  tools/verify-uselistorder \
  tools/yaml2obj \

# LLVM Command Line Utilities
subdirs += \
  utils/count \
  utils/FileCheck \
  utils/not \
  utils/TableGen \
  utils/yaml-bench \

include $(LOCAL_PATH)/llvm.mk
include $(LOCAL_PATH)/shared_llvm.mk

include $(addprefix $(LOCAL_PATH)/,$(addsuffix /Android.mk, $(subdirs)))
