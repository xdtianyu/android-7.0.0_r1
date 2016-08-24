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

#include "dhcp_client/dhcp_options_parser.h"

#include <netinet/in.h>

#include <string>
#include <utility>
#include <vector>

#include <base/logging.h>
#include <base/macros.h>
#include <shill/net/byte_string.h>

using shill::ByteString;

namespace dhcp_client {

bool UInt8Parser::GetOption(const uint8_t* buffer,
                            uint8_t length,
                            void* value) {
  if (length != sizeof(uint8_t)) {
    LOG(ERROR) << "Invalid option length field";
    return false;
  }
  uint8_t* value_uint8 = static_cast<uint8_t*>(value);
  *value_uint8 = *buffer;
  return true;
}

bool UInt16Parser::GetOption(const uint8_t* buffer,
                             uint8_t length,
                             void* value) {
  if (length != sizeof(uint16_t)) {
    LOG(ERROR) << "Invalid option length field";
    return false;
  }
  uint16_t* value_uint16 = static_cast<uint16_t*>(value);
  *value_uint16 = ntohs(*reinterpret_cast<const uint16_t*>(buffer));
  return true;
}

bool UInt32Parser::GetOption(const uint8_t* buffer,
                             uint8_t length,
                             void* value) {
  if (length != sizeof(uint32_t)) {
    LOG(ERROR) << "Invalid option length field";
    return false;
  }
  uint32_t* value_uint32 = static_cast<uint32_t*>(value);
  *value_uint32 = ntohl(*reinterpret_cast<const uint32_t*>(buffer));
  return true;
}

bool UInt8ListParser::GetOption(const uint8_t* buffer,
                                uint8_t length,
                                void* value) {
  if (length == 0) {
    LOG(ERROR) << "Invalid option length field";
    return false;
  }
  std::vector<uint8_t>* value_vector =
      static_cast<std::vector<uint8_t>*>(value);
  for (int i = 0; i < length; i++) {
    uint8_t content = *reinterpret_cast<const uint8_t*>(buffer);
    value_vector->push_back(content);
    buffer += sizeof(uint8_t);
  }
  return true;
}

bool UInt16ListParser::GetOption(const uint8_t* buffer,
                                 uint8_t length,
                                 void* value) {
  if (length == 0 || length % sizeof(uint16_t)) {
    LOG(ERROR) << "Invalid option length field";
    return false;
  }
  int num_int16s = length / sizeof(uint16_t);
  std::vector<uint16_t>* value_vector =
      static_cast<std::vector<uint16_t>*>(value);
  for (int i = 0; i < num_int16s; i++) {
    uint16_t content = *reinterpret_cast<const uint16_t*>(buffer);
    content = ntohs(content);
    value_vector->push_back(content);
    buffer += sizeof(uint16_t);
  }
  return true;
}

bool UInt32ListParser::GetOption(const uint8_t* buffer,
                                 uint8_t length,
                                 void* value) {
  if (length == 0 || length % sizeof(uint32_t)) {
    LOG(ERROR) << "Invalid option length field";
    return false;
  }
  int num_int32s = length / sizeof(uint32_t);
  std::vector<uint32_t>* value_vector =
      static_cast<std::vector<uint32_t>*>(value);
  for (int i = 0; i < num_int32s; i++) {
    uint32_t content = *reinterpret_cast<const uint32_t*>(buffer);
    content = ntohl(content);
    value_vector->push_back(content);
    buffer += sizeof(uint32_t);
  }
  return true;
}

bool UInt32PairListParser::GetOption(const uint8_t* buffer,
                                     uint8_t length,
                                     void* value) {
  if (length == 0 || length % (2 * sizeof(uint32_t))) {
    LOG(ERROR) << "Invalid option length field";
    return false;
  }
  int num_int32pairs = length / (2 * sizeof(uint32_t));
  std::vector<std::pair<uint32_t, uint32_t>>* value_vector =
      static_cast<std::vector<std::pair<uint32_t, uint32_t>>*>(value);
  for (int i = 0; i < num_int32pairs; i++) {
    uint32_t first = *reinterpret_cast<const uint32_t*>(buffer);
    first = ntohl(first);
    buffer += sizeof(uint32_t);
    uint32_t second = *reinterpret_cast<const uint32_t*>(buffer);
    second = ntohl(second);
    value_vector->push_back(std::pair<uint32_t, uint32_t>(first, second));
    buffer += sizeof(uint32_t);
  }
  return true;
}

bool BoolParser::GetOption(const uint8_t* buffer,
                           uint8_t length,
                           void* value) {
  if (length != sizeof(uint8_t)) {
    LOG(ERROR) << "Invalid option length field";
    return false;
  }
  uint8_t content = *buffer;
  bool* enable = static_cast<bool*>(value);
  if (content == 1) {
    *enable = true;
  } else if (content == 0) {
    *enable = false;
  } else {
    LOG(ERROR) << "Invalid option value field";
    return false;
  }
  return true;
}

bool StringParser::GetOption(const uint8_t* buffer,
                             uint8_t length,
                             void* value) {
  if (length == 0) {
    LOG(ERROR) << "Invalid option length field";
    return false;
  }
  std::string* option_string = static_cast<std::string*>(value);
  option_string->assign(reinterpret_cast<const char*>(buffer), length);
  return true;
}

bool ByteArrayParser::GetOption(const uint8_t* buffer,
                                uint8_t length,
                                void* value) {
  if (length == 0) {
    LOG(ERROR) << "Invalid option length field";
    return false;
  }
  ByteString* byte_array =
      static_cast<ByteString*>(value);
  *byte_array = ByteString(buffer, length);
  return true;
}

}  // namespace dhcp_client
