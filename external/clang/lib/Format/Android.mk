LOCAL_PATH:= $(call my-dir)

include $(CLEAR_TBLGEN_VARS)

TBLGEN_TABLES := \
  AttrHasAttributeImpl.inc \
  DiagnosticASTKinds.inc \
  DiagnosticAnalysisKinds.inc \
  DiagnosticCommentKinds.inc \
  DiagnosticCommonKinds.inc \
  DiagnosticDriverKinds.inc \
  DiagnosticFrontendKinds.inc \
  DiagnosticGroups.inc \
  DiagnosticIndexName.inc \
  DiagnosticLexKinds.inc \
  DiagnosticParseKinds.inc \
  DiagnosticSemaKinds.inc \
  DiagnosticSerializationKinds.inc \
  arm_neon.inc

clang_format_SRC_FILES := \
  BreakableToken.cpp \
  ContinuationIndenter.cpp \
  Format.cpp \
  FormatToken.cpp \
  TokenAnnotator.cpp \
  UnwrappedLineFormatter.cpp \
  UnwrappedLineParser.cpp \
  WhitespaceManager.cpp

# For the host
# =====================================================
include $(CLEAR_VARS)

LOCAL_SRC_FILES := $(clang_format_SRC_FILES)
LOCAL_MODULE:= libclangFormat
LOCAL_MODULE_TAGS := optional

include $(CLANG_HOST_BUILD_MK)
include $(CLANG_VERSION_INC_MK)
include $(CLANG_TBLGEN_RULES_MK)
include $(BUILD_HOST_STATIC_LIBRARY)

# For the target
# =====================================================
include $(CLEAR_VARS)

LOCAL_SRC_FILES := $(clang_format_SRC_FILES)
LOCAL_MODULE:= libclangFormat
LOCAL_MODULE_TAGS := optional

include $(CLANG_DEVICE_BUILD_MK)
include $(CLANG_VERSION_INC_MK)
include $(CLANG_TBLGEN_RULES_MK)
include $(BUILD_STATIC_LIBRARY)
