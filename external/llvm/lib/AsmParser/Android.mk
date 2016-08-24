LOCAL_PATH:= $(call my-dir)

asm_parser_SRC_FILES := \
  LLLexer.cpp \
  LLParser.cpp \
  Parser.cpp

# For the host
# =====================================================
include $(CLEAR_VARS)

LOCAL_SRC_FILES := $(asm_parser_SRC_FILES)

LOCAL_MODULE:= libLLVMAsmParser

LOCAL_MODULE_HOST_OS := darwin linux windows

include $(LOCAL_PATH)/../../llvm-host-build.mk
include $(LLVM_GEN_ATTRIBUTES_MK)
include $(LLVM_GEN_INTRINSICS_MK)
include $(BUILD_HOST_STATIC_LIBRARY)

# For the device
# =====================================================
ifneq (true,$(DISABLE_LLVM_DEVICE_BUILDS))
include $(CLEAR_VARS)

LOCAL_SRC_FILES := $(asm_parser_SRC_FILES)

LOCAL_MODULE:= libLLVMAsmParser

include $(LOCAL_PATH)/../../llvm-device-build.mk
include $(LLVM_GEN_ATTRIBUTES_MK)
include $(LLVM_GEN_INTRINSICS_MK)
include $(BUILD_STATIC_LIBRARY)
endif
