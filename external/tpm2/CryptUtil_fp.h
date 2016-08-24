/*
 * Copyright 2015 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef __SOURCE_CRYPTUTIL_FP_H
#define __SOURCE_CRYPTUTIL_FP_H

BOOL CryptAreKeySizesConsistent(
    TPMT_PUBLIC *publicArea  // IN: the public area to check
    );
TPMI_YES_NO CryptCapGetECCCurve(
    TPM_ECC_CURVE curveID,     // IN: the starting ECC curve
    UINT32 maxCount,           // IN: count of returned curve
    TPML_ECC_CURVE *curveList  // OUT: ECC curve list
    );
UINT32 CryptCapGetEccCurveNumber(void);
UINT16 CryptCommit(void);
LIB_EXPORT int CryptCompare(const UINT32 aSize,  //   IN:   size of a
                            const BYTE *a,       //   IN:   a buffer
                            const UINT32 bSize,  //   IN:   size of b
                            const BYTE *b        //   IN:   b buffer
                            );
TPM_RC CryptCommitCompute(
    TPMS_ECC_POINT *K,       //   OUT: [d]B
    TPMS_ECC_POINT *L,       //   OUT: [r]B
    TPMS_ECC_POINT *E,       //   OUT: [r]M
    TPM_ECC_CURVE curveID,   //   IN: The curve for the computation
    TPMS_ECC_POINT *M,       //   IN: M (P1)
    TPMS_ECC_POINT *B,       //   IN: B (x2, y2)
    TPM2B_ECC_PARAMETER *d,  //   IN: the private scalar
    TPM2B_ECC_PARAMETER *r   //   IN: the computed r value
    );
int CryptCompareSigned(UINT32 aSize,  //   IN:   size of a
                       BYTE *a,       //   IN:   a buffer
                       UINT32 bSize,  //   IN:   size of b
                       BYTE *b        //   IN:   b buffer
                       );
void CryptComputeSymmetricUnique(
    TPMI_ALG_HASH nameAlg,      // IN: object name algorithm
    TPMT_SENSITIVE *sensitive,  // IN: sensitive area
    TPM2B_DIGEST *unique        // OUT: unique buffer
    );
LIB_EXPORT UINT16
CryptCompleteHMAC2B(HMAC_STATE *hmacState,  // IN: the state of HMAC stack
                    TPM2B *digest           // OUT: HMAC
                    );
LIB_EXPORT UINT16
CryptCompleteHash(void *state,        // IN: the state of hash stack
                  UINT16 digestSize,  // IN: size of digest buffer
                  BYTE *digest        // OUT: hash digest
                  );
UINT16 CryptCompleteHash2B(
    void *state,   // IN: the state of hash stack
    TPM2B *digest  // IN: the size of the buffer Out: requested number of byte
    );
TPM_RC CryptCreateObject(
    TPM_HANDLE parentHandle,  //   IN/OUT: indication of the seed source
    TPMT_PUBLIC *publicArea,  //   IN/OUT: public area
    TPMS_SENSITIVE_CREATE *sensitiveCreate,  //   IN: sensitive creation
    TPMT_SENSITIVE *sensitive                //   OUT: sensitive area
    );
void CryptDrbgGetPutState(GET_PUT direction  // IN: Get from or put to DRBG
                          );
TPM_RC CryptDecryptRSA(
    UINT16 *dataOutSize,       // OUT: size of plain text in byte
    BYTE *dataOut,             //   OUT: plain text
    OBJECT *rsaKey,            //   IN: internal RSA key
    TPMT_RSA_DECRYPT *scheme,  //   IN: selects the padding scheme
    UINT16 cipherInSize,       //   IN: size of cipher text in byte
    BYTE *cipherIn,            //   IN: cipher text
    const char *label          //   IN: a label, when needed
    );
TPM_RC CryptDivide(
    TPM2B *numerator,    //   IN: numerator
    TPM2B *denominator,  //   IN: denominator
    TPM2B *quotient,     //   OUT: quotient = numerator / denominator.
    TPM2B *remainder     //   OUT: numerator mod denominator.
    );
void CryptDrbgGetPutState(GET_PUT direction  // IN: Get from or put to DRBG
                          );
//
//
//      10.2.6.3    CryptEccGetKeySizeBytes()
//
//  This macro returns the size of the ECC key in bytes. It uses
//  CryptEccGetKeySizeInBits().
//
#define CryptEccGetKeySizeInBytes(curve) \
  ((CryptEccGetKeySizeInBits(curve) + 7) / 8)

TPM_RC CryptEcc2PhaseKeyExchange(
    TPMS_ECC_POINT *outZ1,     //   OUT: the computed point
    TPMS_ECC_POINT *outZ2,     //   OUT: optional second point
    TPM_ALG_ID scheme,         //   IN: the key exchange scheme
    TPM_ECC_CURVE curveId,     //   IN: the curve for the computation
    TPM2B_ECC_PARAMETER *dsA,  //   IN: static private TPM key
    TPM2B_ECC_PARAMETER *deA,  //   IN: ephemeral private TPM key
    TPMS_ECC_POINT *QsB,       //   IN: static public party B key
    TPMS_ECC_POINT *QeB        //   IN: ephemeral public party B key
    );
TPM_RC CryptEncryptRSA(
    UINT16 *cipherOutSize,     //   OUT: size of cipher text in byte
    BYTE *cipherOut,           //   OUT: cipher text
    OBJECT *rsaKey,            //   IN: internal RSA key
    TPMT_RSA_DECRYPT *scheme,  //   IN: selects the padding scheme
    UINT16 dataInSize,         //   IN: size of plain text in byte
    BYTE *dataIn,              //   IN: plain text
    const char *label          //   IN: an optional label
    );
TPM_ALG_ID CryptGetContextAlg(void *state  // IN: the context to check
                              );
LIB_EXPORT TPM_ALG_ID CryptGetHashAlgByIndex(UINT32 index  // IN: the index
                                             );
LIB_EXPORT UINT16
CryptGetHashDigestSize(TPM_ALG_ID hashAlg  // IN: hash algorithm
                       );
LIB_EXPORT const TPM2B *CryptEccGetParameter(
    char p,                // IN: the parameter selector
    TPM_ECC_CURVE curveId  // IN: the curve id
    );
BOOL CryptEccGetParameters(
    TPM_ECC_CURVE curveId,                 // IN: ECC curve ID
    TPMS_ALGORITHM_DETAIL_ECC *parameters  // OUT: ECC parameter
    );
TPM_RC CryptEccPointMultiply(TPMS_ECC_POINT *pOut,      //   OUT: output point
                             TPM_ECC_CURVE curveId,     //   IN: curve selector
                             TPM2B_ECC_PARAMETER *dIn,  //   IN: public scalar
                             TPMS_ECC_POINT *pIn        //   IN: optional point
                             );
BOOL CryptEccIsPointOnCurve(TPM_ECC_CURVE curveID,  // IN: ECC curve ID
                            TPMS_ECC_POINT *Q       // IN: ECC point
                            );
void CryptEndCommit(UINT16 c  // IN: the counter value of the commitment
                    );
BOOL CryptGenerateR(
    TPM2B_ECC_PARAMETER *r,  //   OUT: the generated random value
    UINT16 *c,               //   IN/OUT: count value.
    TPMI_ECC_CURVE curveID,  //   IN: the curve for the value
    TPM2B_NAME *name  //   IN: optional name of a key to associate with 'r'
    );
UINT16 CryptGenerateRandom(UINT16 randomSize,  // IN: size of random number
                           BYTE *buffer        // OUT: buffer of random number
                           );
void CryptGenerateNewSymmetric(
    TPMS_SENSITIVE_CREATE *sensitiveCreate,  //   IN: sensitive creation data
    TPMT_SENSITIVE *sensitive,               //   OUT: sensitive area
    TPM_ALG_ID hashAlg,                      //   IN: hash algorithm for the KDF
    TPM2B_SEED *seed,                        //   IN: seed used in creation
    TPM2B_NAME *name                         //   IN: name of the object
    );
const TPMT_ECC_SCHEME *CryptGetCurveSignScheme(
    TPM_ECC_CURVE curveId  // IN: The curve selector
    );
LIB_EXPORT UINT16
CryptGetHashDigestSize(TPM_ALG_ID hashAlg  // IN: hash algorithm
                       );
TPMI_ALG_HASH CryptGetSignHashAlg(TPMT_SIGNATURE *auth  // IN: signature
                                  );
INT16 CryptGetSymmetricBlockSize(
    TPMI_ALG_SYM algorithm,  // IN: symmetric algorithm
    UINT16 keySize           // IN: key size in bit
    );
TPM_RC CryptGetTestResult(TPM2B_MAX_BUFFER *outData  // OUT: test result data
                          );
LIB_EXPORT UINT16
CryptHashBlock(TPM_ALG_ID algId,  //   IN: the hash algorithm to use
               UINT16 blockSize,  //   IN: size of the data block
               BYTE *block,       //   IN: address of the block to hash
               UINT16 retSize,    //   IN: size of the return buffer
               BYTE *ret          //   OUT: address of the buffer
               );
//
//
//
//      10.2.4.23 CryptKDFa()
//
// This function generates a key using the KDFa() formulation in Part 1 of the
// TPM specification. In this implementation, this is a macro invocation of
// _cpri__KDFa() in the hash module of the CryptoEngine(). This macro sets
// once to FALSE so that KDFa() will iterate as many times as necessary to
// generate sizeInBits number of bits.
//
#define CryptKDFa(hashAlg, key, label, contextU, contextV, sizeInBits,        \
                  keyStream, counterInOut)                                    \
  TEST_HASH(hashAlg);                                                         \
  _cpri__KDFa(((TPM_ALG_ID)hashAlg), ((TPM2B *)key), ((const char *)label),   \
              ((TPM2B *)contextU), ((TPM2B *)contextV), ((UINT32)sizeInBits), \
              ((BYTE *)keyStream), ((UINT32 *)counterInOut), ((BOOL)FALSE))

//
//
//      10.2.4.24 CryptKDFaOnce()
//
// This function generates a key using the KDFa() formulation in Part 1 of the
// TPM specification. In this implementation, this is a macro invocation of
// _cpri__KDFa() in the hash module of the CryptoEngine(). This macro will
// call _cpri__KDFa() with once TRUE so that only one iteration is performed,
// regardless of sizeInBits.
//
#define CryptKDFaOnce(hashAlg, key, label, contextU, contextV, sizeInBits,    \
                      keyStream, counterInOut)                                \
  TEST_HASH(hashAlg);                                                         \
  _cpri__KDFa(((TPM_ALG_ID)hashAlg), ((TPM2B *)key), ((const char *)label),   \
              ((TPM2B *)contextU), ((TPM2B *)contextV), ((UINT32)sizeInBits), \
              ((BYTE *)keyStream), ((UINT32 *)counterInOut), ((BOOL)TRUE))

//
//
//    10.2.4.26 CryptKDFe()
//
//  This function generates a key using the KDFa() formulation in Part 1 of
//  the TPM specification. In this implementation, this is a macro invocation
//  of _cpri__KDFe() in the hash module of the CryptoEngine().
//
#define CryptKDFe(hashAlg, Z, label, partyUInfo, partyVInfo, sizeInBits,  \
                  keyStream)                                              \
  TEST_HASH(hashAlg);                                                     \
  _cpri__KDFe(((TPM_ALG_ID)hashAlg), ((TPM2B *)Z), ((const char *)label), \
              ((TPM2B *)partyUInfo), ((TPM2B *)partyVInfo),               \
              ((UINT32)sizeInBits), ((BYTE *)keyStream))

void CryptHashStateImportExport(
    HASH_STATE *internalFmt,  // IN: state to LIB_EXPORT
    HASH_STATE *externalFmt,  // OUT: exported state
    IMPORT_EXPORT direction);
void CryptInitUnits(void);
BOOL CryptIsAsymAlgorithm(TPM_ALG_ID algID  // IN: algorithm ID
                          );
BOOL CryptIsDecryptScheme(TPMI_ALG_ASYM_SCHEME scheme);
BOOL CryptIsSchemeAnonymous(
    TPM_ALG_ID scheme  // IN: the scheme algorithm to test
    );
BOOL CryptIsSignScheme(TPMI_ALG_ASYM_SCHEME scheme);
BOOL CryptIsSplitSign(TPM_ALG_ID scheme  // IN: the algorithm selector
                      );
TPM_RC CryptNewEccKey(TPM_ECC_CURVE curveID,          // IN: ECC curve
                      TPMS_ECC_POINT *publicPoint,    // OUT: public point
                      TPM2B_ECC_PARAMETER *sensitive  // OUT: private area
                      );
BOOL CryptObjectIsPublicConsistent(TPMT_PUBLIC *publicArea  // IN: public area
                                   );
TPM_RC CryptObjectPublicPrivateMatch(OBJECT *object  // IN: the object to check
                                     );
TPM_RC CryptParameterDecryption(
    TPM_HANDLE handle,   //   IN: encrypted session handle
    TPM2B *nonceCaller,  //   IN: nonce caller
    UINT32 bufferSize,   //   IN: size of parameter buffer
    UINT16
        leadingSizeInByte,  //   IN: the size of the leading size field in byte
    TPM2B_AUTH *extraKey,   //   IN: the authValue
    BYTE *buffer            //   IN/OUT: parameter buffer to be decrypted
    );
void CryptParameterEncryption(
    TPM_HANDLE handle,         // IN: encrypt session handle
    TPM2B *nonceCaller,        // IN: nonce caller
    UINT16 leadingSizeInByte,  // IN: the size of the leading size field in byte
    TPM2B_AUTH *
        extraKey,  // IN: additional key material other than session auth
    BYTE *buffer   // IN/OUT: parameter buffer to be encrypted
    );
TPM_RC CryptSecretDecrypt(
    TPM_HANDLE tpmKey,         // IN: decrypt key
    TPM2B_NONCE *nonceCaller,  // IN: nonceCaller. It is needed for symmetric
                               // decryption. For asymmetric decryption, this
                               // parameter is NULL
    const char *label,         // IN: a null-terminated string as L
    TPM2B_ENCRYPTED_SECRET *secret,  // IN: input secret
    TPM2B_DATA *data                 // OUT: decrypted secret value
    );
TPM_RC CryptSecretEncrypt(
    TPMI_DH_OBJECT keyHandle,       //   IN: encryption key handle
    const char *label,              //   IN: a null-terminated string as L
    TPM2B_DATA *data,               //   OUT: secret value
    TPM2B_ENCRYPTED_SECRET *secret  //   OUT: secret structure
    );
TPMT_RSA_DECRYPT *CryptSelectRSAScheme(
    TPMI_DH_OBJECT rsaHandle,  // IN: handle of sign key
    TPMT_RSA_DECRYPT *scheme   // IN: a sign or decrypt scheme
    );
TPM_RC CryptSelectSignScheme(
    TPMI_DH_OBJECT signHandle,  // IN: handle of signing key
    TPMT_SIG_SCHEME *scheme     // IN/OUT: signing scheme
    );
TPM_RC CryptSign(TPMI_DH_OBJECT signHandle,    //   IN: The handle of sign key
                 TPMT_SIG_SCHEME *signScheme,  //   IN: sign scheme.
                 TPM2B_DIGEST *digest,         //   IN: The digest being signed
                 TPMT_SIGNATURE *signature     //   OUT: signature
                 );
LIB_EXPORT UINT16
CryptStartHMAC2B(TPMI_ALG_HASH hashAlg,  // IN: hash algorithm
                 TPM2B *key,             // IN: HMAC key
                 HMAC_STATE *hmacState  // OUT: the state of HMAC stack. It will
                                        // be used in HMAC update and completion
                 );
UINT16 CryptStartHMACSequence2B(TPMI_ALG_HASH hashAlg,  // IN: hash algorithm
                                TPM2B *key,             // IN: HMAC key
                                HMAC_STATE *hmacState  // OUT: the state of HMAC
                                                       // stack. It will be used
                                                       // in HMAC update and
                                                       // completion
                                );
UINT16 CryptStartHashSequence(TPMI_ALG_HASH hashAlg,  // IN: hash algorithm
                              HASH_STATE *hashState   // OUT: the state of hash
                                                      // stack. It will be used
                                                      // in hash update and
                                                      // completion
                              );
void CryptStirRandom(UINT32 entropySize,  // IN: size of entropy buffer
                     BYTE *buffer         // IN: entropy buffer
                     );
void CryptStopUnits(void);
void CryptSymmetricDecrypt(
    BYTE *decrypted,
    TPM_ALG_ID algorithm,    //   IN: algorithm for encryption
    UINT16 keySizeInBits,    //   IN: key size in bit
    TPMI_ALG_SYM_MODE mode,  //   IN: symmetric encryption mode
    BYTE *key,               //   IN: encryption key
    TPM2B_IV *ivIn,          //   IN/OUT: IV for next block
    UINT32 dataSize,         //   IN: data size in byte
    BYTE *data               //   IN/OUT: data buffer
    );
void CryptSymmetricEncrypt(
    BYTE *encrypted,         //   OUT: the encrypted data
    TPM_ALG_ID algorithm,    //   IN: algorithm for encryption
    UINT16 keySizeInBits,    //   IN: key size in bit
    TPMI_ALG_SYM_MODE mode,  //   IN: symmetric encryption mode
    BYTE *key,               //   IN: encryption key
    TPM2B_IV *ivIn,   //   IN/OUT: Input IV and output chaining value for the
                      //   next block
    UINT32 dataSize,  //   IN: data size in byte
    BYTE *data        //   IN/OUT: data buffer
    );
UINT16 CryptStartHash(TPMI_ALG_HASH hashAlg,  // IN: hash algorithm
                      HASH_STATE *hashState  // OUT: the state of hash stack. It
                                             // will be used in hash update and
                                             // completion
                      );
void CryptUpdateDigest(void *digestState,  // IN: the state of hash stack
                       UINT32 dataSize,    // IN: the size of data
                       BYTE *data          // IN: data to be hashed
                       );
LIB_EXPORT void CryptUpdateDigest2B(void *digestState,  // IN: the digest state
                                    TPM2B *bIn  // IN: 2B containing the data
                                    );
void CryptUpdateDigestInt(void *state,     // IN: the state of hash stack
                          UINT32 intSize,  // IN: the size of 'intValue' in byte
                          void *intValue   // IN: integer value to be hashed
                          );
BOOL CryptUtilStartup(STARTUP_TYPE type  // IN: the startup type
                      );
TPM_RC CryptVerifySignature(
    TPMI_DH_OBJECT keyHandle,  // IN: The handle of sign key
    TPM2B_DIGEST *digest,      // IN: The digest being validated
    TPMT_SIGNATURE *signature  // IN: signature
    );
void KDFa(TPM_ALG_ID hash,      //   IN: hash algorithm used in HMAC
          TPM2B *key,           //   IN: HMAC key
          const char *label,    //   IN: a null-terminated label for KDF
          TPM2B *contextU,      //   IN: context U
          TPM2B *contextV,      //   IN: context V
          UINT32 sizeInBits,    //   IN: size of generated key in bit
          BYTE *keyStream,      //   OUT: key buffer
          UINT32 *counterInOut  //   IN/OUT: caller may provide the iteration
                                //   counter for incremental operations to avoid
                                //   large intermediate buffers.
          );

#endif  // __SOURCE_CRYPTUTIL_FP_H
