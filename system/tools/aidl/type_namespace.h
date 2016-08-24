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

#ifndef AIDL_TYPE_NAMESPACE_H_
#define AIDL_TYPE_NAMESPACE_H_

#include <memory>
#include <string>

#include <android-base/macros.h>
#include <android-base/stringprintf.h>
#include <android-base/strings.h>

#include "aidl_language.h"
#include "logging.h"

namespace android {
namespace aidl {

// Special reserved type names.
extern const char kAidlReservedTypePackage[];
extern const char kUtf8StringClass[];  // UTF8 wire format string
extern const char kUtf8InCppStringClass[];  // UTF16 wire format, UTF8 in C++

// Helpful aliases defined to be <kAidlReservedTypePackage>.<class name>
extern const char kUtf8StringCanonicalName[];
extern const char kUtf8InCppStringCanonicalName[];

// We sometimes special case this class.
extern const char kStringCanonicalName[];

// Note that these aren't the strings recognized by the parser, we just keep
// here for the sake of logging a common string constant.
extern const char kUtf8Annotation[];
extern const char kUtf8InCppAnnotation[];

class ValidatableType {
 public:
  enum {
    KIND_BUILT_IN,
    KIND_PARCELABLE,
    KIND_INTERFACE,
    KIND_GENERATED,
  };

  ValidatableType(int kind,
                  const std::string& package, const std::string& type_name,
                  const std::string& decl_file, int decl_line);
  virtual ~ValidatableType() = default;

  virtual bool CanBeArray() const { return ArrayType() != nullptr; }
  virtual bool CanBeOutParameter() const = 0;
  virtual bool CanWriteToParcel() const = 0;

  virtual const ValidatableType* ArrayType() const = 0;
  virtual const ValidatableType* NullableType() const = 0;

  // ShortName() is the class name without a package.
  std::string ShortName() const { return type_name_; }
  // CanonicalName() returns the canonical AIDL type, with packages.
  std::string CanonicalName() const { return canonical_name_; }

  int Kind() const { return kind_; }
  std::string HumanReadableKind() const;
  std::string DeclFile() const { return origin_file_; }
  int DeclLine() const { return origin_line_; }

 private:
  const int kind_;
  const std::string type_name_;
  const std::string canonical_name_;
  const std::string origin_file_;
  const int origin_line_;

  DISALLOW_COPY_AND_ASSIGN(ValidatableType);
};

class TypeNamespace {
 public:
  // Load the TypeNamespace with built in types.  Don't do work in the
  // constructor because many of the useful methods are virtual.
  virtual void Init() = 0;

  // Load this TypeNamespace with user defined types.
  virtual bool AddParcelableType(const AidlParcelable& p,
                                 const std::string& filename) = 0;
  virtual bool AddBinderType(const AidlInterface& b,
                             const std::string& filename) = 0;
  // Add a container type to this namespace.  Returns false only
  // on error. Silently discards requests to add non-container types.
  virtual bool MaybeAddContainerType(const AidlType& aidl_type) = 0;

  // Returns true iff this has a type for |import|.
  virtual bool HasImportType(const AidlImport& import) const = 0;

  // Returns true iff |package| is a valid package name.
  virtual bool IsValidPackage(const std::string& package) const;

  // Returns a pointer to a type corresponding to |raw_type| or nullptr
  // if this is an invalid return type.
  virtual const ValidatableType* GetReturnType(
      const AidlType& raw_type,
      const std::string& filename) const;

  // Returns a pointer to a type corresponding to |a| or nullptr if |a|
  // has an invalid argument type.
  virtual const ValidatableType* GetArgType(const AidlArgument& a,
                                            int arg_index,
                                            const std::string& filename) const;

  // Returns a pointer to a type corresponding to |interface|.
  virtual const ValidatableType* GetInterfaceType(
      const AidlInterface& interface) const = 0;

 protected:
  TypeNamespace() = default;
  virtual ~TypeNamespace() = default;

  virtual const ValidatableType* GetValidatableType(
      const AidlType& type, std::string* error_msg) const = 0;

 private:
  DISALLOW_COPY_AND_ASSIGN(TypeNamespace);
};

template<typename T>
class LanguageTypeNamespace : public TypeNamespace {
 public:
  LanguageTypeNamespace() = default;
  virtual ~LanguageTypeNamespace() = default;

  // Get a pointer to an existing type.  Searches first by fully-qualified
  // name, and then class name (dropping package qualifiers).
  const T* Find(const AidlType& aidl_type) const;

  // Find a type by its |name|.  If |name| refers to a container type (e.g.
  // List<String>) you must turn it into a canonical name first (e.g.
  // java.util.List<java.lang.String>).
  const T* FindTypeByCanonicalName(const std::string& name) const;
  bool HasTypeByCanonicalName(const std::string& type_name) const {
    return FindTypeByCanonicalName(type_name) != nullptr;
  }
  bool HasImportType(const AidlImport& import) const override {
    return HasTypeByCanonicalName(import.GetNeededClass());
  }
  const ValidatableType* GetInterfaceType(
      const AidlInterface& interface) const override {
    return FindTypeByCanonicalName(interface.GetCanonicalName());
  }

  bool MaybeAddContainerType(const AidlType& aidl_type) override;
  // We dynamically create container types as we discover them in the parse
  // tree.  Returns false if the contained types cannot be canonicalized.
  virtual bool AddListType(const std::string& contained_type_name) = 0;
  virtual bool AddMapType(const std::string& key_type_name,
                          const std::string& value_type_name) = 0;

 protected:
  bool Add(const T* type);

 private:
  // Returns true iff the name can be canonicalized to a container type.
  virtual bool CanonicalizeContainerType(
      const AidlType& aidl_type,
      std::vector<std::string>* container_class,
      std::vector<std::string>* contained_type_names) const;

  // Returns true if this is a container type, rather than a normal type.
  bool IsContainerType(const std::string& type_name) const;

  const ValidatableType* GetValidatableType(
      const AidlType& type, std::string* error_msg) const override;

  std::vector<std::unique_ptr<const T>> types_;

  DISALLOW_COPY_AND_ASSIGN(LanguageTypeNamespace);
};  // class LanguageTypeNamespace

template<typename T>
bool LanguageTypeNamespace<T>::Add(const T* type) {
  const T* existing = FindTypeByCanonicalName(type->CanonicalName());
  if (!existing) {
    types_.emplace_back(type);
    return true;
  }

  if (existing->Kind() == ValidatableType::KIND_BUILT_IN) {
    LOG(ERROR) << type->DeclFile() << ":" << type->DeclLine()
               << " attempt to redefine built in class "
               << type->CanonicalName();
    return false;
  }

  if (type->Kind() != existing->Kind()) {
    LOG(ERROR) << type->DeclFile() << ":" << type->DeclLine()
               << " attempt to redefine " << type->CanonicalName()
               << " as " << type->HumanReadableKind();
    LOG(ERROR) << existing->DeclFile() << ":" << existing->DeclLine()
               << " previously defined here as "
               << existing->HumanReadableKind();
    return false;
  }

  return true;
}

template<typename T>
const T* LanguageTypeNamespace<T>::Find(const AidlType& aidl_type) const {
  using std::string;
  using std::vector;
  using android::base::Join;
  using android::base::Trim;

  string name = Trim(aidl_type.GetName());
  if (IsContainerType(name)) {
    vector<string> container_class;
    vector<string> contained_type_names;
    if (!CanonicalizeContainerType(aidl_type, &container_class,
                                   &contained_type_names)) {
      return nullptr;
    }
    name = Join(container_class, '.') +
           "<" + Join(contained_type_names, ',') + ">";
  }
  // Here, we know that we have the canonical name for this container.
  return FindTypeByCanonicalName(name);
}

template<typename T>
const T* LanguageTypeNamespace<T>::FindTypeByCanonicalName(
    const std::string& raw_name) const {
  using android::base::Trim;

  std::string name = Trim(raw_name);
  const T* ret = nullptr;
  for (const auto& type : types_) {
    // Always prefer a exact match if possible.
    // This works for primitives and class names qualified with a package.
    if (type->CanonicalName() == name) {
      ret = type.get();
      break;
    }
    // We allow authors to drop packages when refering to a class name.
    if (type->ShortName() == name) {
      ret = type.get();
    }
  }

  return ret;
}

template<typename T>
bool LanguageTypeNamespace<T>::MaybeAddContainerType(
    const AidlType& aidl_type) {
  using android::base::Join;

  std::string type_name = aidl_type.GetName();
  if (!IsContainerType(type_name)) {
    return true;
  }

  std::vector<std::string> container_class;
  std::vector<std::string> contained_type_names;
  if (!CanonicalizeContainerType(aidl_type, &container_class,
                                 &contained_type_names)) {
    return false;
  }

  const std::string canonical_name = Join(container_class, ".") +
      "<" + Join(contained_type_names, ",") + ">";
  if (HasTypeByCanonicalName(canonical_name)) {
    return true;
  }


  // We only support two types right now and this type is one of them.
  switch (contained_type_names.size()) {
    case 1:
      return AddListType(contained_type_names[0]);
    case 2:
      return AddMapType(contained_type_names[0], contained_type_names[1]);
    default:
      break;  // Should never get here, will FATAL below.
  }

  LOG(FATAL) << "aidl internal error";
  return false;
}

template<typename T>
bool LanguageTypeNamespace<T>::IsContainerType(
    const std::string& type_name) const {
  const size_t opening_brace = type_name.find('<');
  const size_t closing_brace = type_name.find('>');
  if (opening_brace != std::string::npos ||
      closing_brace != std::string::npos) {
    return true;  // Neither < nor > appear in normal AIDL types.
  }
  return false;
}

template<typename T>
bool LanguageTypeNamespace<T>::CanonicalizeContainerType(
    const AidlType& aidl_type,
    std::vector<std::string>* container_class,
    std::vector<std::string>* contained_type_names) const {
  using android::base::Trim;
  using android::base::Split;

  std::string name = Trim(aidl_type.GetName());
  const size_t opening_brace = name.find('<');
  const size_t closing_brace = name.find('>');
  if (opening_brace == std::string::npos ||
      closing_brace == std::string::npos) {
    return false;
  }

  if (opening_brace != name.rfind('<') ||
      closing_brace != name.rfind('>') ||
      closing_brace != name.length() - 1) {
    // Nested/invalid templates are forbidden.
    LOG(ERROR) << "Invalid template type '" << name << "'";
    return false;
  }

  std::string container = Trim(name.substr(0, opening_brace));
  std::string remainder = name.substr(opening_brace + 1,
                                 (closing_brace - opening_brace) - 1);
  std::vector<std::string> args = Split(remainder, ",");
  for (auto& type_name: args) {
    // Here, we are relying on FindTypeByCanonicalName to do its best when
    // given a non-canonical name for non-compound type (i.e. not another
    // container).
    const T* arg_type = FindTypeByCanonicalName(type_name);
    if (!arg_type) {
      return false;
    }

    // Now get the canonical names for these contained types, remapping them if
    // necessary.
    type_name = arg_type->CanonicalName();
    if (aidl_type.IsUtf8() && type_name == "java.lang.String") {
      type_name = kUtf8StringCanonicalName;
    } else if (aidl_type.IsUtf8InCpp() && type_name == "java.lang.String") {
      type_name = kUtf8InCppStringCanonicalName;
    }
  }

  // Map the container name to its canonical form for supported containers.
  if ((container == "List" || container == "java.util.List") &&
      args.size() == 1) {
    *container_class = {"java", "util", "List"};
    *contained_type_names = args;
    return true;
  }
  if ((container == "Map" || container == "java.util.Map") &&
      args.size() == 2) {
    *container_class = {"java", "util", "Map"};
    *contained_type_names = args;
    return true;
  }

  LOG(ERROR) << "Unknown find container with name " << container
             << " and " << args.size() << "contained types.";
  return false;
}

template<typename T>
const ValidatableType* LanguageTypeNamespace<T>::GetValidatableType(
    const AidlType& aidl_type, std::string* error_msg) const {
  using android::base::StringPrintf;

  const ValidatableType* type = Find(aidl_type);
  if (type == nullptr) {
    *error_msg = "unknown type";
    return nullptr;
  }

  if (aidl_type.GetName() == "void") {
    if (aidl_type.IsArray()) {
      *error_msg = "void type cannot be an array";
      return nullptr;
    }
    if (aidl_type.IsNullable() || aidl_type.IsUtf8() ||
        aidl_type.IsUtf8InCpp()) {
      *error_msg = "void type cannot be annotated";
      return nullptr;
    }
    // We have no more special handling for void.
    return type;
  }

  // No type may be annotated with both these annotations.
  if (aidl_type.IsUtf8() && aidl_type.IsUtf8InCpp()) {
    *error_msg = StringPrintf("Type cannot be marked as both %s and %s.",
                              kUtf8Annotation, kUtf8InCppAnnotation);
    return nullptr;
  }

  // Strings inside containers get remapped to appropriate utf8 versions when
  // we convert the container name to its canonical form and the look up the
  // type.  However, for non-compound types (i.e. those not in a container) we
  // must patch them up here.
  if (!IsContainerType(type->CanonicalName()) &&
      (aidl_type.IsUtf8() || aidl_type.IsUtf8InCpp())) {
    const char* annotation_literal =
        (aidl_type.IsUtf8()) ? kUtf8Annotation : kUtf8InCppAnnotation;
    if (aidl_type.GetName() != "String" &&
        aidl_type.GetName() != "java.lang.String") {
      *error_msg = StringPrintf("type '%s' may not be annotated as %s.",
                                aidl_type.GetName().c_str(),
                                annotation_literal);
      return nullptr;
    }

    if (aidl_type.IsUtf8()) {
      type = FindTypeByCanonicalName(kUtf8StringCanonicalName);
    } else {  // aidl_type.IsUtf8InCpp()
      type = FindTypeByCanonicalName(kUtf8InCppStringCanonicalName);
    }

    if (type == nullptr) {
      *error_msg = StringPrintf(
          "%s is unsupported when generating code for this language.",
          annotation_literal);
      return nullptr;
    }
  }

  if (!type->CanWriteToParcel()) {
    *error_msg = "type cannot be marshalled";
    return nullptr;
  }

  if (aidl_type.IsArray()) {
    type = type->ArrayType();
    if (!type) {
      *error_msg = StringPrintf("type '%s' cannot be an array",
                                aidl_type.GetName().c_str());
      return nullptr;
    }
  }

  if (aidl_type.IsNullable()) {
    type = type->NullableType();
    if (!type) {
      *error_msg = StringPrintf("type '%s%s' cannot be marked as possibly null",
                                aidl_type.GetName().c_str(),
                                (aidl_type.IsArray()) ? "[]" : "");
      return nullptr;
    }
  }

  return type;
}

}  // namespace aidl
}  // namespace android

#endif  // AIDL_TYPE_NAMESPACE_H_
