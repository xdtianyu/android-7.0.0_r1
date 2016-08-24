/*
 * Copyright 2010-2012, The Android Open Source Project
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

#ifndef _FRAMEWORKS_COMPILE_SLANG_SLANG_RS_REFLECTION_H_ // NOLINT
#define _FRAMEWORKS_COMPILE_SLANG_SLANG_RS_REFLECTION_H_

#include <fstream>
#include <iostream>
#include <map>
#include <set>
#include <string>
#include <vector>

#include "llvm/ADT/StringExtras.h"

#include "slang_assert.h"
#include "slang_rs_export_type.h"
#include "slang_rs_reflect_utils.h"

namespace slang {

class RSContext;
class RSExportVar;
class RSExportFunc;
class RSExportForEach;

class RSReflectionJava {
private:
  const RSContext *mRSContext;

  // The name of the Java package name we're creating this file for,
  // e.g. com.example.android.rs.flashlight
  std::string mPackageName;
  // The name of the Java Renderscript package we'll be using,
  // e.g. android.renderscript
  // e.g. android.support.v8.renderscript
  std::string mRSPackageName;

  // The directory under which we'll create the Java files, in appropriate subdirectories,
  // e.g. /tmp/myout
  std::string mOutputBaseDirectory;
  // The output directory for the specfied package (mPackageName),
  // e.g. /tmp/myout/com/example/android/rs/flashlight/
  // TODO This includes the terminating separator.  Needed?
  std::string mOutputDirectory;

  // The full path of the .rs file that we are reflecting.
  std::string mRSSourceFileName;
  // The full path where the generated bit code can be read.
  std::string mBitCodeFileName;

  // The name of the resource we pass to the RenderScript constructor
  // e.g. flashlight
  std::string mResourceId;
  // The name of the Java class we are generating for this script.
  // e.g. ScriptC_flashlight
  std::string mScriptClassName;


  // This is set by startClass() and will change for the multiple classes generated.
  std::string mClassName;

  // This is the token used for determining the size of a given ScriptField.Item.
  std::string mItemSizeof;

  bool mEmbedBitcodeInJava;

  int mNextExportVarSlot;
  int mNextExportFuncSlot;
  int mNextExportForEachSlot;
  int mNextExportReduceSlot;

  GeneratedFile mOut;

  std::string mLastError;
  std::vector<std::string> *mGeneratedFileNames;

  // A mapping from a field in a record type to its index in the rsType
  // instance. Only used when generates TypeClass (ScriptField_*).
  typedef std::map<const RSExportRecordType::Field *, unsigned> FieldIndexMapTy;
  FieldIndexMapTy mFieldIndexMap;
  // Field index of current processing TypeClass.
  unsigned mFieldIndex;

  inline void setError(const std::string &Error) { mLastError = Error; }

  inline void clear() {
    mClassName = "";
    mNextExportVarSlot = 0;
    mNextExportFuncSlot = 0;
    mNextExportForEachSlot = 0;
    mNextExportReduceSlot = 0;
  }

public:
  typedef enum {
    AM_Public,
    AM_Protected,
    AM_Private,
    AM_PublicSynchronized
  } AccessModifier;

  // Generated RS Elements for type-checking code.
  std::set<std::string> mTypesToCheck;

  // Generated FieldPackers for unsigned setters/validation.
  std::set<std::string> mFieldPackerTypes;

  bool addTypeNameForElement(const std::string &TypeName);
  bool addTypeNameForFieldPacker(const std::string &TypeName);

  static const char *AccessModifierStr(AccessModifier AM);

  inline bool getEmbedBitcodeInJava() const { return mEmbedBitcodeInJava; }

  inline int getNextExportVarSlot() { return mNextExportVarSlot++; }
  inline int getNextExportFuncSlot() { return mNextExportFuncSlot++; }
  inline int getNextExportForEachSlot() { return mNextExportForEachSlot++; }
  inline int getNextExportReduceSlot() { return mNextExportReduceSlot++; }

  bool startClass(AccessModifier AM, bool IsStatic,
                  const std::string &ClassName, const char *SuperClassName,
                  std::string &ErrorMsg);
  void endClass();

  void startFunction(AccessModifier AM, bool IsStatic, const char *ReturnType,
                     const std::string &FunctionName, int Argc, ...);

  typedef std::vector<std::pair<std::string, std::string>> ArgTy;
  void startFunction(AccessModifier AM, bool IsStatic, const char *ReturnType,
                     const std::string &FunctionName, const ArgTy &Args);
  void endFunction();

  inline const std::string &getPackageName() const { return mPackageName; }
  inline const std::string &getRSPackageName() const { return mRSPackageName; }
  inline const std::string &getClassName() const { return mClassName; }
  inline const std::string &getResourceId() const { return mResourceId; }

  void startTypeClass(const std::string &ClassName);
  void endTypeClass();

  inline void incFieldIndex() { mFieldIndex++; }

  inline void resetFieldIndex() { mFieldIndex = 0; }

  inline void addFieldIndexMapping(const RSExportRecordType::Field *F) {
    slangAssert((mFieldIndexMap.find(F) == mFieldIndexMap.end()) &&
                "Nested structure never occurs in C language.");
    mFieldIndexMap.insert(std::make_pair(F, mFieldIndex));
  }

  inline unsigned getFieldIndex(const RSExportRecordType::Field *F) const {
    FieldIndexMapTy::const_iterator I = mFieldIndexMap.find(F);
    slangAssert((I != mFieldIndexMap.end()) &&
                "Requesting field is out of scope.");
    return I->second;
  }

  inline void clearFieldIndexMap() { mFieldIndexMap.clear(); }

private:
  static bool exportableReduce(const RSExportType *ResultType);

  bool genScriptClass(const std::string &ClassName, std::string &ErrorMsg);
  void genScriptClassConstructor();

  void genInitBoolExportVariable(const std::string &VarName,
                                 const clang::APValue &Val);
  void genInitPrimitiveExportVariable(const std::string &VarName,
                                      const clang::APValue &Val);
  void genInitExportVariable(const RSExportType *ET, const std::string &VarName,
                             const clang::APValue &Val);
  void genInitValue(const clang::APValue &Val, bool asBool);
  void genExportVariable(const RSExportVar *EV);
  void genPrimitiveTypeExportVariable(const RSExportVar *EV);
  void genPointerTypeExportVariable(const RSExportVar *EV);
  void genVectorTypeExportVariable(const RSExportVar *EV);
  void genMatrixTypeExportVariable(const RSExportVar *EV);
  void genConstantArrayTypeExportVariable(const RSExportVar *EV);
  void genRecordTypeExportVariable(const RSExportVar *EV);
  void genPrivateExportVariable(const std::string &TypeName,
                                const std::string &VarName);
  void genSetExportVariable(const std::string &TypeName, const RSExportVar *EV, unsigned Dimension);
  void genGetExportVariable(const std::string &TypeName,
                            const std::string &VarName);
  void genGetFieldID(const std::string &VarName);

  void genExportFunction(const RSExportFunc *EF);

  void genExportForEach(const RSExportForEach *EF);

  void genExportReduce(const RSExportReduce *ER);
  void genExportReduceAllocationVariant(const RSExportReduce *ER);
  void genExportReduceArrayVariant(const RSExportReduce *ER);
  void genExportReduceResultType(const RSExportType *ResultType);

  void genTypeCheck(const RSExportType *ET, const char *VarName);

  void genTypeInstanceFromPointer(const RSExportType *ET);

  void genTypeInstance(const RSExportType *ET);

  void genFieldPackerInstance(const RSExportType *ET);

  bool genTypeClass(const RSExportRecordType *ERT, std::string &ErrorMsg);
  void genTypeItemClass(const RSExportRecordType *ERT);
  void genTypeClassConstructor(const RSExportRecordType *ERT);
  void genTypeClassCopyToArray(const RSExportRecordType *ERT);
  void genTypeClassCopyToArrayLocal(const RSExportRecordType *ERT);
  void genTypeClassItemSetter(const RSExportRecordType *ERT);
  void genTypeClassItemGetter(const RSExportRecordType *ERT);
  void genTypeClassComponentSetter(const RSExportRecordType *ERT);
  void genTypeClassComponentGetter(const RSExportRecordType *ERT);
  void genTypeClassCopyAll(const RSExportRecordType *ERT);
  void genTypeClassResize();

  void genBuildElement(const char *ElementBuilderName,
                       const RSExportRecordType *ERT,
                       const char *RenderScriptVar, bool IsInline);
  void genAddElementToElementBuilder(const RSExportType *ERT,
                                     const std::string &VarName,
                                     const char *ElementBuilderName,
                                     const char *RenderScriptVar,
                                     unsigned ArraySize);

  bool genCreateFieldPacker(const RSExportType *T, const char *FieldPackerName);
  void genPackVarOfType(const RSExportType *T, const char *VarName,
                        const char *FieldPackerName);
  void genAllocateVarOfType(const RSExportType *T, const std::string &VarName);
  void genNewItemBufferIfNull(const char *Index);
  void genNewItemBufferPackerIfNull();

  void genPairwiseDimCheck(std::string name0, std::string name1);
  void genVectorLengthCompatibilityCheck(const std::string &ArrayName, unsigned VecSize);
  void genNullArrayCheck(const std::string &ArrayName);

public:
  RSReflectionJava(const RSContext *Context,
                   std::vector<std::string> *GeneratedFileNames,
                   const std::string &OutputBaseDirectory,
                   const std::string &RSSourceFilename,
                   const std::string &BitCodeFileName,
                   bool EmbedBitcodeInJava);

  bool reflect();

  inline const char *getLastError() const {
    if (mLastError.empty())
      return nullptr;
    else
      return mLastError.c_str();
  }
}; // class RSReflectionJava

} // namespace slang

#endif // _FRAMEWORKS_COMPILE_SLANG_SLANG_RS_REFLECTION_H_  NOLINT
