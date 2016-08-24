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

#include "generate_java.h"

#include <memory>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <android-base/stringprintf.h>

#include "code_writer.h"
#include "type_java.h"

using std::unique_ptr;
using ::android::aidl::java::Variable;
using std::string;
using android::base::StringPrintf;

namespace android {
namespace aidl {

// =================================================
VariableFactory::VariableFactory(const string& base)
    : base_(base),
      index_(0) {
}

Variable* VariableFactory::Get(const Type* type) {
  Variable* v = new Variable(
      type, StringPrintf("%s%d", base_.c_str(), index_));
  vars_.push_back(v);
  index_++;
  return v;
}

Variable* VariableFactory::Get(int index) {
  return vars_[index];
}

namespace java {

int generate_java(const string& filename, const string& originalSrc,
                  AidlInterface* iface, JavaTypeNamespace* types,
                  const IoDelegate& io_delegate) {
  Class* cl = generate_binder_interface_class(iface, types);

  Document* document = new Document(
      "" /* no comment */,
      (!iface->GetPackage().empty()) ? iface->GetPackage() : "",
      originalSrc,
      unique_ptr<Class>(cl));

  CodeWriterPtr code_writer = io_delegate.GetCodeWriter(filename);
  document->Write(code_writer.get());

  return 0;
}

}  // namespace java
}  // namespace android
}  // namespace aidl
