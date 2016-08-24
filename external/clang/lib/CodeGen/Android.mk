LOCAL_PATH:= $(call my-dir)

include $(CLEAR_TBLGEN_VARS)

clang_codegen_TBLGEN_TABLES := \
  AttrList.inc \
  AttrParsedAttrList.inc \
  Attrs.inc \
  AttrVisitor.inc \
  CommentCommandList.inc \
  CommentNodes.inc \
  DeclNodes.inc \
  DiagnosticCommonKinds.inc \
  DiagnosticFrontendKinds.inc \
  DiagnosticSemaKinds.inc \
  StmtNodes.inc \
  arm_neon.inc

clang_codegen_SRC_FILES := \
  BackendUtil.cpp \
  CGAtomic.cpp \
  CGBlocks.cpp \
  CGBuiltin.cpp \
  CGCall.cpp \
  CGClass.cpp \
  CGCleanup.cpp \
  CGCUDANV.cpp \
  CGCUDARuntime.cpp \
  CGCXXABI.cpp \
  CGCXX.cpp \
  CGDebugInfo.cpp \
  CGDecl.cpp \
  CGDeclCXX.cpp \
  CGException.cpp \
  CGExprAgg.cpp \
  CGExprComplex.cpp \
  CGExprConstant.cpp \
  CGExpr.cpp \
  CGExprCXX.cpp \
  CGExprScalar.cpp \
  CGLoopInfo.cpp \
  CGObjC.cpp \
  CGObjCGNU.cpp \
  CGObjCMac.cpp \
  CGObjCRuntime.cpp \
  CGOpenCLRuntime.cpp \
  CGOpenMPRuntime.cpp \
  CGRecordLayoutBuilder.cpp \
  CGStmt.cpp \
  CGStmtOpenMP.cpp \
  CGVTables.cpp \
  CGVTT.cpp \
  CodeGenABITypes.cpp \
  CodeGenAction.cpp \
  CodeGenFunction.cpp \
  CodeGenModule.cpp \
  CodeGenPGO.cpp \
  CodeGenTBAA.cpp \
  CodeGenTypes.cpp \
  CoverageMappingGen.cpp \
  ItaniumCXXABI.cpp \
  MicrosoftCXXABI.cpp \
  ModuleBuilder.cpp \
  ObjectFilePCHContainerOperations.cpp \
  SanitizerMetadata.cpp \
  TargetInfo.cpp

# For the host
# =====================================================
include $(CLEAR_VARS)

LOCAL_MODULE:= libclangCodeGen
LOCAL_MODULE_TAGS := optional

LOCAL_SRC_FILES := $(clang_codegen_SRC_FILES)
TBLGEN_TABLES := $(clang_codegen_TBLGEN_TABLES)

include $(CLANG_HOST_BUILD_MK)
include $(CLANG_VERSION_INC_MK)
include $(CLANG_TBLGEN_RULES_MK)
include $(LLVM_GEN_ATTRIBUTES_MK)
include $(LLVM_GEN_INTRINSICS_MK)
include $(BUILD_HOST_STATIC_LIBRARY)

# For the target
# =====================================================
include $(CLEAR_VARS)

LOCAL_MODULE:= libclangCodeGen
LOCAL_MODULE_TAGS := optional

LOCAL_SRC_FILES := $(clang_codegen_SRC_FILES)
TBLGEN_TABLES := $(clang_codegen_TBLGEN_TABLES)

include $(CLANG_DEVICE_BUILD_MK)
include $(CLANG_VERSION_INC_MK)
include $(CLANG_TBLGEN_RULES_MK)
include $(LLVM_GEN_ATTRIBUTES_MK)
include $(LLVM_GEN_INTRINSICS_MK)
include $(BUILD_STATIC_LIBRARY)
