LOCAL_PATH:= $(call my-dir)

analysis_SRC_FILES := \
  AliasAnalysis.cpp \
  AliasAnalysisEvaluator.cpp \
  AliasSetTracker.cpp \
  Analysis.cpp \
  AssumptionCache.cpp \
  BasicAliasAnalysis.cpp \
  BlockFrequencyInfo.cpp \
  BlockFrequencyInfoImpl.cpp \
  BranchProbabilityInfo.cpp \
  CallGraph.cpp \
  CallGraphSCCPass.cpp \
  CallPrinter.cpp \
  CaptureTracking.cpp \
  CFG.cpp \
  CFGPrinter.cpp \
  CFLAliasAnalysis.cpp \
  CGSCCPassManager.cpp \
  CodeMetrics.cpp \
  ConstantFolding.cpp \
  CostModel.cpp \
  Delinearization.cpp \
  DemandedBits.cpp \
  DependenceAnalysis.cpp \
  DivergenceAnalysis.cpp \
  DominanceFrontier.cpp \
  DomPrinter.cpp \
  EHPersonalities.cpp \
  GlobalsModRef.cpp \
  InlineCost.cpp \
  InstCount.cpp \
  InstructionSimplify.cpp \
  Interval.cpp \
  IntervalPartition.cpp \
  IteratedDominanceFrontier.cpp \
  IVUsers.cpp \
  LazyCallGraph.cpp \
  LazyValueInfo.cpp \
  Lint.cpp \
  Loads.cpp \
  LoopAccessAnalysis.cpp \
  LoopInfo.cpp \
  LoopPass.cpp \
  MemDepPrinter.cpp \
  MemDerefPrinter.cpp \
  MemoryBuiltins.cpp \
  MemoryDependenceAnalysis.cpp \
  MemoryLocation.cpp \
  ModuleDebugInfoPrinter.cpp \
  ObjCARCAliasAnalysis.cpp \
  ObjCARCAnalysisUtils.cpp \
  ObjCARCInstKind.cpp \
  OrderedBasicBlock.cpp \
  PHITransAddr.cpp \
  PostDominators.cpp \
  PtrUseVisitor.cpp \
  RegionInfo.cpp \
  RegionPass.cpp \
  RegionPrinter.cpp \
  ScalarEvolutionAliasAnalysis.cpp \
  ScalarEvolution.cpp \
  ScalarEvolutionExpander.cpp \
  ScalarEvolutionNormalization.cpp \
  ScopedNoAliasAA.cpp \
  SparsePropagation.cpp \
  TargetLibraryInfo.cpp \
  TargetTransformInfo.cpp \
  Trace.cpp \
  TypeBasedAliasAnalysis.cpp \
  ValueTracking.cpp \
  VectorUtils.cpp

# For the host
# =====================================================
include $(CLEAR_VARS)

LOCAL_MODULE:= libLLVMAnalysis
LOCAL_MODULE_HOST_OS := darwin linux windows
LOCAL_SRC_FILES := $(analysis_SRC_FILES)

include $(LLVM_HOST_BUILD_MK)
include $(LLVM_GEN_ATTRIBUTES_MK)
include $(LLVM_GEN_INTRINSICS_MK)
include $(BUILD_HOST_STATIC_LIBRARY)

# For the device
# =====================================================
ifneq (true,$(DISABLE_LLVM_DEVICE_BUILDS))
include $(CLEAR_VARS)

LOCAL_MODULE:= libLLVMAnalysis
LOCAL_SRC_FILES := $(analysis_SRC_FILES)

include $(LLVM_DEVICE_BUILD_MK)
include $(LLVM_GEN_ATTRIBUTES_MK)
include $(LLVM_GEN_INTRINSICS_MK)
include $(BUILD_STATIC_LIBRARY)
endif
