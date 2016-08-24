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

#include <iostream>
#include <memory>

#include "aidl.h"
#include "io_delegate.h"
#include "logging.h"
#include "options.h"

using android::aidl::JavaOptions;

int main(int argc, char** argv) {
  android::base::InitLogging(argv);
  LOG(DEBUG) << "aidl starting";
  std::unique_ptr<JavaOptions> options = JavaOptions::Parse(argc, argv);
  if (!options) {
    return 1;
  }

  android::aidl::IoDelegate io_delegate;
  switch (options->task) {
    case JavaOptions::COMPILE_AIDL_TO_JAVA:
      return android::aidl::compile_aidl_to_java(*options, io_delegate);
    case JavaOptions::PREPROCESS_AIDL:
      if (android::aidl::preprocess_aidl(*options, io_delegate))
        return 0;
      return 1;
  }
  std::cerr << "aidl: internal error" << std::endl;
  return 1;
}
