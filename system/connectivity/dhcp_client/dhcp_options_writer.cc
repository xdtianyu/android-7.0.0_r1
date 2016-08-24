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

#include "dhcp_client/dhcp_options_writer.h"

#include <netinet/in.h>

#include <string>
#include <utility>
#include <vector>

#include <base/logging.h>
#include <base/macros.h>

#include "dhcp_client/dhcp_options.h"

using shill::ByteString;
namespace {
base::LazyInstance<dhcp_client::DHCPOptionsWriter> g_dhcp_options_writer
    = LAZY_INSTANCE_INITIALIZER;
}  // namespace

namespace dhcp_client {

DHCPOptionsWriter* DHCPOptionsWriter::GetInstance() {
  return g_dhcp_options_writer.Pointer();
}

int DHCPOptionsWriter::WriteUInt8Option(ByteString* buffer,
                                        uint8_t option_code,
                                        uint8_t value) {
  uint8_t length = sizeof(uint8_t);
  buffer->Append(ByteString(reinterpret_cast<const char*>(&option_code),
                            sizeof(uint8_t)));
  buffer->Append(ByteString(reinterpret_cast<const char*>(&length),
                            sizeof(uint8_t)));
  buffer->Append(ByteString(reinterpret_cast<const char*>(&value),
                            sizeof(uint8_t)));
  return length + 2;
}

int DHCPOptionsWriter::WriteUInt16Option(ByteString* buffer,
                                         uint8_t option_code,
                                         uint16_t value) {
  uint8_t length = sizeof(uint16_t);
  uint16_t value_net = htons(value);
  buffer->Append(ByteString(reinterpret_cast<const char*>(&option_code),
                            sizeof(uint8_t)));
  buffer->Append(ByteString(reinterpret_cast<const char*>(&length),
                            sizeof(uint8_t)));
  buffer->Append(ByteString(reinterpret_cast<const char*>(&value_net),
                            sizeof(uint16_t)));
  return length + 2;
}

int DHCPOptionsWriter::WriteUInt32Option(ByteString* buffer,
                                         uint8_t option_code,
                                         uint32_t value) {
  uint8_t length = sizeof(uint32_t);
  uint32_t value_net = htonl(value);
  buffer->Append(ByteString(reinterpret_cast<const char*>(&option_code),
                            sizeof(uint8_t)));
  buffer->Append(ByteString(reinterpret_cast<const char*>(&length),
                            sizeof(uint8_t)));
  buffer->Append(ByteString(reinterpret_cast<const char*>(&value_net),
                            sizeof(uint32_t)));
  return length + 2;
}

int DHCPOptionsWriter::WriteUInt8ListOption(ByteString* buffer,
    uint8_t option_code,
    const std::vector<uint8_t>& value) {
  if (value.size() == 0) {
    LOG(ERROR) << "Faild to write option: " << static_cast<int>(option_code)
               << ", because value size cannot be 0";
    return -1;
  }
  uint8_t length = value.size() * sizeof(uint8_t);
  buffer->Append(ByteString(reinterpret_cast<const char*>(&option_code),
                            sizeof(uint8_t)));
  buffer->Append(ByteString(reinterpret_cast<const char*>(&length),
                            sizeof(uint8_t)));
  buffer->Append(ByteString(reinterpret_cast<const char*>(&value.front()),
                            length * sizeof(uint8_t)));
  return length + 2;
}

int DHCPOptionsWriter::WriteUInt16ListOption(ByteString* buffer,
    uint8_t option_code,
    const std::vector<uint16_t>& value) {
  if (value.size() == 0) {
    LOG(ERROR) << "Faild to write option: " << static_cast<int>(option_code)
               << ", because value size cannot be 0";
    return -1;
  }
  uint8_t length = value.size() * sizeof(uint16_t);
  buffer->Append(ByteString(reinterpret_cast<const char*>(&option_code),
                            sizeof(uint8_t)));
  buffer->Append(ByteString(reinterpret_cast<const char*>(&length),
                            sizeof(uint8_t)));
  for (uint16_t element : value) {
    uint16_t element_net = htons(element);
    buffer->Append(ByteString(reinterpret_cast<const char *>(&element_net),
                              sizeof(uint16_t)));
  }
  return length + 2;
}

int DHCPOptionsWriter::WriteUInt32ListOption(ByteString* buffer,
    uint8_t option_code,
    const std::vector<uint32_t>& value) {
  if (value.size() == 0) {
    LOG(ERROR) << "Faild to write option: " << static_cast<int>(option_code)
               << ", because value size cannot be 0";
    return -1;
  }
  uint8_t length = value.size() * sizeof(uint32_t);
  buffer->Append(ByteString(reinterpret_cast<const char*>(&option_code),
                            sizeof(uint8_t)));
  buffer->Append(ByteString(reinterpret_cast<const char*>(&length),
                            sizeof(uint8_t)));
  for (uint32_t element : value) {
    uint32_t element_net = htonl(element);
    buffer->Append(ByteString(reinterpret_cast<const char*>(&element_net),
                              sizeof(uint32_t)));
  }
  return length + 2;
}

int DHCPOptionsWriter::WriteUInt32PairListOption(ByteString* buffer,
    uint8_t option_code,
    const std::vector<std::pair<uint32_t, uint32_t>>& value) {
  if (value.size() == 0) {
    LOG(ERROR) << "Faild to write option: " << static_cast<int>(option_code)
               << ", because value size cannot be 0";
    return -1;
  }
  uint8_t length = value.size() * sizeof(uint32_t) * 2;
  buffer->Append(ByteString(reinterpret_cast<const char*>(&option_code),
                            sizeof(uint8_t)));
  buffer->Append(ByteString(reinterpret_cast<const char*>(&length),
                            sizeof(uint8_t)));
  for (auto element : value) {
    uint32_t first_net = htonl(element.first);
    uint32_t second_net = htonl(element.second);
    buffer->Append(ByteString(reinterpret_cast<const char*>(&first_net),
                              sizeof(uint32_t)));
    buffer->Append(ByteString(reinterpret_cast<const char*>(&second_net),
                              sizeof(uint32_t)));
  }
  return length + 2;
}

int DHCPOptionsWriter::WriteBoolOption(ByteString* buffer,
                                       uint8_t option_code,
                                       const bool value) {
  uint8_t length = sizeof(uint8_t);
  uint8_t value_uint8 = value ? 1 : 0;
  buffer->Append(ByteString(reinterpret_cast<const char*>(&option_code),
                            sizeof(uint8_t)));
  buffer->Append(ByteString(reinterpret_cast<const char*>(&length),
                            sizeof(uint8_t)));
  buffer->Append(ByteString(reinterpret_cast<const char*>(&value_uint8),
                            sizeof(uint8_t)));
  return length + 2;
}

int DHCPOptionsWriter::WriteStringOption(ByteString* buffer,
    uint8_t option_code,
    const std::string& value) {
  if (value.size() == 0) {
    LOG(ERROR) << "Faild to write option: " << static_cast<int>(option_code)
               << ", because value size cannot be 0";
    return -1;
  }
  uint8_t length = value.size();
  buffer->Append(ByteString(reinterpret_cast<const char*>(&option_code),
                            sizeof(uint8_t)));
  buffer->Append(ByteString(reinterpret_cast<const char*>(&length),
                            sizeof(uint8_t)));
  buffer->Append(ByteString(reinterpret_cast<const char*>(&value.front()),
                            length * sizeof(uint8_t)));
  return length + 2;
}

int DHCPOptionsWriter::WriteByteArrayOption(ByteString* buffer,
                                            uint8_t option_code,
                                            const ByteString& value) {
  uint8_t length = value.GetLength();
  buffer->Append(ByteString(reinterpret_cast<const char*>(&option_code),
                            sizeof(uint8_t)));
  buffer->Append(ByteString(reinterpret_cast<const char*>(&length),
                            sizeof(uint8_t)));

  buffer->Append(value);
  return length + 2;
}

int DHCPOptionsWriter::WriteEndTag(ByteString* buffer) {
  uint8_t tag = kDHCPOptionEnd;
  buffer->Append(ByteString(reinterpret_cast<const char*>(&tag),
                            sizeof(uint8_t)));
  return 1;
}

}  // namespace dhcp_client
