/*
 * Copyright (C) 2016, The Android Open Source Project
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

#ifndef AIDL_GENERATE_JAVA_H_
#define AIDL_GENERATE_JAVA_H_

#include <string>

#include "aidl_language.h"
#include "ast_java.h"
#include "io_delegate.h"

namespace android {
namespace aidl {

namespace java {

class JavaTypeNamespace;

int generate_java(const std::string& filename, const std::string& originalSrc,
                  AidlInterface* iface, java::JavaTypeNamespace* types,
                  const IoDelegate& io_delegate);

android::aidl::java::Class* generate_binder_interface_class(
    const AidlInterface* iface, java::JavaTypeNamespace* types);

}  // namespace java

class VariableFactory {
 public:
  using Variable = ::android::aidl::java::Variable;
  using Type = ::android::aidl::java::Type;

  VariableFactory(const std::string& base); // base must be short
  Variable* Get(const Type* type);
  Variable* Get(int index);

 private:
  std::vector<Variable*> vars_;
  std::string base_;
  int index_;

  DISALLOW_COPY_AND_ASSIGN(VariableFactory);
};

}  // namespace android
}  // namespace aidl

#endif // AIDL_GENERATE_JAVA_H_
