LOCAL_PATH:= $(call my-dir)


include $(CLEAR_TBLGEN_VARS)

TBLGEN_TABLES := \
  AttrList.inc \
  Attrs.inc \
  AttrVisitor.inc \
  CommentCommandList.inc \
  DeclNodes.inc \
  DiagnosticCommonKinds.inc \
  StmtNodes.inc \

clang_astmatchers_SRC_FILES := \
  ASTMatchFinder.cpp \
  ASTMatchersInternal.cpp

# For the host
# =====================================================
#
include $(CLEAR_VARS)

LOCAL_SRC_FILES := $(clang_astmatchers_SRC_FILES)
LOCAL_MODULE:= libclangASTMatchers
LOCAL_MODULE_TAGS := optional

include $(CLANG_HOST_BUILD_MK)
include $(CLANG_VERSION_INC_MK)
include $(CLANG_TBLGEN_RULES_MK)
include $(BUILD_HOST_STATIC_LIBRARY)

# For the target
# =====================================================
include $(CLEAR_VARS)
LOCAL_SRC_FILES := $(clang_astmatchers_SRC_FILES)
LOCAL_MODULE:= libclangASTMatchers
LOCAL_MODULE_TAGS := optional

include $(CLANG_DEVICE_BUILD_MK)
include $(CLANG_VERSION_INC_MK)
include $(CLANG_TBLGEN_RULES_MK)
include $(BUILD_STATIC_LIBRARY)
