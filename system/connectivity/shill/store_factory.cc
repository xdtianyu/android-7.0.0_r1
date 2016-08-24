//
// Copyright (C) 2015 The Android Open Source Project
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//

#include "shill/store_factory.h"

#if defined(ENABLE_JSON_STORE)
#include "shill/json_store.h"
#else
#include "shill/key_file_store.h"
#endif  // ENABLE_JSON_STORE

namespace shill {

namespace {

base::LazyInstance<StoreFactory> g_persistent_store_factory
    = LAZY_INSTANCE_INITIALIZER;

}  // namespace

StoreFactory::StoreFactory() {}

// static
StoreFactory* StoreFactory::GetInstance() {
  return g_persistent_store_factory.Pointer();
}

StoreInterface* StoreFactory::CreateStore(const base::FilePath& path) {
#if defined(ENABLE_JSON_STORE)
  return new JsonStore(path);
#else
  return new KeyFileStore(path);
#endif
}

}  // namespace shill
