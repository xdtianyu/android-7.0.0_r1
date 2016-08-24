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

#ifndef PROXY_UTIL_H
#define PROXY_UTIL_H

#include <string>
#include <vector>

#include <XmlRpcValue.h>

#include <brillo/any.h>
#include <brillo/variant_dictionary.h>

// Conversion functions to go from XmlRpc::XmlRpcValue to brillo::Any and vice
// versa.
void GetXmlRpcValueFromBrilloAnyValue(
    const brillo::Any& any_value_in,
    XmlRpc::XmlRpcValue* xml_rpc_value_out);
// Note: We assume that all the XmlRpcValue array elements are of the same type
// even though they're not mandated by the Xml Rpc spec.
void GetBrilloAnyValueFromXmlRpcValue(
    XmlRpc::XmlRpcValue* xml_rpc_value_in,
    brillo::Any* any_value_out);
// Functions to access members of structs sent over XmlRpc and need to be converted
// to corresponding objects. Returns |true| if the value is found in the incoming data
// or |false| if the value was taken from the default supplied.
bool GetBoolValueFromXmlRpcValueStructMember(
    XmlRpc::XmlRpcValue* xml_rpc_value_in,
    const std::string& member_name,
    bool default_value,
    bool* value_out);
bool GetIntValueFromXmlRpcValueStructMember(
    XmlRpc::XmlRpcValue* xml_rpc_value_in,
    const std::string& member_name,
    int default_value,
    int* value_out);
bool GetDoubleValueFromXmlRpcValueStructMember(
    XmlRpc::XmlRpcValue* xml_rpc_value_in,
    const std::string& member_name,
    double default_value,
    double* value_out);
bool GetStringValueFromXmlRpcValueStructMember(
    XmlRpc::XmlRpcValue* xml_rpc_value_in,
    const std::string& member_name,
    const std::string& default_value,
    std::string* value_out);
bool GetStringVectorFromXmlRpcValueStructMember(
    XmlRpc::XmlRpcValue* xml_rpc_value_in,
    const std::string& member_name,
    const std::vector<std::string>& default_value,
    std::vector<std::string>* value_out);

inline long GetMillisecondsFromSeconds(int time_seconds) {
  return time_seconds * 1000;
}
inline double GetSecondsFromMilliseconds(long time_milliseconds) {
  return static_cast<double>(time_milliseconds) / 1000;
}
#endif // PROXY_UTIL_H
