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

#include "bcc/Renderscript/RSScript.h"

#include "bcc/Assert.h"
#include "bcc/Source.h"
#include "bcc/Support/Log.h"
#include "bcc/Support/CompilerConfig.h"

using namespace bcc;

bool RSScript::LinkRuntime(RSScript &pScript, const char *core_lib) {
  bccAssert(core_lib != nullptr);

  // Using the same context with the source in pScript.
  BCCContext &context = pScript.getSource().getContext();

  Source *libclcore_source = Source::CreateFromFile(context, core_lib);
  if (libclcore_source == nullptr) {
    ALOGE("Failed to load Renderscript library '%s' to link!", core_lib);
    return false;
  }

  if (pScript.mLinkRuntimeCallback != nullptr) {
    pScript.mLinkRuntimeCallback(&pScript,
        &pScript.getSource().getModule(), &libclcore_source->getModule());
  }

  if (!pScript.getSource().merge(*libclcore_source)) {
    ALOGE("Failed to link Renderscript library '%s'!", core_lib);
    delete libclcore_source;
    return false;
  }

  return true;
}

RSScript::RSScript(Source &pSource)
  : Script(pSource), mCompilerVersion(0),
    mOptimizationLevel(kOptLvl3), mLinkRuntimeCallback(nullptr),
    mEmbedInfo(false), mEmbedGlobalInfo(false),
    mEmbedGlobalInfoSkipConstant(false) { }

RSScript::RSScript(Source &pSource, const CompilerConfig * pCompilerConfig): RSScript(pSource)
{
  switch (pCompilerConfig->getOptimizationLevel()) {
    case llvm::CodeGenOpt::None:    mOptimizationLevel = kOptLvl0; break;
    case llvm::CodeGenOpt::Less:    mOptimizationLevel = kOptLvl1; break;
    case llvm::CodeGenOpt::Default: mOptimizationLevel = kOptLvl2; break;
    case llvm::CodeGenOpt::Aggressive: //Intentional fallthrough
    default: {
      mOptimizationLevel = kOptLvl3;
      break;
    }
  }
}

bool RSScript::doReset() {
  mCompilerVersion = 0;
  mOptimizationLevel = kOptLvl3;
  return true;
}
