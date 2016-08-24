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

#ifndef POLO_PAIRING_POLOCHALLENGERESPONSE_H_
#define POLO_PAIRING_POLOCHALLENGERESPONSE_H_

#include <openssl/x509v3.h>
#include <openssl/ssl.h>
#include <stdint.h>
#include <vector>
#include "polo/util/macros.h"

namespace polo {
namespace pairing {

typedef std::vector<uint8_t> Alpha, Gamma, Nonce;

// A Polo challenge response that contains the certificate keys.
class PoloChallengeResponse {
 public:
  // Creates a new challenge response with the given certificates. This does not
  // take ownership of the given pointers.
  // @param client_cert the client certificate
  // @param server_cert the server certificate
  PoloChallengeResponse(X509* client_cert, X509* server_cert);

  virtual ~PoloChallengeResponse() {}

  // Computes the alpha value based on the given nonce.
  virtual Alpha* GetAlpha(const Nonce& nonce) const;

  // Computes the gamma value based on the given nonce.
  virtual Gamma* GetGamma(const Nonce& nonce) const;

  // Extracts the nonce from the given gamma value.
  virtual Nonce* ExtractNonce(const Gamma& gamma) const;

  // Verifies that the given gamma value is correct.
  virtual bool CheckGamma(const Gamma& gamma) const;
 private:
  X509* client_cert;
  X509* server_cert;

  DISALLOW_COPY_AND_ASSIGN(PoloChallengeResponse);
};

}  // namespace pairing
}  // namespace polo

#endif  // POLO_PAIRING_POLOCHALLENGERESPONSE_H_
