LOCAL_PATH:= $(call my-dir)

codegen_SRC_FILES := \
  AggressiveAntiDepBreaker.cpp \
  AllocationOrder.cpp \
  Analysis.cpp \
  AtomicExpandPass.cpp \
  BasicTargetTransformInfo.cpp \
  BranchFolding.cpp \
  CalcSpillWeights.cpp \
  CallingConvLower.cpp \
  CodeGen.cpp \
  CodeGenPrepare.cpp \
  CoreCLRGC.cpp \
  CriticalAntiDepBreaker.cpp \
  DeadMachineInstructionElim.cpp \
  DFAPacketizer.cpp \
  DwarfEHPrepare.cpp \
  EarlyIfConversion.cpp \
  EdgeBundles.cpp \
  ErlangGC.cpp \
  ExecutionDepsFix.cpp \
  ExpandISelPseudos.cpp \
  ExpandPostRAPseudos.cpp \
  FaultMaps.cpp \
  FuncletLayout.cpp \
  GCMetadata.cpp \
  GCMetadataPrinter.cpp \
  GCRootLowering.cpp \
  GCStrategy.cpp \
  GlobalMerge.cpp \
  IfConversion.cpp \
  ImplicitNullChecks.cpp \
  InlineSpiller.cpp \
  InterferenceCache.cpp \
  InterleavedAccessPass.cpp \
  IntrinsicLowering.cpp \
  LatencyPriorityQueue.cpp \
  LexicalScopes.cpp \
  LiveDebugValues.cpp \
  LiveDebugVariables.cpp \
  LiveIntervalAnalysis.cpp \
  LiveInterval.cpp \
  LiveIntervalUnion.cpp \
  LivePhysRegs.cpp \
  LiveRangeCalc.cpp \
  LiveRangeEdit.cpp \
  LiveRegMatrix.cpp \
  LiveStackAnalysis.cpp \
  LiveVariables.cpp \
  LLVMTargetMachine.cpp \
  LocalStackSlotAllocation.cpp \
  LowerEmuTLS.cpp \
  MachineBasicBlock.cpp \
  MachineBlockFrequencyInfo.cpp \
  MachineBlockPlacement.cpp \
  MachineBranchProbabilityInfo.cpp \
  MachineCombiner.cpp \
  MachineCopyPropagation.cpp \
  MachineCSE.cpp \
  MachineDominanceFrontier.cpp \
  MachineDominators.cpp \
  MachineFunctionAnalysis.cpp \
  MachineFunction.cpp \
  MachineFunctionPass.cpp \
  MachineFunctionPrinterPass.cpp \
  MachineInstrBundle.cpp \
  MachineInstr.cpp \
  MachineLICM.cpp \
  MachineLoopInfo.cpp \
  MachineModuleInfo.cpp \
  MachineModuleInfoImpls.cpp \
  MachinePassRegistry.cpp \
  MachinePostDominators.cpp \
  MachineRegionInfo.cpp \
  MachineRegisterInfo.cpp \
  MachineScheduler.cpp \
  MachineSink.cpp \
  MachineSSAUpdater.cpp \
  MachineTraceMetrics.cpp \
  MachineVerifier.cpp \
  MIRPrinter.cpp \
  MIRPrintingPass.cpp \
  OcamlGC.cpp \
  OptimizePHIs.cpp \
  ParallelCG.cpp \
  Passes.cpp \
  PeepholeOptimizer.cpp \
  PHIElimination.cpp \
  PHIEliminationUtils.cpp \
  PostRASchedulerList.cpp \
  ProcessImplicitDefs.cpp \
  PrologEpilogInserter.cpp \
  PseudoSourceValue.cpp \
  RegAllocBase.cpp \
  RegAllocBasic.cpp \
  RegAllocFast.cpp \
  RegAllocGreedy.cpp \
  RegAllocPBQP.cpp \
  RegisterClassInfo.cpp \
  RegisterCoalescer.cpp \
  RegisterPressure.cpp \
  RegisterScavenging.cpp \
  ScheduleDAG.cpp \
  ScheduleDAGInstrs.cpp \
  ScheduleDAGPrinter.cpp \
  ScoreboardHazardRecognizer.cpp \
  ShrinkWrap.cpp \
  ShadowStackGC.cpp \
  ShadowStackGCLowering.cpp \
  SjLjEHPrepare.cpp \
  SlotIndexes.cpp \
  SpillPlacement.cpp \
  SplitKit.cpp \
  StackColoring.cpp \
  StackMapLivenessAnalysis.cpp \
  StackMaps.cpp \
  StackProtector.cpp \
  StackSlotColoring.cpp \
  StatepointExampleGC.cpp \
  TailDuplication.cpp \
  TargetFrameLoweringImpl.cpp \
  TargetInstrInfo.cpp \
  TargetLoweringBase.cpp \
  TargetLoweringObjectFileImpl.cpp \
  TargetOptionsImpl.cpp \
  TargetRegisterInfo.cpp \
  TargetSchedule.cpp \
  TwoAddressInstructionPass.cpp \
  UnreachableBlockElim.cpp \
  VirtRegMap.cpp \
  WinEHPrepare.cpp

# For the host
# =====================================================
include $(CLEAR_VARS)

LOCAL_SRC_FILES := $(codegen_SRC_FILES)
LOCAL_MODULE:= libLLVMCodeGen

LOCAL_MODULE_HOST_OS := darwin linux windows

include $(LLVM_HOST_BUILD_MK)
include $(LLVM_GEN_ATTRIBUTES_MK)
include $(LLVM_GEN_INTRINSICS_MK)
include $(BUILD_HOST_STATIC_LIBRARY)

# For the device
# =====================================================
ifneq (true,$(DISABLE_LLVM_DEVICE_BUILDS))
include $(CLEAR_VARS)

LOCAL_SRC_FILES := $(codegen_SRC_FILES)
LOCAL_MODULE:= libLLVMCodeGen

include $(LLVM_DEVICE_BUILD_MK)
include $(LLVM_GEN_ATTRIBUTES_MK)
include $(LLVM_GEN_INTRINSICS_MK)
include $(BUILD_STATIC_LIBRARY)
endif
