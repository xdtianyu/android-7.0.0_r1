LOCAL_PATH:= $(call my-dir)

include $(CLEAR_TBLGEN_VARS)

TBLGEN_TABLES :=    \
  AttrList.inc \
  AttrParsedAttrList.inc \
  Attrs.inc \
  AttrVisitor.inc \
  CommentCommandList.inc \
  CommentNodes.inc \
  DiagnosticCommonKinds.inc \
  DiagnosticFrontendKinds.inc \
  DiagnosticGroups.inc \
  DiagnosticSerializationKinds.inc \
  DeclNodes.inc \
  StmtNodes.inc \

clang_libclang_SRC_FILES := \
  ARCMigrate.cpp \
  BuildSystem.cpp \
  CIndex.cpp \
  CIndexCXX.cpp \
  CIndexCodeCompletion.cpp \
  CIndexDiagnostic.cpp \
  CIndexHigh.cpp \
  CIndexInclusionStack.cpp \
  CIndexUSRs.cpp \
  CIndexer.cpp \
  CXComment.cpp \
  CXCompilationDatabase.cpp \
  CXCursor.cpp \
  CXLoadedDiagnostic.cpp \
  CXSourceLocation.cpp \
  CXStoredDiagnostic.cpp \
  CXString.cpp \
  CXType.cpp \
  IndexBody.cpp \
  IndexDecl.cpp \
  IndexTypeSourceInfo.cpp \
  Indexing.cpp \
  IndexingContext.cpp \

# For the host
# =====================================================
include $(CLEAR_VARS)

LOCAL_SRC_FILES := $(clang_libclang_SRC_FILES)
LOCAL_MODULE := libclangLibclang
LOCAL_MODULE_TAGS := optional

include $(CLANG_HOST_BUILD_MK)
include $(CLANG_VERSION_INC_MK)
include $(CLANG_TBLGEN_RULES_MK)
include $(BUILD_HOST_STATIC_LIBRARY)

# For the target
# =====================================================
include $(CLEAR_VARS)

LOCAL_SRC_FILES := $(clang_libclang_SRC_FILES)
LOCAL_MODULE := libclangLibclang
LOCAL_MODULE_TAGS := optional

include $(CLANG_DEVICE_BUILD_MK)
include $(CLANG_VERSION_INC_MK)
include $(CLANG_TBLGEN_RULES_MK)
include $(BUILD_STATIC_LIBRARY)
