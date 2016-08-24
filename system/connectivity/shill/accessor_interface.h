//
// Copyright (C) 2012 The Android Open Source Project
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

#ifndef SHILL_ACCESSOR_INTERFACE_H_
#define SHILL_ACCESSOR_INTERFACE_H_

#include <map>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include <base/macros.h>

#include "shill/key_value_store.h"

namespace shill {

class Error;

// A templated abstract base class for objects that can be used to access
// properties stored in objects that are meant to be made available over RPC.
// The intended usage is that an object stores a maps of strings to
// AccessorInterfaces of the appropriate type, and then uses
// map[name]->Get() and map[name]->Set(value) to get and set the properties.
template <class T>
class AccessorInterface {
 public:
  AccessorInterface() {}
  virtual ~AccessorInterface() {}

  // Reset the property to its default value. Sets |error| on failure.
  virtual void Clear(Error* error) = 0;
  // Provides read-only access. Sets |error| on failure.
  virtual T Get(Error* error) = 0;
  // Attempts to set the wrapped value. Sets |error| on failure.  The
  // return value indicates whether or not the wrapped value was
  // modified. If the new value is the same as the old value, Set
  // returns false, but with |error| unchanged.
  virtual bool Set(const T& value, Error* error) = 0;

 private:
  DISALLOW_COPY_AND_ASSIGN(AccessorInterface);
};

typedef std::vector<uint8_t> ByteArray;
typedef std::vector<ByteArray> ByteArrays;
// Note that while the RpcIdentifiers type has the same concrete
// representation as the Strings type, it may be serialized
// differently. Accordingly, PropertyStore tracks RpcIdentifiers
// separately from Strings. We create a separate typedef here, to make
// the PropertyStore-related code read more simply.
typedef std::string RpcIdentifier;
typedef std::vector<std::string> RpcIdentifiers;
typedef std::vector<std::string> Strings;
typedef std::map<std::string, std::string> Stringmap;
typedef std::vector<Stringmap> Stringmaps;
typedef std::vector<uint16_t> Uint16s;

// Using a smart pointer here allows pointers to classes derived from
// AccessorInterface<> to be stored in maps and other STL container types.
typedef std::shared_ptr<AccessorInterface<bool>> BoolAccessor;
typedef std::shared_ptr<AccessorInterface<int16_t>> Int16Accessor;
typedef std::shared_ptr<AccessorInterface<int32_t>> Int32Accessor;
// See comment above RpcIdentifiers typedef, for the reason why the
// RpcIdentifiersAccessor exists (even though it has the same
// underlying type as StringsAccessor).
typedef std::shared_ptr<
    AccessorInterface<RpcIdentifier>> RpcIdentifierAccessor;
typedef std::shared_ptr<
    AccessorInterface<std::vector<std::string>>> RpcIdentifiersAccessor;
typedef std::shared_ptr<AccessorInterface<std::string>> StringAccessor;
typedef std::shared_ptr<AccessorInterface<Stringmap>> StringmapAccessor;
typedef std::shared_ptr<AccessorInterface<Stringmaps>> StringmapsAccessor;
typedef std::shared_ptr<AccessorInterface<Strings>> StringsAccessor;
typedef std::shared_ptr<
    AccessorInterface<KeyValueStore>> KeyValueStoreAccessor;
typedef std::shared_ptr<AccessorInterface<uint8_t>> Uint8Accessor;
typedef std::shared_ptr<AccessorInterface<ByteArray>> ByteArrayAccessor;
typedef std::shared_ptr<AccessorInterface<uint16_t>> Uint16Accessor;
typedef std::shared_ptr<AccessorInterface<Uint16s>> Uint16sAccessor;
typedef std::shared_ptr<AccessorInterface<uint32_t>> Uint32Accessor;
typedef std::shared_ptr<AccessorInterface<uint64_t>> Uint64Accessor;

}  // namespace shill

#endif  // SHILL_ACCESSOR_INTERFACE_H_
