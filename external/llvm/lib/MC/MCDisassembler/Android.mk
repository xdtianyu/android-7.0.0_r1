LOCAL_PATH:= $(call my-dir)

mc_disassembler_SRC_FILES := \
  Disassembler.cpp \
  MCDisassembler.cpp \
  MCExternalSymbolizer.cpp \
  MCRelocationInfo.cpp


# For the host
# =====================================================
include $(CLEAR_VARS)

LOCAL_SRC_FILES := $(mc_disassembler_SRC_FILES)

LOCAL_MODULE:= libLLVMMCDisassembler

LOCAL_MODULE_HOST_OS := darwin linux windows


include $(LLVM_HOST_BUILD_MK)
include $(BUILD_HOST_STATIC_LIBRARY)

# For the device
# =====================================================
include $(CLEAR_VARS)
ifneq (true,$(DISABLE_LLVM_DEVICE_BUILDS))

LOCAL_SRC_FILES := $(mc_disassembler_SRC_FILES)

LOCAL_MODULE:= libLLVMMCDisassembler

LOCAL_MODULE_HOST_OS := darwin linux windows

include $(LLVM_DEVICE_BUILD_MK)
include $(BUILD_STATIC_LIBRARY)
endif
