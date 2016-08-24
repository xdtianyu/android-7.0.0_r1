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

#ifndef POLO_ENCODING_ENCODINGOPTION_H_
#define POLO_ENCODING_ENCODINGOPTION_H_

#include <stdint.h>
#include <functional>
#include <set>
#include <string>

namespace polo {
namespace encoding {

// An encoding option for a challenge message consisting of an encoding scheme
// and symbol length.
class EncodingOption {
 public:
  // Representation of a specific encoding type. The numeric values should
  // be sorted by encoding complexity from least to greatest.
  enum EncodingType {
    // Unknown encoding type.
    kUnknown = 0,

    // Text message composed of characters [0-9].
    kNumeric = 1,

    // Text message composed of characters [0-9A-Za-z]+.
    kAlphaNumeric = 2,

    // Text message composed of characters [0-9A-Fa-f]+.
    kHexadecimal = 3,

    // 2-dimensional barcode, containing binary bitstream.
    kQRCode = 4,
  };

  // Creates a new encoding option.
  // @param encoding_type the encoding type
  // @param symbol_length the encoding symbole length
  EncodingOption(EncodingType encoding_type, uint32_t symbol_length);

  // Gets the encoding scheme for the challenge message.
  EncodingType encoding_type() const;

  // Gets the number of symbols used in the challenge message for the encoding
  // type specified by this encoding option. For example, a single symbol for
  // hexadecimal encoding consists of 4-bits from the set [0-9A-Fa-f].
  uint32_t symbol_length() const;

  // Determines whether the given encoding option is the same as this one.
  bool Equals(const EncodingOption& other) const;

  // Returns a string representation of this encoding option.
  std::string ToString() const;

  // EncodingOption comparator for set ordering.
  struct EncodingOptionComparator : public std::binary_function<
      EncodingOption, EncodingOption, bool> {
    bool operator()(const EncodingOption& option1,
                    const EncodingOption& option2) {
      // Sort encoding options by complexity.
      return (option1.encoding_type() == option2.encoding_type()
          && option1.symbol_length() < option2.symbol_length())
          || (option1.encoding_type() < option2.encoding_type());
    }
  };

  // Predicate for finding an encoding option.
  struct EncodingOptionPredicate
      : public std::unary_function<EncodingOption, bool> {
    const EncodingOption& option_;

    explicit EncodingOptionPredicate(const EncodingOption& option)
        : option_(option) {}

    bool operator()(const EncodingOption& other) const {
      return option_.Equals(other);
    }
  };

  // Definition for a set of EncodingOptions that are ordered by complexity.
  typedef std::set<EncodingOption, EncodingOptionComparator>
      EncodingSet;

 private:
  EncodingType encoding_type_;
  uint32_t symbol_length_;
};

}  // namespace encoding
}  // namespace polo

#endif  // POLO_ENCODING_ENCODINGOPTION_H_
