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

#include "proxy_util.h"

namespace {
template<typename VectorType> void GetXmlRpcArrayFromVector(
    const VectorType& vector_in,
    XmlRpc::XmlRpcValue* xml_rpc_value_out) {
  if (vector_in.empty()) {
    xml_rpc_value_out->setToNil();
    return;
  }
  int i = 0;
  for (const auto& value : vector_in) {
    (*xml_rpc_value_out)[++i] = value;
  }
}

void GetXmlRpcStructFromStringMap(
    const std::map<std::string, std::string>& string_map_in,
    XmlRpc::XmlRpcValue* xml_rpc_value_out) {
  if (string_map_in.empty()) {
    xml_rpc_value_out->setToNil();
    return;
  }
  for (const auto& value : string_map_in) {
    (*xml_rpc_value_out)[value.first] = value.second;
  }
}

void GetXmlRpcStructFromBrilloVariantDictionary(
    const brillo::VariantDictionary& var_dict_in,
    XmlRpc::XmlRpcValue* xml_rpc_value_out) {
  if (var_dict_in.empty()) {
    xml_rpc_value_out->setToNil();
    return;
  }
  for (const auto& value : var_dict_in) {
    XmlRpc::XmlRpcValue tmp_value;
    GetXmlRpcValueFromBrilloAnyValue(value.second, &tmp_value);
    (*xml_rpc_value_out)[value.first] = tmp_value;
  }
}

template<typename ElementType> void GetVectorFromXmlRpcArray(
    XmlRpc::XmlRpcValue* xml_rpc_value_in,
    std::vector<ElementType>* vector_out) {
  int array_size = xml_rpc_value_in->size();
  for (int i = 0; i < array_size; ++i) {
    vector_out->push_back(static_cast<ElementType>((*xml_rpc_value_in)[i]));
  }
}

void GetBrilloAnyVectorFromXmlRpcArray(
    XmlRpc::XmlRpcValue* xml_rpc_value_in,
    brillo::Any* any_value_out) {
  int array_size = xml_rpc_value_in->size();
  if (!array_size) {
    any_value_out->Clear();
    return;
  }
  XmlRpc::XmlRpcValue::Type elem_type = (*xml_rpc_value_in)[0].getType();
  for (int i = 0; i < array_size; ++i) {
    CHECK((*xml_rpc_value_in)[i].getType() == elem_type);
  }
  switch (elem_type) {
    case XmlRpc::XmlRpcValue::TypeBoolean: {
        std::vector<bool> bool_vec;
        GetVectorFromXmlRpcArray(xml_rpc_value_in, &bool_vec);
        *any_value_out = bool_vec;
        return;
    }
    case XmlRpc::XmlRpcValue::TypeInt: {
        std::vector<int> int_vec;
        GetVectorFromXmlRpcArray(xml_rpc_value_in, &int_vec);
        *any_value_out = int_vec;
        return;
    }
    case XmlRpc::XmlRpcValue::TypeDouble: {
        std::vector<double> double_vec;
        GetVectorFromXmlRpcArray(xml_rpc_value_in, &double_vec);
        *any_value_out = double_vec;
        return;
    }
    case XmlRpc::XmlRpcValue::TypeString: {
      std::vector<std::string> string_vec;
      GetVectorFromXmlRpcArray(xml_rpc_value_in, &string_vec);
      *any_value_out = string_vec;
      return;
    }
    default:
      LOG(FATAL) << __func__ << ". Unhandled type: "
                 << (*xml_rpc_value_in)[0].getType();
  }
}

template<typename ValueType> XmlRpc::XmlRpcValue::Type GetXmlRpcType();
template<> XmlRpc::XmlRpcValue::Type GetXmlRpcType<bool>() {
  return XmlRpc::XmlRpcValue::TypeBoolean;
}
template<> XmlRpc::XmlRpcValue::Type GetXmlRpcType<int>() {
  return XmlRpc::XmlRpcValue::TypeInt;
}
template<> XmlRpc::XmlRpcValue::Type GetXmlRpcType<double>() {
  return XmlRpc::XmlRpcValue::TypeDouble;
}
template<> XmlRpc::XmlRpcValue::Type GetXmlRpcType<std::string>() {
  return XmlRpc::XmlRpcValue::TypeString;
}

template<typename ValueType> bool IsMemberValuePresent(
    XmlRpc::XmlRpcValue* xml_rpc_value_in,
    const std::string& member_name) {
  if (xml_rpc_value_in->hasMember(member_name) &&
      ((*xml_rpc_value_in)[member_name].getType() ==
       GetXmlRpcType<ValueType>())) {
    return true;
  }
  return false;
}

template<typename ValueType> bool GetValueFromXmlRpcValueStructMember(
    XmlRpc::XmlRpcValue* xml_rpc_value_in,
    const std::string& member_name,
    ValueType default_value,
    ValueType* value_out) {
  if (!IsMemberValuePresent<ValueType>(xml_rpc_value_in, member_name)) {
    *value_out = default_value;
    return false;
  }
  *value_out = ValueType((*xml_rpc_value_in)[member_name]);
  return true;
}

template<typename ElementType> bool IsMemberVectorPresent(
    XmlRpc::XmlRpcValue* xml_rpc_value_in,
    const std::string& member_name) {
  if (xml_rpc_value_in->hasMember(member_name) &&
      ((*xml_rpc_value_in)[member_name].getType() ==
       XmlRpc::XmlRpcValue::TypeArray) &&
      ((*xml_rpc_value_in)[member_name][0].getType() ==
       GetXmlRpcType<ElementType>())) {
    return true;
  }
  return false;
}

template<typename ElementType> bool GetVectorFromXmlRpcValueStructMember(
    XmlRpc::XmlRpcValue* xml_rpc_value_in,
    const std::string& member_name,
    std::vector<ElementType> default_value,
    std::vector<ElementType>* value_out) {
  if (!IsMemberVectorPresent<ElementType>(xml_rpc_value_in, member_name)) {
    *value_out = default_value;
    return false;
  }
  XmlRpc::XmlRpcValue& xml_rpc_member_array = (*xml_rpc_value_in)[member_name];
  int array_size = xml_rpc_member_array.size();
  for (int array_pos = 0; array_pos < array_size; ++array_pos) {
    value_out->push_back(ElementType(xml_rpc_member_array[array_pos]));
  }
  return true;
}
} // namespace

void GetXmlRpcValueFromBrilloAnyValue(
    const brillo::Any& any_value_in,
    XmlRpc::XmlRpcValue* xml_rpc_value_out) {
  if (any_value_in.IsTypeCompatible<bool>()) {
    *xml_rpc_value_out =  any_value_in.Get<bool>();
    return;
  }
  if (any_value_in.IsTypeCompatible<uint8_t>()) {
    *xml_rpc_value_out =  any_value_in.Get<uint8_t>();
    return;
  }
  if (any_value_in.IsTypeCompatible<uint16_t>()) {
    *xml_rpc_value_out =  any_value_in.Get<uint16_t>();
    return;
  }
  if (any_value_in.IsTypeCompatible<int>()) {
    *xml_rpc_value_out =  any_value_in.Get<int>();
    return;
  }
  if (any_value_in.IsTypeCompatible<double>()) {
    *xml_rpc_value_out =  any_value_in.Get<double>();
    return;
  }
  if (any_value_in.IsTypeCompatible<std::string>()) {
    *xml_rpc_value_out =  any_value_in.Get<std::string>();
    return;
  }
  if (any_value_in.IsTypeCompatible<dbus::ObjectPath>()) {
    *xml_rpc_value_out =  any_value_in.Get<dbus::ObjectPath>().value();
    return;
  }
  if (any_value_in.IsTypeCompatible<std::vector<bool>>()) {
    GetXmlRpcArrayFromVector(
        any_value_in.Get<std::vector<bool>>(), xml_rpc_value_out);
    return;
  }
  if (any_value_in.IsTypeCompatible<std::vector<uint8_t>>()) {
    GetXmlRpcArrayFromVector(
        any_value_in.Get<std::vector<uint8_t>>(), xml_rpc_value_out);
    return;
  }
  if (any_value_in.IsTypeCompatible<std::vector<uint16_t>>()) {
    GetXmlRpcArrayFromVector(
        any_value_in.Get<std::vector<uint16_t>>(), xml_rpc_value_out);
    return;
  }
  if (any_value_in.IsTypeCompatible<std::vector<int>>()) {
    GetXmlRpcArrayFromVector(
        any_value_in.Get<std::vector<int>>(), xml_rpc_value_out);
    return;
  }
  if (any_value_in.IsTypeCompatible<std::vector<double>>()) {
    GetXmlRpcArrayFromVector(
        any_value_in.Get<std::vector<double>>(), xml_rpc_value_out);
    return;
  }
  if (any_value_in.IsTypeCompatible<std::vector<std::string>>()) {
    GetXmlRpcArrayFromVector(
        any_value_in.Get<std::vector<std::string>>(), xml_rpc_value_out);
    return;
  }
  if (any_value_in.IsTypeCompatible<std::vector<dbus::ObjectPath>>()) {
    std::vector<std::string> string_vec;
    for (const auto& object : any_value_in.Get<std::vector<dbus::ObjectPath>>()) {
      string_vec.push_back(object.value());
    }
    GetXmlRpcArrayFromVector(string_vec, xml_rpc_value_out);
    return;
  }
  if (any_value_in.IsTypeCompatible<std::map<std::string, std::string>>()) {
    GetXmlRpcStructFromStringMap(
        any_value_in.Get<std::map<std::string, std::string>>(), xml_rpc_value_out);
    return;
  }
  if (any_value_in.IsTypeCompatible<brillo::VariantDictionary>()) {
    GetXmlRpcStructFromBrilloVariantDictionary(
        any_value_in.Get<brillo::VariantDictionary>(), xml_rpc_value_out);
    return;
  }
  LOG(FATAL) << __func__ << ". Unhandled type: "
             << any_value_in.GetUndecoratedTypeName();
}

void GetBrilloAnyValueFromXmlRpcValue(
    XmlRpc::XmlRpcValue* xml_rpc_value_in,
    brillo::Any* any_value_out) {
  switch (xml_rpc_value_in->getType()) {
    case XmlRpc::XmlRpcValue::TypeBoolean:
      *any_value_out = static_cast<bool>(*xml_rpc_value_in);
      return;
    case XmlRpc::XmlRpcValue::TypeInt:
      *any_value_out = static_cast<int>(*xml_rpc_value_in);
      return;
    case XmlRpc::XmlRpcValue::TypeDouble:
      *any_value_out = static_cast<double>(*xml_rpc_value_in);
      return;
    case XmlRpc::XmlRpcValue::TypeString:
      *any_value_out = static_cast<std::string>(*xml_rpc_value_in);
      return;
    case XmlRpc::XmlRpcValue::TypeArray:
      GetBrilloAnyVectorFromXmlRpcArray(xml_rpc_value_in, any_value_out);
      return;
    default:
      LOG(FATAL) << __func__ << ". Unhandled type: "
                 << xml_rpc_value_in->getType();
  }
}

bool GetBoolValueFromXmlRpcValueStructMember(
    XmlRpc::XmlRpcValue* xml_rpc_value_in,
    const std::string& member_name,
    bool default_value,
    bool* value_out) {
  return GetValueFromXmlRpcValueStructMember(
      xml_rpc_value_in, member_name, default_value, value_out);
}

bool GetIntValueFromXmlRpcValueStructMember(
    XmlRpc::XmlRpcValue* xml_rpc_value_in,
    const std::string& member_name,
    int default_value,
    int* value_out) {
  return GetValueFromXmlRpcValueStructMember(
      xml_rpc_value_in, member_name, default_value, value_out);
}

bool GetDoubleValueFromXmlRpcValueStructMember(
    XmlRpc::XmlRpcValue* xml_rpc_value_in,
    const std::string& member_name,
    double default_value,
    double* value_out){
  return GetValueFromXmlRpcValueStructMember(
      xml_rpc_value_in, member_name, default_value, value_out);
}

bool GetStringValueFromXmlRpcValueStructMember(
    XmlRpc::XmlRpcValue* xml_rpc_value_in,
    const std::string& member_name,
    const std::string& default_value,
    std::string* value_out) {
  return GetValueFromXmlRpcValueStructMember(
      xml_rpc_value_in, member_name, default_value, value_out);
}

bool GetStringVectorFromXmlRpcValueStructMember(
    XmlRpc::XmlRpcValue* xml_rpc_value_in,
    const std::string& member_name,
    const std::vector<std::string>& default_value,
    std::vector<std::string>* value_out) {
  return GetVectorFromXmlRpcValueStructMember(
      xml_rpc_value_in, member_name, default_value, value_out);
}
