/*
 * Copyright 2015 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef __TPM2_CPRISYM_FP_H
#define __TPM2_CPRISYM_FP_H

LIB_EXPORT CRYPT_RESULT _cpri__AESDecryptCBC(
    BYTE *dOut,            // OUT: the decrypted data
    UINT32 keySizeInBits,  // IN: key size in bit
    BYTE *key,  // IN: key buffer. The size of this buffer in bytes is
                // (keySizeInBits + 7) / 8
    BYTE *iv,   // IN/OUT: IV for decryption. The size of this buffer is 16 byte
    UINT32 dInSize,  // IN: data size
    BYTE *dIn        // IN: data buffer
    );
LIB_EXPORT CRYPT_RESULT
_cpri__AESDecryptCFB(BYTE *dOut,            // OUT: the decrypted data
                     UINT32 keySizeInBits,  // IN: key size in bit
                     BYTE *key,  // IN: key buffer. The size of this buffer in
                                 // bytes is (keySizeInBits + 7) / 8
                     BYTE *iv,   // IN/OUT: IV for decryption.
                     UINT32 dInSize,  // IN: data size
                     BYTE *dIn        // IN: data buffer
                     );
//
//       _cpri__AESDecryptCTR()
//
// Counter mode decryption uses the same algorithm as encryption. The
// _cpri__AESDecryptCTR() function is implemented as a macro call to
// _cpri__AESEncryptCTR().
#define _cpri__AESDecryptCTR(dOut, keySize, key, iv, dInSize, dIn)       \
  _cpri__AESEncryptCTR(((BYTE *)dOut), ((UINT32)keySize), ((BYTE *)key), \
                       ((BYTE *)iv), ((UINT32)dInSize), ((BYTE *)dIn))
LIB_EXPORT CRYPT_RESULT
_cpri__AESDecryptECB(BYTE *dOut,            // OUT: the clear text data
                     UINT32 keySizeInBits,  // IN: key size in bit
                     BYTE *key,  // IN: key buffer. The size of this buffer in
                                 // bytes is (keySizeInBits + 7) / 8
                     UINT32 dInSize,  // IN: data size
                     BYTE *dIn        // IN: cipher text buffer
                     );

//
//       _cpri__AESDecryptOFB()
//
// OFB encryption and decryption use the same algorithms for both. The
// _cpri__AESDecryptOFB() function is implemented as a macro call to
// _cpri__AESEncrytOFB().
//
#define _cpri__AESDecryptOFB(dOut, keySizeInBits, key, iv, dInSize, dIn)       \
  _cpri__AESEncryptOFB(((BYTE *)dOut), ((UINT32)keySizeInBits), ((BYTE *)key), \
                       ((BYTE *)iv), ((UINT32)dInSize), ((BYTE *)dIn))

//
//    _cpri__SM4DecryptCTR()
//
// Counter mode decryption uses the same algorithm as encryption. The
// _cpri__SM4DecryptCTR() function is implemented as a macro call to
// _cpri__SM4EncryptCTR().
//
#define _cpri__SM4DecryptCTR(dOut, keySize, key, iv, dInSize, dIn)       \
  _cpri__SM4EncryptCTR(((BYTE *)dOut), ((UINT32)keySize), ((BYTE *)key), \
                       ((BYTE *)iv), ((UINT32)dInSize), ((BYTE *)dIn))

//
//       _cpri__SM4DecryptOFB()
//
// OFB encryption and decryption use the same algorithms for both. The
// _cpri__SM4DecryptOFB() function is implemented as a macro call to
// _cpri__SM4EncrytOFB().
//
#define _cpri__SM4DecryptOFB(dOut, keySizeInBits, key, iv, dInSize, dIn)       \
  _cpri__SM4EncryptOFB(((BYTE *)dOut), ((UINT32)keySizeInBits), ((BYTE *)key), \
                       ((BYTE *)iv), ((UINT32)dInSize), ((BYTE *)dIn))

LIB_EXPORT CRYPT_RESULT _cpri__AESEncryptCBC(
    BYTE *dOut,            // OUT:
    UINT32 keySizeInBits,  // IN: key size in bit
    BYTE *key,       // IN: key buffer. The size of this buffer in bytes is
                     // (keySizeInBits + 7) / 8
    BYTE *iv,        // IN/OUT: IV for decryption.
    UINT32 dInSize,  // IN: data size (is required to be a multiple of 16 bytes)
    BYTE *dIn        // IN: data buffer
    );
LIB_EXPORT CRYPT_RESULT
_cpri__AESEncryptCFB(BYTE *dOut,            // OUT: the encrypted
                     UINT32 keySizeInBits,  // IN: key size in bit
                     BYTE *key,  // IN: key buffer. The size of this buffer in
                                 // bytes is (keySizeInBits + 7) / 8
                     BYTE *iv,   // IN/OUT: IV for decryption.
                     UINT32 dInSize,  // IN: data size
                     BYTE *dIn        // IN: data buffer
                     );
LIB_EXPORT CRYPT_RESULT
_cpri__AESEncryptCTR(BYTE *dOut,            // OUT: the encrypted data
                     UINT32 keySizeInBits,  // IN: key size in bit
                     BYTE *key,  // IN: key buffer. The size of this buffer in
                                 // bytes is (keySizeInBits + 7) / 8
                     BYTE *iv,   // IN/OUT: IV for decryption.
                     UINT32 dInSize,  // IN: data size
                     BYTE *dIn        // IN: data buffer
                     );
LIB_EXPORT CRYPT_RESULT
_cpri__AESEncryptECB(BYTE *dOut,            // OUT: encrypted data
                     UINT32 keySizeInBits,  // IN: key size in bit
                     BYTE *key,  // IN: key buffer. The size of this buffer in
                                 // bytes is (keySizeInBits + 7) / 8
                     UINT32 dInSize,  // IN: data size
                     BYTE *dIn        // IN: clear text buffer
                     );
LIB_EXPORT CRYPT_RESULT _cpri__AESEncryptOFB(
    BYTE *dOut,            // OUT: the encrypted/decrypted data
    UINT32 keySizeInBits,  // IN: key size in bit
    BYTE *key,  // IN: key buffer. The size of this buffer in bytes is
                // (keySizeInBits + 7) / 8
    BYTE *iv,   // IN/OUT: IV for decryption. The size of this buffer is 16 byte
    UINT32 dInSize,  // IN: data size
    BYTE *dIn        // IN: data buffer
    );
LIB_EXPORT INT16 _cpri__GetSymmetricBlockSize(
    TPM_ALG_ID symmetricAlg,  // IN: the symmetric algorithm
    UINT16 keySizeInBits      // IN: the key size
    );
LIB_EXPORT BOOL _cpri__SymStartup(void);

#endif  // __TPM2_CPRISYM_FP_H
