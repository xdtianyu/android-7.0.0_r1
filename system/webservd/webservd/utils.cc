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

#include "webservd/utils.h"

#include <sys/socket.h>

#include <openssl/bio.h>
#include <openssl/evp.h>
#include <openssl/pem.h>
#include <openssl/rsa.h>

#include <base/files/file_util.h>
#include <base/logging.h>
#include <base/strings/string_number_conversions.h>
#include <base/time/time.h>

namespace webservd {

namespace {

// Returns the current date/time. This is used for TLS certificate validation
// very early in process start when the system clock might not be adjusted
// yet on devices that don't have a real-time clock. So, try to get the system
// time and if it is earlier than the build date of this executable, use
// the build date instead as a lower limit to the date/time.
// See http://b/23897170 for more details.
base::Time GetTimeNow() {
  base::Time now = base::Time::Now();

  base::File::Info info;
  base::FilePath exe_path;
  if (!base::ReadSymbolicLink(base::FilePath{"/proc/self/exe"}, &exe_path) ||
      !base::GetFileInfo(exe_path, &info) || info.creation_time < now) {
    return now;
  }

  LOG(WARNING) << "Current time (" << now
               << ") is earlier than the application build time. Using "
               << info.creation_time << " instead!";
  return info.creation_time;
}

}  // anonymous namespace

X509Ptr CreateCertificate(int serial_number,
                          const base::TimeDelta& cert_expiration,
                          const std::string& common_name) {
  auto cert = X509Ptr{X509_new(), X509_free};
  CHECK(cert.get());
  X509_set_version(cert.get(), 2);  // Using X.509 v3 certificate...

  // Set certificate properties...
  ASN1_INTEGER* sn = X509_get_serialNumber(cert.get());
  ASN1_INTEGER_set(sn, serial_number);
  time_t current_time = GetTimeNow().ToTimeT();
  X509_time_adj(X509_get_notBefore(cert.get()), 0, &current_time);
  X509_time_adj(X509_get_notAfter(cert.get()), cert_expiration.InSeconds(),
                &current_time);

  // The issuer is the same as the subject, since this cert is self-signed.
  X509_NAME* name = X509_get_subject_name(cert.get());
  if (!common_name.empty()) {
    X509_NAME_add_entry_by_txt(
        name, "CN", MBSTRING_UTF8,
        reinterpret_cast<const unsigned char*>(common_name.c_str()),
        common_name.length(), -1, 0);
  }
  X509_set_issuer_name(cert.get(), name);
  return cert;
}

std::unique_ptr<RSA, void(*)(RSA*)> GenerateRSAKeyPair(int key_length_bits) {
  // Create RSA key pair.
  auto rsa_key_pair = std::unique_ptr<RSA, void(*)(RSA*)>{RSA_new(), RSA_free};
  CHECK(rsa_key_pair.get());
  auto big_num = std::unique_ptr<BIGNUM, void(*)(BIGNUM*)>{BN_new(), BN_free};
  CHECK(big_num.get());
  CHECK(BN_set_word(big_num.get(), 65537));
  CHECK(RSA_generate_key_ex(rsa_key_pair.get(), key_length_bits, big_num.get(),
                            nullptr));
  return rsa_key_pair;
}

brillo::SecureBlob StoreRSAPrivateKey(RSA* rsa_key_pair) {
  auto bio =
      std::unique_ptr<BIO, void(*)(BIO*)>{BIO_new(BIO_s_mem()), BIO_vfree};
  CHECK(bio);
  CHECK(PEM_write_bio_RSAPrivateKey(bio.get(), rsa_key_pair, nullptr, nullptr,
                                    0, nullptr, nullptr));
  uint8_t* buffer = nullptr;
  size_t size = BIO_get_mem_data(bio.get(), reinterpret_cast<char**>(&buffer));
  CHECK_GT(size, 0u);
  CHECK(buffer);
  brillo::SecureBlob key_blob(buffer, buffer + size);
  brillo::SecureMemset(buffer, 0, size);
  return key_blob;
}

bool ValidateRSAPrivateKey(const brillo::SecureBlob& key) {
  std::unique_ptr<BIO, void(*)(BIO*)> bio{
      BIO_new_mem_buf(const_cast<uint8_t*>(key.data()), key.size()), BIO_vfree};
  std::unique_ptr<RSA, void(*)(RSA*)> rsa_key{
      PEM_read_bio_RSAPrivateKey(bio.get(), nullptr, nullptr, nullptr),
      RSA_free};
  return rsa_key.get() != nullptr;
}

brillo::Blob StoreCertificate(X509* cert) {
  auto bio =
      std::unique_ptr<BIO, void(*)(BIO*)>{BIO_new(BIO_s_mem()), BIO_vfree};
  CHECK(bio);
  CHECK(PEM_write_bio_X509(bio.get(), cert));
  uint8_t* buffer = nullptr;
  size_t size = BIO_get_mem_data(bio.get(), reinterpret_cast<char**>(&buffer));
  CHECK_GT(size, 0u);
  CHECK(buffer);
  return brillo::Blob(buffer, buffer + size);
}

bool StoreCertificate(X509* cert, const base::FilePath& file) {
  std::unique_ptr<BIO, void(*)(BIO*)> bio{
      BIO_new_file(file.value().c_str(), "w"), BIO_vfree};
  return bio && PEM_write_bio_X509(bio.get(), cert) != 0;
}

X509Ptr LoadAndValidateCertificate(const base::FilePath& file) {
  X509Ptr cert{nullptr, X509_free};
  std::unique_ptr<BIO, void(*)(BIO*)> bio{
      BIO_new_file(file.value().c_str(), "r"), BIO_vfree};
  if (!bio)
    return cert;
  LOG(INFO) << "Loading certificate from " << file.value();
  cert.reset(PEM_read_bio_X509(bio.get(), nullptr, nullptr, nullptr));
  if (cert) {
    // Regenerate certificate 30 days before it expires.
    time_t deadline = (GetTimeNow() + base::TimeDelta::FromDays(30)).ToTimeT();
    if (X509_cmp_time(X509_get_notAfter(cert.get()), &deadline) < 0) {
      LOG(WARNING) << "Certificate is expiring soon. Regenerating new one.";
      cert.reset();
    }
  }
  return cert;
}

// Same as openssl x509 -fingerprint -sha256.
brillo::Blob GetSha256Fingerprint(X509* cert) {
  brillo::Blob fingerprint(256 / 8);
  uint32_t len = 0;
  CHECK(X509_digest(cert, EVP_sha256(), fingerprint.data(), &len));
  CHECK_EQ(len, fingerprint.size());
  return fingerprint;
}

int CreateNetworkInterfaceSocket(const std::string& if_name) {
  // The following is basically the steps libmicrohttpd normally takes
  // when creating a new listening socket and binding it to a port.
  int socket_fd = socket(PF_INET6, SOCK_STREAM | SOCK_CLOEXEC, 0);
  if (socket_fd < 0 && errno == EINVAL)
    socket_fd = socket(PF_INET6, SOCK_STREAM, 0);
  if (socket_fd < 0) {
    PLOG(ERROR) << "Unable to create a listening socket";
    return -1;
  }

  // Now, specify that we want this socket to bind only to a particular
  // network interface. Note, this call requires root privileges, so this
  // should be done before the privileges are dropped.
  if (setsockopt(socket_fd, SOL_SOCKET, SO_BINDTODEVICE,
                 if_name.c_str(), if_name.size()) < 0) {
    PLOG(WARNING) << "Failed to bind socket (SO_BINDTODEVICE) to " << if_name;
    close(socket_fd);
    return -1;
  }
  return socket_fd;
}

}  // namespace webservd
