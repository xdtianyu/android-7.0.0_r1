LOCAL_PATH:= $(call my-dir)

include $(CLEAR_TBLGEN_VARS)

TBLGEN_TABLES := \
  AttrList.inc \
  Attrs.inc \
  AttrParsedAttrList.inc \
  CommentCommandList.inc \
  CommentNodes.inc \
  DeclNodes.inc \
  DiagnosticCommonKinds.inc \
  DiagnosticFrontendKinds.inc \
  StmtNodes.inc

clang_rewrite_frontend_SRC_FILES := \
  FixItRewriter.cpp \
  FrontendActions.cpp \
  HTMLPrint.cpp \
  InclusionRewriter.cpp \
  RewriteMacros.cpp \
  RewriteModernObjC.cpp \
  RewriteObjC.cpp \
  RewriteTest.cpp

# For the host
# =====================================================
include $(CLEAR_VARS)

LOCAL_SRC_FILES := $(clang_rewrite_frontend_SRC_FILES)
LOCAL_MODULE:= libclangRewriteFrontend
LOCAL_MODULE_TAGS := optional

include $(CLANG_HOST_BUILD_MK)
include $(CLANG_TBLGEN_RULES_MK)
include $(BUILD_HOST_STATIC_LIBRARY)

# For the target
# =====================================================
include $(CLEAR_VARS)

LOCAL_SRC_FILES := $(clang_rewrite_frontend_SRC_FILES)
LOCAL_MODULE:= libclangRewriteFrontend
LOCAL_MODULE_TAGS := optional

include $(CLANG_DEVICE_BUILD_MK)
include $(CLANG_TBLGEN_RULES_MK)
include $(BUILD_STATIC_LIBRARY)
