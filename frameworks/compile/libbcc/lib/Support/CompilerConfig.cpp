/*
 * Copyright 2012, The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "bcc/Support/CompilerConfig.h"
#include "bcc/Config/Config.h"
#include "bcc/Support/Properties.h"

#include <llvm/CodeGen/SchedulerRegistry.h>
#include <llvm/MC/SubtargetFeature.h>
#include <llvm/Support/Host.h>
#include <llvm/Support/TargetRegistry.h>

#include "bcc/Support/Log.h"

using namespace bcc;

#if defined (PROVIDE_X86_CODEGEN) && !defined(__HOST__)

namespace {

// Utility function to test for f16c feature.  This function is only needed for
// on-device bcc for x86
bool HasF16C() {
  llvm::StringMap<bool> features;
  if (!llvm::sys::getHostCPUFeatures(features))
    return false;

  if (features.count("f16c") && features["f16c"])
    return true;
  else
    return false;
}

}
#endif // (PROVIDE_X86_CODEGEN) && !defined(__HOST__)

CompilerConfig::CompilerConfig(const std::string &pTriple)
  : mTriple(pTriple), mFullPrecision(true), mTarget(nullptr) {
  //===--------------------------------------------------------------------===//
  // Default setting of target options
  //===--------------------------------------------------------------------===//

  // Use soft-float ABI.  This only selects the ABI (and is applicable only to
  // ARM targets).  Codegen still uses hardware FPU by default.  To use software
  // floating point, add 'soft-float' feature to mFeatureString below.
  mTargetOpts.FloatABIType = llvm::FloatABI::Soft;

  //===--------------------------------------------------------------------===//
  // Default setting for code model
  //===--------------------------------------------------------------------===//
  mCodeModel = llvm::CodeModel::Small;

  //===--------------------------------------------------------------------===//
  // Default setting for relocation model
  //===--------------------------------------------------------------------===//
  mRelocModel = llvm::Reloc::Default;

  //===--------------------------------------------------------------------===//
  // Default setting for optimization level (-O2)
  //===--------------------------------------------------------------------===//
  mOptLevel = llvm::CodeGenOpt::Default;

  //===--------------------------------------------------------------------===//
  // Default setting for architecture type
  //===--------------------------------------------------------------------===//
  mArchType = llvm::Triple::UnknownArch;

  initializeTarget();
  initializeArch();

  return;
}

bool CompilerConfig::initializeTarget() {
  std::string error;
  mTarget = llvm::TargetRegistry::lookupTarget(mTriple, error);
  if (mTarget != nullptr) {
    return true;
  } else {
    ALOGE("Cannot initialize llvm::Target for given triple '%s'! (%s)",
          mTriple.c_str(), error.c_str());
    return false;
  }
}

bool CompilerConfig::initializeArch() {
  if (mTarget != nullptr) {
    mArchType = llvm::Triple::getArchTypeForLLVMName(mTarget->getName());
  } else {
    mArchType = llvm::Triple::UnknownArch;
    return false;
  }

  // Configure each architecture for any necessary additional flags.
  std::vector<std::string> attributes;
  switch (mArchType) {
#if defined(PROVIDE_ARM_CODEGEN)
  case llvm::Triple::arm: {
    llvm::StringMap<bool> features;
    llvm::sys::getHostCPUFeatures(features);

#if defined(__HOST__) || defined(ARCH_ARM_HAVE_VFP)
    attributes.push_back("+vfp3");
#if !defined(__HOST__) && !defined(ARCH_ARM_HAVE_VFP_D32)
    attributes.push_back("+d16");
#endif  // !__HOST__ && !ARCH_ARM_HAVE_VFP_D32
#endif  // __HOST__ || ARCH_ARM_HAVE_VFP

#if defined(__HOST__) || defined(ARCH_ARM_HAVE_NEON)
    // Only enable NEON on ARM if we have relaxed precision floats.
    if (!mFullPrecision) {
      attributes.push_back("+neon");
    } else {
#endif  // __HOST__ || ARCH_ARM_HAVE_NEON
      attributes.push_back("-neon");
      attributes.push_back("-neonfp");
#if defined(__HOST__) || defined(ARCH_ARM_HAVE_NEON)
    }
#endif  // __HOST__ || ARCH_ARM_HAVE_NEON

    if (!getProperty("debug.rs.arm-no-hwdiv")) {
      if (features.count("hwdiv-arm") && features["hwdiv-arm"])
        attributes.push_back("+hwdiv-arm");

      if (features.count("hwdiv") && features["hwdiv"])
        attributes.push_back("+hwdiv");
    }

    // Enable fp16 attribute if available in the feature list.  This feature
    // will not be added in the host version of bcc or bcc_compat since
    // 'features' would correspond to features in an x86 host.
    if (features.count("fp16") && features["fp16"])
      attributes.push_back("+fp16");

#if defined(PROVIDE_ARM64_CODEGEN)
    // On AArch64, asimd in /proc/cpuinfo signals the presence of hardware
    // half-precision conversion instructions.  getHostCPUFeatures translates
    // this to "neon".  If PROVIDE_ARM64_CODEGEN is set, enable "+fp16" for ARM
    // codegen if "neon" is present in features.
    if (features.count("neon") && features["neon"])
      attributes.push_back("+fp16");
#endif // PROVIDE_ARM64_CODEGEN

#if defined(TARGET_BUILD)
    if (!getProperty("debug.rs.arm-no-tune-for-cpu")) {
#ifndef FORCE_CPU_VARIANT_32
#ifdef DEFAULT_ARM_CODEGEN
      setCPU(llvm::sys::getHostCPUName());
#endif
#else
#define XSTR(S) #S
#define STR(S) XSTR(S)
      setCPU(STR(FORCE_CPU_VARIANT_32));
#undef STR
#undef XSTR
#endif
    }
#endif  // TARGET_BUILD

    break;
  }
#endif  // PROVIDE_ARM_CODEGEN

#if defined(PROVIDE_ARM64_CODEGEN)
  case llvm::Triple::aarch64:
#if defined(TARGET_BUILD)
    if (!getProperty("debug.rs.arm-no-tune-for-cpu")) {
#ifndef FORCE_CPU_VARIANT_64
#ifdef DEFAULT_ARM64_CODEGEN
      setCPU(llvm::sys::getHostCPUName());
#endif
#else
#define XSTR(S) #S
#define STR(S) XSTR(S)
      setCPU(STR(FORCE_CPU_VARIANT_64));
#undef STR
#undef XSTR
#endif

    }
#endif  // TARGET_BUILD
    break;
#endif  // PROVIDE_ARM64_CODEGEN

#if defined (PROVIDE_MIPS_CODEGEN)
  case llvm::Triple::mips:
  case llvm::Triple::mipsel:
    if (getRelocationModel() == llvm::Reloc::Default) {
      setRelocationModel(llvm::Reloc::Static);
    }
    break;
#endif  // PROVIDE_MIPS_CODEGEN

#if defined (PROVIDE_MIPS64_CODEGEN)
  case llvm::Triple::mips64:
  case llvm::Triple::mips64el:
    // Default revision for MIPS64 Android is R6.
    setCPU("mips64r6");
    break;
#endif // PROVIDE_MIPS64_CODEGEN

#if defined (PROVIDE_X86_CODEGEN)
  case llvm::Triple::x86:
    getTargetOptions().UseInitArray = true;
#if defined (DEFAULT_X86_CODEGEN) && !defined (DEFAULT_X86_64_CODEGEN)
    setCPU(llvm::sys::getHostCPUName());
#else
    // generic fallback for 32bit x86 targets
    setCPU("atom");
#endif // DEFAULT_X86_CODEGEN && !DEFAULT_X86_64_CODEGEN

#ifndef __HOST__
    // If not running on the host, and f16c is available, set it in the feature
    // string
    if (HasF16C())
      attributes.push_back("+f16c");
#if defined(__SSE3__)
    attributes.push_back("+sse3");
    attributes.push_back("+ssse3");
#endif
#if defined(__SSE4_1__)
    attributes.push_back("+sse4.1");
#endif
#if defined(__SSE4_2__)
    attributes.push_back("+sse4.2");
#endif
#endif // __HOST__
    break;
#endif  // PROVIDE_X86_CODEGEN

#if defined (PROVIDE_X86_CODEGEN)
// PROVIDE_X86_CODEGEN is defined for both x86 and x86_64
  case llvm::Triple::x86_64:
#if defined(DEFAULT_X86_64_CODEGEN) && !defined(__HOST__)
    setCPU(llvm::sys::getHostCPUName());
#else
    // generic fallback for 64bit x86 targets
    setCPU("core2");
#endif
    // x86_64 needs small CodeModel if use PIC_ reloc, or else dlopen failed with TEXTREL.
    if (getRelocationModel() == llvm::Reloc::PIC_) {
      setCodeModel(llvm::CodeModel::Small);
    } else {
      setCodeModel(llvm::CodeModel::Medium);
    }
    getTargetOptions().UseInitArray = true;

#ifndef __HOST__
    // If not running on the host, and f16c is available, set it in the feature
    // string
    if (HasF16C())
      attributes.push_back("+f16c");
#endif // __HOST__

    break;
#endif  // PROVIDE_X86_CODEGEN

  default:
    ALOGE("Unsupported architecture type: %s", mTarget->getName());
    return false;
  }

  setFeatureString(attributes);
  return true;
}

void CompilerConfig::setFeatureString(const std::vector<std::string> &pAttrs) {
  llvm::SubtargetFeatures f;

  for (std::vector<std::string>::const_iterator attr_iter = pAttrs.begin(),
           attr_end = pAttrs.end();
       attr_iter != attr_end; attr_iter++) {
    f.AddFeature(*attr_iter);
  }

  mFeatureString = f.getString();
  return;
}
