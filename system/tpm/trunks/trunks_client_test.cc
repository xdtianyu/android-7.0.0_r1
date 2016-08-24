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

#include "trunks/trunks_client_test.h"

#include <algorithm>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include <base/logging.h>
#include <base/stl_util.h>
#include <crypto/openssl_util.h>
#include <crypto/scoped_openssl_types.h>
#include <crypto/sha2.h>
#include <openssl/bn.h>
#include <openssl/err.h>
#include <openssl/rsa.h>

#include "trunks/authorization_delegate.h"
#include "trunks/error_codes.h"
#include "trunks/hmac_session.h"
#include "trunks/policy_session.h"
#include "trunks/scoped_key_handle.h"
#include "trunks/tpm_generated.h"
#include "trunks/tpm_state.h"
#include "trunks/tpm_utility.h"
#include "trunks/trunks_factory_impl.h"

namespace {

std::string GetOpenSSLError() {
  BIO* bio = BIO_new(BIO_s_mem());
  ERR_print_errors(bio);
  char* data = nullptr;
  int data_len = BIO_get_mem_data(bio, &data);
  std::string error_string(data, data_len);
  BIO_free(bio);
  return error_string;
}

}  // namespace

namespace trunks {

TrunksClientTest::TrunksClientTest()
    : factory_(new TrunksFactoryImpl(true /* failure_is_fatal */)) {
  crypto::EnsureOpenSSLInit();
}

TrunksClientTest::TrunksClientTest(scoped_ptr<TrunksFactory> factory)
    : factory_(std::move(factory)) {}

TrunksClientTest::~TrunksClientTest() {}

bool TrunksClientTest::RNGTest() {
  scoped_ptr<TpmUtility> utility = factory_->GetTpmUtility();
  scoped_ptr<HmacSession> session = factory_->GetHmacSession();
  if (utility->StartSession(session.get()) != TPM_RC_SUCCESS) {
    LOG(ERROR) << "Error starting hmac session.";
    return false;
  }
  std::string entropy_data("entropy_data");
  std::string random_data;
  size_t num_bytes = 70;
  TPM_RC result = utility->StirRandom(entropy_data, session->GetDelegate());
  if (result != TPM_RC_SUCCESS) {
    LOG(ERROR) << "Error stirring TPM RNG: " << GetErrorString(result);
    return false;
  }
  result = utility->GenerateRandom(num_bytes, session->GetDelegate(),
                                   &random_data);
  if (result != TPM_RC_SUCCESS) {
    LOG(ERROR) << "Error getting random bytes from TPM: "
               << GetErrorString(result);
    return false;
  }
  if (num_bytes != random_data.size()) {
    LOG(ERROR) << "Error not enough random bytes received.";
    return false;
  }
  return true;
}

bool TrunksClientTest::SignTest() {
  scoped_ptr<TpmUtility> utility = factory_->GetTpmUtility();
  scoped_ptr<HmacSession> session = factory_->GetHmacSession();
  if (utility->StartSession(session.get()) != TPM_RC_SUCCESS) {
    LOG(ERROR) << "Error starting hmac session.";
    return false;
  }
  std::string key_authorization("sign");
  std::string key_blob;
  TPM_RC result = utility->CreateRSAKeyPair(
      TpmUtility::AsymmetricKeyUsage::kSignKey, 2048, 0x10001,
      key_authorization, "", false,  // use_only_policy_authorization
      kNoCreationPCR, session->GetDelegate(), &key_blob, nullptr);
  if (result != TPM_RC_SUCCESS) {
    LOG(ERROR) << "Error creating signing key: " << GetErrorString(result);
    return false;
  }
  TPM_HANDLE signing_key;
  result = utility->LoadKey(key_blob, session->GetDelegate(), &signing_key);
  if (result != TPM_RC_SUCCESS) {
    LOG(ERROR) << "Error loading signing key: " << GetErrorString(result);
  }
  ScopedKeyHandle scoped_key(*factory_.get(), signing_key);
  session->SetEntityAuthorizationValue(key_authorization);
  std::string signature;
  result = utility->Sign(signing_key, TPM_ALG_NULL, TPM_ALG_NULL,
                         std::string(32, 'a'), session->GetDelegate(),
                         &signature);
  if (result != TPM_RC_SUCCESS) {
    LOG(ERROR) << "Error using key to sign: " << GetErrorString(result);
    return false;
  }
  result = utility->Verify(signing_key, TPM_ALG_NULL, TPM_ALG_NULL,
                           std::string(32, 'a'), signature, nullptr);
  if (result != TPM_RC_SUCCESS) {
    LOG(ERROR) << "Error using key to verify: " << GetErrorString(result);
    return false;
  }
  return true;
}

bool TrunksClientTest::DecryptTest() {
  scoped_ptr<TpmUtility> utility = factory_->GetTpmUtility();
  scoped_ptr<HmacSession> session = factory_->GetHmacSession();
  if (utility->StartSession(session.get()) != TPM_RC_SUCCESS) {
    LOG(ERROR) << "Error starting hmac session.";
    return false;
  }
  std::string key_authorization("decrypt");
  std::string key_blob;
  TPM_RC result = utility->CreateRSAKeyPair(
      TpmUtility::AsymmetricKeyUsage::kDecryptKey, 2048, 0x10001,
      key_authorization, "", false,  // use_only_policy_authorization
      kNoCreationPCR, session->GetDelegate(), &key_blob, nullptr);
  if (result != TPM_RC_SUCCESS) {
    LOG(ERROR) << "Error creating decrypt key: " << GetErrorString(result);
    return false;
  }
  TPM_HANDLE decrypt_key;
  result = utility->LoadKey(key_blob, session->GetDelegate(), &decrypt_key);
  if (result != TPM_RC_SUCCESS) {
    LOG(ERROR) << "Error loading decrypt key: " << GetErrorString(result);
  }
  ScopedKeyHandle scoped_key(*factory_.get(), decrypt_key);
  return PerformRSAEncrpytAndDecrpyt(scoped_key.get(),
                                     key_authorization,
                                     session.get());
}

bool TrunksClientTest::ImportTest() {
  scoped_ptr<TpmUtility> utility = factory_->GetTpmUtility();
  scoped_ptr<HmacSession> session = factory_->GetHmacSession();
  if (utility->StartSession(session.get()) != TPM_RC_SUCCESS) {
    LOG(ERROR) << "Error starting hmac session.";
    return false;
  }
  std::string modulus;
  std::string prime_factor;
  GenerateRSAKeyPair(&modulus, &prime_factor, nullptr);
  std::string key_blob;
  std::string key_authorization("import");
  TPM_RC result = utility->ImportRSAKey(
      TpmUtility::AsymmetricKeyUsage::kDecryptAndSignKey, modulus, 0x10001,
      prime_factor, key_authorization, session->GetDelegate(), &key_blob);
  if (result != TPM_RC_SUCCESS) {
    LOG(ERROR) << "Error importing key into TPM: " << GetErrorString(result);
    return false;
  }
  TPM_HANDLE key_handle;
  result = utility->LoadKey(key_blob, session->GetDelegate(), &key_handle);
  if (result != TPM_RC_SUCCESS) {
    LOG(ERROR) << "Error loading key into TPM: " << GetErrorString(result);
    return false;
  }
  ScopedKeyHandle scoped_key(*factory_.get(), key_handle);
  return PerformRSAEncrpytAndDecrpyt(scoped_key.get(), key_authorization,
                                     session.get());
}

bool TrunksClientTest::AuthChangeTest() {
  scoped_ptr<TpmUtility> utility = factory_->GetTpmUtility();
  scoped_ptr<HmacSession> session = factory_->GetHmacSession();
  if (utility->StartSession(session.get()) != TPM_RC_SUCCESS) {
    LOG(ERROR) << "Error starting hmac session.";
    return false;
  }
  std::string key_authorization("new_pass");
  std::string key_blob;
  TPM_RC result = utility->CreateRSAKeyPair(
      TpmUtility::AsymmetricKeyUsage::kDecryptKey, 2048, 0x10001,
      "old_pass", "", false,  // use_only_policy_authorization
      kNoCreationPCR, session->GetDelegate(), &key_blob, nullptr);
  if (result != TPM_RC_SUCCESS) {
    LOG(ERROR) << "Error creating change auth key: " << GetErrorString(result);
    return false;
  }
  TPM_HANDLE key_handle;
  result = utility->LoadKey(key_blob, session->GetDelegate(), &key_handle);
  if (result != TPM_RC_SUCCESS) {
    LOG(ERROR) << "Error loading change auth key: " << GetErrorString(result);
  }
  ScopedKeyHandle scoped_key(*factory_.get(), key_handle);
  session->SetEntityAuthorizationValue("old_pass");
  result = utility->ChangeKeyAuthorizationData(key_handle, key_authorization,
                                               session->GetDelegate(),
                                               &key_blob);
  if (result != TPM_RC_SUCCESS) {
    LOG(ERROR) << "Error changing auth data: " << GetErrorString(result);
    return false;
  }
  session->SetEntityAuthorizationValue("");
  result = utility->LoadKey(key_blob, session->GetDelegate(), &key_handle);
  if (result != TPM_RC_SUCCESS) {
    LOG(ERROR) << "Error reloading key: " << GetErrorString(result);
    return false;
  }
  scoped_key.reset(key_handle);
  return PerformRSAEncrpytAndDecrpyt(scoped_key.get(), key_authorization,
                                     session.get());
}

bool TrunksClientTest::VerifyKeyCreationTest() {
  scoped_ptr<TpmUtility> utility = factory_->GetTpmUtility();
  scoped_ptr<HmacSession> session = factory_->GetHmacSession();
  if (utility->StartSession(session.get()) != TPM_RC_SUCCESS) {
    LOG(ERROR) << "Error starting hmac session.";
    return false;
  }
  std::string key_blob;
  std::string creation_blob;
  session->SetEntityAuthorizationValue("");
  TPM_RC result = utility->CreateRSAKeyPair(
      TpmUtility::AsymmetricKeyUsage::kDecryptKey, 2048, 0x10001,
      "", "", false,  // use_only_policy_authorization
      kNoCreationPCR, session->GetDelegate(), &key_blob, &creation_blob);
  if (result != TPM_RC_SUCCESS) {
    LOG(ERROR) << "Error creating certify key: " << GetErrorString(result);
    return false;
  }
  std::string alternate_key_blob;
  result = utility->CreateRSAKeyPair(
      TpmUtility::AsymmetricKeyUsage::kDecryptKey, 2048, 0x10001,
      "", "", false,  // use_only_policy_authorization
      kNoCreationPCR, session->GetDelegate(), &alternate_key_blob,
      nullptr);
  if (result != TPM_RC_SUCCESS) {
    LOG(ERROR) << "Error creating alternate key: " << GetErrorString(result);
    return false;
  }
  TPM_HANDLE key_handle;
  result = utility->LoadKey(key_blob, session->GetDelegate(), &key_handle);
  if (result != TPM_RC_SUCCESS) {
    LOG(ERROR) << "Error loading certify key: " << GetErrorString(result);
    return false;
  }
  TPM_HANDLE alternate_key_handle;
  result = utility->LoadKey(alternate_key_blob,
                            session->GetDelegate(),
                            &alternate_key_handle);
  if (result != TPM_RC_SUCCESS) {
    LOG(ERROR) << "Error loading alternate key: " << GetErrorString(result);
    return false;
  }
  ScopedKeyHandle certify_key(*factory_.get(), key_handle);
  ScopedKeyHandle alternate_key(*factory_.get(), alternate_key_handle);
  result = utility->CertifyCreation(certify_key.get(), creation_blob);
  if (result != TPM_RC_SUCCESS) {
    LOG(ERROR) << "Error certifying key: " << GetErrorString(result);
    return false;
  }
  result = utility->CertifyCreation(alternate_key.get(), creation_blob);
  if (result == TPM_RC_SUCCESS) {
    LOG(ERROR) << "Error alternate key certified with wrong creation data.";
    return false;
  }
  return true;
}

bool TrunksClientTest::SealedDataTest() {
  scoped_ptr<TpmUtility> utility = factory_->GetTpmUtility();
  scoped_ptr<HmacSession> session = factory_->GetHmacSession();
  if (utility->StartSession(session.get()) != TPM_RC_SUCCESS) {
    LOG(ERROR) << "Error starting hmac session.";
    return false;
  }
  int pcr_index = 5;
  std::string policy_digest;
  TPM_RC result = utility->GetPolicyDigestForPcrValue(pcr_index, "",
                                                      &policy_digest);
  if (result != TPM_RC_SUCCESS) {
    LOG(ERROR) << "Error getting policy_digest: " << GetErrorString(result);
    return false;
  }
  std::string data_to_seal("seal_data");
  std::string sealed_data;
  result = utility->SealData(data_to_seal, policy_digest,
                             session->GetDelegate(), &sealed_data);
  if (result != TPM_RC_SUCCESS) {
    LOG(ERROR) << "Error creating Sealed Object: " << GetErrorString(result);
    return false;
  }
  scoped_ptr<PolicySession> policy_session = factory_->GetPolicySession();
  result = policy_session->StartUnboundSession(false);
  if (result != TPM_RC_SUCCESS) {
    LOG(ERROR) << "Error starting policy session: " << GetErrorString(result);
    return false;
  }
  result = policy_session->PolicyPCR(pcr_index, "");
  if (result != TPM_RC_SUCCESS) {
    LOG(ERROR) << "Error restricting policy to pcr value: "
               << GetErrorString(result);
    return false;
  }
  std::string unsealed_data;
  result = utility->UnsealData(sealed_data, policy_session->GetDelegate(),
                               &unsealed_data);
  if (result != TPM_RC_SUCCESS) {
    LOG(ERROR) << "Error unsealing object: " << GetErrorString(result);
    return false;
  }
  if (data_to_seal != unsealed_data) {
    LOG(ERROR) << "Error unsealed data from TPM does not match original data.";
    return false;
  }
  result = utility->ExtendPCR(pcr_index, "extend", session->GetDelegate());
  if (result != TPM_RC_SUCCESS) {
    LOG(ERROR) << "Error extending pcr: " << GetErrorString(result);
    return false;
  }
  result = policy_session->PolicyPCR(pcr_index, "");
  if (result != TPM_RC_SUCCESS) {
    LOG(ERROR) << "Error restricting policy to pcr value: "
               << GetErrorString(result);
    return false;
  }
  result = utility->UnsealData(sealed_data, policy_session->GetDelegate(),
                               &unsealed_data);
  if (result == TPM_RC_SUCCESS) {
    LOG(ERROR) << "Error object was unsealed with wrong policy_digest.";
    return false;
  }
  return true;
}

bool TrunksClientTest::PCRTest() {
  scoped_ptr<TpmUtility> utility = factory_->GetTpmUtility();
  scoped_ptr<HmacSession> session = factory_->GetHmacSession();
  if (utility->StartSession(session.get()) != TPM_RC_SUCCESS) {
    LOG(ERROR) << "Error starting hmac session.";
    return false;
  }
  // We are using PCR 2 because it is currently not used by ChromeOS.
  uint32_t pcr_index = 2;
  std::string extend_data("data");
  std::string old_data;
  TPM_RC result = utility->ReadPCR(pcr_index, &old_data);
  if (result != TPM_RC_SUCCESS) {
    LOG(ERROR) << "Error reading from PCR: " << GetErrorString(result);
    return false;
  }
  result = utility->ExtendPCR(pcr_index, extend_data, session->GetDelegate());
  if (result != TPM_RC_SUCCESS) {
    LOG(ERROR) << "Error extending PCR value: " << GetErrorString(result);
    return false;
  }
  std::string pcr_data;
  result = utility->ReadPCR(pcr_index, &pcr_data);
  if (result != TPM_RC_SUCCESS) {
    LOG(ERROR) << "Error reading from PCR: " << GetErrorString(result);
    return false;
  }
  std::string hashed_extend_data = crypto::SHA256HashString(extend_data);
  std::string expected_pcr_data =
      crypto::SHA256HashString(old_data + hashed_extend_data);
  if (pcr_data.compare(expected_pcr_data) != 0) {
    LOG(ERROR) << "PCR data does not match expected value.";
    return false;
  }
  return true;
}

bool TrunksClientTest::PolicyAuthValueTest() {
  scoped_ptr<TpmUtility> utility = factory_->GetTpmUtility();
  scoped_ptr<PolicySession> trial_session = factory_->GetTrialSession();
  TPM_RC result;
  result = trial_session->StartUnboundSession(true);
  if (result != TPM_RC_SUCCESS) {
    LOG(ERROR) << "Error starting policy session: " << GetErrorString(result);
    return false;
  }
  result = trial_session->PolicyAuthValue();
  if (result != TPM_RC_SUCCESS) {
    LOG(ERROR) << "Error restricting policy to auth value knowledge: "
               << GetErrorString(result);
    return false;
  }
  std::string policy_digest;
  result = trial_session->GetDigest(&policy_digest);
  if (result != TPM_RC_SUCCESS) {
    LOG(ERROR) << "Error getting policy digest: " << GetErrorString(result);
    return false;
  }
  // Now that we have the digest, we can close the trial session and use hmac.
  trial_session.reset();

  scoped_ptr<HmacSession> hmac_session = factory_->GetHmacSession();
  result = hmac_session->StartUnboundSession(true);
  if (result != TPM_RC_SUCCESS) {
    LOG(ERROR) << "Error starting hmac session: " << GetErrorString(result);
    return false;
  }

  std::string key_blob;
  result = utility->CreateRSAKeyPair(
      TpmUtility::AsymmetricKeyUsage::kDecryptAndSignKey, 2048, 0x10001,
      "password", policy_digest, true,  // use_only_policy_authorization
      kNoCreationPCR, hmac_session->GetDelegate(), &key_blob, nullptr);
  if (result != TPM_RC_SUCCESS) {
    LOG(ERROR) << "Error creating RSA key: " << GetErrorString(result);
    return false;
  }

  TPM_HANDLE key_handle;
  result = utility->LoadKey(key_blob, hmac_session->GetDelegate(), &key_handle);
  if (result != TPM_RC_SUCCESS) {
    LOG(ERROR) << "Error loading RSA key: " << GetErrorString(result);
    return false;
  }
  ScopedKeyHandle scoped_key(*factory_.get(), key_handle);

  // Now we can reset the hmac_session.
  hmac_session.reset();

  scoped_ptr<PolicySession> policy_session = factory_->GetPolicySession();
  result = policy_session->StartUnboundSession(false);
  if (result != TPM_RC_SUCCESS) {
    LOG(ERROR) << "Error starting policy session: " << GetErrorString(result);
    return false;
  }
  result = policy_session->PolicyAuthValue();
  if (result != TPM_RC_SUCCESS) {
    LOG(ERROR) << "Error restricting policy to auth value knowledge: "
               << GetErrorString(result);
    return false;
  }
  std::string signature;
  policy_session->SetEntityAuthorizationValue("password");
  result = utility->Sign(scoped_key.get(), TPM_ALG_NULL, TPM_ALG_NULL,
                         std::string(32, 0), policy_session->GetDelegate(),
                         &signature);
  if (result != TPM_RC_SUCCESS) {
    LOG(ERROR) << "Error signing using RSA key: " << GetErrorString(result);
    return false;
  }
  result = utility->Verify(scoped_key.get(), TPM_ALG_NULL, TPM_ALG_NULL,
                           std::string(32, 0), signature, nullptr);
  if (result != TPM_RC_SUCCESS) {
    LOG(ERROR) << "Error verifying using RSA key: " << GetErrorString(result);
    return false;
  }
  std::string ciphertext;
  result = utility->AsymmetricEncrypt(scoped_key.get(), TPM_ALG_NULL,
                                      TPM_ALG_NULL, "plaintext",
                                      nullptr,
                                      &ciphertext);
  if (result != TPM_RC_SUCCESS) {
    LOG(ERROR) << "Error encrypting using RSA key: " << GetErrorString(result);
    return false;
  }
  result = policy_session->PolicyAuthValue();
  if (result != TPM_RC_SUCCESS) {
    LOG(ERROR) << "Error restricting policy to auth value knowledge: "
               << GetErrorString(result);
    return false;
  }
  std::string plaintext;
  policy_session->SetEntityAuthorizationValue("password");
  result = utility->AsymmetricDecrypt(scoped_key.get(), TPM_ALG_NULL,
                                      TPM_ALG_NULL, ciphertext,
                                      policy_session->GetDelegate(),
                                      &plaintext);
  if (result != TPM_RC_SUCCESS) {
    LOG(ERROR) << "Error encrypting using RSA key: " << GetErrorString(result);
    return false;
  }
  if (plaintext.compare("plaintext") != 0) {
    LOG(ERROR) << "Plaintext changed after encrypt + decrypt.";
    return false;
  }
  return true;
}

bool TrunksClientTest::PolicyAndTest() {
  scoped_ptr<TpmUtility> utility = factory_->GetTpmUtility();
  scoped_ptr<PolicySession> trial_session = factory_->GetTrialSession();
  TPM_RC result;
  result = trial_session->StartUnboundSession(true);
  if (result != TPM_RC_SUCCESS) {
    LOG(ERROR) << "Error starting policy session: " << GetErrorString(result);
    return false;
  }
  result = trial_session->PolicyCommandCode(TPM_CC_Sign);
  if (result != TPM_RC_SUCCESS) {
    LOG(ERROR) << "Error restricting policy: " << GetErrorString(result);
    return false;
  }
  uint32_t pcr_index = 2;
  std::string pcr_value;
  result = utility->ReadPCR(pcr_index, &pcr_value);
  if (result != TPM_RC_SUCCESS) {
    LOG(ERROR) << "Error reading pcr: " << GetErrorString(result);
    return false;
  }
  std::string pcr_extend_data("extend");
  std::string next_pcr_value;
  std::string hashed_extend_data = crypto::SHA256HashString(pcr_extend_data);
  next_pcr_value = crypto::SHA256HashString(pcr_value + hashed_extend_data);

  result = trial_session->PolicyPCR(pcr_index, next_pcr_value);
  if (result != TPM_RC_SUCCESS) {
    LOG(ERROR) << "Error restricting policy: " << GetErrorString(result);
    return false;
  }
  std::string policy_digest;
  result = trial_session->GetDigest(&policy_digest);
  if (result != TPM_RC_SUCCESS) {
    LOG(ERROR) << "Error getting policy digest: " << GetErrorString(result);
    return false;
  }
  // Now that we have the digest, we can close the trial session and use hmac.
  trial_session.reset();

  scoped_ptr<HmacSession> hmac_session = factory_->GetHmacSession();
  result = hmac_session->StartUnboundSession(true);
  if (result != TPM_RC_SUCCESS) {
    LOG(ERROR) << "Error starting hmac session: " << GetErrorString(result);
    return false;
  }
  std::string key_authorization("password");
  std::string key_blob;
  // This key is created with a policy that dictates it can only be used
  // when pcr 2 remains unchanged, and when the command is TPM2_Sign.
  result = utility->CreateRSAKeyPair(
      TpmUtility::AsymmetricKeyUsage::kDecryptAndSignKey, 2048, 0x10001,
      key_authorization, policy_digest, true,  // use_only_policy_authorization
      kNoCreationPCR, hmac_session->GetDelegate(), &key_blob, nullptr);
  if (result != TPM_RC_SUCCESS) {
    LOG(ERROR) << "Error creating RSA key: " << GetErrorString(result);
    return false;
  }
  TPM_HANDLE key_handle;
  result = utility->LoadKey(key_blob, hmac_session->GetDelegate(), &key_handle);
  if (result != TPM_RC_SUCCESS) {
    LOG(ERROR) << "Error loading RSA key: " << GetErrorString(result);
    return false;
  }
  ScopedKeyHandle scoped_key(*factory_.get(), key_handle);

  // Now we can reset the hmac_session.
  hmac_session.reset();

  scoped_ptr<PolicySession> policy_session = factory_->GetPolicySession();
  result = policy_session->StartUnboundSession(false);
  if (result != TPM_RC_SUCCESS) {
    LOG(ERROR) << "Error starting policy session: " << GetErrorString(result);
    return false;
  }
  result = policy_session->PolicyCommandCode(TPM_CC_Sign);
  if (result != TPM_RC_SUCCESS) {
    LOG(ERROR) << "Error restricting policy: " << GetErrorString(result);
    return false;
  }
  result = policy_session->PolicyPCR(pcr_index, "");
  if (result != TPM_RC_SUCCESS) {
    LOG(ERROR) << "Error restricting policy: " << GetErrorString(result);
    return false;
  }
  std::string signature;
  policy_session->SetEntityAuthorizationValue(key_authorization);
  // Signing with this key when pcr 2 is unchanged fails.
  result = utility->Sign(scoped_key.get(), TPM_ALG_NULL, TPM_ALG_NULL,
                         std::string(32, 'a'), policy_session->GetDelegate(),
                         &signature);
  if (GetFormatOneError(result) != TPM_RC_POLICY_FAIL) {
    LOG(ERROR) << "Error using key to sign: " << GetErrorString(result);
    return false;
  }
  scoped_ptr<AuthorizationDelegate> delegate =
      factory_->GetPasswordAuthorization("");
  result = utility->ExtendPCR(pcr_index, pcr_extend_data, delegate.get());
  if (result != TPM_RC_SUCCESS) {
    LOG(ERROR) << "Error extending pcr: " << GetErrorString(result);
    return false;
  }
  // we have to restart the session because we changed the pcr values.
  result = policy_session->StartUnboundSession(false);
  if (result != TPM_RC_SUCCESS) {
    LOG(ERROR) << "Error starting policy session: " << GetErrorString(result);
    return false;
  }
  result = policy_session->PolicyCommandCode(TPM_CC_Sign);
  if (result != TPM_RC_SUCCESS) {
    LOG(ERROR) << "Error restricting policy: " << GetErrorString(result);
    return false;
  }
  result = policy_session->PolicyPCR(pcr_index, "");
  if (result != TPM_RC_SUCCESS) {
    LOG(ERROR) << "Error restricting policy: " << GetErrorString(result);
    return false;
  }
  policy_session->SetEntityAuthorizationValue(key_authorization);
  // Signing with this key when pcr 2 is changed succeeds.
  result = utility->Sign(scoped_key.get(), TPM_ALG_NULL, TPM_ALG_NULL,
                         std::string(32, 'a'), policy_session->GetDelegate(),
                         &signature);
  if (result != TPM_RC_SUCCESS) {
    LOG(ERROR) << "Error using key to sign: " << GetErrorString(result);
    return false;
  }
  result = utility->Verify(scoped_key.get(), TPM_ALG_NULL, TPM_ALG_NULL,
                           std::string(32, 'a'), signature, nullptr);
  if (result != TPM_RC_SUCCESS) {
    LOG(ERROR) << "Error using key to verify: " << GetErrorString(result);
    return false;
  }
  std::string ciphertext;
  result = utility->AsymmetricEncrypt(key_handle, TPM_ALG_NULL, TPM_ALG_NULL,
                                      "plaintext", nullptr, &ciphertext);
  if (result != TPM_RC_SUCCESS) {
    LOG(ERROR) << "Error using key to encrypt: " << GetErrorString(result);
    return false;
  }
  result = policy_session->PolicyCommandCode(TPM_CC_Sign);
  if (result != TPM_RC_SUCCESS) {
    LOG(ERROR) << "Error restricting policy: " << GetErrorString(result);
    return false;
  }
  result = policy_session->PolicyPCR(pcr_index, "");
  if (result != TPM_RC_SUCCESS) {
    LOG(ERROR) << "Error restricting policy: " << GetErrorString(result);
    return false;
  }
  std::string plaintext;
  policy_session->SetEntityAuthorizationValue(key_authorization);
  // This call is not authorized with the policy, because its command code
  // is not TPM_CC_SIGN. It should fail with TPM_RC_POLICY_CC.
  result = utility->AsymmetricDecrypt(key_handle, TPM_ALG_NULL, TPM_ALG_NULL,
                                      ciphertext, policy_session->GetDelegate(),
                                      &plaintext);
  if (GetFormatOneError(result) != TPM_RC_POLICY_CC) {
    LOG(ERROR) << "Error: " << GetErrorString(result);
    return false;
  }
  return true;
}

bool TrunksClientTest::PolicyOrTest() {
  scoped_ptr<TpmUtility> utility = factory_->GetTpmUtility();
  scoped_ptr<PolicySession> trial_session = factory_->GetTrialSession();
  TPM_RC result;
  // Specify a policy that asserts either TPM_CC_RSA_Encrypt or
  // TPM_CC_RSA_Decrypt. A key created under this policy can only be used
  // to encrypt or decrypt.
  result = trial_session->StartUnboundSession(true);
  if (result != TPM_RC_SUCCESS) {
    LOG(ERROR) << "Error starting policy session: " << GetErrorString(result);
    return false;
  }
  result = trial_session->PolicyCommandCode(TPM_CC_Sign);
  if (result != TPM_RC_SUCCESS) {
    LOG(ERROR) << "Error restricting policy: " << GetErrorString(result);
    return false;
  }
  std::string sign_digest;
  result = trial_session->GetDigest(&sign_digest);
  if (result != TPM_RC_SUCCESS) {
    LOG(ERROR) << "Error getting policy digest: " << GetErrorString(result);
    return false;
  }
  result = trial_session->StartUnboundSession(true);
  if (result != TPM_RC_SUCCESS) {
    LOG(ERROR) << "Error starting policy session: " << GetErrorString(result);
    return false;
  }
  result = trial_session->PolicyCommandCode(TPM_CC_RSA_Decrypt);
  if (result != TPM_RC_SUCCESS) {
    LOG(ERROR) << "Error restricting policy: " << GetErrorString(result);
    return false;
  }
  std::string decrypt_digest;
  result = trial_session->GetDigest(&decrypt_digest);
  if (result != TPM_RC_SUCCESS) {
    LOG(ERROR) << "Error getting policy digest: " << GetErrorString(result);
    return false;
  }
  std::vector<std::string> digests;
  digests.push_back(sign_digest);
  digests.push_back(decrypt_digest);
  result = trial_session->PolicyOR(digests);
  if (result != TPM_RC_SUCCESS) {
    LOG(ERROR) << "Error restricting policy: " << GetErrorString(result);
    return false;
  }
  std::string policy_digest;
  result = trial_session->GetDigest(&policy_digest);
  if (result != TPM_RC_SUCCESS) {
    LOG(ERROR) << "Error getting policy digest: " << GetErrorString(result);
    return false;
  }
  // Now that we have the digest, we can close the trial session and use hmac.
  trial_session.reset();

  scoped_ptr<HmacSession> hmac_session = factory_->GetHmacSession();
  result = hmac_session->StartUnboundSession(true);
  if (result != TPM_RC_SUCCESS) {
    LOG(ERROR) << "Error starting hmac session: " << GetErrorString(result);
    return false;
  }
  std::string key_authorization("password");
  std::string key_blob;
  // This key is created with a policy that specifies that it can only be used
  // for encrypt and decrypt operations.
  result = utility->CreateRSAKeyPair(
      TpmUtility::AsymmetricKeyUsage::kDecryptAndSignKey, 2048, 0x10001,
      key_authorization, policy_digest, true,  // use_only_policy_authorization
      kNoCreationPCR, hmac_session->GetDelegate(), &key_blob, nullptr);
  if (result != TPM_RC_SUCCESS) {
    LOG(ERROR) << "Error creating RSA key: " << GetErrorString(result);
    return false;
  }
  TPM_HANDLE key_handle;
  result = utility->LoadKey(key_blob, hmac_session->GetDelegate(), &key_handle);
  if (result != TPM_RC_SUCCESS) {
    LOG(ERROR) << "Error loading RSA key: " << GetErrorString(result);
    return false;
  }
  ScopedKeyHandle scoped_key(*factory_.get(), key_handle);

  // Now we can reset the hmac_session.
  hmac_session.reset();

  scoped_ptr<PolicySession> policy_session = factory_->GetPolicySession();
  result = policy_session->StartUnboundSession(false);
  if (result != TPM_RC_SUCCESS) {
    LOG(ERROR) << "Error starting policy session: " << GetErrorString(result);
    return false;
  }
  std::string ciphertext;
  result = utility->AsymmetricEncrypt(key_handle, TPM_ALG_NULL, TPM_ALG_NULL,
                                      "plaintext", nullptr, &ciphertext);
  if (result != TPM_RC_SUCCESS) {
    LOG(ERROR) << "Error using key to encrypt: " << GetErrorString(result);
    return false;
  }
  result = policy_session->PolicyCommandCode(TPM_CC_RSA_Decrypt);
  if (result != TPM_RC_SUCCESS) {
    LOG(ERROR) << "Error restricting policy: " << GetErrorString(result);
    return false;
  }
  result = policy_session->PolicyOR(digests);
  if (result != TPM_RC_SUCCESS) {
    LOG(ERROR) << "Error restricting policy: " << GetErrorString(result);
    return false;
  }
  std::string plaintext;
  policy_session->SetEntityAuthorizationValue(key_authorization);
  // We can freely use the key for decryption.
  result = utility->AsymmetricDecrypt(key_handle, TPM_ALG_NULL, TPM_ALG_NULL,
                                      ciphertext, policy_session->GetDelegate(),
                                      &plaintext);
  if (result != TPM_RC_SUCCESS) {
    LOG(ERROR) << "Error using key to decrypt: " << GetErrorString(result);
    return false;
  }
  if (plaintext.compare("plaintext") != 0) {
    LOG(ERROR) << "Plaintext changed after encrypt + decrypt.";
    return false;
  }
  result = policy_session->PolicyCommandCode(TPM_CC_Sign);
  if (result != TPM_RC_SUCCESS) {
    LOG(ERROR) << "Error restricting policy: " << GetErrorString(result);
    return false;
  }
  result = policy_session->PolicyOR(digests);
  if (result != TPM_RC_SUCCESS) {
    LOG(ERROR) << "Error restricting policy: " << GetErrorString(result);
    return false;
  }
  std::string signature;
  policy_session->SetEntityAuthorizationValue(key_authorization);
  // However signing with a key only authorized for encrypt/decrypt should
  // fail with TPM_RC_POLICY_CC.
  result = utility->Sign(scoped_key.get(), TPM_ALG_NULL, TPM_ALG_NULL,
                         std::string(32, 'a'), policy_session->GetDelegate(),
                         &signature);
  if (result != TPM_RC_SUCCESS) {
    LOG(ERROR) << "Error using key to sign: " << GetErrorString(result);
    return false;
  }
  return true;
}

bool TrunksClientTest::NvramTest(const std::string& owner_password) {
  scoped_ptr<TpmUtility> utility = factory_->GetTpmUtility();
  scoped_ptr<HmacSession> session = factory_->GetHmacSession();
  TPM_RC result = session->StartUnboundSession(true /* enable encryption */);
  if (result != TPM_RC_SUCCESS) {
    LOG(ERROR) << "Error starting hmac session: " << GetErrorString(result);
    return false;
  }
  uint32_t index = 1;
  session->SetEntityAuthorizationValue(owner_password);
  std::string nv_data("nv_data");
  result = utility->DefineNVSpace(index, nv_data.size(),
                                  session->GetDelegate());
  if (result != TPM_RC_SUCCESS) {
    LOG(ERROR) << "Error defining nvram: " << GetErrorString(result);
    return false;
  }
  session->SetEntityAuthorizationValue(owner_password);
  result = utility->WriteNVSpace(index, 0, nv_data, session->GetDelegate());
  if (result != TPM_RC_SUCCESS) {
    LOG(ERROR) << "Error writing nvram: " << GetErrorString(result);
    return false;
  }
  std::string new_nvdata;
  session->SetEntityAuthorizationValue("");
  result = utility->ReadNVSpace(index, 0, nv_data.size(),
                                &new_nvdata, session->GetDelegate());
  if (result != TPM_RC_SUCCESS) {
    LOG(ERROR) << "Error reading nvram: " << GetErrorString(result);
    return false;
  }
  if (nv_data.compare(new_nvdata) != 0) {
    LOG(ERROR) << "NV space had different data than was written.";
    return false;
  }
  session->SetEntityAuthorizationValue(owner_password);
  result = utility->LockNVSpace(index, session->GetDelegate());
  if (result != TPM_RC_SUCCESS) {
    LOG(ERROR) << "Error locking nvram: " << GetErrorString(result);
    return false;
  }
  session->SetEntityAuthorizationValue("");
  result = utility->ReadNVSpace(index, 0, nv_data.size(),
                            &new_nvdata, session->GetDelegate());
  if (result != TPM_RC_SUCCESS) {
    LOG(ERROR) << "Error reading nvram: " << GetErrorString(result);
    return false;
  }
  if (nv_data.compare(new_nvdata) != 0) {
    LOG(ERROR) << "NV space had different data than was written.";
    return false;
  }
  session->SetEntityAuthorizationValue(owner_password);
  result = utility->WriteNVSpace(index, 0, nv_data, session->GetDelegate());
  if (result == TPM_RC_SUCCESS) {
    LOG(ERROR) << "Wrote nvram after locking: " << GetErrorString(result);
    return false;
  }
  session->SetEntityAuthorizationValue(owner_password);
  result = utility->DestroyNVSpace(index, session->GetDelegate());
  if (result != TPM_RC_SUCCESS) {
    LOG(ERROR) << "Error destroying nvram: " << GetErrorString(result);
    return false;
  }
  return true;
}

bool TrunksClientTest::ManyKeysTest() {
  const size_t kNumKeys = 20;
  std::vector<std::unique_ptr<ScopedKeyHandle>> key_handles;
  std::map<TPM_HANDLE, std::string> public_key_map;
  for (size_t i = 0; i < kNumKeys; ++i) {
    std::unique_ptr<ScopedKeyHandle> key_handle(new ScopedKeyHandle(*factory_));
    std::string public_key;
    if (!LoadSigningKey(key_handle.get(), &public_key)) {
      LOG(ERROR) << "Error loading key " << i << " into TPM.";
    }
    public_key_map[key_handle->get()] = public_key;
    key_handles.push_back(std::move(key_handle));
  }
  CHECK_EQ(key_handles.size(), kNumKeys);
  CHECK_EQ(public_key_map.size(), kNumKeys);
  scoped_ptr<AuthorizationDelegate> delegate =
      factory_->GetPasswordAuthorization("");
  for (size_t i = 0; i < kNumKeys; ++i) {
    const ScopedKeyHandle& key_handle = *key_handles[i];
    const std::string& public_key = public_key_map[key_handle.get()];
    if (!SignAndVerify(key_handle, public_key, delegate.get())) {
      LOG(ERROR) << "Error signing with key " << i;
    }
  }
  std::random_shuffle(key_handles.begin(), key_handles.end());
  for (size_t i = 0; i < kNumKeys; ++i) {
    const ScopedKeyHandle& key_handle = *key_handles[i];
    const std::string& public_key = public_key_map[key_handle.get()];
    if (!SignAndVerify(key_handle, public_key, delegate.get())) {
      LOG(ERROR) << "Error signing with shuffled key " << i;
    }
  }
  return true;
}

bool TrunksClientTest::ManySessionsTest() {
  const size_t kNumSessions = 20;
  scoped_ptr<TpmUtility> utility = factory_->GetTpmUtility();
  std::vector<std::unique_ptr<HmacSession>> sessions;
  for (size_t i = 0; i < kNumSessions; ++i) {
    std::unique_ptr<HmacSession> session(factory_->GetHmacSession().release());
    TPM_RC result = session->StartUnboundSession(true /* enable encryption */);
    if (result != TPM_RC_SUCCESS) {
      LOG(ERROR) << "Error starting hmac session " << i << ": "
                 << GetErrorString(result);
      return false;
    }
    sessions.push_back(std::move(session));
  }
  CHECK_EQ(sessions.size(), kNumSessions);
  ScopedKeyHandle key_handle(*factory_);
  std::string public_key;
  if (!LoadSigningKey(&key_handle, &public_key)) {
    return false;
  }
  for (size_t i = 0; i < kNumSessions; ++i) {
    if (!SignAndVerify(key_handle, public_key, sessions[i]->GetDelegate())) {
      LOG(ERROR) << "Error signing with hmac session " << i;
    }
  }
  std::random_shuffle(sessions.begin(), sessions.end());
  for (size_t i = 0; i < kNumSessions; ++i) {
    if (!SignAndVerify(key_handle, public_key, sessions[i]->GetDelegate())) {
      LOG(ERROR) << "Error signing with shuffled hmac session " << i;
    }
  }
  return true;
}

bool TrunksClientTest::PerformRSAEncrpytAndDecrpyt(
    TPM_HANDLE key_handle,
    const std::string& key_authorization,
    HmacSession* session) {
  scoped_ptr<TpmUtility> utility = factory_->GetTpmUtility();
  std::string ciphertext;
  session->SetEntityAuthorizationValue("");
  TPM_RC result = utility->AsymmetricEncrypt(key_handle, TPM_ALG_NULL,
                                             TPM_ALG_NULL, "plaintext",
                                             session->GetDelegate(),
                                             &ciphertext);
  if (result != TPM_RC_SUCCESS) {
    LOG(ERROR) << "Error using key to encrypt: " << GetErrorString(result);
    return false;
  }
  std::string plaintext;
  session->SetEntityAuthorizationValue(key_authorization);
  result = utility->AsymmetricDecrypt(key_handle, TPM_ALG_NULL,
                                      TPM_ALG_NULL, ciphertext,
                                      session->GetDelegate(), &plaintext);
  if (result != TPM_RC_SUCCESS) {
    LOG(ERROR) << "Error using key to decrypt: " << GetErrorString(result);
    return false;
  }
  if (plaintext.compare("plaintext") != 0) {
    LOG(ERROR) << "Plaintext changed after encrypt + decrypt.";
    return false;
  }
  return true;
}

void TrunksClientTest::GenerateRSAKeyPair(std::string* modulus,
                                          std::string* prime_factor,
                                          std::string* public_key) {
#if defined(OPENSSL_IS_BORINGSSL)
  crypto::ScopedRSA rsa(RSA_new());
  crypto::ScopedBIGNUM exponent(BN_new());
  CHECK(BN_set_word(exponent.get(), RSA_F4));
  CHECK(RSA_generate_key_ex(rsa.get(), 2048, exponent.get(), nullptr))
      << "Failed to generate RSA key: " << GetOpenSSLError();
#else
  crypto::ScopedRSA rsa(RSA_generate_key(2048, 0x10001, nullptr, nullptr));
  CHECK(rsa.get());
#endif
  modulus->resize(BN_num_bytes(rsa.get()->n), 0);
  BN_bn2bin(rsa.get()->n,
            reinterpret_cast<unsigned char*>(string_as_array(modulus)));
  prime_factor->resize(BN_num_bytes(rsa.get()->p), 0);
  BN_bn2bin(rsa.get()->p,
            reinterpret_cast<unsigned char*>(string_as_array(prime_factor)));
  if (public_key) {
    unsigned char* buffer = NULL;
    int length = i2d_RSAPublicKey(rsa.get(), &buffer);
    CHECK_GT(length, 0);
    crypto::ScopedOpenSSLBytes scoped_buffer(buffer);
    public_key->assign(reinterpret_cast<char*>(buffer), length);
  }
}

bool TrunksClientTest::VerifyRSASignature(const std::string& public_key,
                                          const std::string& data,
                                          const std::string& signature) {
  auto asn1_ptr = reinterpret_cast<const unsigned char*>(public_key.data());
  crypto::ScopedRSA rsa(d2i_RSAPublicKey(nullptr, &asn1_ptr,
                                         public_key.size()));
  CHECK(rsa.get());
  std::string digest = crypto::SHA256HashString(data);
  auto digest_buffer = reinterpret_cast<const unsigned char*>(digest.data());
  std::string mutable_signature(signature);
  unsigned char* signature_buffer =
      reinterpret_cast<unsigned char*>(string_as_array(&mutable_signature));
  return (RSA_verify(NID_sha256, digest_buffer, digest.size(),
                     signature_buffer, signature.size(), rsa.get()) == 1);
}

bool TrunksClientTest::LoadSigningKey(ScopedKeyHandle* key_handle,
                                      std::string* public_key) {
  std::string modulus;
  std::string prime_factor;
  GenerateRSAKeyPair(&modulus, &prime_factor, public_key);
  std::string key_blob;
  scoped_ptr<TpmUtility> utility = factory_->GetTpmUtility();
  TPM_RC result = utility->ImportRSAKey(
      TpmUtility::AsymmetricKeyUsage::kSignKey,
      modulus, 0x10001, prime_factor,
      "",  // password
      factory_->GetPasswordAuthorization("").get(),
      &key_blob);
  if (result != TPM_RC_SUCCESS) {
    LOG(ERROR) << "ImportRSAKey: " << GetErrorString(result);
    return false;
  }
  TPM_HANDLE raw_key_handle;
  result = utility->LoadKey(key_blob,
                            factory_->GetPasswordAuthorization("").get(),
                            &raw_key_handle);
  if (result != TPM_RC_SUCCESS) {
    LOG(ERROR) << "LoadKey: " << GetErrorString(result);
    return false;
  }
  key_handle->reset(raw_key_handle);
  return true;
}

bool TrunksClientTest::SignAndVerify(const ScopedKeyHandle& key_handle,
                                     const std::string& public_key,
                                     AuthorizationDelegate* delegate) {
  std::string signature;
  std::string data_to_sign("sign_this");
  scoped_ptr<TpmUtility> utility = factory_->GetTpmUtility();
  TPM_RC result = utility->Sign(key_handle.get(),
                                TPM_ALG_RSASSA,
                                TPM_ALG_SHA256,
                                data_to_sign,
                                delegate,
                                &signature);
  if (result != TPM_RC_SUCCESS) {
    LOG(ERROR) << "Sign: " << GetErrorString(result);
    return false;
  }
  if (!VerifyRSASignature(public_key, data_to_sign, signature)) {
    LOG(ERROR) << "Signature verification failed: " << GetOpenSSLError();
    return false;
  }
  return true;
}

}  // namespace trunks
