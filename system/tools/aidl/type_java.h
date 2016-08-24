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

#ifndef AIDL_TYPE_JAVA_H_
#define AIDL_TYPE_JAVA_H_

#include <string>
#include <vector>

#include "ast_java.h"
#include "type_namespace.h"

namespace android {
namespace aidl {
namespace java {

class JavaTypeNamespace;

class Type : public ValidatableType {
 public:
  // WriteToParcel flags
  enum { PARCELABLE_WRITE_RETURN_VALUE = 0x0001 };

  Type(const JavaTypeNamespace* types, const std::string& name, int kind,
       bool canWriteToParcel, bool canBeOut);
  Type(const JavaTypeNamespace* types, const std::string& package,
       const std::string& name, int kind, bool canWriteToParcel, bool canBeOut,
       const std::string& declFile = "", int declLine = -1);
  virtual ~Type() = default;

  bool CanBeOutParameter() const override { return m_canBeOut; }
  bool CanWriteToParcel() const override { return m_canWriteToParcel; }

  const ValidatableType* ArrayType() const override { return m_array_type.get(); }
  const ValidatableType* NullableType() const override { return nullptr; }

  virtual std::string JavaType() const { return m_javaType; }
  virtual std::string CreatorName() const;
  virtual std::string InstantiableName() const;

  virtual void WriteToParcel(StatementBlock* addTo, Variable* v,
                             Variable* parcel, int flags) const;
  virtual void CreateFromParcel(StatementBlock* addTo, Variable* v,
                                Variable* parcel, Variable** cl) const;
  virtual void ReadFromParcel(StatementBlock* addTo, Variable* v,
                              Variable* parcel, Variable** cl) const;

 protected:
  Expression* BuildWriteToParcelFlags(int flags) const;

  const JavaTypeNamespace* m_types;

  std::unique_ptr<Type> m_array_type;

 private:
  Type();
  Type(const Type&);

  std::string m_javaType;
  std::string m_declFile;
  bool m_canWriteToParcel;
  bool m_canBeOut;
};

class BasicArrayType : public Type {
 public:
  BasicArrayType(const JavaTypeNamespace* types, const std::string& name,
                 const std::string& writeArrayParcel,
                 const std::string& createArrayParcel,
                 const std::string& readArrayParcel);

  void WriteToParcel(StatementBlock* addTo, Variable* v, Variable* parcel,
                     int flags) const override;
  void CreateFromParcel(StatementBlock* addTo, Variable* v, Variable* parcel,
                        Variable** cl) const override;
  void ReadFromParcel(StatementBlock* addTo, Variable* v, Variable* parcel,
                      Variable** cl) const override;
  const ValidatableType* NullableType() const override { return this; }

 private:
  std::string m_writeArrayParcel;
  std::string m_createArrayParcel;
  std::string m_readArrayParcel;
};

class BasicType : public Type {
 public:
  BasicType(const JavaTypeNamespace* types, const std::string& name,
            const std::string& marshallParcel,
            const std::string& unmarshallParcel,
            const std::string& writeArrayParcel,
            const std::string& createArrayParcel,
            const std::string& readArrayParcel);

  void WriteToParcel(StatementBlock* addTo, Variable* v, Variable* parcel,
                     int flags) const override;
  void CreateFromParcel(StatementBlock* addTo, Variable* v, Variable* parcel,
                        Variable** cl) const override;

 private:
  std::string m_marshallParcel;
  std::string m_unmarshallParcel;
};

class FileDescriptorArrayType : public Type {
 public:
  FileDescriptorArrayType(const JavaTypeNamespace* types);

  void WriteToParcel(StatementBlock* addTo, Variable* v, Variable* parcel,
                     int flags) const override;
  void CreateFromParcel(StatementBlock* addTo, Variable* v,
                        Variable* parcel, Variable** cl) const override;
  void ReadFromParcel(StatementBlock* addTo, Variable* v, Variable* parcel,
                      Variable** cl) const override;
  const ValidatableType* NullableType() const override { return this; }
};

class FileDescriptorType : public Type {
 public:
  FileDescriptorType(const JavaTypeNamespace* types);

  void WriteToParcel(StatementBlock* addTo, Variable* v, Variable* parcel,
                     int flags) const override;
  void CreateFromParcel(StatementBlock* addTo, Variable* v, Variable* parcel,
                        Variable** cl) const override;
};

class BooleanArrayType : public Type {
 public:
  BooleanArrayType(const JavaTypeNamespace* types);

  void WriteToParcel(StatementBlock* addTo, Variable* v, Variable* parcel,
                     int flags) const override;
  void CreateFromParcel(StatementBlock* addTo, Variable* v,
                        Variable* parcel, Variable** cl) const override;
  void ReadFromParcel(StatementBlock* addTo, Variable* v, Variable* parcel,
                      Variable** cl) const override;
  const ValidatableType* NullableType() const override { return this; }
};

class BooleanType : public Type {
 public:
  BooleanType(const JavaTypeNamespace* types);

  void WriteToParcel(StatementBlock* addTo, Variable* v, Variable* parcel,
                     int flags) const override;
  void CreateFromParcel(StatementBlock* addTo, Variable* v, Variable* parcel,
                        Variable** cl) const override;
};

class CharArrayType : public Type {
 public:
  CharArrayType(const JavaTypeNamespace* types);

  void WriteToParcel(StatementBlock* addTo, Variable* v, Variable* parcel,
                     int flags) const override;
  void CreateFromParcel(StatementBlock* addTo, Variable* v,
                        Variable* parcel, Variable** cl) const override;
  void ReadFromParcel(StatementBlock* addTo, Variable* v, Variable* parcel,
                      Variable** cl) const override;
  const ValidatableType* NullableType() const override { return this; }
};

class CharType : public Type {
 public:
  CharType(const JavaTypeNamespace* types);

  void WriteToParcel(StatementBlock* addTo, Variable* v, Variable* parcel,
                     int flags) const override;
  void CreateFromParcel(StatementBlock* addTo, Variable* v, Variable* parcel,
                        Variable** cl) const override;
};

class StringArrayType : public Type {
 public:
  StringArrayType(const JavaTypeNamespace* types);

  std::string CreatorName() const override;

  void WriteToParcel(StatementBlock* addTo, Variable* v, Variable* parcel,
                     int flags) const override;
  void CreateFromParcel(StatementBlock* addTo, Variable* v,
                        Variable* parcel, Variable** cl) const override;
  void ReadFromParcel(StatementBlock* addTo, Variable* v, Variable* parcel,
                      Variable** cl) const override;
  const ValidatableType* NullableType() const override { return this; }
};

class StringType : public Type {
 public:
  StringType(const JavaTypeNamespace* types, const std::string& package,
             const std::string& class_name);

  std::string JavaType() const override { return "java.lang.String"; }
  std::string CreatorName() const override;

  void WriteToParcel(StatementBlock* addTo, Variable* v, Variable* parcel,
                     int flags) const override;
  void CreateFromParcel(StatementBlock* addTo, Variable* v, Variable* parcel,
                        Variable** cl) const override;
  const ValidatableType* NullableType() const override { return this; }
};

class CharSequenceType : public Type {
 public:
  CharSequenceType(const JavaTypeNamespace* types);

  std::string CreatorName() const override;

  void WriteToParcel(StatementBlock* addTo, Variable* v, Variable* parcel,
                     int flags) const override;
  void CreateFromParcel(StatementBlock* addTo, Variable* v, Variable* parcel,
                        Variable** cl) const override;
};

class RemoteExceptionType : public Type {
 public:
  RemoteExceptionType(const JavaTypeNamespace* types);

  void WriteToParcel(StatementBlock* addTo, Variable* v, Variable* parcel,
                     int flags) const override;
  void CreateFromParcel(StatementBlock* addTo, Variable* v, Variable* parcel,
                        Variable** cl) const override;
};

class RuntimeExceptionType : public Type {
 public:
  RuntimeExceptionType(const JavaTypeNamespace* types);

  void WriteToParcel(StatementBlock* addTo, Variable* v, Variable* parcel,
                     int flags) const override;
  void CreateFromParcel(StatementBlock* addTo, Variable* v, Variable* parcel,
                        Variable** cl) const override;
};

class IBinderArrayType : public Type {
 public:
  IBinderArrayType(const JavaTypeNamespace* types);

  void WriteToParcel(StatementBlock* addTo, Variable* v, Variable* parcel,
                     int flags) const override;
  void CreateFromParcel(StatementBlock* addTo, Variable* v,
                        Variable* parcel, Variable** cl) const override;
  void ReadFromParcel(StatementBlock* addTo, Variable* v, Variable* parcel,
                      Variable** cl) const override;
  const ValidatableType* NullableType() const override { return this; }
};

class IBinderType : public Type {
 public:
  IBinderType(const JavaTypeNamespace* types);

  void WriteToParcel(StatementBlock* addTo, Variable* v, Variable* parcel,
                     int flags) const override;
  void CreateFromParcel(StatementBlock* addTo, Variable* v, Variable* parcel,
                        Variable** cl) const override;
};

class IInterfaceType : public Type {
 public:
  IInterfaceType(const JavaTypeNamespace* types);

  void WriteToParcel(StatementBlock* addTo, Variable* v, Variable* parcel,
                     int flags) const override;
  void CreateFromParcel(StatementBlock* addTo, Variable* v, Variable* parcel,
                        Variable** cl) const override;
};

class BinderType : public Type {
 public:
  BinderType(const JavaTypeNamespace* types);

  void WriteToParcel(StatementBlock* addTo, Variable* v, Variable* parcel,
                     int flags) const override;
  void CreateFromParcel(StatementBlock* addTo, Variable* v, Variable* parcel,
                        Variable** cl) const override;
};

class BinderProxyType : public Type {
 public:
  BinderProxyType(const JavaTypeNamespace* types);

  void WriteToParcel(StatementBlock* addTo, Variable* v, Variable* parcel,
                     int flags) const override;
  void CreateFromParcel(StatementBlock* addTo, Variable* v, Variable* parcel,
                        Variable** cl) const override;
};

class ParcelType : public Type {
 public:
  ParcelType(const JavaTypeNamespace* types);

  void WriteToParcel(StatementBlock* addTo, Variable* v, Variable* parcel,
                     int flags) const override;
  void CreateFromParcel(StatementBlock* addTo, Variable* v, Variable* parcel,
                        Variable** cl) const override;
  const ValidatableType* NullableType() const override { return this; }
};

class ParcelableInterfaceType : public Type {
 public:
  ParcelableInterfaceType(const JavaTypeNamespace* types);

  void WriteToParcel(StatementBlock* addTo, Variable* v, Variable* parcel,
                     int flags) const override;
  void CreateFromParcel(StatementBlock* addTo, Variable* v, Variable* parcel,
                        Variable** cl) const override;
};

class MapType : public Type {
 public:
  MapType(const JavaTypeNamespace* types);

  void WriteToParcel(StatementBlock* addTo, Variable* v, Variable* parcel,
                     int flags) const override;
  void CreateFromParcel(StatementBlock* addTo, Variable* v, Variable* parcel,
                        Variable** cl) const override;
  void ReadFromParcel(StatementBlock* addTo, Variable* v, Variable* parcel,
                      Variable** cl) const override;
  const ValidatableType* NullableType() const override { return this; }
};

class ListType : public Type {
 public:
  ListType(const JavaTypeNamespace* types);

  std::string InstantiableName() const override;

  void WriteToParcel(StatementBlock* addTo, Variable* v, Variable* parcel,
                     int flags) const override;
  void CreateFromParcel(StatementBlock* addTo, Variable* v, Variable* parcel,
                        Variable** cl) const override;
  void ReadFromParcel(StatementBlock* addTo, Variable* v, Variable* parcel,
                      Variable** cl) const override;
  const ValidatableType* NullableType() const override { return this; }
};

class UserDataArrayType : public Type {
 public:
  UserDataArrayType(const JavaTypeNamespace* types, const std::string& package,
                    const std::string& name, bool builtIn,
                    bool canWriteToParcel, const std::string& declFile = "",
                    int declLine = -1);

  std::string CreatorName() const override;

  void WriteToParcel(StatementBlock* addTo, Variable* v, Variable* parcel,
                     int flags) const override;
  void CreateFromParcel(StatementBlock* addTo, Variable* v,
                        Variable* parcel, Variable** cl) const override;
  void ReadFromParcel(StatementBlock* addTo, Variable* v, Variable* parcel,
                      Variable** cl) const override;
  const ValidatableType* NullableType() const override { return this; }
};

class UserDataType : public Type {
 public:
  UserDataType(const JavaTypeNamespace* types, const std::string& package,
               const std::string& name, bool builtIn, bool canWriteToParcel,
               const std::string& declFile = "", int declLine = -1);

  std::string CreatorName() const override;

  void WriteToParcel(StatementBlock* addTo, Variable* v, Variable* parcel,
                     int flags) const override;
  void CreateFromParcel(StatementBlock* addTo, Variable* v, Variable* parcel,
                        Variable** cl) const override;
  void ReadFromParcel(StatementBlock* addTo, Variable* v, Variable* parcel,
                      Variable** cl) const override;
  const ValidatableType* NullableType() const override { return this; }
};

class InterfaceType : public Type {
 public:
  InterfaceType(const JavaTypeNamespace* types, const std::string& package,
                const std::string& name, bool builtIn, bool oneway,
                const std::string& declFile, int declLine, const Type* stub,
                const Type* proxy);

  bool OneWay() const;

  void WriteToParcel(StatementBlock* addTo, Variable* v, Variable* parcel,
                     int flags) const override;
  void CreateFromParcel(StatementBlock* addTo, Variable* v, Variable* parcel,
                        Variable** cl) const override;
  const Type* GetStub() const { return stub_; }
  const Type* GetProxy() const { return proxy_; }

 private:
  bool m_oneway;
  const Type* stub_;
  const Type* proxy_;
};

class ClassLoaderType : public Type {
 public:
  ClassLoaderType(const JavaTypeNamespace* types);
};

class GenericListType : public Type {
 public:
  GenericListType(const JavaTypeNamespace* types, const Type* arg);

  std::string CreatorName() const override;
  std::string InstantiableName() const override;
  std::string JavaType() const override {
    return "java.util.List<" + m_contained_type->JavaType() + ">";
  }

  void WriteToParcel(StatementBlock* addTo, Variable* v, Variable* parcel,
                     int flags) const override;
  void CreateFromParcel(StatementBlock* addTo, Variable* v, Variable* parcel,
                        Variable** cl) const override;
  void ReadFromParcel(StatementBlock* addTo, Variable* v, Variable* parcel,
                      Variable** cl) const override;
  const ValidatableType* NullableType() const override { return this; }

 private:
  const Type* m_contained_type;
  const std::string m_creator;
};

class JavaTypeNamespace : public LanguageTypeNamespace<Type> {
 public:
  JavaTypeNamespace() = default;
  virtual ~JavaTypeNamespace() = default;

  void Init() override;
  bool AddParcelableType(const AidlParcelable& p,
                         const std::string& filename) override;
  bool AddBinderType(const AidlInterface& b,
                     const std::string& filename) override;
  bool AddListType(const std::string& contained_type_name) override;
  bool AddMapType(const std::string& key_type_name,
                  const std::string& value_type_name) override;

  const Type* BoolType() const { return m_bool_type; }
  const Type* IntType() const { return m_int_type; }
  const Type* StringType() const { return m_string_type; }
  const Type* TextUtilsType() const { return m_text_utils_type; }
  const Type* RemoteExceptionType() const { return m_remote_exception_type; }
  const Type* RuntimeExceptionType() const { return m_runtime_exception_type; }
  const Type* IBinderType() const { return m_ibinder_type; }
  const Type* IInterfaceType() const { return m_iinterface_type; }
  const Type* BinderNativeType() const { return m_binder_native_type; }
  const Type* BinderProxyType() const { return m_binder_proxy_type; }
  const Type* ParcelType() const { return m_parcel_type; }
  const Type* ParcelableInterfaceType() const {
    return m_parcelable_interface_type;
  }
  const Type* ContextType() const { return m_context_type; }
  const Type* ClassLoaderType() const { return m_classloader_type; }

 private:
  const Type* m_bool_type{nullptr};
  const Type* m_int_type{nullptr};
  const Type* m_string_type{nullptr};
  const Type* m_text_utils_type{nullptr};
  const Type* m_remote_exception_type{nullptr};
  const Type* m_runtime_exception_type{nullptr};
  const Type* m_ibinder_type{nullptr};
  const Type* m_iinterface_type{nullptr};
  const Type* m_binder_native_type{nullptr};
  const Type* m_binder_proxy_type{nullptr};
  const Type* m_parcel_type{nullptr};
  const Type* m_parcelable_interface_type{nullptr};
  const Type* m_context_type{nullptr};
  const Type* m_classloader_type{nullptr};

  DISALLOW_COPY_AND_ASSIGN(JavaTypeNamespace);
};

extern Expression* NULL_VALUE;
extern Expression* THIS_VALUE;
extern Expression* SUPER_VALUE;
extern Expression* TRUE_VALUE;
extern Expression* FALSE_VALUE;

}  // namespace java
}  // namespace aidl
}  // namespace android

#endif  // AIDL_TYPE_JAVA_H_
