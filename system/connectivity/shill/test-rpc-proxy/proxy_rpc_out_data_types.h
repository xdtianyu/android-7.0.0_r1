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

#ifndef PROXY_RPC_OUT_DATA_TYPES_H
#define PROXY_RPC_OUT_DATA_TYPES_H

#include <string>

#include <XmlRpcValue.h>

// Describes the result of an association attempt.
class AssociationResult {
 public:
  AssociationResult(bool success,
                    double discovery_time,
                    double association_time,
                    double configuration_time,
                    const std::string& failure_reason);
  XmlRpc::XmlRpcValue ConvertToXmlRpcValue();

 private:
  bool success_;
  double discovery_time_;
  double association_time_;
  double configuration_time_;
  std::string failure_reason_;
};

#endif // PROXY_RPC_OUT_DATA_TYPES_H
