// Copyright 2015 The Android Open Source Project
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

#ifndef WEBSERVER_WEBSERVD_UTILS_H_
#define WEBSERVER_WEBSERVD_UTILS_H_

#include <memory>
#include <string>
#include <vector>
#include <openssl/ossl_typ.h>

#include <base/files/file_path.h>
#include <base/time/time.h>
#include <brillo/secure_blob.h>

namespace webservd {

using X509Ptr = std::unique_ptr<X509, void(*)(X509*)>;

// Creates a new X509 certificate.
X509Ptr CreateCertificate(int serial_number,
                          const base::TimeDelta& cert_expiration,
                          const std::string& common_name);

// Generates an RSA public-private key pair of the specified strength.
std::unique_ptr<RSA, void(*)(RSA*)> GenerateRSAKeyPair(int key_length_bits);

// Serializes a private key from the key pair into a PEM string and returns
// it as a binary blob.
brillo::SecureBlob StoreRSAPrivateKey(RSA* rsa_key_pair);

// Checks if the buffer |key| contains a valid RSA private key.
bool ValidateRSAPrivateKey(const brillo::SecureBlob& key);

// Serializes an X509 certificate using PEM format.
brillo::Blob StoreCertificate(X509* cert);

// Stores/loads an X509 certificate to/from a file (in PEM format).
bool StoreCertificate(X509* cert, const base::FilePath& file);
X509Ptr LoadAndValidateCertificate(const base::FilePath& file);

// Same as openssl x509 -fingerprint -sha256.
brillo::Blob GetSha256Fingerprint(X509* cert);

// Creates a socket bound to a specified network interface.
// Returns a socket file descriptor or -1 on error.
int CreateNetworkInterfaceSocket(const std::string& if_name);

}  // namespace webservd

#endif  // WEBSERVER_WEBSERVD_UTILS_H_
