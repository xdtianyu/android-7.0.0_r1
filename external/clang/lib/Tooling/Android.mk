LOCAL_PATH:= $(call my-dir)

include $(CLEAR_TBLGEN_VARS)

TBLGEN_TABLES := \
  AttrList.inc \
  Attrs.inc \
  CommentCommandList.inc \
  DeclNodes.inc \
  DiagnosticCommonKinds.inc \
  DiagnosticDriverKinds.inc \
  DiagnosticFrontendKinds.inc \
  StmtNodes.inc \

clang_tooling_SRC_FILES := \
  ArgumentsAdjusters.cpp \
  CommonOptionsParser.cpp \
  CompilationDatabase.cpp \
  Core/Replacement.cpp \
  FileMatchTrie.cpp \
  JSONCompilationDatabase.cpp \
  Refactoring.cpp \
  RefactoringCallbacks.cpp \
  Tooling.cpp \

# For the host
# =====================================================
include $(CLEAR_VARS)

LOCAL_SRC_FILES := $(clang_tooling_SRC_FILES)
LOCAL_MODULE:= libclangTooling
LOCAL_MODULE_TAGS := optional

include $(CLANG_HOST_BUILD_MK)
include $(CLANG_VERSION_INC_MK)
include $(CLANG_TBLGEN_RULES_MK)
include $(BUILD_HOST_STATIC_LIBRARY)

# For the target
# =====================================================
include $(CLEAR_VARS)

LOCAL_SRC_FILES := $(clang_tooling_SRC_FILES)
LOCAL_MODULE:= libclangTooling
LOCAL_MODULE_TAGS := optional

include $(CLANG_DEVICE_BUILD_MK)
include $(CLANG_VERSION_INC_MK)
include $(CLANG_TBLGEN_RULES_MK)
include $(BUILD_STATIC_LIBRARY)
