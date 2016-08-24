LOCAL_PATH:= $(call my-dir)

include $(CLEAR_TBLGEN_VARS)

TBLGEN_TABLES := \
  AttrVisitor.inc \
  DiagnosticCommonKinds.inc \
  DiagnosticDriverKinds.inc \
  DiagnosticSemaKinds.inc \
  Options.inc \
  CC1Options.inc

clang_driver_SRC_FILES := \
  Action.cpp \
  Compilation.cpp \
  CrossWindowsToolChain.cpp \
  Driver.cpp \
  DriverOptions.cpp \
  Job.cpp \
  MinGWToolChain.cpp \
  MSVCToolChain.cpp \
  Multilib.cpp \
  Phases.cpp \
  SanitizerArgs.cpp \
  ToolChain.cpp \
  ToolChains.cpp \
  Tool.cpp \
  Tools.cpp \
  Types.cpp

# For the host
# =====================================================
include $(CLEAR_VARS)

LOCAL_SRC_FILES := $(clang_driver_SRC_FILES)
LOCAL_MODULE := libclangDriver
LOCAL_MODULE_TAGS := optional

include $(CLANG_HOST_BUILD_MK)
include $(CLANG_TBLGEN_RULES_MK)
include $(CLANG_VERSION_INC_MK)
include $(BUILD_HOST_STATIC_LIBRARY)

# For the target
# =====================================================
include $(CLEAR_VARS)

LOCAL_SRC_FILES := $(clang_driver_SRC_FILES)
LOCAL_MODULE := libclangDriver
LOCAL_MODULE_TAGS := optional

include $(CLANG_DEVICE_BUILD_MK)
include $(CLANG_TBLGEN_RULES_MK)
include $(CLANG_VERSION_INC_MK)
include $(BUILD_STATIC_LIBRARY)
