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

#ifndef DHCP_CLIENT_PARSER_H_
#define DHCP_CLIENT_PARSER_H_

#include <cstdint>

namespace dhcp_client {

class DHCPOptionsParser {
 public:
  virtual bool GetOption(const uint8_t* buffer,
                         uint8_t length,
                         void* value) = 0;
  virtual ~DHCPOptionsParser() {}
};

class UInt8Parser : public DHCPOptionsParser {
 public:
  UInt8Parser() {}
  bool GetOption(const uint8_t* buffer,
                 uint8_t length,
                 void* value) override;
};

class UInt16Parser : public DHCPOptionsParser {
 public:
  UInt16Parser() {}
  bool GetOption(const uint8_t* buffer,
                 uint8_t length,
                 void* value) override;
};

class UInt32Parser : public DHCPOptionsParser {
 public:
  UInt32Parser() {}
  bool GetOption(const uint8_t* buffer,
                 uint8_t length,
                 void* value) override;
};

class UInt8ListParser : public DHCPOptionsParser {
 public:
  UInt8ListParser() {}
  bool GetOption(const uint8_t* buffer,
                 uint8_t length,
                 void* value) override;
};

class UInt16ListParser : public DHCPOptionsParser {
 public:
  UInt16ListParser() {}
  bool GetOption(const uint8_t* buffer,
                 uint8_t length,
                 void* value) override;
};

class UInt32ListParser : public DHCPOptionsParser {
 public:
  UInt32ListParser() {}
  bool GetOption(const uint8_t* buffer,
                 uint8_t length,
                 void* value) override;
};

class UInt32PairListParser : public DHCPOptionsParser {
 public:
  UInt32PairListParser() {}
  bool GetOption(const uint8_t* buffer,
                 uint8_t length,
                 void* value) override;
};

class BoolParser : public DHCPOptionsParser {
 public:
  BoolParser() {}
  bool GetOption(const uint8_t* buffer,
                 uint8_t length,
                 void* value) override;
};

class StringParser : public DHCPOptionsParser {
 public:
  StringParser() {}
  bool GetOption(const uint8_t* buffer,
                 uint8_t length,
                 void* value) override;
};

class ByteArrayParser : public DHCPOptionsParser {
 public:
  bool GetOption(const uint8_t* buffer,
                 uint8_t length,
                 void* value) override;
};
}  // namespace dhcp_client

#endif  // DHCP_CLIENT_PARSER_H_
