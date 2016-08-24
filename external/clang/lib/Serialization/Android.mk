LOCAL_PATH:= $(call my-dir)

include $(CLEAR_TBLGEN_VARS)

TBLGEN_TABLES := \
  AttrList.inc \
  AttrParsedAttrList.inc \
  AttrPCHRead.inc \
  AttrPCHWrite.inc \
  Attrs.inc \
  CommentCommandList.inc \
  CommentNodes.inc \
  DeclNodes.inc \
  DiagnosticCommonKinds.inc \
  DiagnosticFrontendKinds.inc \
  DiagnosticSemaKinds.inc \
  DiagnosticSerializationKinds.inc \
  StmtNodes.inc

clang_serialization_SRC_FILES :=\
  ASTCommon.cpp \
  ASTReader.cpp \
  ASTReaderDecl.cpp \
  ASTReaderStmt.cpp \
  ASTWriter.cpp \
  ASTWriterDecl.cpp \
  ASTWriterStmt.cpp \
  GeneratePCH.cpp \
  GlobalModuleIndex.cpp \
  Module.cpp \
  ModuleFileExtension.cpp \
  ModuleManager.cpp

# For the host
# =====================================================
include $(CLEAR_VARS)

LOCAL_SRC_FILES := $(clang_serialization_SRC_FILES)
LOCAL_MODULE:= libclangSerialization
LOCAL_MODULE_TAGS := optional

include $(CLANG_HOST_BUILD_MK)
include $(CLANG_TBLGEN_RULES_MK)
include $(CLANG_VERSION_INC_MK)
include $(BUILD_HOST_STATIC_LIBRARY)

# For the target
# =====================================================
include $(CLEAR_VARS)

LOCAL_SRC_FILES := $(clang_serialization_SRC_FILES)
LOCAL_MODULE:= libclangSerialization
LOCAL_MODULE_TAGS := optional

include $(CLANG_DEVICE_BUILD_MK)
include $(CLANG_TBLGEN_RULES_MK)
include $(CLANG_VERSION_INC_MK)
include $(BUILD_STATIC_LIBRARY)
