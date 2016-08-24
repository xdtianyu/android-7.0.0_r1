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

#ifndef TRUNKS_TRUNKS_CLIENT_TEST_H_
#define TRUNKS_TRUNKS_CLIENT_TEST_H_

#include <string>

#include <base/memory/scoped_ptr.h>

#include "trunks/scoped_key_handle.h"
#include "trunks/tpm_generated.h"
#include "trunks/trunks_factory.h"

namespace trunks {

// This class is used to perform integration tests on the TPM. Each public
// method defines a different test to perform.
// NOTE: All these tests require that the TPM be owned, and SRKs exist.
// Example usage:
// TrunksClientTest test;
// CHECK(test.RNGTest());
// CHECK(test.SimplePolicyTest());
class TrunksClientTest {
 public:
  TrunksClientTest();
  // Takes ownership of factory.
  explicit TrunksClientTest(scoped_ptr<TrunksFactory> factory);
  virtual ~TrunksClientTest();

  // This test verifies that the Random Number Generator on the TPM is working
  // correctly.
  bool RNGTest();

  // This test verifies that we can create an unrestricted RSA signing key and
  // use it to sign arbitrary data.
  bool SignTest();

  // This test verfifies that we can create an unrestricted RSA decryption key
  // and use it to encrypt and decrypt arbitrary data.
  bool DecryptTest();

  // This test verifies that we can import a RSA key into the TPM and use it
  // to encrypt and decrypt some data.
  bool ImportTest();

  // This test verifies that we can change a key's authorization data and
  // still use it to encrypt/decrypt data.
  bool AuthChangeTest();

  // This test verifies that we can create a key and then confirm that it
  // was created by the TPM.
  bool VerifyKeyCreationTest();

  // This test verifies that we can seal a secret to the TPM and access
  // it later.
  bool SealedDataTest();

  // This test performs a simple PCR extension and then reads the value in the
  // PCR to verify if it is correct.
  // NOTE: PCR banks need to be configured for this test to succeed. Normally
  // this is done by the platform firmware.
  bool PCRTest();

  // This test sets up a PolicySession with the PolicyAuthValue assertion.
  // This policy is then used to create a key and use it to sign/verify and
  // encrypt/decrypt.
  bool PolicyAuthValueTest();

  // This test sets up a PolicySession that is based on the current PCR value
  // and a CommandCode for signing. The key created this way is restricted to
  // be only used for signing, and only if the PCR remains unchanged. The key
  // is then used to sign arbitrary data, and the signature verified.
  bool PolicyAndTest();

  // This test performs a complex assertion using PolicyOR.
  // We create an unrestricted key, and restricts it to signing
  // and decryption using Policy Sessions.
  bool PolicyOrTest();

  // This test verfies that we can create, write, read, lock and delete
  // NV spaces in the TPM.
  // NOTE: This test needs the |owner_password| to work.
  bool NvramTest(const std::string& owner_password);

  // This test uses many key handles simultaneously.
  bool ManyKeysTest();

  // This test uses many sessions simultaneously.
  bool ManySessionsTest();

 private:
  // This method verifies that plaintext == decrypt(encrypt(plaintext)) using
  // a given key.
  // TODO(usanghi): Remove |session| argument once we can support multiple
  // sessions.
  bool PerformRSAEncrpytAndDecrpyt(TPM_HANDLE key_handle,
                                   const std::string& key_authorization,
                                   HmacSession* session);

  // Generates an RSA key pair in software. The |modulus| and |prime_factor|
  // must not be NULL and will be populated with values that can be imported
  // into the TPM. The |public_key| may be NULL, but if it is not, will be
  // populated with a value that can be used with VerifyRSASignature.
  void GenerateRSAKeyPair(std::string* modulus,
                          std::string* prime_factor,
                          std::string* public_key);

  // Verifies an RSA-SSA-SHA256 |signature| over the given |data|. The
  // |public_key| is as produced by GenerateRSAKeyPair(). Returns true on
  // success.
  bool VerifyRSASignature(const std::string& public_key,
                          const std::string& data,
                          const std::string& signature);

  // Loads an arbitrary RSA signing key and provides the |key_handle| and the
  // |public_key|. Returns true on success.
  bool LoadSigningKey(ScopedKeyHandle* key_handle, std::string* public_key);

  // Signs arbitrary data with |key_handle| authorized by |delegate| and
  // verifies the signature with |public_key|. Returns true on success.
  bool SignAndVerify(const ScopedKeyHandle& key_handle,
                     const std::string& public_key,
                     AuthorizationDelegate* delegate);

  // Factory for instantiation of Tpm classes
  scoped_ptr<TrunksFactory> factory_;

  DISALLOW_COPY_AND_ASSIGN(TrunksClientTest);
};

}  // namespace trunks

#endif  // TRUNKS_TRUNKS_CLIENT_TEST_H_
