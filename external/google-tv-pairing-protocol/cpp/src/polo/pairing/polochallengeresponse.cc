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

// This class performs several computations on the client and server public keys
// to generate and verify challenge hashes.

#include "polo/pairing/polochallengeresponse.h"

#include <glog/logging.h>
#include "polo/util/poloutil.h"

namespace polo {
namespace pairing {

PoloChallengeResponse::PoloChallengeResponse(X509* client_cert,
                                             X509* server_cert)
    : client_cert(client_cert),
      server_cert(server_cert) {
}

Alpha* PoloChallengeResponse::GetAlpha(const Nonce& nonce) const {
  EVP_PKEY* client_pkey = X509_get_pubkey(client_cert);
  if (!client_pkey) {
    return NULL;
  }

  RSA* client_pub_rsa = EVP_PKEY_get1_RSA(client_pkey);
  if (!client_pub_rsa) {
    return NULL;
  }

  EVP_PKEY* server_pkey = X509_get_pubkey(server_cert);
  if (!server_pkey) {
    return NULL;
  }

  RSA* server_pub_rsa = EVP_PKEY_get1_RSA(server_pkey);
  if (!server_pub_rsa) {
    return NULL;
  }

  // Compute a hash of the concatenated public keys. The client and server
  // modulus and exponent are concatenated along with the random nonce then a
  // SHA256 hash is computed on the result.
  size_t client_modulus_size = BN_num_bytes(client_pub_rsa->n);
  size_t client_exponent_size = BN_num_bytes(client_pub_rsa->e);

  size_t server_modulus_size = BN_num_bytes(server_pub_rsa->n);
  size_t server_exponent_size = BN_num_bytes(server_pub_rsa->e);

  size_t buffer_size = client_modulus_size + client_exponent_size
            + server_modulus_size + server_exponent_size
            + nonce.size();

  uint8_t* buffer = new unsigned char[buffer_size];
  uint8_t* pos = buffer;

  BN_bn2bin(client_pub_rsa->n, pos);
  pos += client_modulus_size;

  BN_bn2bin(client_pub_rsa->e, pos);
  pos += client_exponent_size;

  BN_bn2bin(server_pub_rsa->n, pos);
  pos += server_modulus_size;

  BN_bn2bin(server_pub_rsa->e, pos);
  pos += server_exponent_size;

  memcpy(pos, &nonce[0], nonce.size());

  Alpha* alpha = new Alpha(SHA256_DIGEST_LENGTH);
  SHA256(buffer, buffer_size, &(*alpha)[0]);
  delete[] buffer;

  RSA_free(client_pub_rsa);
  EVP_PKEY_free(client_pkey);

  RSA_free(server_pub_rsa);
  EVP_PKEY_free(server_pkey);

  return alpha;
}

Gamma* PoloChallengeResponse::GetGamma(const Nonce& nonce) const {
  const Alpha* alpha = GetAlpha(nonce);
  if (!alpha) {
    return NULL;
  }

  Gamma* gamma = new Gamma(nonce.size() * 2);

  if (alpha->size() >= nonce.size()) {
    memcpy(&(*gamma)[0], &(*alpha)[0], nonce.size());
    memcpy(&(*gamma)[nonce.size()], &nonce[0], nonce.size());
  }
  delete alpha;

  return gamma;
}

Nonce* PoloChallengeResponse::ExtractNonce(const Gamma& gamma) const {
  if ((gamma.size() < 2) || (gamma.size() % 2 != 0)) {
    return NULL;
  }

  Nonce* nonce = new Nonce(gamma.size() / 2);
  memcpy(&(*nonce)[0], &gamma[nonce->size()], nonce->size());

  return nonce;
}

bool PoloChallengeResponse::CheckGamma(const Gamma& gamma) const {
  const Nonce* nonce = ExtractNonce(gamma);

  if (!nonce) {
    return false;
  }

  const Gamma* expected = GetGamma(*nonce);

  LOG(INFO) << "CheckGamma expected: "
      << util::PoloUtil::BytesToHexString(&(*expected)[0], expected->size())
      << " actual: "
      << util::PoloUtil::BytesToHexString(&gamma[0], gamma.size());

  bool check = (gamma == (*expected));

  delete nonce;
  delete expected;

  return check;
}

}  // namespace pairing
}  // namespace polo
