/*
 * Copyright 2015 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef __TPM2_CPRIRSA_FP_H
#define __TPM2_CPRIRSA_FP_H

LIB_EXPORT BOOL _cpri__RsaStartup(void);
LIB_EXPORT CRYPT_RESULT _cpri__DecryptRSA(
    UINT32 *dOutSize,    //   OUT: the size of the decrypted data
    BYTE *dOut,          //   OUT: the decrypted data
    RSA_KEY *key,        //   IN: the key to use for decryption
    TPM_ALG_ID padType,  //   IN: the type of padding
    UINT32 cInSize,      //   IN: the amount of data to decrypt
    BYTE *cIn,           //   IN: the data to decrypt
    TPM_ALG_ID hashAlg,  //   IN: in case this is needed for the scheme
    const char *label    //   IN: in case it is needed for the scheme
    );
LIB_EXPORT CRYPT_RESULT
_cpri__EncryptRSA(UINT32 *cOutSize,    //   OUT: the size of the encrypted data
                  BYTE *cOut,          //   OUT: the encrypted data
                  RSA_KEY *key,        //   IN: the key to use for encryption
                  TPM_ALG_ID padType,  //   IN: the type of padding
                  UINT32 dInSize,      //   IN: the amount of data to encrypt
                  BYTE *dIn,           //   IN: the data to encrypt
                  TPM_ALG_ID hashAlg,  //   IN: in case this is needed
                  const char *label    //   IN: in case it is needed
                  );
LIB_EXPORT CRYPT_RESULT _cpri__GenerateKeyRSA(
    TPM2B *n,              //   OUT: The public modulu
    TPM2B *p,              //   OUT: One of the prime factors of n
    UINT16 keySizeInBits,  //   IN: Size of the public modulus in bit
    UINT32 e,              //   IN: The public exponent
    TPM_ALG_ID
        hashAlg,  //   IN: hash algorithm to use in the key generation proce
    TPM2B *seed,  //   IN: the seed to use
    const char *label,  //   IN: A label for the generation process.
    TPM2B *extra,       //   IN: Party 1 data for the KDF
    UINT32 *counter     //   IN/OUT: Counter value to allow KFD iteration to be
                        //   propagated across multiple routine
    );
LIB_EXPORT CRYPT_RESULT
_cpri__SignRSA(UINT32 *sigOutSize,  //   OUT: size of signature
               BYTE *sigOut,        //   OUT: signature
               RSA_KEY *key,        //   IN: key to use
               TPM_ALG_ID scheme,   //   IN: the scheme to use
               TPM_ALG_ID hashAlg,  //   IN: hash algorithm for PKSC1v1_5
               UINT32 hInSize,      //   IN: size of digest to be signed
               BYTE *hIn            //   IN: digest buffer
               );
LIB_EXPORT CRYPT_RESULT _cpri__TestKeyRSA(
    TPM2B *d,          //   OUT: the address to receive the private exponent
    UINT32 exponent,   //   IN: the public modulu
    TPM2B *publicKey,  //   IN/OUT: an input if only one prime is provided. an
                       //   output if both primes are provided
    TPM2B *prime1,     //   IN: a first prime
    TPM2B *prime2      //   IN: an optional second prime
    );
LIB_EXPORT CRYPT_RESULT _cpri__ValidateSignatureRSA(
    RSA_KEY *key,        //   IN:   key to use
    TPM_ALG_ID scheme,   //   IN:   the scheme to use
    TPM_ALG_ID hashAlg,  //   IN:   hash algorithm
    UINT32 hInSize,      //   IN:   size of digest to be checked
    BYTE *hIn,           //   IN:   digest buffer
    UINT32 sigInSize,    //   IN:   size of signature
    BYTE *sigIn,         //   IN:   signature
    UINT16 saltSize      //   IN:   salt size for PSS
    );

#endif  // __TPM2_CPRIRSA_FP_H
