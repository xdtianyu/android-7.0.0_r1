// Copyright 2012 Google Inc. All Rights Reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "polo/encoding/encodingoption.h"

#include <sstream>

namespace polo {
namespace encoding {

EncodingOption::EncodingOption(EncodingOption::EncodingType encoding_type,
                               uint32_t symbol_length)
    : encoding_type_(encoding_type),
      symbol_length_(symbol_length) {
}

EncodingOption::EncodingType EncodingOption::encoding_type() const {
  return encoding_type_;
}

uint32_t EncodingOption::symbol_length() const {
  return symbol_length_;
}

bool EncodingOption::Equals(const EncodingOption& other) const {
  return encoding_type_ == other.encoding_type_
      && symbol_length_ == other.symbol_length_;
}

std::string EncodingOption::ToString() const {
  std::ostringstream ss;
  ss << encoding_type_ << ':' << symbol_length_;
  return ss.str();
}

}  // namespace encoding
}  // namespace polo
