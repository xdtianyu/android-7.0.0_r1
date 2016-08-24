//
// Copyright (C) 2013 The Android Open Source Project
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

#include <unistd.h>

#include <limits>
#include <string>
#include <vector>

#include <base/command_line.h>
#include <base/logging.h>
#include <base/posix/eintr_wrapper.h>
#include <brillo/syslog_logging.h>
#include <openssl/bio.h>
#include <openssl/conf.h>
#include <openssl/err.h>
#include <openssl/evp.h>
#include <openssl/pem.h>
#include <openssl/rsa.h>
#include <openssl/sha.h>
#include <openssl/x509.h>

#include "shill/shims/protos/crypto_util.pb.h"

using shill_protos::EncryptDataMessage;
using shill_protos::EncryptDataResponse;
using shill_protos::VerifyCredentialsMessage;
using shill_protos::VerifyCredentialsResponse;
using std::numeric_limits;
using std::string;
using std::vector;

namespace {

const char kTrustedCAModulus[] =
    "BC2280BD80F63A21003BAE765E357F3DC3645C559486342F058728CDF7698C17B350A7B8"
    "82FADFC7432DD67EABA06FB7137280A44715C1209950CDEC1462095BA498CDD241B6364E"
    "FFE82E32304A81A842A36C9B336ECAB2F55366E02753861A851EA7393F4A778EFB546666"
    "FB5854C05E39C7F550060BE08AD4CEE16A551F8B1700E669A327E60825693C129D8D052C"
    "D62EA231DEB45250D62049DE71A0F9AD204012F1DD25EBD5E6B836F4D68F7FCA43DCD710"
    "5BE63F518A85B3F3FFF6032DCB234F9CAD18E793058CAC529AF74CE9997ABE6E7E4D0AE3"
    "C61CA993FA3AA5915D1CBD66EBCC60DC8674CACFF8921C987D57FA61479EAB80B7E44880"
    "2A92C51B";
const char kCommandVerify[] = "verify";
const char kCommandEncrypt[] = "encrypt";
const size_t kMacLength = 12;

// Encrypt |data| with |public_key|.  |public_key| is the raw bytes of a key in
// RSAPublicKey format.  |data| is some string of bytes smaller than the
// maximum length permissable for encryption with a key of |public_key| size.
// |rsa_ptr| should point to NULL (but should not be NULL).  This function may
// set *|rsa_ptr| to an RSA object which should be freed in the caller.
// Returns the encrypted result in |encrypted_output| and returns true on
// success.  Returns false on failure.
bool EncryptByteStringImpl(const string& public_key,
                           const string& data,
                           RSA** rsa_ptr,
                           string* encrypted_output) {
  CHECK(rsa_ptr);
  CHECK(!*rsa_ptr);
  CHECK(encrypted_output);

  // This pointer will be incremented internally by the parsing routine.
  const unsigned char* throwaway_ptr =
      reinterpret_cast<const unsigned char*>(public_key.data());
  *rsa_ptr = d2i_RSAPublicKey(NULL, &throwaway_ptr, public_key.length());
  RSA* rsa = *rsa_ptr;
  if (!rsa) {
    LOG(ERROR) << "Failed to parse public key.";
    return false;
  }

  vector<unsigned char> rsa_output(RSA_size(rsa));
  LOG(INFO) << "Encrypting data with public key.";
  const int encrypted_length = RSA_public_encrypt(
      data.length(),
      // The API helpfully tells us that this operation will treat this buffer
      // as read only, but fails to mark the parameter const.
      reinterpret_cast<unsigned char*>(const_cast<char*>(data.data())),
      rsa_output.data(),
      rsa,
      RSA_PKCS1_PADDING);
  if (encrypted_length <= 0) {
    LOG(ERROR) << "Error during encryption.";
    return false;
  }

  encrypted_output->assign(reinterpret_cast<char*>(rsa_output.data()),
                           encrypted_length);
  return true;
}

// Parse the EncryptDataMessage contained in |raw_input| and return an
// EncryptDataResponse in output on success.  Returns true on success and
// false otherwise.
bool EncryptByteString(const string& raw_input, string* output) {
  EncryptDataMessage message;
  if (!message.ParseFromString(raw_input)) {
    LOG(ERROR) << "Failed to read VerifyCredentialsMessage from stdin.";
    return false;
  }

  if (!message.has_public_key() || !message.has_data()) {
    LOG(ERROR) << "Request lacked necessary fields.";
    return false;
  }

  RSA* rsa = NULL;
  string encrypted_output;
  bool operation_successful = EncryptByteStringImpl(
      message.public_key(), message.data(), &rsa, &encrypted_output);
  if (rsa) {
    RSA_free(rsa);
    rsa = NULL;
  }

  if (operation_successful) {
    LOG(INFO) << "Filling out protobuf.";
    EncryptDataResponse response;
    response.set_encrypted_data(encrypted_output);
    response.set_ret(shill_protos::OK);
    output->clear();
    LOG(INFO) << "Serializing protobuf.";
    if (!response.SerializeToString(output)) {
      LOG(ERROR) << "Failed while writing encrypted data.";
      return false;
    }
    LOG(INFO) << "Encoding finished successfully.";
  }

  return operation_successful;
}

// Verify that the destination described by |certificate| is valid.
//
// 1) The MAC address listed in the certificate matches |connected_mac|.
// 2) The certificate is a valid PEM encoded certificate signed by our
//    trusted CA.
// 3) |signed_data| matches the hashed |unsigned_data| encrypted with
//    the public key in |certificate|.
//
// All pointers should be valid, but point to NULL values.  Sets* ptr to
// NULL or a valid object which should be freed with the appropriate destructor
// upon completion.
bool VerifyCredentialsImpl(const string& certificate,
                           const string& signed_data,
                           const string& unsigned_data,
                           const string& connected_mac,
                           RSA** rsa_ptr,
                           EVP_PKEY** pkey_ptr,
                           BIO** raw_certificate_bio_ptr,
                           X509** x509_ptr) {
  CHECK(rsa_ptr);
  CHECK(pkey_ptr);
  CHECK(raw_certificate_bio_ptr);
  CHECK(x509_ptr);
  CHECK(!*rsa_ptr);
  CHECK(!*pkey_ptr);
  CHECK(!*raw_certificate_bio_ptr);
  CHECK(!*x509_ptr);

  *rsa_ptr = RSA_new();
  RSA* rsa = *rsa_ptr;
  *pkey_ptr = EVP_PKEY_new();
  EVP_PKEY* pkey = *pkey_ptr;
  if (!rsa || !pkey) {
    LOG(ERROR) << "Failed to allocate key.";
    return false;
  }

  rsa->e = BN_new();
  rsa->n = BN_new();
  if (!rsa->e || !rsa->n ||
      !BN_set_word(rsa->e, RSA_F4) ||
      !BN_hex2bn(&rsa->n, kTrustedCAModulus)) {
    LOG(ERROR) << "Failed to allocate key pieces.";
    return false;
  }

  if (!EVP_PKEY_assign_RSA(pkey, rsa)) {
    LOG(ERROR) << "Failed to assign RSA to PKEY.";
    return false;
  }

  *rsa_ptr = NULL;  // pkey took ownership
  // Another helpfully unmarked const interface.
  *raw_certificate_bio_ptr = BIO_new_mem_buf(
      const_cast<char*>(certificate.data()), certificate.length());
  BIO* raw_certificate_bio = *raw_certificate_bio_ptr;
  if (!raw_certificate_bio) {
    LOG(ERROR) << "Failed to allocate openssl certificate buffer.";
    return false;
  }

  // No callback for a passphrase, and no passphrase either.
  *x509_ptr = PEM_read_bio_X509(raw_certificate_bio, NULL, NULL, NULL);
  X509* x509 = *x509_ptr;
  if (!x509) {
    LOG(ERROR) << "Failed to parse certificate.";
    return false;
  }

  if (X509_verify(x509, pkey) <= 0) {
    LOG(ERROR) << "Failed to verify certificate.";
    return false;
  }

  // Check that the device listed in the certificate is correct.
  char device_name[100];  // A longer CN will truncate.
  const int device_name_length = X509_NAME_get_text_by_NID(
      x509->cert_info->subject,
      NID_commonName,
      device_name,
      arraysize(device_name));
  if (device_name_length == -1) {
    LOG(ERROR) << "Subject invalid.";
    return false;
  }

  // Something like evt_e161 001a11ffacdf
  string device_cn(device_name, device_name_length);
  const size_t space_idx = device_cn.rfind(' ');
  if (space_idx == string::npos) {
    LOG(ERROR) << "Badly formatted subject";
    return false;
  }

  string device_mac;
  for (size_t i = space_idx + 1; i < device_cn.length(); ++i) {
    device_mac.push_back(tolower(device_cn[i]));
  }
  if (connected_mac != device_mac) {
    LOG(ERROR) << "MAC addresses don't match.";
    return false;
  }

  // Excellent, the certificate checks out, now make sure that the certificate
  // matches the unsigned data presented.
  // We're going to verify that hash(unsigned_data) == public(signed_data)
  EVP_PKEY* cert_pubkey = X509_get_pubkey(x509);
  if (!cert_pubkey) {
    LOG(ERROR) << "Unable to extract public key from certificate.";
    return false;
  }

  RSA* cert_rsa = EVP_PKEY_get1_RSA(cert_pubkey);
  if (!cert_rsa) {
    LOG(ERROR) << "Failed to extract RSA key from certificate.";
    return false;
  }

  const unsigned char* signature =
      reinterpret_cast<const unsigned char*>(signed_data.data());
  const size_t signature_len = signed_data.length();
  unsigned char* unsigned_data_bytes =
      reinterpret_cast<unsigned char*>(const_cast<char*>(
          unsigned_data.data()));
  const size_t unsigned_data_len = unsigned_data.length();
  unsigned char digest[SHA_DIGEST_LENGTH];
  if (signature_len > numeric_limits<unsigned int>::max()) {
    LOG(ERROR) << "Arguments to signature match were too large.";
    return false;
  }
  SHA1(unsigned_data_bytes, unsigned_data_len, digest);
  if (RSA_verify(NID_sha1, digest, arraysize(digest),
                 signature, signature_len, cert_rsa) != 1) {
    LOG(ERROR) << "Signed blobs did not match.";
    return false;
  }

  return true;
}

// Verify the credentials of the destination described in |raw_input|.  Takes
// a serialized VerifyCredentialsMessage protobuffer in |raw_input|, returns a
// serialized VerifyCredentialsResponse protobuffer in |output| on success.
// Returns false if the credentials fail to meet a check, and true on success.
bool VerifyCredentials(const string& raw_input, string* output) {
  VerifyCredentialsMessage message;
  if (!message.ParseFromString(raw_input)) {
    LOG(ERROR) << "Failed to read VerifyCredentialsMessage from stdin.";
    return false;
  }

  if (!message.has_certificate() || !message.has_signed_data() ||
      !message.has_unsigned_data() || !message.has_mac_address()) {
    LOG(ERROR) << "Request lacked necessary fields.";
    return false;
  }

  string connected_mac;
  for (size_t i = 0; i < message.mac_address().length(); ++i) {
    const char c = message.mac_address()[i];
    if (c != ':') {
      connected_mac.push_back(tolower(c));
    }
  }
  if (connected_mac.length() != kMacLength) {
    LOG(ERROR) << "shill gave us a bad MAC?";
    return false;
  }

  RSA* rsa = NULL;
  EVP_PKEY* pkey = NULL;
  BIO* raw_certificate_bio = NULL;
  X509* x509 = NULL;
  bool operation_successful = VerifyCredentialsImpl(message.certificate(),
      message.signed_data(), message.unsigned_data(), connected_mac,
      &rsa, &pkey, &raw_certificate_bio, &x509);
  if (x509) {
    X509_free(x509);
    x509 = NULL;
  }
  if (raw_certificate_bio) {
    BIO_free(raw_certificate_bio);
    raw_certificate_bio = NULL;
  }
  if (pkey) {
    EVP_PKEY_free(pkey);
    pkey = NULL;
  }
  if (rsa) {
    RSA_free(rsa);
    rsa = NULL;
  }

  if (operation_successful) {
    LOG(INFO) << "Filling out protobuf.";
    VerifyCredentialsResponse response;
    response.set_ret(shill_protos::OK);
    output->clear();
    LOG(INFO) << "Serializing protobuf.";
    if (!response.SerializeToString(output)) {
      LOG(ERROR) << "Failed while writing encrypted data.";
      return false;
    }
    LOG(INFO) << "Encoding finished successfully.";
  }

  return operation_successful;
}

// Read the full stdin stream into a buffer, and execute the operation
// described in |command| with the contends of the stdin buffer.  Write
// the serialized protocol buffer output of the command to stdout.
bool ParseAndExecuteCommand(const string& command) {
  string raw_input;
  char input_buffer[512];
  LOG(INFO) << "Reading input for command " << command << ".";
  while (true) {
    const ssize_t bytes_read = HANDLE_EINTR(read(STDIN_FILENO,
                                                 input_buffer,
                                                 arraysize(input_buffer)));
    if (bytes_read < 0) {
      // Abort abort abort.
      LOG(ERROR) << "Failed while reading from stdin.";
      return false;
    } else if (bytes_read > 0) {
      raw_input.append(input_buffer, bytes_read);
    } else {
      break;
    }
  }
  LOG(INFO) << "Read " << raw_input.length() << " bytes.";
  ERR_clear_error();
  string raw_output;
  bool ret = false;
  if (command == kCommandVerify) {
    ret = VerifyCredentials(raw_input, &raw_output);
  } else if (command == kCommandEncrypt) {
    ret = EncryptByteString(raw_input, &raw_output);
  } else {
    LOG(ERROR) << "Invalid usage.";
    return false;
  }
  if (!ret) {
    LOG(ERROR) << "Last OpenSSL error: "
               << ERR_reason_error_string(ERR_get_error());
  }
  size_t total_bytes_written = 0;
  while (total_bytes_written < raw_output.length()) {
    const ssize_t bytes_written = HANDLE_EINTR(write(
        STDOUT_FILENO,
        raw_output.data() + total_bytes_written,
        raw_output.length() - total_bytes_written));
    if (bytes_written < 0) {
      LOG(ERROR) << "Result write failed with: " << errno;
      return false;
    }
    total_bytes_written += bytes_written;
  }
  return ret;
}

}  // namespace

int main(int argc, char** argv) {
  base::CommandLine::Init(argc, argv);
  brillo::InitLog(brillo::kLogToStderr | brillo::kLogHeader);
  LOG(INFO) << "crypto-util in action";

  if (argc != 2) {
    LOG(ERROR) << "Invalid usage";
    return EXIT_FAILURE;
  }
  const char* command = argv[1];
  if (strcmp(kCommandVerify, command) && strcmp(kCommandEncrypt, command)) {
    LOG(ERROR) << "Invalid command";
    return EXIT_FAILURE;
  }

  CRYPTO_malloc_init();
  ERR_load_crypto_strings();
  OpenSSL_add_all_algorithms();
  int return_code = EXIT_FAILURE;
  if (ParseAndExecuteCommand(command)) {
    return_code = EXIT_SUCCESS;
  }
  close(STDOUT_FILENO);
  close(STDIN_FILENO);

  CONF_modules_unload(1);
  OBJ_cleanup();
  EVP_cleanup();
  CRYPTO_cleanup_all_ex_data();
  ERR_remove_thread_state(NULL);
  ERR_free_strings();

  return return_code;
}
