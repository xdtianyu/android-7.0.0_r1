/*
 * Copyright (C) 2015, The Android Open Source Project
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

#include "type_java.h"

#include <sys/types.h>

#include <android-base/strings.h>

#include "aidl_language.h"
#include "logging.h"

using std::string;
using android::base::Split;
using android::base::Join;
using android::base::Trim;

namespace android {
namespace aidl {
namespace java {

Expression* NULL_VALUE;
Expression* THIS_VALUE;
Expression* SUPER_VALUE;
Expression* TRUE_VALUE;
Expression* FALSE_VALUE;

// ================================================================

Type::Type(const JavaTypeNamespace* types, const string& name, int kind,
           bool canWriteToParcel, bool canBeOut)
    : Type(types, "", name, kind, canWriteToParcel, canBeOut, "", -1) {}

Type::Type(const JavaTypeNamespace* types, const string& package,
           const string& name, int kind, bool canWriteToParcel,
           bool canBeOut, const string& declFile, int declLine)
    : ValidatableType(kind, package, name, declFile, declLine),
      m_types(types),
      m_javaType((package.empty()) ? name : package + "." + name),
      m_canWriteToParcel(canWriteToParcel),
      m_canBeOut(canBeOut) {
}

string Type::CreatorName() const { return ""; }

string Type::InstantiableName() const { return JavaType(); }

void Type::WriteToParcel(StatementBlock* addTo, Variable* v, Variable* parcel,
                         int flags) const {
  fprintf(stderr, "aidl:internal error %s:%d qualifiedName=%sn", __FILE__,
          __LINE__, m_javaType.c_str());
  addTo->Add(new LiteralExpression("/* WriteToParcel error " + m_javaType +
                                   " */"));
}

void Type::CreateFromParcel(StatementBlock* addTo, Variable* v,
                            Variable* parcel, Variable**) const {
  fprintf(stderr, "aidl:internal error %s:%d qualifiedName=%s\n", __FILE__,
          __LINE__, m_javaType.c_str());
  addTo->Add(new LiteralExpression("/* CreateFromParcel error " +
                                   m_javaType + " */"));
}

void Type::ReadFromParcel(StatementBlock* addTo, Variable* v, Variable* parcel,
                          Variable**) const {
  fprintf(stderr, "aidl:internal error %s:%d qualifiedName=%s\n", __FILE__,
          __LINE__, m_javaType.c_str());
  addTo->Add(new LiteralExpression("/* ReadFromParcel error " +
                                   m_javaType + " */"));
}

Expression* Type::BuildWriteToParcelFlags(int flags) const {
  if (flags == 0) {
    return new LiteralExpression("0");
  }
  if ((flags & PARCELABLE_WRITE_RETURN_VALUE) != 0) {
    return new FieldVariable(m_types->ParcelableInterfaceType(),
                             "PARCELABLE_WRITE_RETURN_VALUE");
  }
  return new LiteralExpression("0");
}

// ================================================================

BasicType::BasicType(const JavaTypeNamespace* types, const string& name,
                     const string& marshallParcel,
                     const string& unmarshallParcel,
                     const string& writeArrayParcel,
                     const string& createArrayParcel,
                     const string& readArrayParcel)
    : Type(types, name, ValidatableType::KIND_BUILT_IN, true, false),
      m_marshallParcel(marshallParcel),
      m_unmarshallParcel(unmarshallParcel) {
  m_array_type.reset(new BasicArrayType(types, name, writeArrayParcel,
                                        createArrayParcel, readArrayParcel));
}

void BasicType::WriteToParcel(StatementBlock* addTo, Variable* v,
                              Variable* parcel, int flags) const {
  addTo->Add(new MethodCall(parcel, m_marshallParcel, 1, v));
}

void BasicType::CreateFromParcel(StatementBlock* addTo, Variable* v,
                                 Variable* parcel, Variable**) const {
  addTo->Add(new Assignment(v, new MethodCall(parcel, m_unmarshallParcel)));
}

BasicArrayType::BasicArrayType(const JavaTypeNamespace* types,
                               const string& name,
                               const string& writeArrayParcel,
                               const string& createArrayParcel,
                               const string& readArrayParcel)
    : Type(types, name, ValidatableType::KIND_BUILT_IN, true, true),
      m_writeArrayParcel(writeArrayParcel),
      m_createArrayParcel(createArrayParcel),
      m_readArrayParcel(readArrayParcel) {}


void BasicArrayType::WriteToParcel(StatementBlock* addTo, Variable* v,
                                   Variable* parcel, int flags) const {
  addTo->Add(new MethodCall(parcel, m_writeArrayParcel, 1, v));
}

void BasicArrayType::CreateFromParcel(StatementBlock* addTo, Variable* v,
                                      Variable* parcel, Variable**) const {
  addTo->Add(new Assignment(v, new MethodCall(parcel, m_createArrayParcel)));
}

void BasicArrayType::ReadFromParcel(StatementBlock* addTo, Variable* v,
                                    Variable* parcel, Variable**) const {
  addTo->Add(new MethodCall(parcel, m_readArrayParcel, 1, v));
}

// ================================================================

FileDescriptorType::FileDescriptorType(const JavaTypeNamespace* types)
    : Type(types, "java.io", "FileDescriptor", ValidatableType::KIND_BUILT_IN,
           true, false) {
    m_array_type.reset(new FileDescriptorArrayType(types));
}

void FileDescriptorType::WriteToParcel(StatementBlock* addTo, Variable* v,
                                       Variable* parcel, int flags) const {
  addTo->Add(new MethodCall(parcel, "writeRawFileDescriptor", 1, v));
}

void FileDescriptorType::CreateFromParcel(StatementBlock* addTo, Variable* v,
                                          Variable* parcel, Variable**) const {
  addTo->Add(new Assignment(v, new MethodCall(parcel, "readRawFileDescriptor")));
}

FileDescriptorArrayType::FileDescriptorArrayType(const JavaTypeNamespace* types)
    : Type(types, "java.io", "FileDescriptor", ValidatableType::KIND_BUILT_IN,
           true, true) {}

void FileDescriptorArrayType::WriteToParcel(StatementBlock* addTo, Variable* v,
                                            Variable* parcel, int flags) const {
  addTo->Add(new MethodCall(parcel, "writeRawFileDescriptorArray", 1, v));
}

void FileDescriptorArrayType::CreateFromParcel(StatementBlock* addTo, Variable* v,
                                               Variable* parcel, Variable**) const {
  addTo->Add(new Assignment(v, new MethodCall(parcel, "createRawFileDescriptorArray")));
}

void FileDescriptorArrayType::ReadFromParcel(StatementBlock* addTo, Variable* v,
                                             Variable* parcel, Variable**) const {
  addTo->Add(new MethodCall(parcel, "readRawFileDescriptorArray", 1, v));
}

// ================================================================

BooleanType::BooleanType(const JavaTypeNamespace* types)
    : Type(types, "boolean", ValidatableType::KIND_BUILT_IN, true, false) {
    m_array_type.reset(new BooleanArrayType(types));
}

void BooleanType::WriteToParcel(StatementBlock* addTo, Variable* v,
                                Variable* parcel, int flags) const {
  addTo->Add(new MethodCall(
      parcel, "writeInt", 1,
      new Ternary(v, new LiteralExpression("1"), new LiteralExpression("0"))));
}

void BooleanType::CreateFromParcel(StatementBlock* addTo, Variable* v,
                                   Variable* parcel, Variable**) const {
  addTo->Add(
      new Assignment(v, new Comparison(new LiteralExpression("0"), "!=",
                                       new MethodCall(parcel, "readInt"))));
}

BooleanArrayType::BooleanArrayType(const JavaTypeNamespace* types)
    : Type(types, "boolean", ValidatableType::KIND_BUILT_IN, true, true) {}

void BooleanArrayType::WriteToParcel(StatementBlock* addTo, Variable* v,
                                     Variable* parcel, int flags) const {
  addTo->Add(new MethodCall(parcel, "writeBooleanArray", 1, v));
}

void BooleanArrayType::CreateFromParcel(StatementBlock* addTo, Variable* v,
                                        Variable* parcel, Variable**) const {
  addTo->Add(new Assignment(v, new MethodCall(parcel, "createBooleanArray")));
}

void BooleanArrayType::ReadFromParcel(StatementBlock* addTo, Variable* v,
                                      Variable* parcel, Variable**) const {
  addTo->Add(new MethodCall(parcel, "readBooleanArray", 1, v));
}

// ================================================================

CharType::CharType(const JavaTypeNamespace* types)
    : Type(types, "char", ValidatableType::KIND_BUILT_IN, true, false) {
    m_array_type.reset(new CharArrayType(types));
}

void CharType::WriteToParcel(StatementBlock* addTo, Variable* v,
                             Variable* parcel, int flags) const {
  addTo->Add(
      new MethodCall(parcel, "writeInt", 1, new Cast(m_types->IntType(), v)));
}

void CharType::CreateFromParcel(StatementBlock* addTo, Variable* v,
                                Variable* parcel, Variable**) const {
  addTo->Add(new Assignment(v, new MethodCall(parcel, "readInt"), this));
}

CharArrayType::CharArrayType(const JavaTypeNamespace* types)
    : Type(types, "char", ValidatableType::KIND_BUILT_IN, true, true) {}

void CharArrayType::WriteToParcel(StatementBlock* addTo, Variable* v,
                                  Variable* parcel, int flags) const {
  addTo->Add(new MethodCall(parcel, "writeCharArray", 1, v));
}

void CharArrayType::CreateFromParcel(StatementBlock* addTo, Variable* v,
                                     Variable* parcel, Variable**) const {
  addTo->Add(new Assignment(v, new MethodCall(parcel, "createCharArray")));
}

void CharArrayType::ReadFromParcel(StatementBlock* addTo, Variable* v,
                                   Variable* parcel, Variable**) const {
  addTo->Add(new MethodCall(parcel, "readCharArray", 1, v));
}

// ================================================================

StringType::StringType(const JavaTypeNamespace* types,
                       const std::string& package,
                       const std::string& class_name)
    : Type(types, package, class_name,
           ValidatableType::KIND_BUILT_IN, true, false) {
  m_array_type.reset(new StringArrayType(types));
}

string StringType::CreatorName() const {
  return "android.os.Parcel.STRING_CREATOR";
}

void StringType::WriteToParcel(StatementBlock* addTo, Variable* v,
                               Variable* parcel, int flags) const {
  addTo->Add(new MethodCall(parcel, "writeString", 1, v));
}

void StringType::CreateFromParcel(StatementBlock* addTo, Variable* v,
                                  Variable* parcel, Variable**) const {
  addTo->Add(new Assignment(v, new MethodCall(parcel, "readString")));
}

StringArrayType::StringArrayType(const JavaTypeNamespace* types)
    : Type(types, "java.lang", "String", ValidatableType::KIND_BUILT_IN,
           true, true) {}

string StringArrayType::CreatorName() const {
  return "android.os.Parcel.STRING_CREATOR";
}

void StringArrayType::WriteToParcel(StatementBlock* addTo, Variable* v,
                                    Variable* parcel, int flags) const {
  addTo->Add(new MethodCall(parcel, "writeStringArray", 1, v));
}

void StringArrayType::CreateFromParcel(StatementBlock* addTo, Variable* v,
                                       Variable* parcel, Variable**) const {
  addTo->Add(new Assignment(v, new MethodCall(parcel, "createStringArray")));
}

void StringArrayType::ReadFromParcel(StatementBlock* addTo, Variable* v,
                                     Variable* parcel, Variable**) const {
  addTo->Add(new MethodCall(parcel, "readStringArray", 1, v));
}

// ================================================================

CharSequenceType::CharSequenceType(const JavaTypeNamespace* types)
    : Type(types, "java.lang", "CharSequence", ValidatableType::KIND_BUILT_IN,
           true, false) {}

string CharSequenceType::CreatorName() const {
  return "android.os.Parcel.STRING_CREATOR";
}

void CharSequenceType::WriteToParcel(StatementBlock* addTo, Variable* v,
                                     Variable* parcel, int flags) const {
  // if (v != null) {
  //     parcel.writeInt(1);
  //     v.writeToParcel(parcel);
  // } else {
  //     parcel.writeInt(0);
  // }
  IfStatement* elsepart = new IfStatement();
  elsepart->statements->Add(
      new MethodCall(parcel, "writeInt", 1, new LiteralExpression("0")));
  IfStatement* ifpart = new IfStatement;
  ifpart->expression = new Comparison(v, "!=", NULL_VALUE);
  ifpart->elseif = elsepart;
  ifpart->statements->Add(
      new MethodCall(parcel, "writeInt", 1, new LiteralExpression("1")));
  ifpart->statements->Add(new MethodCall(m_types->TextUtilsType(),
                                         "writeToParcel", 3, v, parcel,
                                         BuildWriteToParcelFlags(flags)));

  addTo->Add(ifpart);
}

void CharSequenceType::CreateFromParcel(StatementBlock* addTo, Variable* v,
                                        Variable* parcel, Variable**) const {
  // if (0 != parcel.readInt()) {
  //     v = TextUtils.createFromParcel(parcel)
  // } else {
  //     v = null;
  // }
  IfStatement* elsepart = new IfStatement();
  elsepart->statements->Add(new Assignment(v, NULL_VALUE));

  IfStatement* ifpart = new IfStatement();
  ifpart->expression = new Comparison(new LiteralExpression("0"), "!=",
                                      new MethodCall(parcel, "readInt"));
  ifpart->elseif = elsepart;
  ifpart->statements->Add(new Assignment(
      v, new MethodCall(m_types->TextUtilsType(),
                        "CHAR_SEQUENCE_CREATOR.createFromParcel", 1, parcel)));

  addTo->Add(ifpart);
}

// ================================================================

RemoteExceptionType::RemoteExceptionType(const JavaTypeNamespace* types)
    : Type(types, "android.os", "RemoteException",
           ValidatableType::KIND_BUILT_IN, false, false) {}

void RemoteExceptionType::WriteToParcel(StatementBlock* addTo, Variable* v,
                                        Variable* parcel, int flags) const {
  fprintf(stderr, "aidl:internal error %s:%d\n", __FILE__, __LINE__);
}

void RemoteExceptionType::CreateFromParcel(StatementBlock* addTo, Variable* v,
                                           Variable* parcel, Variable**) const {
  fprintf(stderr, "aidl:internal error %s:%d\n", __FILE__, __LINE__);
}

// ================================================================

RuntimeExceptionType::RuntimeExceptionType(const JavaTypeNamespace* types)
    : Type(types, "java.lang", "RuntimeException",
           ValidatableType::KIND_BUILT_IN, false, false) {}

void RuntimeExceptionType::WriteToParcel(StatementBlock* addTo, Variable* v,
                                         Variable* parcel, int flags) const {
  fprintf(stderr, "aidl:internal error %s:%d\n", __FILE__, __LINE__);
}

void RuntimeExceptionType::CreateFromParcel(StatementBlock* addTo, Variable* v,
                                            Variable* parcel,
                                            Variable**) const {
  fprintf(stderr, "aidl:internal error %s:%d\n", __FILE__, __LINE__);
}

// ================================================================

IBinderType::IBinderType(const JavaTypeNamespace* types)
    : Type(types, "android.os", "IBinder", ValidatableType::KIND_BUILT_IN,
           true, false) {
  m_array_type.reset(new IBinderArrayType(types));
}

void IBinderType::WriteToParcel(StatementBlock* addTo, Variable* v,
                                Variable* parcel, int flags) const {
  addTo->Add(new MethodCall(parcel, "writeStrongBinder", 1, v));
}

void IBinderType::CreateFromParcel(StatementBlock* addTo, Variable* v,
                                   Variable* parcel, Variable**) const {
  addTo->Add(new Assignment(v, new MethodCall(parcel, "readStrongBinder")));
}

IBinderArrayType::IBinderArrayType(const JavaTypeNamespace* types)
    : Type(types, "android.os", "IBinder", ValidatableType::KIND_BUILT_IN,
           true, true) {}

void IBinderArrayType::WriteToParcel(StatementBlock* addTo, Variable* v,
                                     Variable* parcel, int flags) const {
  addTo->Add(new MethodCall(parcel, "writeBinderArray", 1, v));
}

void IBinderArrayType::CreateFromParcel(StatementBlock* addTo, Variable* v,
                                        Variable* parcel, Variable**) const {
  addTo->Add(new Assignment(v, new MethodCall(parcel, "createBinderArray")));
}

void IBinderArrayType::ReadFromParcel(StatementBlock* addTo, Variable* v,
                                      Variable* parcel, Variable**) const {
  addTo->Add(new MethodCall(parcel, "readBinderArray", 1, v));
}

// ================================================================

IInterfaceType::IInterfaceType(const JavaTypeNamespace* types)
    : Type(types, "android.os", "IInterface", ValidatableType::KIND_BUILT_IN,
           false, false) {}

void IInterfaceType::WriteToParcel(StatementBlock* addTo, Variable* v,
                                   Variable* parcel, int flags) const {
  fprintf(stderr, "aidl:internal error %s:%d\n", __FILE__, __LINE__);
}

void IInterfaceType::CreateFromParcel(StatementBlock* addTo, Variable* v,
                                      Variable* parcel, Variable**) const {
  fprintf(stderr, "aidl:internal error %s:%d\n", __FILE__, __LINE__);
}

// ================================================================

BinderType::BinderType(const JavaTypeNamespace* types)
    : Type(types, "android.os", "Binder", ValidatableType::KIND_BUILT_IN,
           false, false) {}

void BinderType::WriteToParcel(StatementBlock* addTo, Variable* v,
                               Variable* parcel, int flags) const {
  fprintf(stderr, "aidl:internal error %s:%d\n", __FILE__, __LINE__);
}

void BinderType::CreateFromParcel(StatementBlock* addTo, Variable* v,
                                  Variable* parcel, Variable**) const {
  fprintf(stderr, "aidl:internal error %s:%d\n", __FILE__, __LINE__);
}

// ================================================================

BinderProxyType::BinderProxyType(const JavaTypeNamespace* types)
    : Type(types, "android.os", "BinderProxy", ValidatableType::KIND_BUILT_IN,
           false, false) {}

void BinderProxyType::WriteToParcel(StatementBlock* addTo, Variable* v,
                                    Variable* parcel, int flags) const {
  fprintf(stderr, "aidl:internal error %s:%d\n", __FILE__, __LINE__);
}

void BinderProxyType::CreateFromParcel(StatementBlock* addTo, Variable* v,
                                       Variable* parcel, Variable**) const {
  fprintf(stderr, "aidl:internal error %s:%d\n", __FILE__, __LINE__);
}

// ================================================================

ParcelType::ParcelType(const JavaTypeNamespace* types)
    : Type(types, "android.os", "Parcel", ValidatableType::KIND_BUILT_IN,
           false, false) {}

void ParcelType::WriteToParcel(StatementBlock* addTo, Variable* v,
                               Variable* parcel, int flags) const {
  fprintf(stderr, "aidl:internal error %s:%d\n", __FILE__, __LINE__);
}

void ParcelType::CreateFromParcel(StatementBlock* addTo, Variable* v,
                                  Variable* parcel, Variable**) const {
  fprintf(stderr, "aidl:internal error %s:%d\n", __FILE__, __LINE__);
}

// ================================================================

ParcelableInterfaceType::ParcelableInterfaceType(const JavaTypeNamespace* types)
    : Type(types, "android.os", "Parcelable", ValidatableType::KIND_BUILT_IN,
           false, false) {}

void ParcelableInterfaceType::WriteToParcel(StatementBlock* addTo, Variable* v,
                                            Variable* parcel, int flags) const {
  fprintf(stderr, "aidl:internal error %s:%d\n", __FILE__, __LINE__);
}

void ParcelableInterfaceType::CreateFromParcel(StatementBlock* addTo,
                                               Variable* v, Variable* parcel,
                                               Variable**) const {
  fprintf(stderr, "aidl:internal error %s:%d\n", __FILE__, __LINE__);
}

// ================================================================

MapType::MapType(const JavaTypeNamespace* types)
    : Type(types, "java.util", "Map", ValidatableType::KIND_BUILT_IN,
           true, true) {}

void MapType::WriteToParcel(StatementBlock* addTo, Variable* v,
                            Variable* parcel, int flags) const {
  addTo->Add(new MethodCall(parcel, "writeMap", 1, v));
}

static void EnsureClassLoader(StatementBlock* addTo, Variable** cl,
                              const JavaTypeNamespace* types) {
  // We don't want to look up the class loader once for every
  // collection argument, so ensure we do it at most once per method.
  if (*cl == NULL) {
    *cl = new Variable(types->ClassLoaderType(), "cl");
    addTo->Add(new VariableDeclaration(
        *cl, new LiteralExpression("this.getClass().getClassLoader()"),
        types->ClassLoaderType()));
  }
}

void MapType::CreateFromParcel(StatementBlock* addTo, Variable* v,
                               Variable* parcel, Variable** cl) const {
  EnsureClassLoader(addTo, cl, m_types);
  addTo->Add(new Assignment(v, new MethodCall(parcel, "readHashMap", 1, *cl)));
}

void MapType::ReadFromParcel(StatementBlock* addTo, Variable* v,
                             Variable* parcel, Variable** cl) const {
  EnsureClassLoader(addTo, cl, m_types);
  addTo->Add(new MethodCall(parcel, "readMap", 2, v, *cl));
}

// ================================================================

ListType::ListType(const JavaTypeNamespace* types)
    : Type(types, "java.util", "List", ValidatableType::KIND_BUILT_IN,
           true, true) {}

string ListType::InstantiableName() const { return "java.util.ArrayList"; }

void ListType::WriteToParcel(StatementBlock* addTo, Variable* v,
                             Variable* parcel, int flags) const {
  addTo->Add(new MethodCall(parcel, "writeList", 1, v));
}

void ListType::CreateFromParcel(StatementBlock* addTo, Variable* v,
                                Variable* parcel, Variable** cl) const {
  EnsureClassLoader(addTo, cl, m_types);
  addTo->Add(
      new Assignment(v, new MethodCall(parcel, "readArrayList", 1, *cl)));
}

void ListType::ReadFromParcel(StatementBlock* addTo, Variable* v,
                              Variable* parcel, Variable** cl) const {
  EnsureClassLoader(addTo, cl, m_types);
  addTo->Add(new MethodCall(parcel, "readList", 2, v, *cl));
}

// ================================================================

UserDataType::UserDataType(const JavaTypeNamespace* types,
                           const string& package, const string& name,
                           bool builtIn, bool canWriteToParcel,
                           const string& declFile, int declLine)
    : Type(types, package, name,
           builtIn ? ValidatableType::KIND_BUILT_IN
                   : ValidatableType::KIND_PARCELABLE,
           canWriteToParcel, true, declFile, declLine) {
  m_array_type.reset(new UserDataArrayType(types, package, name, builtIn,
                                           canWriteToParcel, declFile,
                                           declLine));
}

string UserDataType::CreatorName() const {
  return JavaType() + ".CREATOR";
}

void UserDataType::WriteToParcel(StatementBlock* addTo, Variable* v,
                                 Variable* parcel, int flags) const {
  // if (v != null) {
  //     parcel.writeInt(1);
  //     v.writeToParcel(parcel);
  // } else {
  //     parcel.writeInt(0);
  // }
  IfStatement* elsepart = new IfStatement();
  elsepart->statements->Add(
      new MethodCall(parcel, "writeInt", 1, new LiteralExpression("0")));
  IfStatement* ifpart = new IfStatement;
  ifpart->expression = new Comparison(v, "!=", NULL_VALUE);
  ifpart->elseif = elsepart;
  ifpart->statements->Add(
      new MethodCall(parcel, "writeInt", 1, new LiteralExpression("1")));
  ifpart->statements->Add(new MethodCall(v, "writeToParcel", 2, parcel,
                                         BuildWriteToParcelFlags(flags)));

  addTo->Add(ifpart);
}

void UserDataType::CreateFromParcel(StatementBlock* addTo, Variable* v,
                                    Variable* parcel, Variable**) const {
  // if (0 != parcel.readInt()) {
  //     v = CLASS.CREATOR.createFromParcel(parcel)
  // } else {
  //     v = null;
  // }
  IfStatement* elsepart = new IfStatement();
  elsepart->statements->Add(new Assignment(v, NULL_VALUE));

  IfStatement* ifpart = new IfStatement();
  ifpart->expression = new Comparison(new LiteralExpression("0"), "!=",
                                      new MethodCall(parcel, "readInt"));
  ifpart->elseif = elsepart;
  ifpart->statements->Add(new Assignment(
      v, new MethodCall(v->type, "CREATOR.createFromParcel", 1, parcel)));

  addTo->Add(ifpart);
}

void UserDataType::ReadFromParcel(StatementBlock* addTo, Variable* v,
                                  Variable* parcel, Variable**) const {
  // TODO: really, we don't need to have this extra check, but we
  // don't have two separate marshalling code paths
  // if (0 != parcel.readInt()) {
  //     v.readFromParcel(parcel)
  // }
  IfStatement* ifpart = new IfStatement();
  ifpart->expression = new Comparison(new LiteralExpression("0"), "!=",
                                      new MethodCall(parcel, "readInt"));
  ifpart->statements->Add(new MethodCall(v, "readFromParcel", 1, parcel));
  addTo->Add(ifpart);
}

UserDataArrayType::UserDataArrayType(const JavaTypeNamespace* types,
                           const string& package, const string& name,
                           bool builtIn, bool canWriteToParcel,
                           const string& declFile, int declLine)
    : Type(types, package, name,
           builtIn ? ValidatableType::KIND_BUILT_IN
                   : ValidatableType::KIND_PARCELABLE,
           canWriteToParcel, true, declFile, declLine) {}

string UserDataArrayType::CreatorName() const {
  return JavaType() + ".CREATOR";
}

void UserDataArrayType::WriteToParcel(StatementBlock* addTo, Variable* v,
                                      Variable* parcel, int flags) const {
  addTo->Add(new MethodCall(parcel, "writeTypedArray", 2, v,
                            BuildWriteToParcelFlags(flags)));
}

void UserDataArrayType::CreateFromParcel(StatementBlock* addTo, Variable* v,
                                         Variable* parcel, Variable**) const {
  string creator = v->type->JavaType() + ".CREATOR";
  addTo->Add(new Assignment(v, new MethodCall(parcel, "createTypedArray", 1,
                                              new LiteralExpression(creator))));
}

void UserDataArrayType::ReadFromParcel(StatementBlock* addTo, Variable* v,
                                       Variable* parcel, Variable**) const {
  string creator = v->type->JavaType() + ".CREATOR";
  addTo->Add(new MethodCall(parcel, "readTypedArray", 2, v,
                            new LiteralExpression(creator)));
}

// ================================================================

InterfaceType::InterfaceType(const JavaTypeNamespace* types,
                             const string& package, const string& name,
                             bool builtIn, bool oneway, const string& declFile,
                             int declLine, const Type* stub, const Type* proxy)
    : Type(types, package, name, builtIn ? ValidatableType::KIND_BUILT_IN
                                         : ValidatableType::KIND_INTERFACE,
           true, false, declFile, declLine),
      m_oneway(oneway),
      stub_(stub),
      proxy_(proxy) {}

bool InterfaceType::OneWay() const { return m_oneway; }

void InterfaceType::WriteToParcel(StatementBlock* addTo, Variable* v,
                                  Variable* parcel, int flags) const {
  // parcel.writeStrongBinder(v != null ? v.asBinder() : null);
  addTo->Add(
      new MethodCall(parcel, "writeStrongBinder", 1,
                     new Ternary(new Comparison(v, "!=", NULL_VALUE),
                                 new MethodCall(v, "asBinder"), NULL_VALUE)));
}

void InterfaceType::CreateFromParcel(StatementBlock* addTo, Variable* v,
                                     Variable* parcel, Variable**) const {
  // v = Interface.asInterface(parcel.readStrongBinder());
  addTo->Add(new Assignment(
      v, new MethodCall(stub_, "asInterface", 1,
                        new MethodCall(parcel, "readStrongBinder"))));
}

// ================================================================

GenericListType::GenericListType(const JavaTypeNamespace* types,
                                 const Type* contained_type)
    : Type(types, "java.util", "List<" + contained_type->CanonicalName() + ">",
           ValidatableType::KIND_BUILT_IN, true, true),
      m_contained_type(contained_type),
      m_creator(contained_type->CreatorName()) {}

string GenericListType::CreatorName() const {
  return "android.os.Parcel.arrayListCreator";
}

string GenericListType::InstantiableName() const {
  return "java.util.ArrayList<" + m_contained_type->JavaType() + ">";
}

void GenericListType::WriteToParcel(StatementBlock* addTo, Variable* v,
                                    Variable* parcel, int flags) const {
  if (m_creator == m_types->StringType()->CreatorName()) {
    addTo->Add(new MethodCall(parcel, "writeStringList", 1, v));
  } else if (m_creator == m_types->IBinderType()->CreatorName()) {
    addTo->Add(new MethodCall(parcel, "writeBinderList", 1, v));
  } else {
    // parcel.writeTypedListXX(arg);
    addTo->Add(new MethodCall(parcel, "writeTypedList", 1, v));
  }
}

void GenericListType::CreateFromParcel(StatementBlock* addTo, Variable* v,
                                       Variable* parcel, Variable**) const {
  if (m_creator == m_types->StringType()->CreatorName()) {
    addTo->Add(
        new Assignment(v, new MethodCall(parcel, "createStringArrayList", 0)));
  } else if (m_creator == m_types->IBinderType()->CreatorName()) {
    addTo->Add(
        new Assignment(v, new MethodCall(parcel, "createBinderArrayList", 0)));
  } else {
    // v = _data.readTypedArrayList(XXX.creator);
    addTo->Add(
        new Assignment(v, new MethodCall(parcel, "createTypedArrayList", 1,
                                         new LiteralExpression(m_creator))));
  }
}

void GenericListType::ReadFromParcel(StatementBlock* addTo, Variable* v,
                                     Variable* parcel, Variable**) const {
  if (m_creator == m_types->StringType()->CreatorName()) {
    addTo->Add(new MethodCall(parcel, "readStringList", 1, v));
  } else if (m_creator == m_types->IBinderType()->CreatorName()) {
    addTo->Add(new MethodCall(parcel, "readBinderList", 1, v));
  } else {
    // v = _data.readTypedList(v, XXX.creator);
    addTo->Add(new MethodCall(parcel, "readTypedList", 2, v,
                              new LiteralExpression(m_creator)));
  }
}

// ================================================================

ClassLoaderType::ClassLoaderType(const JavaTypeNamespace* types)
    : Type(types, "java.lang", "ClassLoader", ValidatableType::KIND_BUILT_IN,
           false, false) {}

// ================================================================

void JavaTypeNamespace::Init() {
  Add(new BasicType(this, "void", "XXX", "XXX", "XXX", "XXX", "XXX"));

  m_bool_type = new BooleanType(this);
  Add(m_bool_type);

  Add(new BasicType(this, "byte", "writeByte", "readByte", "writeByteArray",
                    "createByteArray", "readByteArray"));

  Add(new CharType(this));

  m_int_type = new BasicType(this, "int", "writeInt", "readInt",
                             "writeIntArray", "createIntArray", "readIntArray");
  Add(m_int_type);

  Add(new BasicType(this, "long", "writeLong", "readLong", "writeLongArray",
                    "createLongArray", "readLongArray"));

  Add(new BasicType(this, "float", "writeFloat", "readFloat", "writeFloatArray",
                    "createFloatArray", "readFloatArray"));

  Add(new BasicType(this, "double", "writeDouble", "readDouble",
                    "writeDoubleArray", "createDoubleArray",
                    "readDoubleArray"));

  m_string_type = new class StringType(this, "java.lang", "String");
  Add(m_string_type);
  Add(new class StringType(this, ::android::aidl::kAidlReservedTypePackage,
                           ::android::aidl::kUtf8InCppStringClass));

  Add(new Type(this, "java.lang", "Object", ValidatableType::KIND_BUILT_IN,
               false, false));

  Add(new FileDescriptorType(this));

  Add(new CharSequenceType(this));

  Add(new MapType(this));

  Add(new ListType(this));

  m_text_utils_type =
      new Type(this, "android.text", "TextUtils",
               ValidatableType::KIND_BUILT_IN, false, false);
  Add(m_text_utils_type);

  m_remote_exception_type = new class RemoteExceptionType(this);
  Add(m_remote_exception_type);

  m_runtime_exception_type = new class RuntimeExceptionType(this);
  Add(m_runtime_exception_type);

  m_ibinder_type = new class IBinderType(this);
  Add(m_ibinder_type);

  m_iinterface_type = new class IInterfaceType(this);
  Add(m_iinterface_type);

  m_binder_native_type = new class BinderType(this);
  Add(m_binder_native_type);

  m_binder_proxy_type = new class BinderProxyType(this);
  Add(m_binder_proxy_type);

  m_parcel_type = new class ParcelType(this);
  Add(m_parcel_type);

  m_parcelable_interface_type = new class ParcelableInterfaceType(this);
  Add(m_parcelable_interface_type);

  m_context_type = new class Type(this, "android.content", "Context",
                                  ValidatableType::KIND_BUILT_IN, false, false);
  Add(m_context_type);

  m_classloader_type = new class ClassLoaderType(this);
  Add(m_classloader_type);

  NULL_VALUE = new LiteralExpression("null");
  THIS_VALUE = new LiteralExpression("this");
  SUPER_VALUE = new LiteralExpression("super");
  TRUE_VALUE = new LiteralExpression("true");
  FALSE_VALUE = new LiteralExpression("false");
}

bool JavaTypeNamespace::AddParcelableType(const AidlParcelable& p,
                                          const std::string& filename) {
  Type* type =
      new UserDataType(this, p.GetPackage(), p.GetName(), false,
                       true, filename, p.GetLine());
  return Add(type);
}

bool JavaTypeNamespace::AddBinderType(const AidlInterface& b,
                                      const std::string& filename) {
  // for interfaces, add the stub, proxy, and interface types.
  Type* stub = new Type(this, b.GetPackage(),
                        b.GetName() + ".Stub", ValidatableType::KIND_GENERATED,
                        false, false, filename, b.GetLine());
  Type* proxy = new Type(this, b.GetPackage(),
                         b.GetName() + ".Stub.Proxy",
                         ValidatableType::KIND_GENERATED,
                         false, false, filename, b.GetLine());
  Type* type =
      new InterfaceType(this, b.GetPackage(), b.GetName(), false,
                        b.IsOneway(), filename, b.GetLine(), stub, proxy);

  bool success = true;
  success &= Add(type);
  success &= Add(stub);
  success &= Add(proxy);
  return success;
}

bool JavaTypeNamespace::AddListType(const std::string& contained_type_name) {
  const Type* contained_type = FindTypeByCanonicalName(contained_type_name);
  if (!contained_type) {
    return false;
  }
  Add(new GenericListType(this, contained_type));
  return true;
}

bool JavaTypeNamespace::AddMapType(const string& key_type_name,
                                   const string& value_type_name) {
  LOG(ERROR) << "Don't know how to create a Map<K,V> container.";
  return false;
}

}  // namespace java
}  // namespace aidl
}  // namespace android
