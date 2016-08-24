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

#include <gtest/gtest.h>
#include <openssl/err.h>

#include <polo/pairing/polochallengeresponse.h>
#include <polo/util/poloutil.h>

namespace polo {
namespace pairing {

class PoloChallengeResponseTest : public ::testing::Test {
 protected:
  PoloChallengeResponseTest() : nonce(4) { }

  virtual void SetUp() {
    // Test certificates generated using:
    // openssl req -x509 -nodes -days 365 -newkey rsa:1024 -out cert.pem

    char client_pem[] = "-----BEGIN CERTIFICATE-----\n"
        "MIICsDCCAhmgAwIBAgIJAI1seGT4bQoOMA0GCSqGSIb3DQEBBAUAMEUxCzAJBgNV\n"
        "BAYTAkFVMRMwEQYDVQQIEwpTb21lLVN0YXRlMSEwHwYDVQQKExhJbnRlcm5ldCBX\n"
        "aWRnaXRzIFB0eSBMdGQwHhcNMTAxMjEyMTYwMzI3WhcNMTExMjEyMTYwMzI3WjBF\n"
        "MQswCQYDVQQGEwJBVTETMBEGA1UECBMKU29tZS1TdGF0ZTEhMB8GA1UEChMYSW50\n"
        "ZXJuZXQgV2lkZ2l0cyBQdHkgTHRkMIGfMA0GCSqGSIb3DQEBAQUAA4GNADCBiQKB\n"
        "gQDa7AitkkzqAZjsoJ3Y5eeq2LZtkF8xMWKuZMOaKDzOaTOBpfiFXbIsrOrHJvh0\n"
        "WIUI7MEu4KTknpqyTEhwqyYozeOoJnhVVaKE03TQTMKgLhc4PwO35NJXHkFxJts1\n"
        "OSCFZ7SQm8OMIr6eEMLh6v7UQQ/GryNY+v5SYiVsbfgW3QIDAQABo4GnMIGkMB0G\n"
        "A1UdDgQWBBRBiLSqlUt+9ZXMBLBp141te487bTB1BgNVHSMEbjBsgBRBiLSqlUt+\n"
        "9ZXMBLBp141te487baFJpEcwRTELMAkGA1UEBhMCQVUxEzARBgNVBAgTClNvbWUt\n"
        "U3RhdGUxITAfBgNVBAoTGEludGVybmV0IFdpZGdpdHMgUHR5IEx0ZIIJAI1seGT4\n"
        "bQoOMAwGA1UdEwQFMAMBAf8wDQYJKoZIhvcNAQEEBQADgYEAchrbHb8S0WCGRupi\n"
        "lxwnD6aVVmVsnNiOaLSI1I6RCKeS0SG/fseThd9nh92WZh6Rbx3U3rAMD08wDfSt\n"
        "S9h7bukJ0X9Rs/BTirzT7Cl09PUjoawP8MeLEDFRUzcBsSYr/k/IPAWOrazWQ2tu\n"
        "XO5L5nPKzpxd3tF4Aj4/3kBm4nw=\n"
        "-----END CERTIFICATE-----\n";

    char server_pem[] = "-----BEGIN CERTIFICATE-----\n"
        "MIICsDCCAhmgAwIBAgIJAPa14A4WCQpNMA0GCSqGSIb3DQEBBAUAMEUxCzAJBgNV\n"
        "BAYTAkFVMRMwEQYDVQQIEwpTb21lLVN0YXRlMSEwHwYDVQQKExhJbnRlcm5ldCBX\n"
        "aWRnaXRzIFB0eSBMdGQwHhcNMTAxMjEyMTYwNzMzWhcNMTExMjEyMTYwNzMzWjBF\n"
        "MQswCQYDVQQGEwJBVTETMBEGA1UECBMKU29tZS1TdGF0ZTEhMB8GA1UEChMYSW50\n"
        "ZXJuZXQgV2lkZ2l0cyBQdHkgTHRkMIGfMA0GCSqGSIb3DQEBAQUAA4GNADCBiQKB\n"
        "gQDBkfualV4+vxIEBg1TWXy2T1nf0Dch8XoQG824o3EAzuIRHdBGHvzRNfmQOlje\n"
        "XVU/Cds376EYOblxoZNVNQYMf1fkwTUnDWXNl3wR5A4m4Govi2y61b7NA8/AMxO9\n"
        "wtuIAI+Yty2UAjacvt3yqG2J1r55kIOsYeDoy1E5Hpo8gwIDAQABo4GnMIGkMB0G\n"
        "A1UdDgQWBBRgMM6zsFJ2DGv7B1URsUmx1BBAPzB1BgNVHSMEbjBsgBRgMM6zsFJ2\n"
        "DGv7B1URsUmx1BBAP6FJpEcwRTELMAkGA1UEBhMCQVUxEzARBgNVBAgTClNvbWUt\n"
        "U3RhdGUxITAfBgNVBAoTGEludGVybmV0IFdpZGdpdHMgUHR5IEx0ZIIJAPa14A4W\n"
        "CQpNMAwGA1UdEwQFMAMBAf8wDQYJKoZIhvcNAQEEBQADgYEAoU/4pb2QTEgCwhzG\n"
        "k6BPIz2WhOeIAAZ9fQmVxL5pbcgIUC4SnoJ3MtwB02Abbk5pIeSgtgJ50R4SmluM\n"
        "T+0G1p772RqN+tLWihJqWgmODhfppUm9pp07UfL6yn4wAnyvzevadVXl6GCPocL9\n"
        "cvcuBiBPlRU/giP3n15OtJ6KL9U=\n"
        "-----END CERTIFICATE-----\n";

    SSL_load_error_strings();

    client_bio = BIO_new_mem_buf(client_pem, -1);
    client_cert = PEM_read_bio_X509(client_bio, NULL, NULL, NULL);

    server_bio = BIO_new_mem_buf(server_pem, -1);
    server_cert = PEM_read_bio_X509(server_bio, NULL, NULL, NULL);

    nonce[0] = 0x1;
    nonce[1] = 0x2;
    nonce[2] = 0x3;
    nonce[3] = 0x4;

    response = new PoloChallengeResponse(client_cert, server_cert);
  }

  virtual void TearDown() {
    X509_free(client_cert);
    BIO_free(client_bio);

    X509_free(server_cert);
    BIO_free(server_bio);

    delete response;
  }

  BIO* client_bio;
  X509* client_cert;
  BIO* server_bio;
  X509* server_cert;
  Nonce nonce;
  PoloChallengeResponse* response;
};

TEST_F(PoloChallengeResponseTest, GetAlpha) {
  const Alpha* alpha = response->GetAlpha(nonce);
  ASSERT_TRUE(alpha);

  ASSERT_EQ("E4DA87E4A544B30C98FC8A4731C10828506A97BA143950D7C68D9BF58ED4C397",
            util::PoloUtil::BytesToHexString(&(*alpha)[0], alpha->size()));
  delete alpha;
}

TEST_F(PoloChallengeResponseTest, TestGetGamma) {
  const Gamma* gamma = response->GetGamma(nonce);
  ASSERT_TRUE(gamma);

  ASSERT_EQ("E4DA87E401020304",
            util::PoloUtil::BytesToHexString(&(*gamma)[0], gamma->size()));
  delete gamma;
}

TEST_F(PoloChallengeResponseTest, TestExtractNonce) {
  const Gamma* gamma = response->GetGamma(nonce);
  ASSERT_TRUE(gamma);
  ASSERT_EQ("E4DA87E401020304",
            util::PoloUtil::BytesToHexString(&(*gamma)[0], gamma->size()));

  const Nonce* extracted = response->ExtractNonce(*gamma);
  ASSERT_TRUE(extracted);
  ASSERT_EQ("01020304",
            util::PoloUtil::BytesToHexString(&(*extracted)[0],
                                             extracted->size()));

  delete gamma;
  delete extracted;
}

TEST_F(PoloChallengeResponseTest, TestCheckGamma) {
  Gamma gamma(8);
  gamma[0] = 0xE4;
  gamma[1] = 0xDA;
  gamma[2] = 0x87;
  gamma[3] = 0xE4;
  gamma[4] = 0x01;
  gamma[5] = 0x02;
  gamma[6] = 0x03;
  gamma[7] = 0x04;

  ASSERT_TRUE(response->CheckGamma(gamma));
}

}  // namespace pairing
}  // namespace polo
