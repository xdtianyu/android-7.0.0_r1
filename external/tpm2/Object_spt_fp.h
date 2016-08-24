/*
 * Copyright 2015 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef __TPM2_OBJECT_SPT_FP_H
#define __TPM2_OBJECT_SPT_FP_H

BOOL AreAttributesForParent(OBJECT *parentObject  // IN: parent handle
                            );
TPM_RC CredentialToSecret(
    TPM2B_ID_OBJECT *inIDObject,  //   IN: input credential blob
    TPM2B_NAME *name,             //   IN: the name of the object
    TPM2B_SEED *seed,             //   IN: an external seed.
    TPM_HANDLE protector,         //   IN: The protector's handle
    TPM2B_DIGEST *secret          //   OUT: secret information
    );
TPM_RC DuplicateToSensitive(
    TPM2B_PRIVATE *inPrivate,  //   IN: input private structure
    TPM2B_NAME *name,          //   IN: the name of the object
    TPM_HANDLE parentHandle,   //   IN: The parent's handle
    TPM_ALG_ID nameAlg,        //   IN: hash algorithm in public area.
    TPM2B_SEED *seed,  //   IN: an external seed may be provided. If external
                       //   seed is provided with size of 0, no outer wrap is
                       //   applied
    TPMT_SYM_DEF_OBJECT *symDef,  //   IN: Symmetric key definition. If the
                                  //   symmetric key algorithm is NULL, no inner
                                  //   wrap is applied
    TPM2B_DATA *innerSymKey,      //   IN: a symmetric key may be provided to
                              //   decrypt the inner wrap of a duplication blob.
    TPMT_SENSITIVE *sensitive  //   OUT: sensitive structure
    );
void FillInCreationData(
    TPMI_DH_OBJECT parentHandle,       //   IN: handle of parent
    TPMI_ALG_HASH nameHashAlg,         //   IN: name hash algorithm
    TPML_PCR_SELECTION *creationPCR,   //   IN: PCR selection
    TPM2B_DATA *outsideData,           //   IN: outside data
    TPM2B_CREATION_DATA *outCreation,  //   OUT: creation data for output
    TPM2B_DIGEST *creationDigest       //   OUT: creation digest
    );
TPM2B_SEED *GetSeedForKDF(
    TPM_HANDLE protectorHandle,  // IN: the protector handle
    TPM2B_SEED *seedIn           // IN: the optional input seed
    );
TPM_RC PrivateToSensitive(
    TPM2B_PRIVATE *inPrivate,  // IN: input private structure
    TPM2B_NAME *name,          // IN: the name of the object
    TPM_HANDLE parentHandle,   // IN: The parent's handle
    TPM_ALG_ID nameAlg,  // IN: hash algorithm in public area. It is passed
                         // separately because we only pass name, rather than
                         // the whole public area of the object. This parameter
                         // is used in the following two cases: 1. primary
                         // objects. 2. duplication blob with inner wrap. In
                         // other cases, this parameter will be ignored
    TPMT_SENSITIVE *sensitive  // OUT: sensitive structure
    );
UINT16 ProduceOuterWrap(
    TPM_HANDLE protector,  //   IN: The handle of the object that provides
                           //   protection. For object, it is parent handle. For
                           //   credential, it is the handle of encrypt object.
    TPM2B_NAME *name,      //   IN: the name of the object
    TPM_ALG_ID hashAlg,    //   IN: hash algorithm for outer wrap
    TPM2B_SEED *seed,  //   IN: an external seed may be provided for duplication
                       //   blob. For non duplication blob, this parameter
                       //   should be NULL
    BOOL useIV,        //   IN: indicate if an IV is used
    UINT16 dataSize,  //   IN: the size of sensitive data, excluding the leading
                      //   integrity buffer size or the optional iv size
    BYTE *outerBuffer  //   IN/OUT: outer buffer with sensitive data in it
    );
TPM_RC PublicAttributesValidation(
    BOOL load,  // IN: TRUE if load checks, FALSE if TPM2_Create()
    TPMI_DH_OBJECT parentHandle,  // IN: input parent handle
    TPMT_PUBLIC *publicArea       // IN: public area of the object
    );
TPM_RC SchemeChecks(
    BOOL load,  // IN: TRUE if load checks, FALSE if TPM2_Create()
    TPMI_DH_OBJECT parentHandle,  // IN: input parent handle
    TPMT_PUBLIC *publicArea       // IN: public area of the object
    );
void SecretToCredential(
    TPM2B_DIGEST *secret,         //   IN: secret information
    TPM2B_NAME *name,             //   IN: the name of the object
    TPM2B_SEED *seed,             //   IN: an external seed.
    TPM_HANDLE protector,         //   IN: The protector's handle
    TPM2B_ID_OBJECT *outIDObject  //   OUT: output credential
    );
void SensitiveToDuplicate(
    TPMT_SENSITIVE *sensitive,  //   IN: sensitive structure
    TPM2B_NAME *name,           //   IN: the name of the object
    TPM_HANDLE parentHandle,    //   IN: The new parent's handle
    TPM_ALG_ID nameAlg,  //   IN: hash algorithm in public area. It is passed
                         //   separately because we only pass name, rather than
                         //   the whole public area of the object.
    TPM2B_SEED *seed,  //   IN: the external seed. If external seed is provided
                       //   with size of 0, no outer wrap should be applied to
                       //   duplication blob.
    TPMT_SYM_DEF_OBJECT *symDef,  //   IN: Symmetric key definition. If the
                                  //   symmetric key algorithm is NULL, no inner
                                  //   wrap should be applied.
    TPM2B_DATA *innerSymKey,  //   IN/OUT: a symmetric key may be provided to
                              //   encrypt the inner wrap of a duplication blob.
                              //   May be generated here if needed.
    TPM2B_PRIVATE *outPrivate  //   OUT: output private structure
    );
void SensitiveToPrivate(
    TPMT_SENSITIVE *sensitive,  //   IN: sensitive structure
    TPM2B_NAME *name,           //   IN: the name of the object
    TPM_HANDLE parentHandle,    //   IN: The parent's handle
    TPM_ALG_ID nameAlg,  //   IN: hash algorithm in public area. This parameter
                         //   is used when parentHandle is NULL, in which case
                         //   the object is temporary.
    TPM2B_PRIVATE *outPrivate  //   OUT: output private structure
    );
TPM_RC UnwrapOuter(
    TPM_HANDLE protector,  //   IN: The handle of the object that provides
                           //   protection. For object, it is parent handle. For
                           //   credential, it is the handle of encrypt object.
    TPM2B_NAME *name,      //   IN: the name of the object
    TPM_ALG_ID hashAlg,    //   IN: hash algorithm for outer wrap
    TPM2B_SEED *seed,  //   IN: an external seed may be provided for duplication
                       //   blob. For non duplication blob, this parameter
                       //   should be NULL.
    BOOL useIV,        //   IN: indicates if an IV is used
    UINT16 dataSize,   //   IN: size of sensitive data in outerBuffer, including
                      //   the leading integrity buffer size, and an optional iv
                      //   area
    BYTE *outerBuffer  //   IN/OUT: sensitive data
    );

#endif  // __TPM2_OBJECT_SPT_FP_H
