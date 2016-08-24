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

#ifndef ATTESTATION_SERVER_MOCK_DATABASE_H_
#define ATTESTATION_SERVER_MOCK_DATABASE_H_

#include "attestation/server/database.h"

#include <gmock/gmock.h>

namespace attestation {

class MockDatabase : public Database {
 public:
  MockDatabase();
  ~MockDatabase() override;

  MOCK_CONST_METHOD0(GetProtobuf, const AttestationDatabase&());
  MOCK_METHOD0(GetMutableProtobuf, AttestationDatabase*());
  MOCK_METHOD0(SaveChanges, bool());
  MOCK_METHOD0(Reload, bool());

 private:
  AttestationDatabase fake_;
};

}  // namespace attestation

#endif  // ATTESTATION_SERVER_MOCK_DATABASE_H_
