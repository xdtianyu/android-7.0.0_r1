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

// Utilities for converting SSL certificates using OpenSSL functions.

#include "polo/util/certificateutil.h"

#include <glog/logging.h>

namespace polo {
namespace util {

X509* CertificateUtil::X509FromPEM(std::string pem) {
  BIO* bio = BIO_new_mem_buf(&pem[0], pem.size());
  X509* x509 = PEM_read_bio_X509(bio, NULL, 0, NULL);
  BIO_free(bio);

  // Ensure that the certificate is not expired.
  if (X509_cmp_current_time(X509_get_notBefore(x509)) > 0
      || X509_cmp_current_time(X509_get_notAfter(x509)) < 0) {
    LOG(ERROR) << "Expired certificate";
    X509_free(x509);
    return NULL;
  }

  return x509;
}

std::string CertificateUtil::X509ToPEM(X509* x509) {
  BIO* bio = BIO_new(BIO_s_mem());
  PEM_write_bio_X509(bio, x509);
  BIO_flush(bio);

  char* data = NULL;
  size_t data_size = BIO_get_mem_data(bio, &data);

  std::string pem(data, data_size);
  BIO_free(bio);

  return pem;
}

EVP_PKEY* CertificateUtil::PKEYFromPEM(std::string pem,
                                       std::string passphrase) {
  BIO* bio = BIO_new_mem_buf(&pem[0], pem.size());
  EVP_PKEY* pkey = PEM_read_bio_PrivateKey(bio, NULL, 0, &passphrase[0]);
  BIO_free(bio);

  return pkey;
}

std::string CertificateUtil::PKEYToPEM(EVP_PKEY* pkey,
                                       std::string passphrase) {
  BIO* bio = BIO_new(BIO_s_mem());
  PEM_write_bio_PrivateKey(bio, pkey, EVP_des_ede3_cbc(), NULL, 0, 0,
      &passphrase[0]);
  BIO_flush(bio);

  char* data = NULL;
  int data_size = BIO_get_mem_data(bio, &data);

  std::string pem(data, data_size);
  BIO_free(bio);

  return pem;
}

EVP_PKEY* CertificateUtil::GeneratePrivateKey() {
  EVP_PKEY* pkey = EVP_PKEY_new();
  RSA* rsa = RSA_generate_key(1025, RSA_F4, NULL, NULL);
  EVP_PKEY_assign_RSA(pkey, rsa);
  return pkey;
}

X509* CertificateUtil::GenerateSelfSignedCert(EVP_PKEY* pkey,
                                              std::string subject_name,
                                              uint32_t days) {
  X509* x509 = X509_new();
  X509_set_version(x509, 2);
  ASN1_INTEGER_set(X509_get_serialNumber(x509), 0);
  X509_gmtime_adj(X509_get_notBefore(x509), 0);
  X509_gmtime_adj(X509_get_notAfter(x509), (int64_t) 60 * 60 * 24 * days);
  X509_set_pubkey(x509, pkey);

  X509_NAME* name = X509_get_subject_name(x509);
  X509_NAME_add_entry_by_NID(name, NID_commonName, MBSTRING_ASC,
      (unsigned char*) &subject_name[0], -1, -1, 0);

  X509_set_issuer_name(x509, name);
  X509_sign(x509, pkey, EVP_sha256());

  return x509;
}

}  // namespace util
}  // namespace polo
