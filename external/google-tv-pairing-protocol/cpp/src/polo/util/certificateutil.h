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

#ifndef POLO_UTIL_CERTIFICATEUTIL_H_
#define POLO_UTIL_CERTIFICATEUTIL_H_

#include <stdint.h>
#include <openssl/ssl.h>
#include <string>

// Fixes a problem with X509_NAME on Windows.
#undef X509_NAME

namespace polo {
namespace util {

class CertificateUtil {
 public:
  // Reads an X509 certificate from a PEM encoded string.
  // @param pem the PEM encoded string
  // @return a pointer to a new X509 certificate or NULL if there was an error
  //         loading the certificate
  static X509* X509FromPEM(std::string pem);

  // Converts an X509 certificate to a PEM encoded string.
  // @param x509 the X509 certificate
  // @return a PEM encoded string of the given certificate
  static std::string X509ToPEM(X509* x509);

  // Loads a private key from a PEM encoded string.
  // @param pem the PEM encoded string
  // @param passphrase the private key passphrase
  // @return a pointer to a new EVP_PKEY or NULL if there was an error loading
  //         the private key
  static EVP_PKEY* PKEYFromPEM(std::string pem,
                               std::string passphrase);

  // Converts a private key to a PEM encoded string.
  // @param pkey the private key
  // @param passphrase the private key passphrase to use
  // @return a PEM encoded string of the given private key
  static std::string PKEYToPEM(EVP_PKEY* pkey,
                               std::string passphrase);

  // Generates a new private key.
  // @return a new RSA private key that can be used to create a self-signed cert
  static EVP_PKEY* GeneratePrivateKey();

  // Generates a self-signed X509 certificate.
  // @param pkey the private key
  // @param subject_name the subject name
  // @param days the number of days before the certificate expires
  // @return a new self-signed X509 certificate
  static X509* GenerateSelfSignedCert(EVP_PKEY* pkey,
                                      std::string subject_name,
                                      uint32_t days);
};

}  // namespace util
}  // namespace polo

#endif  // POLO_UTIL_CERTIFICATEUTIL_H_
