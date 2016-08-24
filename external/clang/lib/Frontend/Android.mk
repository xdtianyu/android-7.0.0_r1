LOCAL_PATH:= $(call my-dir)

include $(CLEAR_TBLGEN_VARS)

TBLGEN_TABLES := \
  AttrList.inc \
  Attrs.inc \
  AttrParsedAttrList.inc \
  AttrVisitor.inc \
  CC1Options.inc \
  CommentCommandList.inc \
  CommentNodes.inc \
  DiagnosticASTKinds.inc \
  DiagnosticCommonKinds.inc \
  DiagnosticDriverKinds.inc \
  DiagnosticFrontendKinds.inc \
  DiagnosticLexKinds.inc \
  DiagnosticSemaKinds.inc \
  DeclNodes.inc \
  StmtNodes.inc

clang_frontend_SRC_FILES := \
  ASTConsumers.cpp \
  ASTMerge.cpp \
  ASTUnit.cpp \
  CacheTokens.cpp \
  ChainedDiagnosticConsumer.cpp \
  ChainedIncludesSource.cpp \
  CodeGenOptions.cpp \
  CompilerInstance.cpp \
  CompilerInvocation.cpp \
  CreateInvocationFromCommandLine.cpp \
  DependencyFile.cpp \
  DependencyGraph.cpp \
  DiagnosticRenderer.cpp \
  FrontendAction.cpp \
  FrontendActions.cpp \
  FrontendOptions.cpp \
  HeaderIncludeGen.cpp \
  InitHeaderSearch.cpp \
  InitPreprocessor.cpp \
  LangStandards.cpp \
  LayoutOverrideSource.cpp \
  LogDiagnosticPrinter.cpp \
  ModuleDependencyCollector.cpp \
  MultiplexConsumer.cpp \
  PCHContainerOperations.cpp \
  PrintPreprocessedOutput.cpp \
  SerializedDiagnosticPrinter.cpp \
  SerializedDiagnosticReader.cpp \
  TestModuleFileExtension.cpp \
  TextDiagnosticBuffer.cpp \
  TextDiagnostic.cpp \
  TextDiagnosticPrinter.cpp \
  VerifyDiagnosticConsumer.cpp

# For the host
# =====================================================
include $(CLEAR_VARS)

LOCAL_SRC_FILES := $(clang_frontend_SRC_FILES)
LOCAL_MODULE:= libclangFrontend
LOCAL_MODULE_TAGS:= optional

include $(CLANG_HOST_BUILD_MK)
include $(CLANG_TBLGEN_RULES_MK)
include $(CLANG_VERSION_INC_MK)
include $(LLVM_GEN_ATTRIBUTES_MK)
include $(BUILD_HOST_STATIC_LIBRARY)

# For the target
# =====================================================
include $(CLEAR_VARS)

LOCAL_SRC_FILES := $(clang_frontend_SRC_FILES)
LOCAL_MODULE:= libclangFrontend
LOCAL_MODULE_TAGS:= optional

include $(CLANG_DEVICE_BUILD_MK)
include $(CLANG_TBLGEN_RULES_MK)
include $(CLANG_VERSION_INC_MK)
include $(LLVM_GEN_ATTRIBUTES_MK)
include $(BUILD_STATIC_LIBRARY)
