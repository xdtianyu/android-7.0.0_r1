//
// Copyright (C) 2014 The Android Open Source Project
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

#ifndef TRUNKS_MOCK_TPM_UTILITY_H_
#define TRUNKS_MOCK_TPM_UTILITY_H_

#include <string>

#include <gmock/gmock.h>

#include "trunks/tpm_utility.h"

namespace trunks {

class MockTpmUtility : public TpmUtility {
 public:
  MockTpmUtility();
  ~MockTpmUtility() override;

  MOCK_METHOD0(Startup, TPM_RC());
  MOCK_METHOD0(Clear, TPM_RC());
  MOCK_METHOD0(Shutdown, void());
  MOCK_METHOD0(InitializeTpm, TPM_RC());
  MOCK_METHOD1(AllocatePCR, TPM_RC(const std::string&));
  MOCK_METHOD3(TakeOwnership, TPM_RC(const std::string&,
                                     const std::string&,
                                     const std::string&));
  MOCK_METHOD2(StirRandom, TPM_RC(const std::string&, AuthorizationDelegate*));
  MOCK_METHOD3(GenerateRandom, TPM_RC(size_t,
                                      AuthorizationDelegate*,
                                      std::string*));
  MOCK_METHOD3(ExtendPCR,
               TPM_RC(int, const std::string&, AuthorizationDelegate*));
  MOCK_METHOD2(ReadPCR, TPM_RC(int, std::string*));
  MOCK_METHOD6(AsymmetricEncrypt, TPM_RC(TPM_HANDLE,
                                         TPM_ALG_ID,
                                         TPM_ALG_ID,
                                         const std::string&,
                                         AuthorizationDelegate*,
                                         std::string*));
  MOCK_METHOD6(AsymmetricDecrypt, TPM_RC(TPM_HANDLE,
                                         TPM_ALG_ID,
                                         TPM_ALG_ID,
                                         const std::string&,
                                         AuthorizationDelegate*,
                                         std::string*));
  MOCK_METHOD6(Sign, TPM_RC(TPM_HANDLE,
                            TPM_ALG_ID,
                            TPM_ALG_ID,
                            const std::string&,
                            AuthorizationDelegate*,
                            std::string*));
  MOCK_METHOD6(Verify, TPM_RC(TPM_HANDLE,
                              TPM_ALG_ID,
                              TPM_ALG_ID,
                              const std::string&,
                              const std::string&,
                              AuthorizationDelegate*));
  MOCK_METHOD2(CertifyCreation, TPM_RC(TPM_HANDLE,
                                       const std::string&));
  MOCK_METHOD4(ChangeKeyAuthorizationData, TPM_RC(TPM_HANDLE,
                                                  const std::string&,
                                                  AuthorizationDelegate*,
                                                  std::string*));
  MOCK_METHOD7(ImportRSAKey, TPM_RC(AsymmetricKeyUsage,
                                    const std::string&,
                                    uint32_t,
                                    const std::string&,
                                    const std::string&,
                                    AuthorizationDelegate*,
                                    std::string*));
  MOCK_METHOD10(CreateRSAKeyPair, TPM_RC(AsymmetricKeyUsage,
                                         int,
                                         uint32_t,
                                         const std::string&,
                                         const std::string&,
                                         bool,
                                         int,
                                         AuthorizationDelegate*,
                                         std::string*,
                                         std::string*));
  MOCK_METHOD3(LoadKey, TPM_RC(const std::string&,
                               AuthorizationDelegate*,
                               TPM_HANDLE*));
  MOCK_METHOD2(GetKeyName, TPM_RC(TPM_HANDLE, std::string*));
  MOCK_METHOD2(GetKeyPublicArea, TPM_RC(TPM_HANDLE, TPMT_PUBLIC*));
  MOCK_METHOD4(SealData, TPM_RC(const std::string&,
                                const std::string&,
                                AuthorizationDelegate*,
                                std::string*));
  MOCK_METHOD3(UnsealData, TPM_RC(const std::string&,
                                  AuthorizationDelegate*,
                                  std::string*));
  MOCK_METHOD1(StartSession, TPM_RC(HmacSession*));
  MOCK_METHOD3(GetPolicyDigestForPcrValue, TPM_RC(int,
                                                  const std::string&,
                                                  std::string*));
  MOCK_METHOD3(DefineNVSpace, TPM_RC(uint32_t,
                                     size_t,
                                     AuthorizationDelegate*));
  MOCK_METHOD2(DestroyNVSpace, TPM_RC(uint32_t,
                                      AuthorizationDelegate*));
  MOCK_METHOD2(LockNVSpace, TPM_RC(uint32_t,
                                   AuthorizationDelegate*));
  MOCK_METHOD4(WriteNVSpace, TPM_RC(uint32_t,
                                    uint32_t,
                                    const std::string&,
                                    AuthorizationDelegate*));
  MOCK_METHOD5(ReadNVSpace, TPM_RC(uint32_t,
                                   uint32_t,
                                   size_t,
                                   std::string*,
                                   AuthorizationDelegate*));
  MOCK_METHOD2(GetNVSpaceName, TPM_RC(uint32_t, std::string*));
  MOCK_METHOD2(GetNVSpacePublicArea, TPM_RC(uint32_t, TPMS_NV_PUBLIC*));
};

}  // namespace trunks

#endif  // TRUNKS_MOCK_TPM_UTILITY_H_
