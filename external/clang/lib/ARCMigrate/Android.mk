LOCAL_PATH := $(call my-dir)

include $(CLEAR_TBLGEN_VARS)

TBLGEN_TABLES := \
  Attrs.inc \
  AttrList.inc \
  AttrParsedAttrList.inc    \
  AttrVisitor.inc \
  CommentCommandList.inc \
  CommentNodes.inc \
  DeclNodes.inc \
  DiagnosticCommonKinds.inc \
  DiagnosticGroups.inc \
  DiagnosticSemaKinds.inc \
  StmtNodes.inc

clang_arc_migrate_SRC_FILES := \
  ARCMT.cpp \
  ARCMTActions.cpp \
  FileRemapper.cpp \
  ObjCMT.cpp \
  PlistReporter.cpp \
  TransAPIUses.cpp \
  TransARCAssign.cpp \
  TransAutoreleasePool.cpp \
  TransBlockObjCVariable.cpp \
  TransEmptyStatementsAndDealloc.cpp \
  TransformActions.cpp \
  Transforms.cpp \
  TransGCAttrs.cpp \
  TransGCCalls.cpp \
  TransProperties.cpp \
  TransProtectedScope.cpp \
  TransRetainReleaseDealloc.cpp \
  TransUnbridgedCasts.cpp \
  TransUnusedInitDelegate.cpp \
  TransZeroOutPropsInDealloc.cpp

# For the host
# =====================================================
include $(CLEAR_VARS)

LOCAL_SRC_FILES := $(clang_arc_migrate_SRC_FILES)
LOCAL_MODULE := libclangARCMigrate
LOCAL_MODULE_TAGS := optional

include $(CLANG_HOST_BUILD_MK)
include $(CLANG_VERSION_INC_MK)
include $(CLANG_TBLGEN_RULES_MK)
include $(BUILD_HOST_STATIC_LIBRARY)

# For the target
# ============================
include $(CLEAR_VARS)

LOCAL_SRC_FILES := $(clang_arc_migrate_SRC_FILES)
LOCAL_MODULE := libclangARCMigrate
LOCAL_MODULE_TAGS := optional

include $(CLANG_DEVICE_BUILD_MK)
include $(CLANG_VERSION_INC_MK)
include $(CLANG_TBLGEN_RULES_MK)
include $(BUILD_STATIC_LIBRARY)
