LOCAL_PATH:= $(call my-dir)

transforms_scalar_SRC_FILES := \
  ADCE.cpp \
  BDCE.cpp \
  AlignmentFromAssumptions.cpp \
  ConstantProp.cpp \
  ConstantHoisting.cpp \
  CorrelatedValuePropagation.cpp \
  DCE.cpp \
  DeadStoreElimination.cpp \
  EarlyCSE.cpp \
  FlattenCFGPass.cpp \
  Float2Int.cpp \
  GVN.cpp \
  IndVarSimplify.cpp \
  InductiveRangeCheckElimination.cpp \
  JumpThreading.cpp \
  LICM.cpp \
  LoadCombine.cpp \
  LoopDeletion.cpp \
  LoopDistribute.cpp \
  LoopIdiomRecognize.cpp \
  LoopInstSimplify.cpp \
  LoopInterchange.cpp \
  LoopLoadElimination.cpp \
  LoopRerollPass.cpp \
  LoopRotation.cpp \
  LoopStrengthReduce.cpp \
  LoopUnrollPass.cpp \
  LoopUnswitch.cpp \
  LowerAtomic.cpp \
  LowerExpectIntrinsic.cpp \
  MemCpyOptimizer.cpp \
  MergedLoadStoreMotion.cpp \
  NaryReassociate.cpp \
  PartiallyInlineLibCalls.cpp \
  PlaceSafepoints.cpp \
  Reassociate.cpp \
  Reg2Mem.cpp \
  RewriteStatepointsForGC.cpp \
  SCCP.cpp \
  SROA.cpp \
  Scalar.cpp \
  Scalarizer.cpp \
  ScalarReplAggregates.cpp \
  SeparateConstOffsetFromGEP.cpp \
  SimplifyCFGPass.cpp \
  Sink.cpp \
  SpeculativeExecution.cpp \
  StraightLineStrengthReduce.cpp \
  StructurizeCFG.cpp \
  TailRecursionElimination.cpp

# For the host
# =====================================================
include $(CLEAR_VARS)

LOCAL_SRC_FILES :=	\
	$(transforms_scalar_SRC_FILES)

LOCAL_MODULE:= libLLVMScalarOpts

LOCAL_MODULE_HOST_OS := darwin linux windows

include $(LLVM_HOST_BUILD_MK)
include $(LLVM_GEN_ATTRIBUTES_MK)
include $(LLVM_GEN_INTRINSICS_MK)
include $(BUILD_HOST_STATIC_LIBRARY)

# For the device
# =====================================================
ifneq (true,$(DISABLE_LLVM_DEVICE_BUILDS))
include $(CLEAR_VARS)

LOCAL_SRC_FILES := $(transforms_scalar_SRC_FILES)
LOCAL_MODULE:= libLLVMScalarOpts

include $(LLVM_DEVICE_BUILD_MK)
include $(LLVM_GEN_ATTRIBUTES_MK)
include $(LLVM_GEN_INTRINSICS_MK)
include $(BUILD_STATIC_LIBRARY)
endif
