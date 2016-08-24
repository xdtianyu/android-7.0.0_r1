LOCAL_PATH:= $(call my-dir)

mcld_aarch64_target_SRC_FILES := \
  AArch64CA53Erratum835769Stub.cpp \
  AArch64CA53Erratum843419Stub2.cpp \
  AArch64CA53Erratum843419Stub.cpp \
  AArch64CA53ErratumStub.cpp \
  AArch64Diagnostic.cpp \
  AArch64ELFDynamic.cpp \
  AArch64Emulation.cpp \
  AArch64GOT.cpp \
  AArch64LDBackend.cpp \
  AArch64LongBranchStub.cpp \
  AArch64PLT.cpp \
  AArch64Relocator.cpp

# For the host
# =====================================================
include $(CLEAR_VARS)

LOCAL_SRC_FILES := $(mcld_aarch64_target_SRC_FILES)
LOCAL_MODULE:= libmcldAArch64Target

LOCAL_MODULE_TAGS := optional

include $(MCLD_HOST_BUILD_MK)
include $(BUILD_HOST_STATIC_LIBRARY)

# For the device
# =====================================================
include $(CLEAR_VARS)

LOCAL_SRC_FILES := $(mcld_aarch64_target_SRC_FILES)
LOCAL_MODULE:= libmcldAArch64Target

LOCAL_MODULE_TAGS := optional

include $(MCLD_DEVICE_BUILD_MK)
include $(BUILD_STATIC_LIBRARY)

