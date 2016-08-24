LOCAL_PATH:= $(call my-dir)

include $(CLEAR_TBLGEN_VARS)

clang_edit_SRC_FILES := \
  Commit.cpp \
  EditedSource.cpp \
  RewriteObjCFoundationAPI.cpp

TBLGEN_TABLES := \
  Attrs.inc \
  AttrList.inc \
  CommentCommandList.inc \
  CommentNodes.inc \
  DeclNodes.inc \
  DiagnosticCommonKinds.inc \
  StmtNodes.inc

# For the host
# =====================================================
include $(CLEAR_VARS)

LOCAL_SRC_FILES := $(clang_edit_SRC_FILES)
LOCAL_MODULE:= libclangEdit
LOCAL_MODULE_TAGS := optional

include $(CLANG_HOST_BUILD_MK)
include $(CLANG_VERSION_INC_MK)
include $(CLANG_TBLGEN_RULES_MK)
include $(BUILD_HOST_STATIC_LIBRARY)

# For the target
# =====================================================
include $(CLEAR_VARS)

LOCAL_SRC_FILES := $(clang_edit_SRC_FILES)
LOCAL_MODULE:= libclangEdit
LOCAL_MODULE_TAGS := optional

include $(CLANG_DEVICE_BUILD_MK)
include $(CLANG_VERSION_INC_MK)
include $(CLANG_TBLGEN_RULES_MK)
include $(BUILD_STATIC_LIBRARY)
