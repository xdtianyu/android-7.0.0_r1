LOCAL_PATH:= $(call my-dir)

LibDriver_SRC_FILES := \
  LibDriver.cpp

LibDriver_TBLGEN_TABLES := \
  Options.inc


# For the host
# =====================================================
include $(CLEAR_VARS)
include $(CLEAR_TBLGEN_VARS)

LOCAL_SRC_FILES := $(LibDriver_SRC_FILES)
TBLGEN_TABLES := $(LibDriver_TBLGEN_TABLES)

LOCAL_MODULE:= libLLVMLibDriver
LOCAL_MODULE_HOST_OS := darwin linux windows

include $(LLVM_HOST_BUILD_MK)
include $(LLVM_TBLGEN_RULES_MK)
include $(BUILD_HOST_STATIC_LIBRARY)

# For the device
# =====================================================
include $(CLEAR_VARS)
include $(CLEAR_TBLGEN_VARS)

ifneq (true,$(DISABLE_LLVM_DEVICE_BUILDS))

LOCAL_SRC_FILES := $(LibDriver_SRC_FILES)
TBLGEN_TABLES := $(LibDriver_TBLGEN_TABLES)

LOCAL_MODULE:= libLLVMLibDriver

include $(LLVM_DEVICE_BUILD_MK)
include $(LLVM_TBLGEN_RULES_MK)
include $(BUILD_STATIC_LIBRARY)
endif
