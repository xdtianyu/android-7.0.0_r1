LOCAL_PATH:= $(call my-dir)

# For the host
# =====================================================
include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
  RTDyldMemoryManager.cpp \
  RuntimeDyldChecker.cpp \
  RuntimeDyld.cpp \
  RuntimeDyldCOFF.cpp \
  RuntimeDyldELF.cpp \
  RuntimeDyldMachO.cpp

LOCAL_MODULE:= libLLVMRuntimeDyld

LOCAL_MODULE_HOST_OS := darwin linux windows

include $(LLVM_HOST_BUILD_MK)
include $(BUILD_HOST_STATIC_LIBRARY)
