LOCAL_PATH:= $(call my-dir)

clang_static_analyzer_core_TBLGEN_TABLES := \
  AttrList.inc \
  Attrs.inc \
  CommentCommandList.inc \
  CommentNodes.inc \
  DeclNodes.inc \
  DiagnosticCommonKinds.inc \
  DiagnosticFrontendKinds.inc \
  StmtNodes.inc

clang_static_analyzer_core_SRC_FILES := \
  AnalysisManager.cpp \
  AnalyzerOptions.cpp \
  APSIntType.cpp \
  BasicValueFactory.cpp \
  BlockCounter.cpp \
  BugReporter.cpp \
  BugReporterVisitors.cpp \
  CallEvent.cpp \
  CheckerContext.cpp \
  Checker.cpp \
  CheckerHelpers.cpp \
  CheckerManager.cpp \
  CheckerRegistry.cpp \
  CommonBugCategories.cpp \
  ConstraintManager.cpp \
  CoreEngine.cpp \
  DynamicTypeMap.cpp \
  Environment.cpp \
  ExplodedGraph.cpp \
  ExprEngineCallAndReturn.cpp \
  ExprEngineC.cpp \
  ExprEngine.cpp \
  ExprEngineCXX.cpp \
  ExprEngineObjC.cpp \
  FunctionSummary.cpp \
  HTMLDiagnostics.cpp \
  IssueHash.cpp \
  LoopWidening.cpp \
  MemRegion.cpp \
  PathDiagnostic.cpp \
  PlistDiagnostics.cpp \
  ProgramState.cpp \
  RangeConstraintManager.cpp \
  RegionStore.cpp \
  SimpleConstraintManager.cpp \
  SimpleSValBuilder.cpp \
  Store.cpp \
  SubEngine.cpp \
  SValBuilder.cpp \
  SVals.cpp \
  SymbolManager.cpp

# For the host only
# =====================================================
include $(CLEAR_VARS)
include $(CLEAR_TBLGEN_VARS)

TBLGEN_TABLES := $(clang_static_analyzer_core_TBLGEN_TABLES)

LOCAL_SRC_FILES := $(clang_static_analyzer_core_SRC_FILES)

LOCAL_MODULE:= libclangStaticAnalyzerCore

LOCAL_MODULE_TAGS := optional

include $(CLANG_HOST_BUILD_MK)
include $(CLANG_TBLGEN_RULES_MK)
include $(CLANG_VERSION_INC_MK)
include $(BUILD_HOST_STATIC_LIBRARY)
