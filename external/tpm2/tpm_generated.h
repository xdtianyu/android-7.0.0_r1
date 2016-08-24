// Copyright 2015 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// THIS CODE IS GENERATED - DO NOT MODIFY!

#ifndef TPM2_TPM_GENERATED_H_
#define TPM2_TPM_GENERATED_H_

#include <endian.h>
#include <string.h>

#include "TPM_Types.h"
#include "Tpm.h"

UINT16 uint8_t_Marshal(uint8_t* source, BYTE** buffer, INT32* size);

TPM_RC uint8_t_Unmarshal(uint8_t* target, BYTE** buffer, INT32* size);

UINT16 int8_t_Marshal(int8_t* source, BYTE** buffer, INT32* size);

TPM_RC int8_t_Unmarshal(int8_t* target, BYTE** buffer, INT32* size);

UINT16 uint16_t_Marshal(uint16_t* source, BYTE** buffer, INT32* size);

TPM_RC uint16_t_Unmarshal(uint16_t* target, BYTE** buffer, INT32* size);

UINT16 int16_t_Marshal(int16_t* source, BYTE** buffer, INT32* size);

TPM_RC int16_t_Unmarshal(int16_t* target, BYTE** buffer, INT32* size);

UINT16 uint32_t_Marshal(uint32_t* source, BYTE** buffer, INT32* size);

TPM_RC uint32_t_Unmarshal(uint32_t* target, BYTE** buffer, INT32* size);

UINT16 int32_t_Marshal(int32_t* source, BYTE** buffer, INT32* size);

TPM_RC int32_t_Unmarshal(int32_t* target, BYTE** buffer, INT32* size);

UINT16 uint64_t_Marshal(uint64_t* source, BYTE** buffer, INT32* size);

TPM_RC uint64_t_Unmarshal(uint64_t* target, BYTE** buffer, INT32* size);

UINT16 int64_t_Marshal(int64_t* source, BYTE** buffer, INT32* size);

TPM_RC int64_t_Unmarshal(int64_t* target, BYTE** buffer, INT32* size);

UINT16 BYTE_Marshal(BYTE* source, BYTE** buffer, INT32* size);

TPM_RC BYTE_Unmarshal(BYTE* target, BYTE** buffer, INT32* size);

UINT16 INT16_Marshal(INT16* source, BYTE** buffer, INT32* size);

TPM_RC INT16_Unmarshal(INT16* target, BYTE** buffer, INT32* size);

UINT16 INT32_Marshal(INT32* source, BYTE** buffer, INT32* size);

TPM_RC INT32_Unmarshal(INT32* target, BYTE** buffer, INT32* size);

UINT16 INT64_Marshal(INT64* source, BYTE** buffer, INT32* size);

TPM_RC INT64_Unmarshal(INT64* target, BYTE** buffer, INT32* size);

UINT16 INT8_Marshal(INT8* source, BYTE** buffer, INT32* size);

TPM_RC INT8_Unmarshal(INT8* target, BYTE** buffer, INT32* size);

UINT16 TPM2B_ATTEST_Marshal(TPM2B_ATTEST* source, BYTE** buffer, INT32* size);

TPM_RC TPM2B_ATTEST_Unmarshal(TPM2B_ATTEST* target, BYTE** buffer, INT32* size);

UINT16 TPM2B_DIGEST_Marshal(TPM2B_DIGEST* source, BYTE** buffer, INT32* size);

TPM_RC TPM2B_DIGEST_Unmarshal(TPM2B_DIGEST* target, BYTE** buffer, INT32* size);

UINT16 TPM2B_AUTH_Marshal(TPM2B_AUTH* source, BYTE** buffer, INT32* size);

TPM_RC TPM2B_AUTH_Unmarshal(TPM2B_AUTH* target, BYTE** buffer, INT32* size);

UINT16 TPM2B_CONTEXT_DATA_Marshal(TPM2B_CONTEXT_DATA* source,
                                  BYTE** buffer,
                                  INT32* size);

TPM_RC TPM2B_CONTEXT_DATA_Unmarshal(TPM2B_CONTEXT_DATA* target,
                                    BYTE** buffer,
                                    INT32* size);

UINT16 TPM2B_CONTEXT_SENSITIVE_Marshal(TPM2B_CONTEXT_SENSITIVE* source,
                                       BYTE** buffer,
                                       INT32* size);

TPM_RC TPM2B_CONTEXT_SENSITIVE_Unmarshal(TPM2B_CONTEXT_SENSITIVE* target,
                                         BYTE** buffer,
                                         INT32* size);

UINT16 TPM2B_CREATION_DATA_Marshal(TPM2B_CREATION_DATA* source,
                                   BYTE** buffer,
                                   INT32* size);

TPM_RC TPM2B_CREATION_DATA_Unmarshal(TPM2B_CREATION_DATA* target,
                                     BYTE** buffer,
                                     INT32* size);

UINT16 TPM2B_DATA_Marshal(TPM2B_DATA* source, BYTE** buffer, INT32* size);

TPM_RC TPM2B_DATA_Unmarshal(TPM2B_DATA* target, BYTE** buffer, INT32* size);

UINT16 TPM2B_DIGEST_VALUES_Marshal(TPM2B_DIGEST_VALUES* source,
                                   BYTE** buffer,
                                   INT32* size);

TPM_RC TPM2B_DIGEST_VALUES_Unmarshal(TPM2B_DIGEST_VALUES* target,
                                     BYTE** buffer,
                                     INT32* size);

UINT16 TPM2B_ECC_PARAMETER_Marshal(TPM2B_ECC_PARAMETER* source,
                                   BYTE** buffer,
                                   INT32* size);

TPM_RC TPM2B_ECC_PARAMETER_Unmarshal(TPM2B_ECC_PARAMETER* target,
                                     BYTE** buffer,
                                     INT32* size);

UINT16 TPM2B_ECC_POINT_Marshal(TPM2B_ECC_POINT* source,
                               BYTE** buffer,
                               INT32* size);

TPM_RC TPM2B_ECC_POINT_Unmarshal(TPM2B_ECC_POINT* target,
                                 BYTE** buffer,
                                 INT32* size);

UINT16 TPM2B_ENCRYPTED_SECRET_Marshal(TPM2B_ENCRYPTED_SECRET* source,
                                      BYTE** buffer,
                                      INT32* size);

TPM_RC TPM2B_ENCRYPTED_SECRET_Unmarshal(TPM2B_ENCRYPTED_SECRET* target,
                                        BYTE** buffer,
                                        INT32* size);

UINT16 TPM2B_EVENT_Marshal(TPM2B_EVENT* source, BYTE** buffer, INT32* size);

TPM_RC TPM2B_EVENT_Unmarshal(TPM2B_EVENT* target, BYTE** buffer, INT32* size);

UINT16 TPM2B_ID_OBJECT_Marshal(TPM2B_ID_OBJECT* source,
                               BYTE** buffer,
                               INT32* size);

TPM_RC TPM2B_ID_OBJECT_Unmarshal(TPM2B_ID_OBJECT* target,
                                 BYTE** buffer,
                                 INT32* size);

UINT16 TPM2B_IV_Marshal(TPM2B_IV* source, BYTE** buffer, INT32* size);

TPM_RC TPM2B_IV_Unmarshal(TPM2B_IV* target, BYTE** buffer, INT32* size);

UINT16 TPM2B_MAX_BUFFER_Marshal(TPM2B_MAX_BUFFER* source,
                                BYTE** buffer,
                                INT32* size);

TPM_RC TPM2B_MAX_BUFFER_Unmarshal(TPM2B_MAX_BUFFER* target,
                                  BYTE** buffer,
                                  INT32* size);

UINT16 TPM2B_MAX_NV_BUFFER_Marshal(TPM2B_MAX_NV_BUFFER* source,
                                   BYTE** buffer,
                                   INT32* size);

TPM_RC TPM2B_MAX_NV_BUFFER_Unmarshal(TPM2B_MAX_NV_BUFFER* target,
                                     BYTE** buffer,
                                     INT32* size);

UINT16 TPM2B_NAME_Marshal(TPM2B_NAME* source, BYTE** buffer, INT32* size);

TPM_RC TPM2B_NAME_Unmarshal(TPM2B_NAME* target, BYTE** buffer, INT32* size);

UINT16 TPM2B_NONCE_Marshal(TPM2B_NONCE* source, BYTE** buffer, INT32* size);

TPM_RC TPM2B_NONCE_Unmarshal(TPM2B_NONCE* target, BYTE** buffer, INT32* size);

UINT16 TPM2B_NV_PUBLIC_Marshal(TPM2B_NV_PUBLIC* source,
                               BYTE** buffer,
                               INT32* size);

TPM_RC TPM2B_NV_PUBLIC_Unmarshal(TPM2B_NV_PUBLIC* target,
                                 BYTE** buffer,
                                 INT32* size);

UINT16 TPM2B_OPERAND_Marshal(TPM2B_OPERAND* source, BYTE** buffer, INT32* size);

TPM_RC TPM2B_OPERAND_Unmarshal(TPM2B_OPERAND* target,
                               BYTE** buffer,
                               INT32* size);

UINT16 TPM2B_PRIVATE_Marshal(TPM2B_PRIVATE* source, BYTE** buffer, INT32* size);

TPM_RC TPM2B_PRIVATE_Unmarshal(TPM2B_PRIVATE* target,
                               BYTE** buffer,
                               INT32* size);

UINT16 TPM2B_PRIVATE_KEY_RSA_Marshal(TPM2B_PRIVATE_KEY_RSA* source,
                                     BYTE** buffer,
                                     INT32* size);

TPM_RC TPM2B_PRIVATE_KEY_RSA_Unmarshal(TPM2B_PRIVATE_KEY_RSA* target,
                                       BYTE** buffer,
                                       INT32* size);

UINT16 TPM2B_PRIVATE_VENDOR_SPECIFIC_Marshal(
    TPM2B_PRIVATE_VENDOR_SPECIFIC* source,
    BYTE** buffer,
    INT32* size);

TPM_RC TPM2B_PRIVATE_VENDOR_SPECIFIC_Unmarshal(
    TPM2B_PRIVATE_VENDOR_SPECIFIC* target,
    BYTE** buffer,
    INT32* size);

UINT16 TPM2B_PUBLIC_Marshal(TPM2B_PUBLIC* source, BYTE** buffer, INT32* size);

TPM_RC TPM2B_PUBLIC_Unmarshal(TPM2B_PUBLIC* target, BYTE** buffer, INT32* size);

UINT16 TPM2B_PUBLIC_KEY_RSA_Marshal(TPM2B_PUBLIC_KEY_RSA* source,
                                    BYTE** buffer,
                                    INT32* size);

TPM_RC TPM2B_PUBLIC_KEY_RSA_Unmarshal(TPM2B_PUBLIC_KEY_RSA* target,
                                      BYTE** buffer,
                                      INT32* size);

UINT16 TPM2B_SENSITIVE_Marshal(TPM2B_SENSITIVE* source,
                               BYTE** buffer,
                               INT32* size);

TPM_RC TPM2B_SENSITIVE_Unmarshal(TPM2B_SENSITIVE* target,
                                 BYTE** buffer,
                                 INT32* size);

UINT16 TPM2B_SENSITIVE_CREATE_Marshal(TPM2B_SENSITIVE_CREATE* source,
                                      BYTE** buffer,
                                      INT32* size);

TPM_RC TPM2B_SENSITIVE_CREATE_Unmarshal(TPM2B_SENSITIVE_CREATE* target,
                                        BYTE** buffer,
                                        INT32* size);

UINT16 TPM2B_SENSITIVE_DATA_Marshal(TPM2B_SENSITIVE_DATA* source,
                                    BYTE** buffer,
                                    INT32* size);

TPM_RC TPM2B_SENSITIVE_DATA_Unmarshal(TPM2B_SENSITIVE_DATA* target,
                                      BYTE** buffer,
                                      INT32* size);

UINT16 TPM2B_SYM_KEY_Marshal(TPM2B_SYM_KEY* source, BYTE** buffer, INT32* size);

TPM_RC TPM2B_SYM_KEY_Unmarshal(TPM2B_SYM_KEY* target,
                               BYTE** buffer,
                               INT32* size);

UINT16 TPM2B_TIMEOUT_Marshal(TPM2B_TIMEOUT* source, BYTE** buffer, INT32* size);

TPM_RC TPM2B_TIMEOUT_Unmarshal(TPM2B_TIMEOUT* target,
                               BYTE** buffer,
                               INT32* size);

UINT16 UINT32_Marshal(UINT32* source, BYTE** buffer, INT32* size);

TPM_RC UINT32_Unmarshal(UINT32* target, BYTE** buffer, INT32* size);

UINT16 TPMA_ALGORITHM_Marshal(TPMA_ALGORITHM* source,
                              BYTE** buffer,
                              INT32* size);

TPM_RC TPMA_ALGORITHM_Unmarshal(TPMA_ALGORITHM* target,
                                BYTE** buffer,
                                INT32* size);

UINT16 TPM_CC_Marshal(TPM_CC* source, BYTE** buffer, INT32* size);

TPM_RC TPM_CC_Unmarshal(TPM_CC* target, BYTE** buffer, INT32* size);

UINT16 TPMA_CC_Marshal(TPMA_CC* source, BYTE** buffer, INT32* size);

TPM_RC TPMA_CC_Unmarshal(TPMA_CC* target, BYTE** buffer, INT32* size);

UINT16 UINT8_Marshal(UINT8* source, BYTE** buffer, INT32* size);

TPM_RC UINT8_Unmarshal(UINT8* target, BYTE** buffer, INT32* size);

UINT16 TPMA_LOCALITY_Marshal(TPMA_LOCALITY* source, BYTE** buffer, INT32* size);

TPM_RC TPMA_LOCALITY_Unmarshal(TPMA_LOCALITY* target,
                               BYTE** buffer,
                               INT32* size);

UINT16 TPMA_MEMORY_Marshal(TPMA_MEMORY* source, BYTE** buffer, INT32* size);

TPM_RC TPMA_MEMORY_Unmarshal(TPMA_MEMORY* target, BYTE** buffer, INT32* size);

UINT16 TPMA_NV_Marshal(TPMA_NV* source, BYTE** buffer, INT32* size);

TPM_RC TPMA_NV_Unmarshal(TPMA_NV* target, BYTE** buffer, INT32* size);

UINT16 TPMA_OBJECT_Marshal(TPMA_OBJECT* source, BYTE** buffer, INT32* size);

TPM_RC TPMA_OBJECT_Unmarshal(TPMA_OBJECT* target, BYTE** buffer, INT32* size);

UINT16 TPMA_PERMANENT_Marshal(TPMA_PERMANENT* source,
                              BYTE** buffer,
                              INT32* size);

TPM_RC TPMA_PERMANENT_Unmarshal(TPMA_PERMANENT* target,
                                BYTE** buffer,
                                INT32* size);

UINT16 TPMA_SESSION_Marshal(TPMA_SESSION* source, BYTE** buffer, INT32* size);

TPM_RC TPMA_SESSION_Unmarshal(TPMA_SESSION* target, BYTE** buffer, INT32* size);

UINT16 TPMA_STARTUP_CLEAR_Marshal(TPMA_STARTUP_CLEAR* source,
                                  BYTE** buffer,
                                  INT32* size);

TPM_RC TPMA_STARTUP_CLEAR_Unmarshal(TPMA_STARTUP_CLEAR* target,
                                    BYTE** buffer,
                                    INT32* size);

UINT16 UINT16_Marshal(UINT16* source, BYTE** buffer, INT32* size);

TPM_RC UINT16_Unmarshal(UINT16* target, BYTE** buffer, INT32* size);

UINT16 TPM_KEY_BITS_Marshal(TPM_KEY_BITS* source, BYTE** buffer, INT32* size);

TPM_RC TPM_KEY_BITS_Unmarshal(TPM_KEY_BITS* target, BYTE** buffer, INT32* size);

UINT16 TPMI_AES_KEY_BITS_Marshal(TPMI_AES_KEY_BITS* source,
                                 BYTE** buffer,
                                 INT32* size);

TPM_RC TPMI_AES_KEY_BITS_Unmarshal(TPMI_AES_KEY_BITS* target,
                                   BYTE** buffer,
                                   INT32* size);

UINT16 TPM_ALG_ID_Marshal(TPM_ALG_ID* source, BYTE** buffer, INT32* size);

TPM_RC TPM_ALG_ID_Unmarshal(TPM_ALG_ID* target, BYTE** buffer, INT32* size);

UINT16 TPMI_ALG_ASYM_Marshal(TPMI_ALG_ASYM* source, BYTE** buffer, INT32* size);

TPM_RC TPMI_ALG_ASYM_Unmarshal(TPMI_ALG_ASYM* target,
                               BYTE** buffer,
                               INT32* size,
                               BOOL allow_conditioanl_value);

UINT16 TPMI_ALG_ASYM_SCHEME_Marshal(TPMI_ALG_ASYM_SCHEME* source,
                                    BYTE** buffer,
                                    INT32* size);

TPM_RC TPMI_ALG_ASYM_SCHEME_Unmarshal(TPMI_ALG_ASYM_SCHEME* target,
                                      BYTE** buffer,
                                      INT32* size,
                                      BOOL allow_conditioanl_value);

UINT16 TPMI_ALG_ECC_SCHEME_Marshal(TPMI_ALG_ECC_SCHEME* source,
                                   BYTE** buffer,
                                   INT32* size);

TPM_RC TPMI_ALG_ECC_SCHEME_Unmarshal(TPMI_ALG_ECC_SCHEME* target,
                                     BYTE** buffer,
                                     INT32* size,
                                     BOOL allow_conditioanl_value);

UINT16 TPMI_ALG_HASH_Marshal(TPMI_ALG_HASH* source, BYTE** buffer, INT32* size);

TPM_RC TPMI_ALG_HASH_Unmarshal(TPMI_ALG_HASH* target,
                               BYTE** buffer,
                               INT32* size,
                               BOOL allow_conditioanl_value);

UINT16 TPMI_ALG_KDF_Marshal(TPMI_ALG_KDF* source, BYTE** buffer, INT32* size);

TPM_RC TPMI_ALG_KDF_Unmarshal(TPMI_ALG_KDF* target,
                              BYTE** buffer,
                              INT32* size,
                              BOOL allow_conditioanl_value);

UINT16 TPMI_ALG_KEYEDHASH_SCHEME_Marshal(TPMI_ALG_KEYEDHASH_SCHEME* source,
                                         BYTE** buffer,
                                         INT32* size);

TPM_RC TPMI_ALG_KEYEDHASH_SCHEME_Unmarshal(TPMI_ALG_KEYEDHASH_SCHEME* target,
                                           BYTE** buffer,
                                           INT32* size,
                                           BOOL allow_conditioanl_value);

UINT16 TPMI_ALG_PUBLIC_Marshal(TPMI_ALG_PUBLIC* source,
                               BYTE** buffer,
                               INT32* size);

TPM_RC TPMI_ALG_PUBLIC_Unmarshal(TPMI_ALG_PUBLIC* target,
                                 BYTE** buffer,
                                 INT32* size);

UINT16 TPMI_ALG_RSA_DECRYPT_Marshal(TPMI_ALG_RSA_DECRYPT* source,
                                    BYTE** buffer,
                                    INT32* size);

TPM_RC TPMI_ALG_RSA_DECRYPT_Unmarshal(TPMI_ALG_RSA_DECRYPT* target,
                                      BYTE** buffer,
                                      INT32* size,
                                      BOOL allow_conditioanl_value);

UINT16 TPMI_ALG_RSA_SCHEME_Marshal(TPMI_ALG_RSA_SCHEME* source,
                                   BYTE** buffer,
                                   INT32* size);

TPM_RC TPMI_ALG_RSA_SCHEME_Unmarshal(TPMI_ALG_RSA_SCHEME* target,
                                     BYTE** buffer,
                                     INT32* size,
                                     BOOL allow_conditioanl_value);

UINT16 TPMI_ALG_SIG_SCHEME_Marshal(TPMI_ALG_SIG_SCHEME* source,
                                   BYTE** buffer,
                                   INT32* size);

TPM_RC TPMI_ALG_SIG_SCHEME_Unmarshal(TPMI_ALG_SIG_SCHEME* target,
                                     BYTE** buffer,
                                     INT32* size,
                                     BOOL allow_conditioanl_value);

UINT16 TPMI_ALG_SYM_Marshal(TPMI_ALG_SYM* source, BYTE** buffer, INT32* size);

TPM_RC TPMI_ALG_SYM_Unmarshal(TPMI_ALG_SYM* target,
                              BYTE** buffer,
                              INT32* size,
                              BOOL allow_conditioanl_value);

UINT16 TPMI_ALG_SYM_MODE_Marshal(TPMI_ALG_SYM_MODE* source,
                                 BYTE** buffer,
                                 INT32* size);

TPM_RC TPMI_ALG_SYM_MODE_Unmarshal(TPMI_ALG_SYM_MODE* target,
                                   BYTE** buffer,
                                   INT32* size,
                                   BOOL allow_conditioanl_value);

UINT16 TPMI_ALG_SYM_OBJECT_Marshal(TPMI_ALG_SYM_OBJECT* source,
                                   BYTE** buffer,
                                   INT32* size);

TPM_RC TPMI_ALG_SYM_OBJECT_Unmarshal(TPMI_ALG_SYM_OBJECT* target,
                                     BYTE** buffer,
                                     INT32* size,
                                     BOOL allow_conditioanl_value);

UINT16 TPMI_CAMELLIA_KEY_BITS_Marshal(TPMI_CAMELLIA_KEY_BITS* source,
                                      BYTE** buffer,
                                      INT32* size);

TPM_RC TPMI_CAMELLIA_KEY_BITS_Unmarshal(TPMI_CAMELLIA_KEY_BITS* target,
                                        BYTE** buffer,
                                        INT32* size);

UINT16 TPM_HANDLE_Marshal(TPM_HANDLE* source, BYTE** buffer, INT32* size);

TPM_RC TPM_HANDLE_Unmarshal(TPM_HANDLE* target, BYTE** buffer, INT32* size);

UINT16 TPMI_DH_CONTEXT_Marshal(TPMI_DH_CONTEXT* source,
                               BYTE** buffer,
                               INT32* size);

TPM_RC TPMI_DH_CONTEXT_Unmarshal(TPMI_DH_CONTEXT* target,
                                 BYTE** buffer,
                                 INT32* size);

UINT16 TPMI_DH_ENTITY_Marshal(TPMI_DH_ENTITY* source,
                              BYTE** buffer,
                              INT32* size);

TPM_RC TPMI_DH_ENTITY_Unmarshal(TPMI_DH_ENTITY* target,
                                BYTE** buffer,
                                INT32* size,
                                BOOL allow_conditioanl_value);

UINT16 TPMI_DH_OBJECT_Marshal(TPMI_DH_OBJECT* source,
                              BYTE** buffer,
                              INT32* size);

TPM_RC TPMI_DH_OBJECT_Unmarshal(TPMI_DH_OBJECT* target,
                                BYTE** buffer,
                                INT32* size,
                                BOOL allow_conditioanl_value);

UINT16 TPMI_DH_PCR_Marshal(TPMI_DH_PCR* source, BYTE** buffer, INT32* size);

TPM_RC TPMI_DH_PCR_Unmarshal(TPMI_DH_PCR* target,
                             BYTE** buffer,
                             INT32* size,
                             BOOL allow_conditioanl_value);

UINT16 TPMI_DH_PERSISTENT_Marshal(TPMI_DH_PERSISTENT* source,
                                  BYTE** buffer,
                                  INT32* size);

TPM_RC TPMI_DH_PERSISTENT_Unmarshal(TPMI_DH_PERSISTENT* target,
                                    BYTE** buffer,
                                    INT32* size);

UINT16 TPM_ECC_CURVE_Marshal(TPM_ECC_CURVE* source, BYTE** buffer, INT32* size);

TPM_RC TPM_ECC_CURVE_Unmarshal(TPM_ECC_CURVE* target,
                               BYTE** buffer,
                               INT32* size);

UINT16 TPMI_ECC_CURVE_Marshal(TPMI_ECC_CURVE* source,
                              BYTE** buffer,
                              INT32* size);

TPM_RC TPMI_ECC_CURVE_Unmarshal(TPMI_ECC_CURVE* target,
                                BYTE** buffer,
                                INT32* size);

UINT16 TPMI_ECC_KEY_EXCHANGE_Marshal(TPMI_ECC_KEY_EXCHANGE* source,
                                     BYTE** buffer,
                                     INT32* size);

TPM_RC TPMI_ECC_KEY_EXCHANGE_Unmarshal(TPMI_ECC_KEY_EXCHANGE* target,
                                       BYTE** buffer,
                                       INT32* size,
                                       BOOL allow_conditioanl_value);

UINT16 TPMI_RH_CLEAR_Marshal(TPMI_RH_CLEAR* source, BYTE** buffer, INT32* size);

TPM_RC TPMI_RH_CLEAR_Unmarshal(TPMI_RH_CLEAR* target,
                               BYTE** buffer,
                               INT32* size);

UINT16 TPMI_RH_ENABLES_Marshal(TPMI_RH_ENABLES* source,
                               BYTE** buffer,
                               INT32* size);

TPM_RC TPMI_RH_ENABLES_Unmarshal(TPMI_RH_ENABLES* target,
                                 BYTE** buffer,
                                 INT32* size,
                                 BOOL allow_conditioanl_value);

UINT16 TPMI_RH_ENDORSEMENT_Marshal(TPMI_RH_ENDORSEMENT* source,
                                   BYTE** buffer,
                                   INT32* size);

TPM_RC TPMI_RH_ENDORSEMENT_Unmarshal(TPMI_RH_ENDORSEMENT* target,
                                     BYTE** buffer,
                                     INT32* size,
                                     BOOL allow_conditioanl_value);

UINT16 TPMI_RH_HIERARCHY_Marshal(TPMI_RH_HIERARCHY* source,
                                 BYTE** buffer,
                                 INT32* size);

TPM_RC TPMI_RH_HIERARCHY_Unmarshal(TPMI_RH_HIERARCHY* target,
                                   BYTE** buffer,
                                   INT32* size,
                                   BOOL allow_conditioanl_value);

UINT16 TPMI_RH_HIERARCHY_AUTH_Marshal(TPMI_RH_HIERARCHY_AUTH* source,
                                      BYTE** buffer,
                                      INT32* size);

TPM_RC TPMI_RH_HIERARCHY_AUTH_Unmarshal(TPMI_RH_HIERARCHY_AUTH* target,
                                        BYTE** buffer,
                                        INT32* size);

UINT16 TPMI_RH_LOCKOUT_Marshal(TPMI_RH_LOCKOUT* source,
                               BYTE** buffer,
                               INT32* size);

TPM_RC TPMI_RH_LOCKOUT_Unmarshal(TPMI_RH_LOCKOUT* target,
                                 BYTE** buffer,
                                 INT32* size);

UINT16 TPMI_RH_NV_AUTH_Marshal(TPMI_RH_NV_AUTH* source,
                               BYTE** buffer,
                               INT32* size);

TPM_RC TPMI_RH_NV_AUTH_Unmarshal(TPMI_RH_NV_AUTH* target,
                                 BYTE** buffer,
                                 INT32* size);

UINT16 TPMI_RH_NV_INDEX_Marshal(TPMI_RH_NV_INDEX* source,
                                BYTE** buffer,
                                INT32* size);

TPM_RC TPMI_RH_NV_INDEX_Unmarshal(TPMI_RH_NV_INDEX* target,
                                  BYTE** buffer,
                                  INT32* size);

UINT16 TPMI_RH_OWNER_Marshal(TPMI_RH_OWNER* source, BYTE** buffer, INT32* size);

TPM_RC TPMI_RH_OWNER_Unmarshal(TPMI_RH_OWNER* target,
                               BYTE** buffer,
                               INT32* size,
                               BOOL allow_conditioanl_value);

UINT16 TPMI_RH_PLATFORM_Marshal(TPMI_RH_PLATFORM* source,
                                BYTE** buffer,
                                INT32* size);

TPM_RC TPMI_RH_PLATFORM_Unmarshal(TPMI_RH_PLATFORM* target,
                                  BYTE** buffer,
                                  INT32* size);

UINT16 TPMI_RH_PROVISION_Marshal(TPMI_RH_PROVISION* source,
                                 BYTE** buffer,
                                 INT32* size);

TPM_RC TPMI_RH_PROVISION_Unmarshal(TPMI_RH_PROVISION* target,
                                   BYTE** buffer,
                                   INT32* size);

UINT16 TPMI_RSA_KEY_BITS_Marshal(TPMI_RSA_KEY_BITS* source,
                                 BYTE** buffer,
                                 INT32* size);

TPM_RC TPMI_RSA_KEY_BITS_Unmarshal(TPMI_RSA_KEY_BITS* target,
                                   BYTE** buffer,
                                   INT32* size);

UINT16 TPMI_SH_AUTH_SESSION_Marshal(TPMI_SH_AUTH_SESSION* source,
                                    BYTE** buffer,
                                    INT32* size);

TPM_RC TPMI_SH_AUTH_SESSION_Unmarshal(TPMI_SH_AUTH_SESSION* target,
                                      BYTE** buffer,
                                      INT32* size,
                                      BOOL allow_conditioanl_value);

UINT16 TPMI_SH_HMAC_Marshal(TPMI_SH_HMAC* source, BYTE** buffer, INT32* size);

TPM_RC TPMI_SH_HMAC_Unmarshal(TPMI_SH_HMAC* target, BYTE** buffer, INT32* size);

UINT16 TPMI_SH_POLICY_Marshal(TPMI_SH_POLICY* source,
                              BYTE** buffer,
                              INT32* size);

TPM_RC TPMI_SH_POLICY_Unmarshal(TPMI_SH_POLICY* target,
                                BYTE** buffer,
                                INT32* size);

UINT16 TPMI_SM4_KEY_BITS_Marshal(TPMI_SM4_KEY_BITS* source,
                                 BYTE** buffer,
                                 INT32* size);

TPM_RC TPMI_SM4_KEY_BITS_Unmarshal(TPMI_SM4_KEY_BITS* target,
                                   BYTE** buffer,
                                   INT32* size);

UINT16 TPM_ST_Marshal(TPM_ST* source, BYTE** buffer, INT32* size);

TPM_RC TPM_ST_Unmarshal(TPM_ST* target, BYTE** buffer, INT32* size);

UINT16 TPMI_ST_ATTEST_Marshal(TPMI_ST_ATTEST* source,
                              BYTE** buffer,
                              INT32* size);

TPM_RC TPMI_ST_ATTEST_Unmarshal(TPMI_ST_ATTEST* target,
                                BYTE** buffer,
                                INT32* size);

UINT16 TPMI_ST_COMMAND_TAG_Marshal(TPMI_ST_COMMAND_TAG* source,
                                   BYTE** buffer,
                                   INT32* size);

TPM_RC TPMI_ST_COMMAND_TAG_Unmarshal(TPMI_ST_COMMAND_TAG* target,
                                     BYTE** buffer,
                                     INT32* size);

UINT16 TPMI_YES_NO_Marshal(TPMI_YES_NO* source, BYTE** buffer, INT32* size);

TPM_RC TPMI_YES_NO_Unmarshal(TPMI_YES_NO* target, BYTE** buffer, INT32* size);

UINT16 TPML_ALG_Marshal(TPML_ALG* source, BYTE** buffer, INT32* size);

TPM_RC TPML_ALG_Unmarshal(TPML_ALG* target, BYTE** buffer, INT32* size);

UINT16 TPML_ALG_PROPERTY_Marshal(TPML_ALG_PROPERTY* source,
                                 BYTE** buffer,
                                 INT32* size);

TPM_RC TPML_ALG_PROPERTY_Unmarshal(TPML_ALG_PROPERTY* target,
                                   BYTE** buffer,
                                   INT32* size);

UINT16 TPML_CC_Marshal(TPML_CC* source, BYTE** buffer, INT32* size);

TPM_RC TPML_CC_Unmarshal(TPML_CC* target, BYTE** buffer, INT32* size);

UINT16 TPML_CCA_Marshal(TPML_CCA* source, BYTE** buffer, INT32* size);

TPM_RC TPML_CCA_Unmarshal(TPML_CCA* target, BYTE** buffer, INT32* size);

UINT16 TPML_DIGEST_Marshal(TPML_DIGEST* source, BYTE** buffer, INT32* size);

TPM_RC TPML_DIGEST_Unmarshal(TPML_DIGEST* target, BYTE** buffer, INT32* size);

UINT16 TPML_DIGEST_VALUES_Marshal(TPML_DIGEST_VALUES* source,
                                  BYTE** buffer,
                                  INT32* size);

TPM_RC TPML_DIGEST_VALUES_Unmarshal(TPML_DIGEST_VALUES* target,
                                    BYTE** buffer,
                                    INT32* size);

UINT16 TPML_ECC_CURVE_Marshal(TPML_ECC_CURVE* source,
                              BYTE** buffer,
                              INT32* size);

TPM_RC TPML_ECC_CURVE_Unmarshal(TPML_ECC_CURVE* target,
                                BYTE** buffer,
                                INT32* size);

UINT16 TPML_HANDLE_Marshal(TPML_HANDLE* source, BYTE** buffer, INT32* size);

TPM_RC TPML_HANDLE_Unmarshal(TPML_HANDLE* target, BYTE** buffer, INT32* size);

UINT16 TPML_PCR_SELECTION_Marshal(TPML_PCR_SELECTION* source,
                                  BYTE** buffer,
                                  INT32* size);

TPM_RC TPML_PCR_SELECTION_Unmarshal(TPML_PCR_SELECTION* target,
                                    BYTE** buffer,
                                    INT32* size);

UINT16 TPML_TAGGED_PCR_PROPERTY_Marshal(TPML_TAGGED_PCR_PROPERTY* source,
                                        BYTE** buffer,
                                        INT32* size);

TPM_RC TPML_TAGGED_PCR_PROPERTY_Unmarshal(TPML_TAGGED_PCR_PROPERTY* target,
                                          BYTE** buffer,
                                          INT32* size);

UINT16 TPML_TAGGED_TPM_PROPERTY_Marshal(TPML_TAGGED_TPM_PROPERTY* source,
                                        BYTE** buffer,
                                        INT32* size);

TPM_RC TPML_TAGGED_TPM_PROPERTY_Unmarshal(TPML_TAGGED_TPM_PROPERTY* target,
                                          BYTE** buffer,
                                          INT32* size);

UINT16 TPMS_ALGORITHM_DESCRIPTION_Marshal(TPMS_ALGORITHM_DESCRIPTION* source,
                                          BYTE** buffer,
                                          INT32* size);

TPM_RC TPMS_ALGORITHM_DESCRIPTION_Unmarshal(TPMS_ALGORITHM_DESCRIPTION* target,
                                            BYTE** buffer,
                                            INT32* size);

UINT16 TPMS_ALGORITHM_DETAIL_ECC_Marshal(TPMS_ALGORITHM_DETAIL_ECC* source,
                                         BYTE** buffer,
                                         INT32* size);

TPM_RC TPMS_ALGORITHM_DETAIL_ECC_Unmarshal(TPMS_ALGORITHM_DETAIL_ECC* target,
                                           BYTE** buffer,
                                           INT32* size);

UINT16 TPMS_ALG_PROPERTY_Marshal(TPMS_ALG_PROPERTY* source,
                                 BYTE** buffer,
                                 INT32* size);

TPM_RC TPMS_ALG_PROPERTY_Unmarshal(TPMS_ALG_PROPERTY* target,
                                   BYTE** buffer,
                                   INT32* size);

UINT16 TPMS_ASYM_PARMS_Marshal(TPMS_ASYM_PARMS* source,
                               BYTE** buffer,
                               INT32* size);

TPM_RC TPMS_ASYM_PARMS_Unmarshal(TPMS_ASYM_PARMS* target,
                                 BYTE** buffer,
                                 INT32* size);

UINT16 TPMS_ATTEST_Marshal(TPMS_ATTEST* source, BYTE** buffer, INT32* size);

TPM_RC TPMS_ATTEST_Unmarshal(TPMS_ATTEST* target, BYTE** buffer, INT32* size);

UINT16 TPMS_AUTH_COMMAND_Marshal(TPMS_AUTH_COMMAND* source,
                                 BYTE** buffer,
                                 INT32* size);

TPM_RC TPMS_AUTH_COMMAND_Unmarshal(TPMS_AUTH_COMMAND* target,
                                   BYTE** buffer,
                                   INT32* size);

UINT16 TPMS_AUTH_RESPONSE_Marshal(TPMS_AUTH_RESPONSE* source,
                                  BYTE** buffer,
                                  INT32* size);

TPM_RC TPMS_AUTH_RESPONSE_Unmarshal(TPMS_AUTH_RESPONSE* target,
                                    BYTE** buffer,
                                    INT32* size);

UINT16 TPMS_CAPABILITY_DATA_Marshal(TPMS_CAPABILITY_DATA* source,
                                    BYTE** buffer,
                                    INT32* size);

TPM_RC TPMS_CAPABILITY_DATA_Unmarshal(TPMS_CAPABILITY_DATA* target,
                                      BYTE** buffer,
                                      INT32* size);

UINT16 TPMS_CERTIFY_INFO_Marshal(TPMS_CERTIFY_INFO* source,
                                 BYTE** buffer,
                                 INT32* size);

TPM_RC TPMS_CERTIFY_INFO_Unmarshal(TPMS_CERTIFY_INFO* target,
                                   BYTE** buffer,
                                   INT32* size);

UINT16 TPMS_CLOCK_INFO_Marshal(TPMS_CLOCK_INFO* source,
                               BYTE** buffer,
                               INT32* size);

TPM_RC TPMS_CLOCK_INFO_Unmarshal(TPMS_CLOCK_INFO* target,
                                 BYTE** buffer,
                                 INT32* size);

UINT16 TPMS_COMMAND_AUDIT_INFO_Marshal(TPMS_COMMAND_AUDIT_INFO* source,
                                       BYTE** buffer,
                                       INT32* size);

TPM_RC TPMS_COMMAND_AUDIT_INFO_Unmarshal(TPMS_COMMAND_AUDIT_INFO* target,
                                         BYTE** buffer,
                                         INT32* size);

UINT16 TPMS_CONTEXT_Marshal(TPMS_CONTEXT* source, BYTE** buffer, INT32* size);

TPM_RC TPMS_CONTEXT_Unmarshal(TPMS_CONTEXT* target, BYTE** buffer, INT32* size);

UINT16 TPMS_CONTEXT_DATA_Marshal(TPMS_CONTEXT_DATA* source,
                                 BYTE** buffer,
                                 INT32* size);

TPM_RC TPMS_CONTEXT_DATA_Unmarshal(TPMS_CONTEXT_DATA* target,
                                   BYTE** buffer,
                                   INT32* size);

UINT16 TPMS_CREATION_DATA_Marshal(TPMS_CREATION_DATA* source,
                                  BYTE** buffer,
                                  INT32* size);

TPM_RC TPMS_CREATION_DATA_Unmarshal(TPMS_CREATION_DATA* target,
                                    BYTE** buffer,
                                    INT32* size);

UINT16 TPMS_CREATION_INFO_Marshal(TPMS_CREATION_INFO* source,
                                  BYTE** buffer,
                                  INT32* size);

TPM_RC TPMS_CREATION_INFO_Unmarshal(TPMS_CREATION_INFO* target,
                                    BYTE** buffer,
                                    INT32* size);

UINT16 TPMS_ECC_PARMS_Marshal(TPMS_ECC_PARMS* source,
                              BYTE** buffer,
                              INT32* size);

TPM_RC TPMS_ECC_PARMS_Unmarshal(TPMS_ECC_PARMS* target,
                                BYTE** buffer,
                                INT32* size);

UINT16 TPMS_ECC_POINT_Marshal(TPMS_ECC_POINT* source,
                              BYTE** buffer,
                              INT32* size);

TPM_RC TPMS_ECC_POINT_Unmarshal(TPMS_ECC_POINT* target,
                                BYTE** buffer,
                                INT32* size);

UINT16 TPMS_EMPTY_Marshal(TPMS_EMPTY* source, BYTE** buffer, INT32* size);

TPM_RC TPMS_EMPTY_Unmarshal(TPMS_EMPTY* target, BYTE** buffer, INT32* size);

UINT16 TPMS_SCHEME_HASH_Marshal(TPMS_SCHEME_HASH* source,
                                BYTE** buffer,
                                INT32* size);

TPM_RC TPMS_SCHEME_HASH_Unmarshal(TPMS_SCHEME_HASH* target,
                                  BYTE** buffer,
                                  INT32* size);

UINT16 TPMS_ENC_SCHEME_OAEP_Marshal(TPMS_ENC_SCHEME_OAEP* source,
                                    BYTE** buffer,
                                    INT32* size);

TPM_RC TPMS_ENC_SCHEME_OAEP_Unmarshal(TPMS_ENC_SCHEME_OAEP* target,
                                      BYTE** buffer,
                                      INT32* size);

UINT16 TPMS_ENC_SCHEME_RSAES_Marshal(TPMS_ENC_SCHEME_RSAES* source,
                                     BYTE** buffer,
                                     INT32* size);

TPM_RC TPMS_ENC_SCHEME_RSAES_Unmarshal(TPMS_ENC_SCHEME_RSAES* target,
                                       BYTE** buffer,
                                       INT32* size);

UINT16 TPMS_KEYEDHASH_PARMS_Marshal(TPMS_KEYEDHASH_PARMS* source,
                                    BYTE** buffer,
                                    INT32* size);

TPM_RC TPMS_KEYEDHASH_PARMS_Unmarshal(TPMS_KEYEDHASH_PARMS* target,
                                      BYTE** buffer,
                                      INT32* size);

UINT16 TPMS_KEY_SCHEME_ECDH_Marshal(TPMS_KEY_SCHEME_ECDH* source,
                                    BYTE** buffer,
                                    INT32* size);

TPM_RC TPMS_KEY_SCHEME_ECDH_Unmarshal(TPMS_KEY_SCHEME_ECDH* target,
                                      BYTE** buffer,
                                      INT32* size);

UINT16 TPMS_KEY_SCHEME_ECMQV_Marshal(TPMS_KEY_SCHEME_ECMQV* source,
                                     BYTE** buffer,
                                     INT32* size);

TPM_RC TPMS_KEY_SCHEME_ECMQV_Unmarshal(TPMS_KEY_SCHEME_ECMQV* target,
                                       BYTE** buffer,
                                       INT32* size);

UINT16 TPMS_NV_CERTIFY_INFO_Marshal(TPMS_NV_CERTIFY_INFO* source,
                                    BYTE** buffer,
                                    INT32* size);

TPM_RC TPMS_NV_CERTIFY_INFO_Unmarshal(TPMS_NV_CERTIFY_INFO* target,
                                      BYTE** buffer,
                                      INT32* size);

UINT16 TPMS_NV_PUBLIC_Marshal(TPMS_NV_PUBLIC* source,
                              BYTE** buffer,
                              INT32* size);

TPM_RC TPMS_NV_PUBLIC_Unmarshal(TPMS_NV_PUBLIC* target,
                                BYTE** buffer,
                                INT32* size);

UINT16 TPMS_PCR_SELECT_Marshal(TPMS_PCR_SELECT* source,
                               BYTE** buffer,
                               INT32* size);

TPM_RC TPMS_PCR_SELECT_Unmarshal(TPMS_PCR_SELECT* target,
                                 BYTE** buffer,
                                 INT32* size);

UINT16 TPMS_PCR_SELECTION_Marshal(TPMS_PCR_SELECTION* source,
                                  BYTE** buffer,
                                  INT32* size);

TPM_RC TPMS_PCR_SELECTION_Unmarshal(TPMS_PCR_SELECTION* target,
                                    BYTE** buffer,
                                    INT32* size);

UINT16 TPMS_QUOTE_INFO_Marshal(TPMS_QUOTE_INFO* source,
                               BYTE** buffer,
                               INT32* size);

TPM_RC TPMS_QUOTE_INFO_Unmarshal(TPMS_QUOTE_INFO* target,
                                 BYTE** buffer,
                                 INT32* size);

UINT16 TPMS_RSA_PARMS_Marshal(TPMS_RSA_PARMS* source,
                              BYTE** buffer,
                              INT32* size);

TPM_RC TPMS_RSA_PARMS_Unmarshal(TPMS_RSA_PARMS* target,
                                BYTE** buffer,
                                INT32* size);

UINT16 TPMS_SCHEME_ECDAA_Marshal(TPMS_SCHEME_ECDAA* source,
                                 BYTE** buffer,
                                 INT32* size);

TPM_RC TPMS_SCHEME_ECDAA_Unmarshal(TPMS_SCHEME_ECDAA* target,
                                   BYTE** buffer,
                                   INT32* size);

UINT16 TPMS_SCHEME_HMAC_Marshal(TPMS_SCHEME_HMAC* source,
                                BYTE** buffer,
                                INT32* size);

TPM_RC TPMS_SCHEME_HMAC_Unmarshal(TPMS_SCHEME_HMAC* target,
                                  BYTE** buffer,
                                  INT32* size);

UINT16 TPMS_SCHEME_KDF1_SP800_108_Marshal(TPMS_SCHEME_KDF1_SP800_108* source,
                                          BYTE** buffer,
                                          INT32* size);

TPM_RC TPMS_SCHEME_KDF1_SP800_108_Unmarshal(TPMS_SCHEME_KDF1_SP800_108* target,
                                            BYTE** buffer,
                                            INT32* size);

UINT16 TPMS_SCHEME_KDF1_SP800_56A_Marshal(TPMS_SCHEME_KDF1_SP800_56A* source,
                                          BYTE** buffer,
                                          INT32* size);

TPM_RC TPMS_SCHEME_KDF1_SP800_56A_Unmarshal(TPMS_SCHEME_KDF1_SP800_56A* target,
                                            BYTE** buffer,
                                            INT32* size);

UINT16 TPMS_SCHEME_KDF2_Marshal(TPMS_SCHEME_KDF2* source,
                                BYTE** buffer,
                                INT32* size);

TPM_RC TPMS_SCHEME_KDF2_Unmarshal(TPMS_SCHEME_KDF2* target,
                                  BYTE** buffer,
                                  INT32* size);

UINT16 TPMS_SCHEME_MGF1_Marshal(TPMS_SCHEME_MGF1* source,
                                BYTE** buffer,
                                INT32* size);

TPM_RC TPMS_SCHEME_MGF1_Unmarshal(TPMS_SCHEME_MGF1* target,
                                  BYTE** buffer,
                                  INT32* size);

UINT16 TPMS_SCHEME_XOR_Marshal(TPMS_SCHEME_XOR* source,
                               BYTE** buffer,
                               INT32* size);

TPM_RC TPMS_SCHEME_XOR_Unmarshal(TPMS_SCHEME_XOR* target,
                                 BYTE** buffer,
                                 INT32* size);

UINT16 TPMS_SENSITIVE_CREATE_Marshal(TPMS_SENSITIVE_CREATE* source,
                                     BYTE** buffer,
                                     INT32* size);

TPM_RC TPMS_SENSITIVE_CREATE_Unmarshal(TPMS_SENSITIVE_CREATE* target,
                                       BYTE** buffer,
                                       INT32* size);

UINT16 TPMS_SESSION_AUDIT_INFO_Marshal(TPMS_SESSION_AUDIT_INFO* source,
                                       BYTE** buffer,
                                       INT32* size);

TPM_RC TPMS_SESSION_AUDIT_INFO_Unmarshal(TPMS_SESSION_AUDIT_INFO* target,
                                         BYTE** buffer,
                                         INT32* size);

UINT16 TPMS_SIGNATURE_ECC_Marshal(TPMS_SIGNATURE_ECC* source,
                                  BYTE** buffer,
                                  INT32* size);

TPM_RC TPMS_SIGNATURE_ECC_Unmarshal(TPMS_SIGNATURE_ECC* target,
                                    BYTE** buffer,
                                    INT32* size);

UINT16 TPMS_SIGNATURE_ECDAA_Marshal(TPMS_SIGNATURE_ECDAA* source,
                                    BYTE** buffer,
                                    INT32* size);

TPM_RC TPMS_SIGNATURE_ECDAA_Unmarshal(TPMS_SIGNATURE_ECDAA* target,
                                      BYTE** buffer,
                                      INT32* size);

UINT16 TPMS_SIGNATURE_ECDSA_Marshal(TPMS_SIGNATURE_ECDSA* source,
                                    BYTE** buffer,
                                    INT32* size);

TPM_RC TPMS_SIGNATURE_ECDSA_Unmarshal(TPMS_SIGNATURE_ECDSA* target,
                                      BYTE** buffer,
                                      INT32* size);

UINT16 TPMS_SIGNATURE_ECSCHNORR_Marshal(TPMS_SIGNATURE_ECSCHNORR* source,
                                        BYTE** buffer,
                                        INT32* size);

TPM_RC TPMS_SIGNATURE_ECSCHNORR_Unmarshal(TPMS_SIGNATURE_ECSCHNORR* target,
                                          BYTE** buffer,
                                          INT32* size);

UINT16 TPMS_SIGNATURE_RSA_Marshal(TPMS_SIGNATURE_RSA* source,
                                  BYTE** buffer,
                                  INT32* size);

TPM_RC TPMS_SIGNATURE_RSA_Unmarshal(TPMS_SIGNATURE_RSA* target,
                                    BYTE** buffer,
                                    INT32* size);

UINT16 TPMS_SIGNATURE_RSAPSS_Marshal(TPMS_SIGNATURE_RSAPSS* source,
                                     BYTE** buffer,
                                     INT32* size);

TPM_RC TPMS_SIGNATURE_RSAPSS_Unmarshal(TPMS_SIGNATURE_RSAPSS* target,
                                       BYTE** buffer,
                                       INT32* size);

UINT16 TPMS_SIGNATURE_RSASSA_Marshal(TPMS_SIGNATURE_RSASSA* source,
                                     BYTE** buffer,
                                     INT32* size);

TPM_RC TPMS_SIGNATURE_RSASSA_Unmarshal(TPMS_SIGNATURE_RSASSA* target,
                                       BYTE** buffer,
                                       INT32* size);

UINT16 TPMS_SIGNATURE_SM2_Marshal(TPMS_SIGNATURE_SM2* source,
                                  BYTE** buffer,
                                  INT32* size);

TPM_RC TPMS_SIGNATURE_SM2_Unmarshal(TPMS_SIGNATURE_SM2* target,
                                    BYTE** buffer,
                                    INT32* size);

UINT16 TPMS_SIG_SCHEME_ECDAA_Marshal(TPMS_SIG_SCHEME_ECDAA* source,
                                     BYTE** buffer,
                                     INT32* size);

TPM_RC TPMS_SIG_SCHEME_ECDAA_Unmarshal(TPMS_SIG_SCHEME_ECDAA* target,
                                       BYTE** buffer,
                                       INT32* size);

UINT16 TPMS_SIG_SCHEME_ECDSA_Marshal(TPMS_SIG_SCHEME_ECDSA* source,
                                     BYTE** buffer,
                                     INT32* size);

TPM_RC TPMS_SIG_SCHEME_ECDSA_Unmarshal(TPMS_SIG_SCHEME_ECDSA* target,
                                       BYTE** buffer,
                                       INT32* size);

UINT16 TPMS_SIG_SCHEME_ECSCHNORR_Marshal(TPMS_SIG_SCHEME_ECSCHNORR* source,
                                         BYTE** buffer,
                                         INT32* size);

TPM_RC TPMS_SIG_SCHEME_ECSCHNORR_Unmarshal(TPMS_SIG_SCHEME_ECSCHNORR* target,
                                           BYTE** buffer,
                                           INT32* size);

UINT16 TPMS_SIG_SCHEME_RSAPSS_Marshal(TPMS_SIG_SCHEME_RSAPSS* source,
                                      BYTE** buffer,
                                      INT32* size);

TPM_RC TPMS_SIG_SCHEME_RSAPSS_Unmarshal(TPMS_SIG_SCHEME_RSAPSS* target,
                                        BYTE** buffer,
                                        INT32* size);

UINT16 TPMS_SIG_SCHEME_RSASSA_Marshal(TPMS_SIG_SCHEME_RSASSA* source,
                                      BYTE** buffer,
                                      INT32* size);

TPM_RC TPMS_SIG_SCHEME_RSASSA_Unmarshal(TPMS_SIG_SCHEME_RSASSA* target,
                                        BYTE** buffer,
                                        INT32* size);

UINT16 TPMS_SIG_SCHEME_SM2_Marshal(TPMS_SIG_SCHEME_SM2* source,
                                   BYTE** buffer,
                                   INT32* size);

TPM_RC TPMS_SIG_SCHEME_SM2_Unmarshal(TPMS_SIG_SCHEME_SM2* target,
                                     BYTE** buffer,
                                     INT32* size);

UINT16 TPMS_SYMCIPHER_PARMS_Marshal(TPMS_SYMCIPHER_PARMS* source,
                                    BYTE** buffer,
                                    INT32* size);

TPM_RC TPMS_SYMCIPHER_PARMS_Unmarshal(TPMS_SYMCIPHER_PARMS* target,
                                      BYTE** buffer,
                                      INT32* size);

UINT16 TPMS_TAGGED_PCR_SELECT_Marshal(TPMS_TAGGED_PCR_SELECT* source,
                                      BYTE** buffer,
                                      INT32* size);

TPM_RC TPMS_TAGGED_PCR_SELECT_Unmarshal(TPMS_TAGGED_PCR_SELECT* target,
                                        BYTE** buffer,
                                        INT32* size);

UINT16 TPMS_TAGGED_PROPERTY_Marshal(TPMS_TAGGED_PROPERTY* source,
                                    BYTE** buffer,
                                    INT32* size);

TPM_RC TPMS_TAGGED_PROPERTY_Unmarshal(TPMS_TAGGED_PROPERTY* target,
                                      BYTE** buffer,
                                      INT32* size);

UINT16 TPMS_TIME_ATTEST_INFO_Marshal(TPMS_TIME_ATTEST_INFO* source,
                                     BYTE** buffer,
                                     INT32* size);

TPM_RC TPMS_TIME_ATTEST_INFO_Unmarshal(TPMS_TIME_ATTEST_INFO* target,
                                       BYTE** buffer,
                                       INT32* size);

UINT16 TPMS_TIME_INFO_Marshal(TPMS_TIME_INFO* source,
                              BYTE** buffer,
                              INT32* size);

TPM_RC TPMS_TIME_INFO_Unmarshal(TPMS_TIME_INFO* target,
                                BYTE** buffer,
                                INT32* size);

UINT16 TPMT_ASYM_SCHEME_Marshal(TPMT_ASYM_SCHEME* source,
                                BYTE** buffer,
                                INT32* size);

TPM_RC TPMT_ASYM_SCHEME_Unmarshal(TPMT_ASYM_SCHEME* target,
                                  BYTE** buffer,
                                  INT32* size);

UINT16 TPMT_ECC_SCHEME_Marshal(TPMT_ECC_SCHEME* source,
                               BYTE** buffer,
                               INT32* size);

TPM_RC TPMT_ECC_SCHEME_Unmarshal(TPMT_ECC_SCHEME* target,
                                 BYTE** buffer,
                                 INT32* size);

UINT16 TPMT_HA_Marshal(TPMT_HA* source, BYTE** buffer, INT32* size);

TPM_RC TPMT_HA_Unmarshal(TPMT_HA* target, BYTE** buffer, INT32* size);

UINT16 TPMT_KDF_SCHEME_Marshal(TPMT_KDF_SCHEME* source,
                               BYTE** buffer,
                               INT32* size);

TPM_RC TPMT_KDF_SCHEME_Unmarshal(TPMT_KDF_SCHEME* target,
                                 BYTE** buffer,
                                 INT32* size);

UINT16 TPMT_KEYEDHASH_SCHEME_Marshal(TPMT_KEYEDHASH_SCHEME* source,
                                     BYTE** buffer,
                                     INT32* size);

TPM_RC TPMT_KEYEDHASH_SCHEME_Unmarshal(TPMT_KEYEDHASH_SCHEME* target,
                                       BYTE** buffer,
                                       INT32* size);

UINT16 TPMT_PUBLIC_Marshal(TPMT_PUBLIC* source, BYTE** buffer, INT32* size);

TPM_RC TPMT_PUBLIC_Unmarshal(TPMT_PUBLIC* target, BYTE** buffer, INT32* size);

UINT16 TPMT_PUBLIC_PARMS_Marshal(TPMT_PUBLIC_PARMS* source,
                                 BYTE** buffer,
                                 INT32* size);

TPM_RC TPMT_PUBLIC_PARMS_Unmarshal(TPMT_PUBLIC_PARMS* target,
                                   BYTE** buffer,
                                   INT32* size);

UINT16 TPMT_RSA_DECRYPT_Marshal(TPMT_RSA_DECRYPT* source,
                                BYTE** buffer,
                                INT32* size);

TPM_RC TPMT_RSA_DECRYPT_Unmarshal(TPMT_RSA_DECRYPT* target,
                                  BYTE** buffer,
                                  INT32* size);

UINT16 TPMT_RSA_SCHEME_Marshal(TPMT_RSA_SCHEME* source,
                               BYTE** buffer,
                               INT32* size);

TPM_RC TPMT_RSA_SCHEME_Unmarshal(TPMT_RSA_SCHEME* target,
                                 BYTE** buffer,
                                 INT32* size);

UINT16 TPMT_SENSITIVE_Marshal(TPMT_SENSITIVE* source,
                              BYTE** buffer,
                              INT32* size);

TPM_RC TPMT_SENSITIVE_Unmarshal(TPMT_SENSITIVE* target,
                                BYTE** buffer,
                                INT32* size);

UINT16 TPMT_SIGNATURE_Marshal(TPMT_SIGNATURE* source,
                              BYTE** buffer,
                              INT32* size);

TPM_RC TPMT_SIGNATURE_Unmarshal(TPMT_SIGNATURE* target,
                                BYTE** buffer,
                                INT32* size);

UINT16 TPMT_SIG_SCHEME_Marshal(TPMT_SIG_SCHEME* source,
                               BYTE** buffer,
                               INT32* size);

TPM_RC TPMT_SIG_SCHEME_Unmarshal(TPMT_SIG_SCHEME* target,
                                 BYTE** buffer,
                                 INT32* size);

UINT16 TPMT_SYM_DEF_Marshal(TPMT_SYM_DEF* source, BYTE** buffer, INT32* size);

TPM_RC TPMT_SYM_DEF_Unmarshal(TPMT_SYM_DEF* target, BYTE** buffer, INT32* size);

UINT16 TPMT_SYM_DEF_OBJECT_Marshal(TPMT_SYM_DEF_OBJECT* source,
                                   BYTE** buffer,
                                   INT32* size);

TPM_RC TPMT_SYM_DEF_OBJECT_Unmarshal(TPMT_SYM_DEF_OBJECT* target,
                                     BYTE** buffer,
                                     INT32* size);

UINT16 TPMT_TK_AUTH_Marshal(TPMT_TK_AUTH* source, BYTE** buffer, INT32* size);

TPM_RC TPMT_TK_AUTH_Unmarshal(TPMT_TK_AUTH* target, BYTE** buffer, INT32* size);

UINT16 TPMT_TK_CREATION_Marshal(TPMT_TK_CREATION* source,
                                BYTE** buffer,
                                INT32* size);

TPM_RC TPMT_TK_CREATION_Unmarshal(TPMT_TK_CREATION* target,
                                  BYTE** buffer,
                                  INT32* size);

UINT16 TPMT_TK_HASHCHECK_Marshal(TPMT_TK_HASHCHECK* source,
                                 BYTE** buffer,
                                 INT32* size);

TPM_RC TPMT_TK_HASHCHECK_Unmarshal(TPMT_TK_HASHCHECK* target,
                                   BYTE** buffer,
                                   INT32* size);

UINT16 TPMT_TK_VERIFIED_Marshal(TPMT_TK_VERIFIED* source,
                                BYTE** buffer,
                                INT32* size);

TPM_RC TPMT_TK_VERIFIED_Unmarshal(TPMT_TK_VERIFIED* target,
                                  BYTE** buffer,
                                  INT32* size);

UINT16 TPMU_ASYM_SCHEME_Marshal(TPMU_ASYM_SCHEME* source,
                                BYTE** buffer,
                                INT32* size,
                                UINT32 selector);

TPM_RC TPMU_ASYM_SCHEME_Unmarshal(TPMU_ASYM_SCHEME* target,
                                  BYTE** buffer,
                                  INT32* size,
                                  UINT32 selector);

UINT16 TPMU_ATTEST_Marshal(TPMU_ATTEST* source,
                           BYTE** buffer,
                           INT32* size,
                           UINT32 selector);

TPM_RC TPMU_ATTEST_Unmarshal(TPMU_ATTEST* target,
                             BYTE** buffer,
                             INT32* size,
                             UINT32 selector);

UINT16 TPMU_CAPABILITIES_Marshal(TPMU_CAPABILITIES* source,
                                 BYTE** buffer,
                                 INT32* size,
                                 UINT32 selector);

TPM_RC TPMU_CAPABILITIES_Unmarshal(TPMU_CAPABILITIES* target,
                                   BYTE** buffer,
                                   INT32* size,
                                   UINT32 selector);

UINT16 TPMU_HA_Marshal(TPMU_HA* source,
                       BYTE** buffer,
                       INT32* size,
                       UINT32 selector);

TPM_RC TPMU_HA_Unmarshal(TPMU_HA* target,
                         BYTE** buffer,
                         INT32* size,
                         UINT32 selector);

UINT16 TPMU_KDF_SCHEME_Marshal(TPMU_KDF_SCHEME* source,
                               BYTE** buffer,
                               INT32* size,
                               UINT32 selector);

TPM_RC TPMU_KDF_SCHEME_Unmarshal(TPMU_KDF_SCHEME* target,
                                 BYTE** buffer,
                                 INT32* size,
                                 UINT32 selector);

UINT16 TPMU_PUBLIC_ID_Marshal(TPMU_PUBLIC_ID* source,
                              BYTE** buffer,
                              INT32* size,
                              UINT32 selector);

TPM_RC TPMU_PUBLIC_ID_Unmarshal(TPMU_PUBLIC_ID* target,
                                BYTE** buffer,
                                INT32* size,
                                UINT32 selector);

UINT16 TPMU_PUBLIC_PARMS_Marshal(TPMU_PUBLIC_PARMS* source,
                                 BYTE** buffer,
                                 INT32* size,
                                 UINT32 selector);

TPM_RC TPMU_PUBLIC_PARMS_Unmarshal(TPMU_PUBLIC_PARMS* target,
                                   BYTE** buffer,
                                   INT32* size,
                                   UINT32 selector);

UINT16 TPMU_SCHEME_KEYEDHASH_Marshal(TPMU_SCHEME_KEYEDHASH* source,
                                     BYTE** buffer,
                                     INT32* size,
                                     UINT32 selector);

TPM_RC TPMU_SCHEME_KEYEDHASH_Unmarshal(TPMU_SCHEME_KEYEDHASH* target,
                                       BYTE** buffer,
                                       INT32* size,
                                       UINT32 selector);

UINT16 TPMU_SENSITIVE_COMPOSITE_Marshal(TPMU_SENSITIVE_COMPOSITE* source,
                                        BYTE** buffer,
                                        INT32* size,
                                        UINT32 selector);

TPM_RC TPMU_SENSITIVE_COMPOSITE_Unmarshal(TPMU_SENSITIVE_COMPOSITE* target,
                                          BYTE** buffer,
                                          INT32* size,
                                          UINT32 selector);

UINT16 TPMU_SIGNATURE_Marshal(TPMU_SIGNATURE* source,
                              BYTE** buffer,
                              INT32* size,
                              UINT32 selector);

TPM_RC TPMU_SIGNATURE_Unmarshal(TPMU_SIGNATURE* target,
                                BYTE** buffer,
                                INT32* size,
                                UINT32 selector);

UINT16 TPMU_SIG_SCHEME_Marshal(TPMU_SIG_SCHEME* source,
                               BYTE** buffer,
                               INT32* size,
                               UINT32 selector);

TPM_RC TPMU_SIG_SCHEME_Unmarshal(TPMU_SIG_SCHEME* target,
                                 BYTE** buffer,
                                 INT32* size,
                                 UINT32 selector);

UINT16 TPMU_SYM_DETAILS_Marshal(TPMU_SYM_DETAILS* source,
                                BYTE** buffer,
                                INT32* size,
                                UINT32 selector);

TPM_RC TPMU_SYM_DETAILS_Unmarshal(TPMU_SYM_DETAILS* target,
                                  BYTE** buffer,
                                  INT32* size,
                                  UINT32 selector);

UINT16 TPMU_SYM_KEY_BITS_Marshal(TPMU_SYM_KEY_BITS* source,
                                 BYTE** buffer,
                                 INT32* size,
                                 UINT32 selector);

TPM_RC TPMU_SYM_KEY_BITS_Unmarshal(TPMU_SYM_KEY_BITS* target,
                                   BYTE** buffer,
                                   INT32* size,
                                   UINT32 selector);

UINT16 TPMU_SYM_MODE_Marshal(TPMU_SYM_MODE* source,
                             BYTE** buffer,
                             INT32* size,
                             UINT32 selector);

TPM_RC TPMU_SYM_MODE_Unmarshal(TPMU_SYM_MODE* target,
                               BYTE** buffer,
                               INT32* size,
                               UINT32 selector);

UINT16 TPM_ALGORITHM_ID_Marshal(TPM_ALGORITHM_ID* source,
                                BYTE** buffer,
                                INT32* size);

TPM_RC TPM_ALGORITHM_ID_Unmarshal(TPM_ALGORITHM_ID* target,
                                  BYTE** buffer,
                                  INT32* size);

UINT16 TPM_AUTHORIZATION_SIZE_Marshal(TPM_AUTHORIZATION_SIZE* source,
                                      BYTE** buffer,
                                      INT32* size);

TPM_RC TPM_AUTHORIZATION_SIZE_Unmarshal(TPM_AUTHORIZATION_SIZE* target,
                                        BYTE** buffer,
                                        INT32* size);

UINT16 TPM_CAP_Marshal(TPM_CAP* source, BYTE** buffer, INT32* size);

TPM_RC TPM_CAP_Unmarshal(TPM_CAP* target, BYTE** buffer, INT32* size);

UINT16 TPM_CLOCK_ADJUST_Marshal(TPM_CLOCK_ADJUST* source,
                                BYTE** buffer,
                                INT32* size);

TPM_RC TPM_CLOCK_ADJUST_Unmarshal(TPM_CLOCK_ADJUST* target,
                                  BYTE** buffer,
                                  INT32* size);

UINT16 TPM_EO_Marshal(TPM_EO* source, BYTE** buffer, INT32* size);

TPM_RC TPM_EO_Unmarshal(TPM_EO* target, BYTE** buffer, INT32* size);

UINT16 TPM_GENERATED_Marshal(TPM_GENERATED* source, BYTE** buffer, INT32* size);

TPM_RC TPM_GENERATED_Unmarshal(TPM_GENERATED* target,
                               BYTE** buffer,
                               INT32* size);

UINT16 TPM_HC_Marshal(TPM_HC* source, BYTE** buffer, INT32* size);

TPM_RC TPM_HC_Unmarshal(TPM_HC* target, BYTE** buffer, INT32* size);

UINT16 TPM_HT_Marshal(TPM_HT* source, BYTE** buffer, INT32* size);

TPM_RC TPM_HT_Unmarshal(TPM_HT* target, BYTE** buffer, INT32* size);

UINT16 TPM_KEY_SIZE_Marshal(TPM_KEY_SIZE* source, BYTE** buffer, INT32* size);

TPM_RC TPM_KEY_SIZE_Unmarshal(TPM_KEY_SIZE* target, BYTE** buffer, INT32* size);

UINT16 TPM_MODIFIER_INDICATOR_Marshal(TPM_MODIFIER_INDICATOR* source,
                                      BYTE** buffer,
                                      INT32* size);

TPM_RC TPM_MODIFIER_INDICATOR_Unmarshal(TPM_MODIFIER_INDICATOR* target,
                                        BYTE** buffer,
                                        INT32* size);

UINT16 TPM_NV_INDEX_Marshal(TPM_NV_INDEX* source, BYTE** buffer, INT32* size);

TPM_RC TPM_NV_INDEX_Unmarshal(TPM_NV_INDEX* target, BYTE** buffer, INT32* size);

UINT16 TPM_PARAMETER_SIZE_Marshal(TPM_PARAMETER_SIZE* source,
                                  BYTE** buffer,
                                  INT32* size);

TPM_RC TPM_PARAMETER_SIZE_Unmarshal(TPM_PARAMETER_SIZE* target,
                                    BYTE** buffer,
                                    INT32* size);

UINT16 TPM_PS_Marshal(TPM_PS* source, BYTE** buffer, INT32* size);

TPM_RC TPM_PS_Unmarshal(TPM_PS* target, BYTE** buffer, INT32* size);

UINT16 TPM_PT_Marshal(TPM_PT* source, BYTE** buffer, INT32* size);

TPM_RC TPM_PT_Unmarshal(TPM_PT* target, BYTE** buffer, INT32* size);

UINT16 TPM_PT_PCR_Marshal(TPM_PT_PCR* source, BYTE** buffer, INT32* size);

TPM_RC TPM_PT_PCR_Unmarshal(TPM_PT_PCR* target, BYTE** buffer, INT32* size);

UINT16 TPM_RC_Marshal(TPM_RC* source, BYTE** buffer, INT32* size);

TPM_RC TPM_RC_Unmarshal(TPM_RC* target, BYTE** buffer, INT32* size);

UINT16 TPM_RH_Marshal(TPM_RH* source, BYTE** buffer, INT32* size);

TPM_RC TPM_RH_Unmarshal(TPM_RH* target, BYTE** buffer, INT32* size);

UINT16 TPM_SE_Marshal(TPM_SE* source, BYTE** buffer, INT32* size);

TPM_RC TPM_SE_Unmarshal(TPM_SE* target, BYTE** buffer, INT32* size);

UINT16 TPM_SPEC_Marshal(TPM_SPEC* source, BYTE** buffer, INT32* size);

TPM_RC TPM_SPEC_Unmarshal(TPM_SPEC* target, BYTE** buffer, INT32* size);

UINT16 TPM_SU_Marshal(TPM_SU* source, BYTE** buffer, INT32* size);

TPM_RC TPM_SU_Unmarshal(TPM_SU* target, BYTE** buffer, INT32* size);

UINT16 UINT64_Marshal(UINT64* source, BYTE** buffer, INT32* size);

TPM_RC UINT64_Unmarshal(UINT64* target, BYTE** buffer, INT32* size);

UINT16 _ID_OBJECT_Marshal(_ID_OBJECT* source, BYTE** buffer, INT32* size);

TPM_RC _ID_OBJECT_Unmarshal(_ID_OBJECT* target, BYTE** buffer, INT32* size);

UINT16 _PRIVATE_Marshal(_PRIVATE* source, BYTE** buffer, INT32* size);

TPM_RC _PRIVATE_Unmarshal(_PRIVATE* target, BYTE** buffer, INT32* size);

#endif  // TPM2_TPM_GENERATED_H_
