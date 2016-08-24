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

#ifndef BCC_RS_SCRIPT_H
#define BCC_RS_SCRIPT_H

#include "bcc/Script.h"
#include "bcc/Support/Sha1Util.h"

namespace llvm {
  class Module;
}

namespace bcc {

class RSScript;
class Source;
class CompilerConfig;

typedef llvm::Module* (*RSLinkRuntimeCallback) (bcc::RSScript *, llvm::Module *, llvm::Module *);


class RSScript : public Script {
public:
  // This is one-one mapping with the llvm::CodeGenOpt::Level in
  // llvm/Support/CodeGen.h. Therefore, value of this type can safely cast
  // to llvm::CodeGenOpt::Level. This makes RSScript LLVM-free.
  enum OptimizationLevel {
    kOptLvl0, // -O0
    kOptLvl1, // -O1
    kOptLvl2, // -O2, -Os
    kOptLvl3  // -O3
  };

private:
  unsigned mCompilerVersion;

  OptimizationLevel mOptimizationLevel;

  RSLinkRuntimeCallback mLinkRuntimeCallback;

  bool mEmbedInfo;

  // Specifies whether we should embed global variable information in the
  // code via special RS variables that can be examined later by the driver.
  bool mEmbedGlobalInfo;

  // Specifies whether we should skip constant (immutable) global variables
  // when potentially embedding information about globals.
  bool mEmbedGlobalInfoSkipConstant;

private:
  // This will be invoked when the containing source has been reset.
  virtual bool doReset();

public:
  static bool LinkRuntime(RSScript &pScript, const char *rt_path = nullptr);

  RSScript(Source &pSource);

  // Passing in the CompilerConfig allows the optimization level to
  // be derived rather than defaulted to aggressive (-O3)
  RSScript(Source &pSource, const CompilerConfig * pCompilerConfig);

  virtual ~RSScript() { }

  void setCompilerVersion(unsigned pCompilerVersion) {
    mCompilerVersion = pCompilerVersion;
  }

  unsigned getCompilerVersion() const {
    return mCompilerVersion;
  }

  void setOptimizationLevel(OptimizationLevel pOptimizationLevel) {
    mOptimizationLevel = pOptimizationLevel;
  }

  OptimizationLevel getOptimizationLevel() const {
    return mOptimizationLevel;
  }

  void setLinkRuntimeCallback(RSLinkRuntimeCallback fn){
    mLinkRuntimeCallback = fn;
  }

  void setEmbedInfo(bool pEnable) {
    mEmbedInfo = pEnable;
  }

  bool getEmbedInfo() const {
    return mEmbedInfo;
  }

  // Set to true if we should embed global variable information in the code.
  void setEmbedGlobalInfo(bool pEnable) {
    mEmbedGlobalInfo = pEnable;
  }

  // Returns true if we should embed global variable information in the code.
  bool getEmbedGlobalInfo() const {
    return mEmbedGlobalInfo;
  }

  // Set to true if we should skip constant (immutable) global variables when
  // potentially embedding information about globals.
  void setEmbedGlobalInfoSkipConstant(bool pEnable) {
    mEmbedGlobalInfoSkipConstant = pEnable;
  }

  // Returns true if we should skip constant (immutable) global variables when
  // potentially embedding information about globals.
  bool getEmbedGlobalInfoSkipConstant() const {
    return mEmbedGlobalInfoSkipConstant;
  }
};

} // end namespace bcc

#endif // BCC_RS_SCRIPT_H
