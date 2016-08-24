LOCAL_PATH:= $(call my-dir)

mc_SRC_FILES := \
  ConstantPools.cpp \
  ELFObjectWriter.cpp \
  MCAsmBackend.cpp \
  MCAsmInfo.cpp \
  MCAsmInfoCOFF.cpp \
  MCAsmInfoDarwin.cpp \
  MCAsmInfoELF.cpp \
  MCAsmStreamer.cpp \
  MCAssembler.cpp \
  MCCodeEmitter.cpp \
  MCCodeGenInfo.cpp \
  MCContext.cpp \
  MCDwarf.cpp \
  MCELFObjectTargetWriter.cpp \
  MCELFStreamer.cpp \
  MCExpr.cpp \
  MCInst.cpp \
  MCInstPrinter.cpp \
  MCInstrAnalysis.cpp \
  MCInstrDesc.cpp \
  MCLabel.cpp \
  MCLinkerOptimizationHint.cpp \
  MCMachOStreamer.cpp \
  MCMachObjectTargetWriter.cpp \
  MCNullStreamer.cpp \
  MCObjectFileInfo.cpp \
  MCObjectStreamer.cpp \
  MCObjectWriter.cpp \
  MCRegisterInfo.cpp \
  MCSchedule.cpp \
  MCSection.cpp \
  MCSectionCOFF.cpp \
  MCSectionELF.cpp \
  MCSectionMachO.cpp \
  MCStreamer.cpp \
  MCSubtargetInfo.cpp \
  MCSymbol.cpp \
  MCSymbolELF.cpp \
  MCSymbolizer.cpp \
  MCTargetOptions.cpp \
  MCValue.cpp \
  MCWin64EH.cpp \
  MCWinEH.cpp \
  MachObjectWriter.cpp \
  StringTableBuilder.cpp \
  SubtargetFeature.cpp \
  WinCOFFObjectWriter.cpp \
  WinCOFFStreamer.cpp \
  YAML.cpp

# For the host
# =====================================================
include $(CLEAR_VARS)

LOCAL_SRC_FILES := $(mc_SRC_FILES)

LOCAL_MODULE:= libLLVMMC

LOCAL_MODULE_HOST_OS := darwin linux windows


include $(LLVM_HOST_BUILD_MK)
include $(BUILD_HOST_STATIC_LIBRARY)

# For the device
# =====================================================
include $(CLEAR_VARS)
ifneq (true,$(DISABLE_LLVM_DEVICE_BUILDS))

LOCAL_SRC_FILES := $(mc_SRC_FILES)

LOCAL_MODULE:= libLLVMMC

include $(LLVM_DEVICE_BUILD_MK)
include $(BUILD_STATIC_LIBRARY)
endif
