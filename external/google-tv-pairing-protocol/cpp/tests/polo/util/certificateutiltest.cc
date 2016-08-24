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

// Tests for CertificateUtil.

#include <polo/util/certificateutil.h>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <openssl/err.h>

namespace polo {
namespace util {

// Tests reading an X509 certificate from a PEM encoded string.
TEST(CertificateUtilTest, X509FromPEM) {
  std::string pem = "-----BEGIN CERTIFICATE-----\n"
      "MIICAzCCAWwCCQD5/Q86s0olWDANBgkqhkiG9w0BAQUFADBFMQswCQYDVQQGEwJB\n"
      "VTETMBEGA1UECAwKU29tZS1TdGF0ZTEhMB8GA1UECgwYSW50ZXJuZXQgV2lkZ2l0\n"
      "cyBQdHkgTHRkMCAXDTExMDExOTE3MjUzMFoYDzIyODQxMTAyMTcyNTMwWjBFMQsw\n"
      "CQYDVQQGEwJBVTETMBEGA1UECAwKU29tZS1TdGF0ZTEhMB8GA1UECgwYSW50ZXJu\n"
      "ZXQgV2lkZ2l0cyBQdHkgTHRkMIGfMA0GCSqGSIb3DQEBAQUAA4GNADCBiQKBgQCg\n"
      "/IcUHnAdzIChv9kQzX07F6t4LtEwPbu3vLagYjh4pzCNFQe3Wz51ce7mknqbDlKT\n"
      "7iTvwLPw6WBZe72VDpIRRX4+3tT9drMBpdB52Ix3sOu1HxwusAUUvOzXXHiQYGQt\n"
      "CZUfYBX/siwBZ4/llK5C/035NGG9OkvQ1J8BPKyWoQIDAQABMA0GCSqGSIb3DQEB\n"
      "BQUAA4GBAJMEBv/UT1Qnkp+xIrlPGkiXOOz1I0ydSz1DKBzGfmDGZ4a3+uFGAh8P\n"
      "XO45IugMw/natOEXfhe9s0ZKHhszQg3bVU3+15/uw/XIN31EzyZwkOGvQfrCLcDi\n"
      "N9HU05VV+pQLN916Fo7EEmCx0cu/c82qhrACYQMsBWXPyLiJh0Lq\n"
      "-----END CERTIFICATE-----\n";

  X509* x509 = CertificateUtil::X509FromPEM(pem);

  if (!x509) {
    std::cerr << ERR_error_string(ERR_get_error(), NULL);
  }

  ASSERT_TRUE(x509);
  X509_free(x509);
}

// Tests converting an X509 certificate to a PEM encoded string.
TEST(CertificateUtilTest, X509ToPEM) {
  X509* x509 = X509_new();
  EVP_PKEY* pkey = EVP_PKEY_new();

  std::string rsa_string = "-----BEGIN RSA PRIVATE KEY-----\n"
      "MIICXAIBAAKBgQDP5u0Bvw3N2H2g3kZB4snFiaylHh7JsF2HAdG1zIkNSyQ7jtrZ\n"
      "b31R8GC/sqrtpGuyysQBb6DJKc9+YCH348PS52moieCUaIz48xJyx2UyfUgns1YH\n"
      "D+lcLG1NozBTKj75z0+s2InvCNM5WaZ2RzZf8wme2AZFKQ310AGrCLMmyQIDAQAB\n"
      "AoGAKpJy/eSNgxVNxF8/q8Yw4w5qF/WvAEXpIPgyZTPY7KvyY2/BSL0XwGukpByF\n"
      "+9urYhU7RcACAK9bGdm9mvE869hLpDVcsPAza8DjGQFpJk/NLSzoP71fKtxxtRZW\n"
      "VmehimP8BYMUWLG0wXaH+80wEo8Ux9vGZDBA8qIALdzcUnECQQD5NVFyn0FogoBt\n"
      "MtPftNNSYEGgUVIOIN5VS1i39p3Bo8NAlkw2iL0u1yT5eVGOCyTRLcILj0ALht45\n"
      "Gz9KY5y/AkEA1ZFr5xcVjvOUmNGe+L1sztEQsED5ksgbCLgrjTrWys+E5IUoL1xO\n"
      "WZ0Y0J7xzmJAQsIrE3YHWqAkH5VP8us2dwJAV6oH4rhe+/KcVs2AdrtXcyzlKQ4y\n"
      "PUIWtA5zQROB3zJKZxf3618ina2VFiU1KTCGXQcpsYNM1kE1PwV0uCheZQJAFOD1\n"
      "oo7wLZyEj3gWyYyDQajQr9p6S65CblTK9TCmZQdqn4ihCBhHFJ22GlcfnqSeUah3\n"
      "25wzVdnIDkpjmYUDOwJBAKKqyoUlxeuofTQ+IfqQXnrqmwV8plYOPrXS36RrU84L\n"
      "VNB7JoD+vW2xKBXx2BxIbJ4dM7KrqaOP3j0tKoIX4Xc=\n"
      "-----END RSA PRIVATE KEY-----\n";

  BIO* rsa_bio = BIO_new_mem_buf(&rsa_string[0], rsa_string.size());
  RSA* rsa = PEM_read_bio_RSAPrivateKey(rsa_bio, NULL, NULL, NULL);
  BIO_free(rsa_bio);

  EVP_PKEY_assign_RSA(pkey, rsa);

  X509_set_version(x509, 2);
  ASN1_INTEGER_set(X509_get_serialNumber(x509), 0);
  ASN1_TIME_set(X509_get_notBefore(x509), 0);
  ASN1_TIME_set(X509_get_notAfter(x509), 60*60*24*365);
  X509_set_pubkey(x509, pkey);
  X509_NAME* name = X509_get_subject_name(x509);
  X509_NAME_add_entry_by_NID(name,
                             NID_commonName,
                             MBSTRING_ASC,
                             (unsigned char*) "testing",
                             -1,
                             -1,
                             0);
  X509_set_issuer_name(x509, name);
  X509_sign(x509, pkey, EVP_sha256());

  std::string pem = CertificateUtil::X509ToPEM(x509);

  X509_free(x509);
  EVP_PKEY_free(pkey);

  std::string expected = "-----BEGIN CERTIFICATE-----\n"
      "MIIBmDCCAQGgAwIBAgIBADANBgkqhkiG9w0BAQsFADASMRAwDgYDVQQDEwd0ZXN0\n"
      "aW5nMB4XDTcwMDEwMTAwMDAwMFoXDTcxMDEwMTAwMDAwMFowEjEQMA4GA1UEAxMH\n"
      "dGVzdGluZzCBnzANBgkqhkiG9w0BAQEFAAOBjQAwgYkCgYEAz+btAb8Nzdh9oN5G\n"
      "QeLJxYmspR4eybBdhwHRtcyJDUskO47a2W99UfBgv7Kq7aRrssrEAW+gySnPfmAh\n"
      "9+PD0udpqInglGiM+PMScsdlMn1IJ7NWBw/pXCxtTaMwUyo++c9PrNiJ7wjTOVmm\n"
      "dkc2X/MJntgGRSkN9dABqwizJskCAwEAATANBgkqhkiG9w0BAQsFAAOBgQC+Ibl9\n"
      "EPNGkZRR4oGt0WIOpJSRzJMvH/EvS6i5COAS0st7FXpxkuiCTpBCAabBmf4D6Lvt\n"
      "pRS73QFpXgH6ZhSHb/K1Xs8tzJ7QlnxE+iGm0r+w/3/wKANwy9s+S1KFgkJwZ10z\n"
      "qyJ0wjxMi/8exSg3PGs71P1L/pkzI24VN/+mPA==\n"
      "-----END CERTIFICATE-----\n";

  ASSERT_EQ(expected, pem);
}

// Tests reading a private key from a PEM encoded string.
TEST(CertificateUtilTest, PKEYFromPEM) {
  std::string pem = "-----BEGIN RSA PRIVATE KEY-----\n"
      "Proc-Type: 4,ENCRYPTED\n"
      "DEK-Info: DES-EDE3-CBC,5351BC9DC2349695\n"
      "\n"
      "Dmr011r9Nn86mHljRTE59DThzsQaYAnJPUvboEY/jriqc8n/kE0IvtaM/Stutlzp\n"
      "jMbL/1ddjIeyStWM17DTlEeu1DFCoLnmVqwn1p2x2Y5gW72CYx5oawDj7rg8Jczj\n"
      "mUfuRBU69pa17dT/3qjiNwEWz90NoNwxcMe7lP2uULyB75hDNCQ9mjN1WN1iAyiS\n"
      "zehrScLk/3Y3QD0KLk2TM8CLuWyaf1K7NhyWBWatxhWcVe2Zw48MGA1sUTnb5m67\n"
      "yyS+/Doonqhko+a/5ycnu+MiE4V4KrGyBrkqK0KO6kWVB7bxudC/5S+x85b9VDNc\n"
      "GPfquXpHisouUaW9EnqGnk3E/kaOUamACgZHdrXDqeBXaAulSbZ0f1I9hHP6yULg\n"
      "IWqkeLx3f+GPYbYwdVzp7gc94xdjcsXUG3BWwuL4PD8VJXUrNJH0RJMK5SDZrNhF\n"
      "WLizlzjwYfM0wZhcaWjBY/6tz7gkz4bSG9skl9HLvFK7bKyarRjP6P6LQJJXz3hB\n"
      "LAj95Vye8mWfY+WHV+POB2sxQ9riXiyy5UnSnhqvAhLBWNBjSYq8WM+MtLmZf2OA\n"
      "H6w0JPK/smd4K+xyFUNh2g2w4feS1glVl9LYzKopZNEu4Vb0jc3Akd92hMR1bSww\n"
      "fXi8D/4XV3mHSsF91bT0Jn/1n93qtr++FpztTU4KcFB3OJur2QUoHvH7ei/NdxW5\n"
      "yJaxcFwWhGtmx1SVGuNb3yC6rm/hKrzi5998UTPE/9gQiJgVXenR9ve2IcbIaBur\n"
      "avtnvFQ+6xAApwgi0q6rw6I5AeF7dD226+LY9gpfu6ZzrFbOlv+7Tg==\n"
      "-----END RSA PRIVATE KEY-----\n";

  EVP_PKEY* pkey = CertificateUtil::PKEYFromPEM(pem, "testing");

  ASSERT_TRUE(pkey);

  RSA* rsa = EVP_PKEY_get1_RSA(pkey);
  ASSERT_TRUE(rsa);

  EVP_PKEY_free(pkey);
}

// Tests converting a private key to a PEM encoded string.
TEST(CertificateUtilTest, PKEYToPEM) {
  EVP_PKEY* pkey = EVP_PKEY_new();
  RSA* rsa = RSA_generate_key(1025, RSA_F4, NULL, NULL);
  EVP_PKEY_assign_RSA(pkey, rsa);

  std::string pem = CertificateUtil::PKEYToPEM(pkey, "testing");

  ASSERT_TRUE(pem.size());

  // Difficult to verify the PEM because the encryption is random, so just make
  // sure we can read it back in with the supplied passphrase.
  EVP_PKEY* verify = CertificateUtil::PKEYFromPEM(pem, "testing");
  ASSERT_TRUE(verify);

  EVP_PKEY_free(pkey);
  EVP_PKEY_free(verify);
}

// Tests generating a new private key.
TEST(CertificateUtilTest, GeneratePrivateKey) {
  EVP_PKEY* pkey = CertificateUtil::GeneratePrivateKey();
  ASSERT_TRUE(pkey);
  EVP_PKEY_free(pkey);
}

// Tests generating a self-signed certificate.
TEST(CertificateUtilTest, GenerateSelfSignedCert) {
  EVP_PKEY* pkey = CertificateUtil::GeneratePrivateKey();
  ASSERT_TRUE(pkey);

  X509* x509 = CertificateUtil::GenerateSelfSignedCert(pkey, "test", 365);
  ASSERT_TRUE(x509);

  EVP_PKEY_free(pkey);
  X509_free(x509);
}

}  // namespace util
}  // namespace polo
