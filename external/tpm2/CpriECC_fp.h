/*
 * Copyright 2015 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef __TPM2_CPRIECC_FP_H
#define __TPM2_CPRIECC_FP_H

LIB_EXPORT CRYPT_RESULT _cpri__C_2_2_KeyExchange(
    TPMS_ECC_POINT *outZ1,     //   OUT: a computed point
    TPMS_ECC_POINT *outZ2,     //   OUT: and optional second point
    TPM_ECC_CURVE curveId,     //   IN: the curve for the computations
    TPM_ALG_ID scheme,         //   IN: the key exchange scheme
    TPM2B_ECC_PARAMETER *dsA,  //   IN: static private TPM key
    TPM2B_ECC_PARAMETER *deA,  //   IN: ephemeral private TPM key
    TPMS_ECC_POINT *QsB,       //   IN: static public party B key
    TPMS_ECC_POINT *QeB        //   IN: ephemeral public party B key
    );
LIB_EXPORT CRYPT_RESULT _cpri__C_2_2_KeyExchange(
    TPMS_ECC_POINT *outZ1,     //   OUT: a computed point
    TPMS_ECC_POINT *outZ2,     //   OUT: and optional second point
    TPM_ECC_CURVE curveId,     //   IN: the curve for the computations
    TPM_ALG_ID scheme,         //   IN: the key exchange scheme
    TPM2B_ECC_PARAMETER *dsA,  //   IN: static private TPM key
    TPM2B_ECC_PARAMETER *deA,  //   IN: ephemeral private TPM key
    TPMS_ECC_POINT *QsB,       //   IN: static public party B key
    TPMS_ECC_POINT *QeB        //   IN: ephemeral public party B key
    );
LIB_EXPORT CRYPT_RESULT _cpri__EccCommitCompute(
    TPMS_ECC_POINT *K,       //   OUT: [d]B or [r]Q
    TPMS_ECC_POINT *L,       //   OUT: [r]B
    TPMS_ECC_POINT *E,       //   OUT: [r]M
    TPM_ECC_CURVE curveId,   //   IN: the curve for the computations
    TPMS_ECC_POINT *M,       //   IN: M (optional)
    TPMS_ECC_POINT *B,       //   IN: B (optional)
    TPM2B_ECC_PARAMETER *d,  //   IN: d (required)
    TPM2B_ECC_PARAMETER *r   //   IN: the computed r value (required)
    );
LIB_EXPORT UINT32 _cpri__EccGetCurveCount(void);
LIB_EXPORT const ECC_CURVE *_cpri__EccGetParametersByCurveId(
    TPM_ECC_CURVE curveId  // IN: the curveID
    );
LIB_EXPORT CRYPT_RESULT _cpri__EccPointMultiply(
    TPMS_ECC_POINT *Rout,   //   OUT: the product point R
    TPM_ECC_CURVE curveId,  //   IN: the curve to use
    TPM2B_ECC_PARAMETER *
        dIn,              //   IN: value to multiply against the curve generator
    TPMS_ECC_POINT *Qin,  //   IN: point Q
    TPM2B_ECC_PARAMETER *uIn  //   IN: scalar value for the multiplier of Q
    );
LIB_EXPORT BOOL
_cpri__EccIsPointOnCurve(TPM_ECC_CURVE curveId,  // IN: the curve selector
                         TPMS_ECC_POINT *Q       // IN: the point.
                         );
LIB_EXPORT CRYPT_RESULT _cpri__GenerateKeyEcc(
    TPMS_ECC_POINT *Qout,       //   OUT: the public point
    TPM2B_ECC_PARAMETER *dOut,  //   OUT: the private scalar
    TPM_ECC_CURVE curveId,      //   IN: the curve identifier
    TPM_ALG_ID
        hashAlg,  //   IN: hash algorithm to use in the key generation process
    TPM2B *seed,  //   IN: the seed to use
    const char *label,  //   IN: A label for the generation process.
    TPM2B *extra,       //   IN: Party 1 data for the KDF
    UINT32 *counter     //   IN/OUT: Counter value to allow KDF iteration to be
                        //   propagated across multiple functions
    );
LIB_EXPORT TPM_ECC_CURVE _cpri__GetCurveIdByIndex(UINT16 i);
LIB_EXPORT CRYPT_RESULT
_cpri__GetEphemeralEcc(TPMS_ECC_POINT *Qout,       // OUT: the public point
                       TPM2B_ECC_PARAMETER *dOut,  // OUT: the private scalar
                       TPM_ECC_CURVE curveId       // IN: the curve for the key
                       );
LIB_EXPORT CRYPT_RESULT _cpri__SignEcc(
    TPM2B_ECC_PARAMETER *rOut,  //   OUT: r component of the signature
    TPM2B_ECC_PARAMETER *sOut,  //   OUT: s component of the signature
    TPM_ALG_ID scheme,          //   IN: the scheme selector
    TPM_ALG_ID hashAlg,         //   IN: the hash algorithm if need
    TPM_ECC_CURVE curveId,      //   IN: the curve used in the signature process
    TPM2B_ECC_PARAMETER *dIn,   //   IN: the private key
    TPM2B *digest,              //   IN: the digest to sign
    TPM2B_ECC_PARAMETER *kIn    //   IN: k for input
    );
LIB_EXPORT BOOL _cpri__EccStartup(void);
LIB_EXPORT CRYPT_RESULT _cpri__ValidateSignatureEcc(
    TPM2B_ECC_PARAMETER *rIn,  //   IN: r component of the signature
    TPM2B_ECC_PARAMETER *sIn,  //   IN: s component of the signature
    TPM_ALG_ID scheme,         //   IN: the scheme selector
    TPM_ALG_ID
        hashAlg,  //   IN: the hash algorithm used (not used in all schemes)
    TPM_ECC_CURVE curveId,  //   IN: the curve used in the signature process
    TPMS_ECC_POINT *Qin,    //   IN: the public point of the key
    TPM2B *digest           //   IN: the digest that was signed
    );

#endif  // __TPM2_CPRIECC_FP_H
