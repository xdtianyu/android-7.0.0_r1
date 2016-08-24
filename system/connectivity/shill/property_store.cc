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

#include "shill/property_store.h"

#include <map>
#include <string>
#include <vector>

#include <base/stl_util.h>
#include <dbus/object_path.h>

#include "shill/error.h"
#include "shill/logging.h"
#include "shill/property_accessor.h"

using std::map;
using std::string;
using std::vector;

namespace shill {

namespace Logging {
static auto kModuleLogScope = ScopeLogger::kProperty;
static string ObjectID(const PropertyStore* p) { return "(property_store)"; }
}

PropertyStore::PropertyStore() {}

PropertyStore::PropertyStore(PropertyChangeCallback on_property_changed) :
    property_changed_callback_(on_property_changed) {}

PropertyStore::~PropertyStore() {}

bool PropertyStore::Contains(const string& prop) const {
  return (ContainsKey(bool_properties_, prop)  ||
          ContainsKey(int16_properties_, prop) ||
          ContainsKey(int32_properties_, prop) ||
          ContainsKey(key_value_store_properties_, prop) ||
          ContainsKey(string_properties_, prop) ||
          ContainsKey(stringmap_properties_, prop) ||
          ContainsKey(stringmaps_properties_, prop) ||
          ContainsKey(strings_properties_, prop) ||
          ContainsKey(uint8_properties_, prop) ||
          ContainsKey(bytearray_properties_, prop) ||
          ContainsKey(uint16_properties_, prop) ||
          ContainsKey(uint16s_properties_, prop) ||
          ContainsKey(uint32_properties_, prop) ||
          ContainsKey(uint64_properties_, prop) ||
          ContainsKey(rpc_identifier_properties_, prop) ||
          ContainsKey(rpc_identifiers_properties_, prop));
}

bool PropertyStore::SetAnyProperty(const string& name,
                                   const brillo::Any& value,
                                   Error* error) {
  bool ret = false;
  if (value.IsTypeCompatible<bool>()) {
    ret = SetBoolProperty(name, value.Get<bool>(), error);
  } else if (value.IsTypeCompatible<uint8_t>()) {
    ret = SetUint8Property(name, value.Get<uint8_t>(), error);
  } else if (value.IsTypeCompatible<int16_t>()) {
    ret = SetInt16Property(name, value.Get<int16_t>(), error);
  } else if (value.IsTypeCompatible<int32_t>()) {
    ret = SetInt32Property(name, value.Get<int32_t>(), error);
  } else if (value.IsTypeCompatible<dbus::ObjectPath>()) {
    ret = SetStringProperty(name, value.Get<dbus::ObjectPath>().value(), error);
  } else if (value.IsTypeCompatible<string>()) {
    ret = SetStringProperty(name, value.Get<string>(), error);
  } else if (value.IsTypeCompatible<Stringmap>()) {
    ret = SetStringmapProperty(name, value.Get<Stringmap>(), error);
  } else if (value.IsTypeCompatible<Stringmaps>()) {
    SLOG(nullptr, 1) << " can't yet handle setting type "
                     << value.GetUndecoratedTypeName();
    error->Populate(Error::kInternalError);
  } else if (value.IsTypeCompatible<Strings>()) {
    ret = SetStringsProperty(name, value.Get<Strings>(), error);
  } else if (value.IsTypeCompatible<ByteArray>()) {
    ret = SetByteArrayProperty(name, value.Get<ByteArray>(), error);
  } else if (value.IsTypeCompatible<uint16_t>()) {
    ret = SetUint16Property(name, value.Get<uint16_t>(), error);
  } else if (value.IsTypeCompatible<Uint16s>()) {
    ret = SetUint16sProperty(name, value.Get<Uint16s>(), error);
  } else if (value.IsTypeCompatible<uint32_t>()) {
    ret = SetUint32Property(name, value.Get<uint32_t>(), error);
  } else if (value.IsTypeCompatible<uint64_t>()) {
    ret = SetUint64Property(name, value.Get<uint64_t>(), error);
  } else if (value.IsTypeCompatible<brillo::VariantDictionary>()) {
    KeyValueStore store;
    KeyValueStore::ConvertFromVariantDictionary(
        value.Get<brillo::VariantDictionary>(), &store);
    ret = SetKeyValueStoreProperty(name, store, error);
  } else {
    NOTREACHED() << " unknown type: " << value.GetUndecoratedTypeName();
    error->Populate(Error::kInternalError);
  }
  return ret;
}

bool PropertyStore::SetProperties(const brillo::VariantDictionary& in,
                                  Error* error) {
  for (const auto& kv : in) {
    if (!SetAnyProperty(kv.first, kv.second, error)) {
      return false;
    }
  }
  return true;
}

bool PropertyStore::GetProperties(brillo::VariantDictionary* out,
                                  Error* error) const {
  {
    ReadablePropertyConstIterator<bool> it = GetBoolPropertiesIter();
    for ( ; !it.AtEnd(); it.Advance()) {
      out->insert(std::make_pair(it.Key(), brillo::Any(it.value())));
    }
  }
  {
    ReadablePropertyConstIterator<int16_t> it = GetInt16PropertiesIter();
    for ( ; !it.AtEnd(); it.Advance()) {
      out->insert(std::make_pair(it.Key(), brillo::Any(it.value())));
    }
  }
  {
    ReadablePropertyConstIterator<int32_t> it = GetInt32PropertiesIter();
    for ( ; !it.AtEnd(); it.Advance()) {
      out->insert(std::make_pair(it.Key(), brillo::Any(it.value())));
    }
  }
  {
    ReadablePropertyConstIterator<RpcIdentifier> it =
        GetRpcIdentifierPropertiesIter();
    for ( ; !it.AtEnd(); it.Advance()) {
      out->insert(
          std::make_pair(it.Key(),
                         brillo::Any(dbus::ObjectPath(it.value()))));
    }
  }
  {
    ReadablePropertyConstIterator<RpcIdentifiers> it =
        GetRpcIdentifiersPropertiesIter();
    for ( ; !it.AtEnd(); it.Advance()) {
      vector<dbus::ObjectPath> rpc_identifiers_as_paths;
      for (const auto& path : it.value()) {
        rpc_identifiers_as_paths.push_back(dbus::ObjectPath(path));
      }
      out->insert(
          std::make_pair(it.Key(), brillo::Any(rpc_identifiers_as_paths)));
    }
  }
  {
    ReadablePropertyConstIterator<string> it = GetStringPropertiesIter();
    for ( ; !it.AtEnd(); it.Advance()) {
      out->insert(std::make_pair(it.Key(), brillo::Any(it.value())));
    }
  }
  {
    ReadablePropertyConstIterator<Stringmap> it = GetStringmapPropertiesIter();
    for ( ; !it.AtEnd(); it.Advance()) {
      out->insert(std::make_pair(it.Key(), brillo::Any(it.value())));
    }
  }
  {
    ReadablePropertyConstIterator<Stringmaps> it =
        GetStringmapsPropertiesIter();
    for ( ; !it.AtEnd(); it.Advance()) {
      out->insert(std::make_pair(it.Key(), brillo::Any(it.value())));
    }
  }
  {
    ReadablePropertyConstIterator<Strings> it = GetStringsPropertiesIter();
    for ( ; !it.AtEnd(); it.Advance()) {
      out->insert(std::make_pair(it.Key(), brillo::Any(it.value())));
    }
  }
  {
    ReadablePropertyConstIterator<uint8_t> it = GetUint8PropertiesIter();
    for ( ; !it.AtEnd(); it.Advance()) {
      out->insert(std::make_pair(it.Key(), brillo::Any(it.value())));
    }
  }
  {
    ReadablePropertyConstIterator<ByteArray> it = GetByteArrayPropertiesIter();
    for ( ; !it.AtEnd(); it.Advance()) {
      out->insert(std::make_pair(it.Key(), brillo::Any(it.value())));
    }
  }
  {
    ReadablePropertyConstIterator<uint16_t> it = GetUint16PropertiesIter();
    for ( ; !it.AtEnd(); it.Advance()) {
      out->insert(std::make_pair(it.Key(), brillo::Any(it.value())));
    }
  }
  {
    ReadablePropertyConstIterator<Uint16s> it = GetUint16sPropertiesIter();
    for ( ; !it.AtEnd(); it.Advance()) {
      out->insert(std::make_pair(it.Key(), brillo::Any(it.value())));
    }
  }
  {
    ReadablePropertyConstIterator<uint32_t> it = GetUint32PropertiesIter();
    for ( ; !it.AtEnd(); it.Advance()) {
      out->insert(std::make_pair(it.Key(), brillo::Any(it.value())));
    }
  }
  {
    ReadablePropertyConstIterator<uint64_t> it = GetUint64PropertiesIter();
    for ( ; !it.AtEnd(); it.Advance()) {
      out->insert(std::make_pair(it.Key(), brillo::Any(it.value())));
    }
  }
  {
    ReadablePropertyConstIterator<KeyValueStore> it =
        GetKeyValueStorePropertiesIter();
    for ( ; !it.AtEnd(); it.Advance()) {
      brillo::VariantDictionary dict;
      KeyValueStore::ConvertToVariantDictionary(it.value(), &dict);
      out->insert(std::make_pair(it.Key(), dict));
    }
  }

  return true;
}

bool PropertyStore::GetBoolProperty(const string& name,
                                    bool* value,
                                    Error* error) const {
  return GetProperty(name, value, error, bool_properties_, "a bool");
}

bool PropertyStore::GetInt16Property(const string& name,
                                     int16_t* value,
                                     Error* error) const {
  return GetProperty(name, value, error, int16_properties_, "an int16_t");
}

bool PropertyStore::GetInt32Property(const string& name,
                                     int32_t* value,
                                     Error* error) const {
  return GetProperty(name, value, error, int32_properties_, "an int32_t");
}

bool PropertyStore::GetKeyValueStoreProperty(const string& name,
                                             KeyValueStore* value,
                                             Error* error) const {
  return GetProperty(name, value, error, key_value_store_properties_,
                     "a key value store");
}

bool PropertyStore::GetRpcIdentifierProperty(const string& name,
                                             RpcIdentifier* value,
                                             Error* error) const {
  return GetProperty(name, value, error, rpc_identifier_properties_,
                     "an rpc_identifier");
}

bool PropertyStore::GetStringProperty(const string& name,
                                      string* value,
                                      Error* error) const {
  return GetProperty(name, value, error, string_properties_, "a string");
}

bool PropertyStore::GetStringmapProperty(const string& name,
                                         Stringmap* values,
                                         Error* error) const {
  return GetProperty(name, values, error, stringmap_properties_,
                     "a string map");
}

bool PropertyStore::GetStringmapsProperty(const string& name,
                                          Stringmaps* values,
                                          Error* error) const {
  return GetProperty(name, values, error, stringmaps_properties_,
                     "a string map list");
}

bool PropertyStore::GetStringsProperty(const string& name,
                                       Strings* values,
                                       Error* error) const {
  return GetProperty(name, values, error, strings_properties_, "a string list");
}

bool PropertyStore::GetUint8Property(const string& name,
                                     uint8_t* value,
                                     Error* error) const {
  return GetProperty(name, value, error, uint8_properties_, "a uint8_t");
}

bool PropertyStore::GetByteArrayProperty(const string& name,
                                         ByteArray* value,
                                         Error *error) const {
  return GetProperty(name, value, error, bytearray_properties_, "a byte array");
}

bool PropertyStore::GetUint16Property(const string& name,
                                      uint16_t* value,
                                      Error* error) const {
  return GetProperty(name, value, error, uint16_properties_, "a uint16_t");
}

bool PropertyStore::GetUint16sProperty(const string& name,
                                       Uint16s* value,
                                       Error* error) const {
  return GetProperty(name, value, error, uint16s_properties_,
                     "a uint16_t list");
}

bool PropertyStore::GetUint32Property(const string& name,
                                      uint32_t* value,
                                      Error* error) const {
  return GetProperty(name, value, error, uint32_properties_, "a uint32_t");
}

bool PropertyStore::GetUint64Property(const string& name,
                                      uint64_t* value,
                                      Error* error) const {
  return GetProperty(name, value, error, uint64_properties_, "a uint64_t");
}

bool PropertyStore::SetBoolProperty(const string& name,
                                    bool value,
                                    Error* error) {
  return SetProperty(name, value, error, &bool_properties_, "a bool");
}

bool PropertyStore::SetInt16Property(const string& name,
                                     int16_t value,
                                     Error* error) {
  return SetProperty(name, value, error, &int16_properties_, "an int16_t");
}

bool PropertyStore::SetInt32Property(const string& name,
                                     int32_t value,
                                     Error* error) {
  return SetProperty(name, value, error, &int32_properties_, "an int32_t.");
}

bool PropertyStore::SetKeyValueStoreProperty(const string& name,
                                             const KeyValueStore& value,
                                             Error* error) {
  return SetProperty(name, value, error, &key_value_store_properties_,
                     "a key value store");
}

bool PropertyStore::SetStringProperty(const string& name,
                                      const string& value,
                                      Error* error) {
  return SetProperty(name, value, error, &string_properties_, "a string");
}

bool PropertyStore::SetStringmapProperty(const string& name,
                                         const map<string, string>& values,
                                         Error* error) {
  return SetProperty(name, values, error, &stringmap_properties_,
                     "a string map");
}

bool PropertyStore::SetStringmapsProperty(
    const string& name,
    const vector<map<string, string>>& values,
    Error* error) {
  return SetProperty(name, values, error, &stringmaps_properties_,
                     "a stringmaps");
}

bool PropertyStore::SetStringsProperty(const string& name,
                                       const vector<string>& values,
                                       Error* error) {
  return SetProperty(name, values, error, &strings_properties_,
                     "a string list");
}

bool PropertyStore::SetUint8Property(const string& name,
                                     uint8_t value,
                                     Error* error) {
  return SetProperty(name, value, error, &uint8_properties_, "a uint8_t");
}

bool PropertyStore::SetByteArrayProperty(const string& name,
                                         const ByteArray& value,
                                         Error *error) {
  return SetProperty(name, value, error, &bytearray_properties_, "a byte array");
}

bool PropertyStore::SetUint16Property(const string& name,
                                      uint16_t value,
                                      Error* error) {
  return SetProperty(name, value, error, &uint16_properties_, "a uint16_t");
}

bool PropertyStore::SetUint16sProperty(const string& name,
                                       const vector<uint16_t>& value,
                                       Error* error) {
  return SetProperty(name, value, error, &uint16s_properties_,
                     "a uint16_t list");
}

bool PropertyStore::SetUint32Property(const string& name,
                                      uint32_t value,
                                      Error* error) {
  return SetProperty(name, value, error, &uint32_properties_, "a uint32_t");
}

bool PropertyStore::SetUint64Property(const string& name,
                                      uint64_t value,
                                      Error* error) {
  return SetProperty(name, value, error, &uint64_properties_, "a uint64_t");
}

bool PropertyStore::SetRpcIdentifierProperty(const string& name,
                                             const RpcIdentifier& value,
                                             Error* error) {
  return SetProperty(name, value, error, &rpc_identifier_properties_,
                     "an rpc_identifier");
}

bool PropertyStore::ClearProperty(const string& name, Error* error) {
  SLOG(this, 2) << "Clearing " << name << ".";

  if (ContainsKey(bool_properties_, name)) {
    bool_properties_[name]->Clear(error);
  } else if (ContainsKey(int16_properties_, name)) {
    int16_properties_[name]->Clear(error);
  } else if (ContainsKey(int32_properties_, name)) {
    int32_properties_[name]->Clear(error);
  } else if (ContainsKey(key_value_store_properties_, name)) {
    key_value_store_properties_[name]->Clear(error);
  } else if (ContainsKey(string_properties_, name)) {
    string_properties_[name]->Clear(error);
  } else if (ContainsKey(stringmap_properties_, name)) {
    stringmap_properties_[name]->Clear(error);
  } else if (ContainsKey(stringmaps_properties_, name)) {
    stringmaps_properties_[name]->Clear(error);
  } else if (ContainsKey(strings_properties_, name)) {
    strings_properties_[name]->Clear(error);
  } else if (ContainsKey(uint8_properties_, name)) {
    uint8_properties_[name]->Clear(error);
  } else if (ContainsKey(uint16_properties_, name)) {
    uint16_properties_[name]->Clear(error);
  } else if (ContainsKey(uint16s_properties_, name)) {
    uint16s_properties_[name]->Clear(error);
  } else if (ContainsKey(uint32_properties_, name)) {
    uint32_properties_[name]->Clear(error);
  } else if (ContainsKey(uint64_properties_, name)) {
    uint64_properties_[name]->Clear(error);
  } else if (ContainsKey(rpc_identifier_properties_, name)) {
    rpc_identifier_properties_[name]->Clear(error);
  } else if (ContainsKey(rpc_identifiers_properties_, name)) {
    rpc_identifiers_properties_[name]->Clear(error);
  } else {
    error->Populate(
        Error::kInvalidProperty, "Property " + name + " does not exist.");
  }
  if (error->IsSuccess()) {
    if (!property_changed_callback_.is_null()) {
      property_changed_callback_.Run(name);
    }
  }
  return error->IsSuccess();
}

ReadablePropertyConstIterator<bool> PropertyStore::GetBoolPropertiesIter()
    const {
  return ReadablePropertyConstIterator<bool>(bool_properties_);
}

ReadablePropertyConstIterator<int16_t> PropertyStore::GetInt16PropertiesIter()
    const {
  return ReadablePropertyConstIterator<int16_t>(int16_properties_);
}

ReadablePropertyConstIterator<int32_t> PropertyStore::GetInt32PropertiesIter()
    const {
  return ReadablePropertyConstIterator<int32_t>(int32_properties_);
}

ReadablePropertyConstIterator<KeyValueStore>
PropertyStore::GetKeyValueStorePropertiesIter() const {
  return
      ReadablePropertyConstIterator<KeyValueStore>(key_value_store_properties_);
}

ReadablePropertyConstIterator<RpcIdentifier>
PropertyStore::GetRpcIdentifierPropertiesIter() const {
  return ReadablePropertyConstIterator<RpcIdentifier>(
      rpc_identifier_properties_);
}

ReadablePropertyConstIterator<RpcIdentifiers>
PropertyStore::GetRpcIdentifiersPropertiesIter() const {
  return ReadablePropertyConstIterator<RpcIdentifiers>(
      rpc_identifiers_properties_);
}

ReadablePropertyConstIterator<string>
PropertyStore::GetStringPropertiesIter() const {
  return ReadablePropertyConstIterator<string>(string_properties_);
}

ReadablePropertyConstIterator<Stringmap>
PropertyStore::GetStringmapPropertiesIter() const {
  return ReadablePropertyConstIterator<Stringmap>(stringmap_properties_);
}

ReadablePropertyConstIterator<Stringmaps>
PropertyStore::GetStringmapsPropertiesIter()
    const {
  return ReadablePropertyConstIterator<Stringmaps>(stringmaps_properties_);
}

ReadablePropertyConstIterator<Strings> PropertyStore::GetStringsPropertiesIter()
    const {
  return ReadablePropertyConstIterator<Strings>(strings_properties_);
}

ReadablePropertyConstIterator<uint8_t> PropertyStore::GetUint8PropertiesIter()
    const {
  return ReadablePropertyConstIterator<uint8_t>(uint8_properties_);
}

ReadablePropertyConstIterator<ByteArray> PropertyStore::GetByteArrayPropertiesIter()
    const {
  return ReadablePropertyConstIterator<ByteArray>(bytearray_properties_);
}

ReadablePropertyConstIterator<uint16_t> PropertyStore::GetUint16PropertiesIter()
    const {
  return ReadablePropertyConstIterator<uint16_t>(uint16_properties_);
}

ReadablePropertyConstIterator<Uint16s> PropertyStore::GetUint16sPropertiesIter()
    const {
  return ReadablePropertyConstIterator<Uint16s>(uint16s_properties_);
}

ReadablePropertyConstIterator<uint32_t> PropertyStore::GetUint32PropertiesIter()
    const {
  return ReadablePropertyConstIterator<uint32_t>(uint32_properties_);
}

ReadablePropertyConstIterator<uint64_t> PropertyStore::GetUint64PropertiesIter()
    const {
  return ReadablePropertyConstIterator<uint64_t>(uint64_properties_);
}

void PropertyStore::RegisterBool(const string& name, bool* prop) {
  DCHECK(!Contains(name) || ContainsKey(bool_properties_, name))
      << "(Already registered " << name << ")";
  bool_properties_[name] = BoolAccessor(new PropertyAccessor<bool>(prop));
}

void PropertyStore::RegisterConstBool(const string& name, const bool* prop) {
  DCHECK(!Contains(name) || ContainsKey(bool_properties_, name))
      << "(Already registered " << name << ")";
  bool_properties_[name] = BoolAccessor(new ConstPropertyAccessor<bool>(prop));
}

void PropertyStore::RegisterWriteOnlyBool(const string& name, bool* prop) {
  DCHECK(!Contains(name) || ContainsKey(bool_properties_, name))
      << "(Already registered " << name << ")";
  bool_properties_[name] = BoolAccessor(
      new WriteOnlyPropertyAccessor<bool>(prop));
}

void PropertyStore::RegisterInt16(const string& name, int16_t* prop) {
  DCHECK(!Contains(name) || ContainsKey(int16_properties_, name))
      << "(Already registered " << name << ")";
  int16_properties_[name] = Int16Accessor(new PropertyAccessor<int16_t>(prop));
}

void PropertyStore::RegisterConstInt16(const string& name,
                                       const int16_t* prop) {
  DCHECK(!Contains(name) || ContainsKey(int16_properties_, name))
      << "(Already registered " << name << ")";
  int16_properties_[name] =
      Int16Accessor(new ConstPropertyAccessor<int16_t>(prop));
}

void PropertyStore::RegisterWriteOnlyInt16(const string& name, int16_t* prop) {
  DCHECK(!Contains(name) || ContainsKey(int16_properties_, name))
      << "(Already registered " << name << ")";
  int16_properties_[name] =
      Int16Accessor(new WriteOnlyPropertyAccessor<int16_t>(prop));
}
void PropertyStore::RegisterInt32(const string& name, int32_t* prop) {
  DCHECK(!Contains(name) || ContainsKey(int32_properties_, name))
      << "(Already registered " << name << ")";
  int32_properties_[name] = Int32Accessor(new PropertyAccessor<int32_t>(prop));
}

void PropertyStore::RegisterConstInt32(const string& name,
                                       const int32_t* prop) {
  DCHECK(!Contains(name) || ContainsKey(int32_properties_, name))
      << "(Already registered " << name << ")";
  int32_properties_[name] =
      Int32Accessor(new ConstPropertyAccessor<int32_t>(prop));
}

void PropertyStore::RegisterWriteOnlyInt32(const string& name, int32_t* prop) {
  DCHECK(!Contains(name) || ContainsKey(int32_properties_, name))
      << "(Already registered " << name << ")";
  int32_properties_[name] =
      Int32Accessor(new WriteOnlyPropertyAccessor<int32_t>(prop));
}

void PropertyStore::RegisterString(const string& name, string* prop) {
  DCHECK(!Contains(name) || ContainsKey(string_properties_, name))
      << "(Already registered " << name << ")";
  string_properties_[name] = StringAccessor(new PropertyAccessor<string>(prop));
}

void PropertyStore::RegisterConstString(const string& name,
                                        const string* prop) {
  DCHECK(!Contains(name) || ContainsKey(string_properties_, name))
      << "(Already registered " << name << ")";
  string_properties_[name] =
      StringAccessor(new ConstPropertyAccessor<string>(prop));
}

void PropertyStore::RegisterWriteOnlyString(const string& name, string* prop) {
  DCHECK(!Contains(name) || ContainsKey(string_properties_, name))
      << "(Already registered " << name << ")";
  string_properties_[name] =
      StringAccessor(new WriteOnlyPropertyAccessor<string>(prop));
}

void PropertyStore::RegisterStringmap(const string& name, Stringmap* prop) {
  DCHECK(!Contains(name) || ContainsKey(stringmap_properties_, name))
      << "(Already registered " << name << ")";
  stringmap_properties_[name] =
      StringmapAccessor(new PropertyAccessor<Stringmap>(prop));
}

void PropertyStore::RegisterConstStringmap(const string& name,
                                           const Stringmap* prop) {
  DCHECK(!Contains(name) || ContainsKey(stringmap_properties_, name))
      << "(Already registered " << name << ")";
  stringmap_properties_[name] =
      StringmapAccessor(new ConstPropertyAccessor<Stringmap>(prop));
}

void PropertyStore::RegisterWriteOnlyStringmap(const string& name,
                                               Stringmap* prop) {
  DCHECK(!Contains(name) || ContainsKey(stringmap_properties_, name))
      << "(Already registered " << name << ")";
  stringmap_properties_[name] =
      StringmapAccessor(new WriteOnlyPropertyAccessor<Stringmap>(prop));
}

void PropertyStore::RegisterStringmaps(const string& name, Stringmaps* prop) {
  DCHECK(!Contains(name) || ContainsKey(stringmaps_properties_, name))
      << "(Already registered " << name << ")";
  stringmaps_properties_[name] =
      StringmapsAccessor(new PropertyAccessor<Stringmaps>(prop));
}

void PropertyStore::RegisterConstStringmaps(const string& name,
                                            const Stringmaps* prop) {
  DCHECK(!Contains(name) || ContainsKey(stringmaps_properties_, name))
      << "(Already registered " << name << ")";
  stringmaps_properties_[name] =
      StringmapsAccessor(new ConstPropertyAccessor<Stringmaps>(prop));
}

void PropertyStore::RegisterWriteOnlyStringmaps(const string& name,
                                                Stringmaps* prop) {
  DCHECK(!Contains(name) || ContainsKey(stringmaps_properties_, name))
      << "(Already registered " << name << ")";
  stringmaps_properties_[name] =
      StringmapsAccessor(new WriteOnlyPropertyAccessor<Stringmaps>(prop));
}

void PropertyStore::RegisterStrings(const string& name, Strings* prop) {
  DCHECK(!Contains(name) || ContainsKey(strings_properties_, name))
      << "(Already registered " << name << ")";
  strings_properties_[name] =
      StringsAccessor(new PropertyAccessor<Strings>(prop));
}

void PropertyStore::RegisterConstStrings(const string& name,
                                         const Strings* prop) {
  DCHECK(!Contains(name) || ContainsKey(strings_properties_, name))
      << "(Already registered " << name << ")";
  strings_properties_[name] =
      StringsAccessor(new ConstPropertyAccessor<Strings>(prop));
}

void PropertyStore::RegisterWriteOnlyStrings(const string& name,
                                             Strings* prop) {
  DCHECK(!Contains(name) || ContainsKey(strings_properties_, name))
      << "(Already registered " << name << ")";
  strings_properties_[name] =
      StringsAccessor(new WriteOnlyPropertyAccessor<Strings>(prop));
}

void PropertyStore::RegisterUint8(const string& name, uint8_t* prop) {
  DCHECK(!Contains(name) || ContainsKey(uint8_properties_, name))
      << "(Already registered " << name << ")";
  uint8_properties_[name] = Uint8Accessor(new PropertyAccessor<uint8_t>(prop));
}

void PropertyStore::RegisterConstUint8(const string& name,
                                       const uint8_t* prop) {
  DCHECK(!Contains(name) || ContainsKey(uint8_properties_, name))
      << "(Already registered " << name << ")";
  uint8_properties_[name] =
      Uint8Accessor(new ConstPropertyAccessor<uint8_t>(prop));
}

void PropertyStore::RegisterWriteOnlyUint8(const string& name, uint8_t* prop) {
  DCHECK(!Contains(name) || ContainsKey(uint8_properties_, name))
      << "(Already registered " << name << ")";
  uint8_properties_[name] =
      Uint8Accessor(new WriteOnlyPropertyAccessor<uint8_t>(prop));
}

void PropertyStore::RegisterByteArray(const string& name, ByteArray* prop) {
  DCHECK(!Contains(name) || ContainsKey(bytearray_properties_, name))
      << "(Already registered " << name << ")";
  bytearray_properties_[name] =
      ByteArrayAccessor(new PropertyAccessor<ByteArray>(prop));
}

void PropertyStore::RegisterConstByteArray(const string& name,
                                           const ByteArray* prop) {
  DCHECK(!Contains(name) || ContainsKey(bytearray_properties_, name))
      << "(Already registered " << name << ")";
  bytearray_properties_[name] =
      ByteArrayAccessor(new ConstPropertyAccessor<ByteArray>(prop));
}

void PropertyStore::RegisterWriteOnlyByteArray(const string& name,
                                               ByteArray* prop) {
  DCHECK(!Contains(name) || ContainsKey(bytearray_properties_, name))
      << "(Already registered " << name << ")";
  bytearray_properties_[name] =
      ByteArrayAccessor(new WriteOnlyPropertyAccessor<ByteArray>(prop));
}

void PropertyStore::RegisterUint16(const string& name, uint16_t* prop) {
  DCHECK(!Contains(name) || ContainsKey(uint16_properties_, name))
      << "(Already registered " << name << ")";
  uint16_properties_[name] =
      Uint16Accessor(new PropertyAccessor<uint16_t>(prop));
}

void PropertyStore::RegisterUint16s(const string& name, Uint16s* prop) {
  DCHECK(!Contains(name) || ContainsKey(uint16s_properties_, name))
      << "(Already registered " << name << ")";
  uint16s_properties_[name] =
      Uint16sAccessor(new PropertyAccessor<Uint16s>(prop));
}

void PropertyStore::RegisterUint32(const std::string& name, uint32_t* prop) {
  DCHECK(!Contains(name) || ContainsKey(uint32_properties_, name))
      << "(Already registered " << name << ")";
  uint32_properties_[name] =
      Uint32Accessor(new PropertyAccessor<uint32_t>(prop));
}

void PropertyStore::RegisterConstUint32(const string& name,
                                        const uint32_t* prop) {
  DCHECK(!Contains(name) || ContainsKey(uint32_properties_, name))
      << "(Already registered " << name << ")";
  uint32_properties_[name] =
      Uint32Accessor(new ConstPropertyAccessor<uint32_t>(prop));
}

void PropertyStore::RegisterConstUint16(const string& name,
                                        const uint16_t* prop) {
  DCHECK(!Contains(name) || ContainsKey(uint16_properties_, name))
      << "(Already registered " << name << ")";
  uint16_properties_[name] =
      Uint16Accessor(new ConstPropertyAccessor<uint16_t>(prop));
}

void PropertyStore::RegisterConstUint16s(const string& name,
                                         const Uint16s* prop) {
  DCHECK(!Contains(name) || ContainsKey(uint16s_properties_, name))
      << "(Already registered " << name << ")";
  uint16s_properties_[name] =
      Uint16sAccessor(new ConstPropertyAccessor<Uint16s>(prop));
}

void PropertyStore::RegisterWriteOnlyUint16(const string& name,
                                            uint16_t* prop) {
  DCHECK(!Contains(name) || ContainsKey(uint16_properties_, name))
      << "(Already registered " << name << ")";
  uint16_properties_[name] =
      Uint16Accessor(new WriteOnlyPropertyAccessor<uint16_t>(prop));
}

void PropertyStore::RegisterDerivedBool(const string& name,
                                        const BoolAccessor& accessor) {
  DCHECK(!Contains(name) || ContainsKey(bool_properties_, name))
      << "(Already registered " << name << ")";
  bool_properties_[name] = accessor;
}

void PropertyStore::RegisterDerivedInt32(const string& name,
                                         const Int32Accessor& accessor) {
  DCHECK(!Contains(name) || ContainsKey(int32_properties_, name))
      << "(Already registered " << name << ")";
  int32_properties_[name] = accessor;
}

void PropertyStore::RegisterDerivedKeyValueStore(
    const string& name,
    const KeyValueStoreAccessor& acc) {
  DCHECK(!Contains(name) || ContainsKey(key_value_store_properties_, name))
      << "(Already registered " << name << ")";
  key_value_store_properties_[name] = acc;
}

void PropertyStore::RegisterDerivedRpcIdentifier(
    const string& name,
    const RpcIdentifierAccessor& acc) {
  DCHECK(!Contains(name) || ContainsKey(rpc_identifier_properties_, name))
      << "(Already registered " << name << ")";
  rpc_identifier_properties_[name] = acc;
}

void PropertyStore::RegisterDerivedRpcIdentifiers(
    const string& name,
    const RpcIdentifiersAccessor& accessor) {
  DCHECK(!Contains(name) || ContainsKey(rpc_identifiers_properties_, name))
      << "(Already registered " << name << ")";
  rpc_identifiers_properties_[name] = accessor;
}

void PropertyStore::RegisterDerivedString(const string& name,
                                          const StringAccessor& accessor) {
  DCHECK(!Contains(name) || ContainsKey(string_properties_, name))
      << "(Already registered " << name << ")";
  string_properties_[name] = accessor;
}

void PropertyStore::RegisterDerivedStrings(const string& name,
                                           const StringsAccessor& accessor) {
  DCHECK(!Contains(name) || ContainsKey(strings_properties_, name))
      << "(Already registered " << name << ")";
  strings_properties_[name] = accessor;
}

void PropertyStore::RegisterDerivedStringmap(const string& name,
                                             const StringmapAccessor& acc) {
  DCHECK(!Contains(name) || ContainsKey(stringmap_properties_, name))
      << "(Already registered " << name << ")";
  stringmap_properties_[name] = acc;
}

void PropertyStore::RegisterDerivedStringmaps(const string& name,
                                              const StringmapsAccessor& acc) {
  DCHECK(!Contains(name) || ContainsKey(stringmaps_properties_, name))
      << "(Already registered " << name << ")";
  stringmaps_properties_[name] = acc;
}

void PropertyStore::RegisterDerivedUint16(const string& name,
                                          const Uint16Accessor& acc) {
  DCHECK(!Contains(name) || ContainsKey(uint16_properties_, name))
      << "(Already registered " << name << ")";
  uint16_properties_[name] = acc;
}

void PropertyStore::RegisterDerivedUint64(const string& name,
                                          const Uint64Accessor& acc) {
  DCHECK(!Contains(name) || ContainsKey(uint64_properties_, name))
      << "(Already registered " << name << ")";
  uint64_properties_[name] = acc;
}

void PropertyStore::RegisterDerivedByteArray(const string& name,
                                             const ByteArrayAccessor& acc) {
  DCHECK(!Contains(name) || ContainsKey(bytearray_properties_, name))
      << "(Already registered " << name << ")";
  bytearray_properties_[name] = acc;
}

// private methods

template <class V>
bool PropertyStore::GetProperty(
    const string& name,
    V* value,
    Error* error,
    const map<string, std::shared_ptr<AccessorInterface<V>>>& collection,
    const string& value_type_english) const {
  SLOG(this, 2) << "Getting " << name << " as " << value_type_english
                << ".";
  typename map<string, std::shared_ptr<AccessorInterface<V>>>::const_iterator
      it = collection.find(name);
  if (it != collection.end()) {
    V val = it->second->Get(error);
    if (error->IsSuccess()) {
      *value = val;
    }
  } else {
    if (Contains(name)) {
      error->Populate(
          Error::kInvalidArguments,
          "Property " + name + " is not " + value_type_english + ".");
    } else {
      error->Populate(
          Error::kInvalidProperty, "Property " + name + " does not exist.");
    }
  }
  return error->IsSuccess();
}

template <class V>
bool PropertyStore::SetProperty(
    const string& name,
    const V& value,
    Error* error,
    map<string, std::shared_ptr<AccessorInterface<V>>>* collection,
    const string& value_type_english) {
  bool ret = false;
  SLOG(this, 2) << "Setting " << name << " as " << value_type_english
                << ".";
  if (ContainsKey(*collection, name)) {
    ret = (*collection)[name]->Set(value, error);
    if (ret) {
      if (!property_changed_callback_.is_null()) {
        property_changed_callback_.Run(name);
      }
    }
  } else {
    if (Contains(name)) {
      error->Populate(
          Error::kInvalidArguments,
          "Property " + name + " is not " + value_type_english + ".");
    } else {
      error->Populate(
          Error::kInvalidProperty, "Property " + name + " does not exist.");
    }
  }
  return ret;
}

}  // namespace shill
