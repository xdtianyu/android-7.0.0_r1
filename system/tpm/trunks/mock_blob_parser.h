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

#ifndef TRUNKS_MOCK_BLOB_PARSER_H_
#define TRUNKS_MOCK_BLOB_PARSER_H_

#include <string>

#include <gmock/gmock.h>

#include "trunks/blob_parser.h"

namespace trunks {

class MockBlobParser : public BlobParser {
 public:
  MockBlobParser();
  ~MockBlobParser() override;

  MOCK_METHOD3(SerializeKeyBlob, bool(const TPM2B_PUBLIC&,
                                      const TPM2B_PRIVATE&,
                                      std::string*));
  MOCK_METHOD3(ParseKeyBlob, bool(const std::string&,
                                  TPM2B_PUBLIC*,
                                  TPM2B_PRIVATE*));
  MOCK_METHOD4(SerializeCreationBlob, bool(const TPM2B_CREATION_DATA&,
                                           const TPM2B_DIGEST&,
                                           const TPMT_TK_CREATION&,
                                           std::string*));
  MOCK_METHOD4(ParseCreationBlob, bool(const std::string&,
                                       TPM2B_CREATION_DATA*,
                                       TPM2B_DIGEST*,
                                       TPMT_TK_CREATION*));
};

}  // namespace trunks

#endif  // TRUNKS_MOCK_BLOB_PARSER_H_
