LOCAL_PATH:= $(call my-dir)

include $(CLEAR_TBLGEN_VARS)

TBLGEN_TABLES := \
  DiagnosticLexKinds.inc \
  DiagnosticCommonKinds.inc

clang_lex_SRC_FILES := \
  HeaderMap.cpp \
  HeaderSearch.cpp \
  Lexer.cpp \
  LiteralSupport.cpp \
  MacroArgs.cpp \
  MacroInfo.cpp \
  ModuleMap.cpp \
  PPCaching.cpp \
  PPCallbacks.cpp \
  PPConditionalDirectiveRecord.cpp \
  PPDirectives.cpp \
  PPExpressions.cpp \
  PPLexerChange.cpp \
  PPMacroExpansion.cpp \
  PTHLexer.cpp \
  Pragma.cpp \
  PreprocessingRecord.cpp \
  Preprocessor.cpp \
  PreprocessorLexer.cpp \
  ScratchBuffer.cpp \
  TokenConcatenation.cpp \
  TokenLexer.cpp

# For the host
# =====================================================
include $(CLEAR_VARS)

LOCAL_SRC_FILES := $(clang_lex_SRC_FILES)
LOCAL_MODULE:= libclangLex
LOCAL_MODULE_TAGS := optional

include $(CLANG_HOST_BUILD_MK)
include $(CLANG_TBLGEN_RULES_MK)
include $(BUILD_HOST_STATIC_LIBRARY)

# For the target
# =====================================================
include $(CLEAR_VARS)

LOCAL_SRC_FILES := $(clang_lex_SRC_FILES)
LOCAL_MODULE:= libclangLex
LOCAL_MODULE_TAGS := optional

include $(CLANG_DEVICE_BUILD_MK)
include $(CLANG_TBLGEN_RULES_MK)
include $(BUILD_STATIC_LIBRARY)
