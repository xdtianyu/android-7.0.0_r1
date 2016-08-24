LOCAL_PATH:= $(call my-dir)

# No dia support
debuginfo_pdb_SRC_FILES := \
  IPDBSourceFile.cpp \
  PDB.cpp \
  PDBContext.cpp \
  PDBExtras.cpp \
  PDBInterfaceAnchors.cpp \
  PDBSymbolAnnotation.cpp \
  PDBSymbolBlock.cpp \
  PDBSymbolCompiland.cpp \
  PDBSymbolCompilandDetails.cpp \
  PDBSymbolCompilandEnv.cpp \
  PDBSymbol.cpp \
  PDBSymbolCustom.cpp \
  PDBSymbolData.cpp \
  PDBSymbolExe.cpp \
  PDBSymbolFunc.cpp \
  PDBSymbolFuncDebugEnd.cpp \
  PDBSymbolFuncDebugStart.cpp \
  PDBSymbolLabel.cpp \
  PDBSymbolPublicSymbol.cpp \
  PDBSymbolThunk.cpp \
  PDBSymbolTypeArray.cpp \
  PDBSymbolTypeBaseClass.cpp \
  PDBSymbolTypeBuiltin.cpp \
  PDBSymbolTypeCustom.cpp \
  PDBSymbolTypeDimension.cpp \
  PDBSymbolTypeEnum.cpp \
  PDBSymbolTypeFriend.cpp \
  PDBSymbolTypeFunctionArg.cpp \
  PDBSymbolTypeFunctionSig.cpp \
  PDBSymbolTypeManaged.cpp \
  PDBSymbolTypePointer.cpp \
  PDBSymbolTypeTypedef.cpp \
  PDBSymbolTypeUDT.cpp \
  PDBSymbolTypeVTable.cpp \
  PDBSymbolTypeVTableShape.cpp \
  PDBSymbolUnknown.cpp \
  PDBSymbolUsingNamespace.cpp \
  PDBSymDumper.cpp

# For the host
# =====================================================
include $(CLEAR_VARS)

REQUIRES_RTTI := 1

LOCAL_SRC_FILES := $(debuginfo_pdb_SRC_FILES)

LOCAL_MODULE:= libLLVMDebugInfoPDB

LOCAL_MODULE_HOST_OS := darwin linux windows

include $(LLVM_HOST_BUILD_MK)
include $(LLVM_GEN_INTRINSICS_MK)
include $(BUILD_HOST_STATIC_LIBRARY)

# For the device
# =====================================================
ifneq (true,$(DISABLE_LLVM_DEVICE_BUILDS))
include $(CLEAR_VARS)

REQUIRES_RTTI := 1

LOCAL_SRC_FILES := $(debuginfo_pdb_SRC_FILES)

LOCAL_MODULE:= libLLVMDebugInfoPDB

include $(LLVM_DEVICE_BUILD_MK)
include $(LLVM_GEN_INTRINSICS_MK)
include $(BUILD_STATIC_LIBRARY)
endif
