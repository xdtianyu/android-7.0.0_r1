// Copyright 2015 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// THIS CODE IS GENERATED - DO NOT MODIFY!

#include "tpm_generated.h"

UINT16 uint8_t_Marshal(uint8_t* source, BYTE** buffer, INT32* size) {
  uint8_t value_net = *source;
  if (!size || *size < sizeof(uint8_t)) {
    return 0;  // Nothing has been marshaled.
  }
  switch (sizeof(uint8_t)) {
    case 2:
      value_net = htobe16(*source);
      break;
    case 4:
      value_net = htobe32(*source);
      break;
    case 8:
      value_net = htobe64(*source);
      break;
    default:
      break;
  }
  memcpy(*buffer, &value_net, sizeof(uint8_t));
  *buffer += sizeof(uint8_t);
  *size -= sizeof(uint8_t);
  return sizeof(uint8_t);
}

TPM_RC uint8_t_Unmarshal(uint8_t* target, BYTE** buffer, INT32* size) {
  uint8_t value_net = 0;
  if (!size || *size < sizeof(uint8_t)) {
    return TPM_RC_INSUFFICIENT;
  }
  memcpy(&value_net, *buffer, sizeof(uint8_t));
  switch (sizeof(uint8_t)) {
    case 2:
      *target = be16toh(value_net);
      break;
    case 4:
      *target = be32toh(value_net);
      break;
    case 8:
      *target = be64toh(value_net);
      break;
    default:
      *target = value_net;
  }
  *buffer += sizeof(uint8_t);
  *size -= sizeof(uint8_t);
  return TPM_RC_SUCCESS;
}

UINT16 int8_t_Marshal(int8_t* source, BYTE** buffer, INT32* size) {
  int8_t value_net = *source;
  if (!size || *size < sizeof(int8_t)) {
    return 0;  // Nothing has been marshaled.
  }
  switch (sizeof(int8_t)) {
    case 2:
      value_net = htobe16(*source);
      break;
    case 4:
      value_net = htobe32(*source);
      break;
    case 8:
      value_net = htobe64(*source);
      break;
    default:
      break;
  }
  memcpy(*buffer, &value_net, sizeof(int8_t));
  *buffer += sizeof(int8_t);
  *size -= sizeof(int8_t);
  return sizeof(int8_t);
}

TPM_RC int8_t_Unmarshal(int8_t* target, BYTE** buffer, INT32* size) {
  int8_t value_net = 0;
  if (!size || *size < sizeof(int8_t)) {
    return TPM_RC_INSUFFICIENT;
  }
  memcpy(&value_net, *buffer, sizeof(int8_t));
  switch (sizeof(int8_t)) {
    case 2:
      *target = be16toh(value_net);
      break;
    case 4:
      *target = be32toh(value_net);
      break;
    case 8:
      *target = be64toh(value_net);
      break;
    default:
      *target = value_net;
  }
  *buffer += sizeof(int8_t);
  *size -= sizeof(int8_t);
  return TPM_RC_SUCCESS;
}

UINT16 uint16_t_Marshal(uint16_t* source, BYTE** buffer, INT32* size) {
  uint16_t value_net = *source;
  if (!size || *size < sizeof(uint16_t)) {
    return 0;  // Nothing has been marshaled.
  }
  switch (sizeof(uint16_t)) {
    case 2:
      value_net = htobe16(*source);
      break;
    case 4:
      value_net = htobe32(*source);
      break;
    case 8:
      value_net = htobe64(*source);
      break;
    default:
      break;
  }
  memcpy(*buffer, &value_net, sizeof(uint16_t));
  *buffer += sizeof(uint16_t);
  *size -= sizeof(uint16_t);
  return sizeof(uint16_t);
}

TPM_RC uint16_t_Unmarshal(uint16_t* target, BYTE** buffer, INT32* size) {
  uint16_t value_net = 0;
  if (!size || *size < sizeof(uint16_t)) {
    return TPM_RC_INSUFFICIENT;
  }
  memcpy(&value_net, *buffer, sizeof(uint16_t));
  switch (sizeof(uint16_t)) {
    case 2:
      *target = be16toh(value_net);
      break;
    case 4:
      *target = be32toh(value_net);
      break;
    case 8:
      *target = be64toh(value_net);
      break;
    default:
      *target = value_net;
  }
  *buffer += sizeof(uint16_t);
  *size -= sizeof(uint16_t);
  return TPM_RC_SUCCESS;
}

UINT16 int16_t_Marshal(int16_t* source, BYTE** buffer, INT32* size) {
  int16_t value_net = *source;
  if (!size || *size < sizeof(int16_t)) {
    return 0;  // Nothing has been marshaled.
  }
  switch (sizeof(int16_t)) {
    case 2:
      value_net = htobe16(*source);
      break;
    case 4:
      value_net = htobe32(*source);
      break;
    case 8:
      value_net = htobe64(*source);
      break;
    default:
      break;
  }
  memcpy(*buffer, &value_net, sizeof(int16_t));
  *buffer += sizeof(int16_t);
  *size -= sizeof(int16_t);
  return sizeof(int16_t);
}

TPM_RC int16_t_Unmarshal(int16_t* target, BYTE** buffer, INT32* size) {
  int16_t value_net = 0;
  if (!size || *size < sizeof(int16_t)) {
    return TPM_RC_INSUFFICIENT;
  }
  memcpy(&value_net, *buffer, sizeof(int16_t));
  switch (sizeof(int16_t)) {
    case 2:
      *target = be16toh(value_net);
      break;
    case 4:
      *target = be32toh(value_net);
      break;
    case 8:
      *target = be64toh(value_net);
      break;
    default:
      *target = value_net;
  }
  *buffer += sizeof(int16_t);
  *size -= sizeof(int16_t);
  return TPM_RC_SUCCESS;
}

UINT16 uint32_t_Marshal(uint32_t* source, BYTE** buffer, INT32* size) {
  uint32_t value_net = *source;
  if (!size || *size < sizeof(uint32_t)) {
    return 0;  // Nothing has been marshaled.
  }
  switch (sizeof(uint32_t)) {
    case 2:
      value_net = htobe16(*source);
      break;
    case 4:
      value_net = htobe32(*source);
      break;
    case 8:
      value_net = htobe64(*source);
      break;
    default:
      break;
  }
  memcpy(*buffer, &value_net, sizeof(uint32_t));
  *buffer += sizeof(uint32_t);
  *size -= sizeof(uint32_t);
  return sizeof(uint32_t);
}

TPM_RC uint32_t_Unmarshal(uint32_t* target, BYTE** buffer, INT32* size) {
  uint32_t value_net = 0;
  if (!size || *size < sizeof(uint32_t)) {
    return TPM_RC_INSUFFICIENT;
  }
  memcpy(&value_net, *buffer, sizeof(uint32_t));
  switch (sizeof(uint32_t)) {
    case 2:
      *target = be16toh(value_net);
      break;
    case 4:
      *target = be32toh(value_net);
      break;
    case 8:
      *target = be64toh(value_net);
      break;
    default:
      *target = value_net;
  }
  *buffer += sizeof(uint32_t);
  *size -= sizeof(uint32_t);
  return TPM_RC_SUCCESS;
}

UINT16 int32_t_Marshal(int32_t* source, BYTE** buffer, INT32* size) {
  int32_t value_net = *source;
  if (!size || *size < sizeof(int32_t)) {
    return 0;  // Nothing has been marshaled.
  }
  switch (sizeof(int32_t)) {
    case 2:
      value_net = htobe16(*source);
      break;
    case 4:
      value_net = htobe32(*source);
      break;
    case 8:
      value_net = htobe64(*source);
      break;
    default:
      break;
  }
  memcpy(*buffer, &value_net, sizeof(int32_t));
  *buffer += sizeof(int32_t);
  *size -= sizeof(int32_t);
  return sizeof(int32_t);
}

TPM_RC int32_t_Unmarshal(int32_t* target, BYTE** buffer, INT32* size) {
  int32_t value_net = 0;
  if (!size || *size < sizeof(int32_t)) {
    return TPM_RC_INSUFFICIENT;
  }
  memcpy(&value_net, *buffer, sizeof(int32_t));
  switch (sizeof(int32_t)) {
    case 2:
      *target = be16toh(value_net);
      break;
    case 4:
      *target = be32toh(value_net);
      break;
    case 8:
      *target = be64toh(value_net);
      break;
    default:
      *target = value_net;
  }
  *buffer += sizeof(int32_t);
  *size -= sizeof(int32_t);
  return TPM_RC_SUCCESS;
}

UINT16 uint64_t_Marshal(uint64_t* source, BYTE** buffer, INT32* size) {
  uint64_t value_net = *source;
  if (!size || *size < sizeof(uint64_t)) {
    return 0;  // Nothing has been marshaled.
  }
  switch (sizeof(uint64_t)) {
    case 2:
      value_net = htobe16(*source);
      break;
    case 4:
      value_net = htobe32(*source);
      break;
    case 8:
      value_net = htobe64(*source);
      break;
    default:
      break;
  }
  memcpy(*buffer, &value_net, sizeof(uint64_t));
  *buffer += sizeof(uint64_t);
  *size -= sizeof(uint64_t);
  return sizeof(uint64_t);
}

TPM_RC uint64_t_Unmarshal(uint64_t* target, BYTE** buffer, INT32* size) {
  uint64_t value_net = 0;
  if (!size || *size < sizeof(uint64_t)) {
    return TPM_RC_INSUFFICIENT;
  }
  memcpy(&value_net, *buffer, sizeof(uint64_t));
  switch (sizeof(uint64_t)) {
    case 2:
      *target = be16toh(value_net);
      break;
    case 4:
      *target = be32toh(value_net);
      break;
    case 8:
      *target = be64toh(value_net);
      break;
    default:
      *target = value_net;
  }
  *buffer += sizeof(uint64_t);
  *size -= sizeof(uint64_t);
  return TPM_RC_SUCCESS;
}

UINT16 int64_t_Marshal(int64_t* source, BYTE** buffer, INT32* size) {
  int64_t value_net = *source;
  if (!size || *size < sizeof(int64_t)) {
    return 0;  // Nothing has been marshaled.
  }
  switch (sizeof(int64_t)) {
    case 2:
      value_net = htobe16(*source);
      break;
    case 4:
      value_net = htobe32(*source);
      break;
    case 8:
      value_net = htobe64(*source);
      break;
    default:
      break;
  }
  memcpy(*buffer, &value_net, sizeof(int64_t));
  *buffer += sizeof(int64_t);
  *size -= sizeof(int64_t);
  return sizeof(int64_t);
}

TPM_RC int64_t_Unmarshal(int64_t* target, BYTE** buffer, INT32* size) {
  int64_t value_net = 0;
  if (!size || *size < sizeof(int64_t)) {
    return TPM_RC_INSUFFICIENT;
  }
  memcpy(&value_net, *buffer, sizeof(int64_t));
  switch (sizeof(int64_t)) {
    case 2:
      *target = be16toh(value_net);
      break;
    case 4:
      *target = be32toh(value_net);
      break;
    case 8:
      *target = be64toh(value_net);
      break;
    default:
      *target = value_net;
  }
  *buffer += sizeof(int64_t);
  *size -= sizeof(int64_t);
  return TPM_RC_SUCCESS;
}

UINT16 BYTE_Marshal(BYTE* source, BYTE** buffer, INT32* size) {
  return uint8_t_Marshal(source, buffer, size);
}

TPM_RC BYTE_Unmarshal(BYTE* target, BYTE** buffer, INT32* size) {
  return uint8_t_Unmarshal(target, buffer, size);
}

UINT16 INT16_Marshal(INT16* source, BYTE** buffer, INT32* size) {
  return int16_t_Marshal(source, buffer, size);
}

TPM_RC INT16_Unmarshal(INT16* target, BYTE** buffer, INT32* size) {
  return int16_t_Unmarshal(target, buffer, size);
}

UINT16 INT32_Marshal(INT32* source, BYTE** buffer, INT32* size) {
  return int32_t_Marshal(source, buffer, size);
}

TPM_RC INT32_Unmarshal(INT32* target, BYTE** buffer, INT32* size) {
  return int32_t_Unmarshal(target, buffer, size);
}

UINT16 INT64_Marshal(INT64* source, BYTE** buffer, INT32* size) {
  return int64_t_Marshal(source, buffer, size);
}

TPM_RC INT64_Unmarshal(INT64* target, BYTE** buffer, INT32* size) {
  return int64_t_Unmarshal(target, buffer, size);
}

UINT16 INT8_Marshal(INT8* source, BYTE** buffer, INT32* size) {
  return int8_t_Marshal(source, buffer, size);
}

TPM_RC INT8_Unmarshal(INT8* target, BYTE** buffer, INT32* size) {
  return int8_t_Unmarshal(target, buffer, size);
}

UINT16 UINT16_Marshal(UINT16* source, BYTE** buffer, INT32* size) {
  return uint16_t_Marshal(source, buffer, size);
}

TPM_RC UINT16_Unmarshal(UINT16* target, BYTE** buffer, INT32* size) {
  return uint16_t_Unmarshal(target, buffer, size);
}

UINT16 TPM2B_ATTEST_Marshal(TPM2B_ATTEST* source, BYTE** buffer, INT32* size) {
  UINT16 total_size = 0;
  INT32 i;
  total_size += UINT16_Marshal(&source->t.size, buffer, size);
  for (i = 0; i < source->t.size; ++i) {
    total_size += BYTE_Marshal(&source->t.attestationData[i], buffer, size);
  }
  return total_size;
}

TPM_RC TPM2B_ATTEST_Unmarshal(TPM2B_ATTEST* target,
                              BYTE** buffer,
                              INT32* size) {
  TPM_RC result;
  INT32 i;
  result = UINT16_Unmarshal(&target->t.size, buffer, size);
  if (result != TPM_RC_SUCCESS) {
    return result;
  }
  if (target->t.size == 0) {
    return TPM_RC_SUCCESS;
  }
  if (target->t.size > sizeof(TPMS_ATTEST)) {
    return TPM_RC_SIZE;
  }
  for (i = 0; i < target->t.size; ++i) {
    result = BYTE_Unmarshal(&target->t.attestationData[i], buffer, size);
    if (result != TPM_RC_SUCCESS) {
      return result;
    }
  }
  return TPM_RC_SUCCESS;
}

UINT16 TPM2B_DIGEST_Marshal(TPM2B_DIGEST* source, BYTE** buffer, INT32* size) {
  UINT16 total_size = 0;
  INT32 i;
  total_size += UINT16_Marshal(&source->t.size, buffer, size);
  for (i = 0; i < source->t.size; ++i) {
    total_size += BYTE_Marshal(&source->t.buffer[i], buffer, size);
  }
  return total_size;
}

TPM_RC TPM2B_DIGEST_Unmarshal(TPM2B_DIGEST* target,
                              BYTE** buffer,
                              INT32* size) {
  TPM_RC result;
  INT32 i;
  result = UINT16_Unmarshal(&target->t.size, buffer, size);
  if (result != TPM_RC_SUCCESS) {
    return result;
  }
  if (target->t.size == 0) {
    return TPM_RC_SUCCESS;
  }
  if (target->t.size > sizeof(TPMU_HA)) {
    return TPM_RC_SIZE;
  }
  for (i = 0; i < target->t.size; ++i) {
    result = BYTE_Unmarshal(&target->t.buffer[i], buffer, size);
    if (result != TPM_RC_SUCCESS) {
      return result;
    }
  }
  return TPM_RC_SUCCESS;
}

UINT16 TPM2B_AUTH_Marshal(TPM2B_AUTH* source, BYTE** buffer, INT32* size) {
  return TPM2B_DIGEST_Marshal(source, buffer, size);
}

TPM_RC TPM2B_AUTH_Unmarshal(TPM2B_AUTH* target, BYTE** buffer, INT32* size) {
  return TPM2B_DIGEST_Unmarshal(target, buffer, size);
}

UINT16 TPM2B_CONTEXT_DATA_Marshal(TPM2B_CONTEXT_DATA* source,
                                  BYTE** buffer,
                                  INT32* size) {
  UINT16 total_size = 0;
  INT32 i;
  total_size += UINT16_Marshal(&source->t.size, buffer, size);
  for (i = 0; i < source->t.size; ++i) {
    total_size += BYTE_Marshal(&source->t.buffer[i], buffer, size);
  }
  return total_size;
}

TPM_RC TPM2B_CONTEXT_DATA_Unmarshal(TPM2B_CONTEXT_DATA* target,
                                    BYTE** buffer,
                                    INT32* size) {
  TPM_RC result;
  INT32 i;
  result = UINT16_Unmarshal(&target->t.size, buffer, size);
  if (result != TPM_RC_SUCCESS) {
    return result;
  }
  if (target->t.size == 0) {
    return TPM_RC_SUCCESS;
  }
  if (target->t.size > sizeof(TPMS_CONTEXT_DATA)) {
    return TPM_RC_SIZE;
  }
  for (i = 0; i < target->t.size; ++i) {
    result = BYTE_Unmarshal(&target->t.buffer[i], buffer, size);
    if (result != TPM_RC_SUCCESS) {
      return result;
    }
  }
  return TPM_RC_SUCCESS;
}

UINT16 TPM2B_CONTEXT_SENSITIVE_Marshal(TPM2B_CONTEXT_SENSITIVE* source,
                                       BYTE** buffer,
                                       INT32* size) {
  UINT16 total_size = 0;
  INT32 i;
  total_size += UINT16_Marshal(&source->t.size, buffer, size);
  for (i = 0; i < source->t.size; ++i) {
    total_size += BYTE_Marshal(&source->t.buffer[i], buffer, size);
  }
  return total_size;
}

TPM_RC TPM2B_CONTEXT_SENSITIVE_Unmarshal(TPM2B_CONTEXT_SENSITIVE* target,
                                         BYTE** buffer,
                                         INT32* size) {
  TPM_RC result;
  INT32 i;
  result = UINT16_Unmarshal(&target->t.size, buffer, size);
  if (result != TPM_RC_SUCCESS) {
    return result;
  }
  if (target->t.size == 0) {
    return TPM_RC_SUCCESS;
  }
  if (target->t.size > MAX_CONTEXT_SIZE) {
    return TPM_RC_SIZE;
  }
  for (i = 0; i < target->t.size; ++i) {
    result = BYTE_Unmarshal(&target->t.buffer[i], buffer, size);
    if (result != TPM_RC_SUCCESS) {
      return result;
    }
  }
  return TPM_RC_SUCCESS;
}

UINT16 TPM_ALG_ID_Marshal(TPM_ALG_ID* source, BYTE** buffer, INT32* size) {
  return uint16_t_Marshal(source, buffer, size);
}

TPM_RC TPM_ALG_ID_Unmarshal(TPM_ALG_ID* target, BYTE** buffer, INT32* size) {
  TPM_RC result;
  result = uint16_t_Unmarshal(target, buffer, size);
  if (result != TPM_RC_SUCCESS) {
    return result;
  }
#ifdef TPM_ALG_ERROR
  if (*target == TPM_ALG_ERROR) {
    return TPM_RC_SUCCESS;
  }
#endif
#ifdef TPM_ALG_RSA
  if (*target == TPM_ALG_RSA) {
    return TPM_RC_SUCCESS;
  }
#endif
#ifdef TPM_ALG_SHA
  if (*target == TPM_ALG_SHA) {
    return TPM_RC_SUCCESS;
  }
#endif
#ifdef TPM_ALG_SHA1
  if (*target == TPM_ALG_SHA1) {
    return TPM_RC_SUCCESS;
  }
#endif
#ifdef TPM_ALG_HMAC
  if (*target == TPM_ALG_HMAC) {
    return TPM_RC_SUCCESS;
  }
#endif
#ifdef TPM_ALG_AES
  if (*target == TPM_ALG_AES) {
    return TPM_RC_SUCCESS;
  }
#endif
#ifdef TPM_ALG_MGF1
  if (*target == TPM_ALG_MGF1) {
    return TPM_RC_SUCCESS;
  }
#endif
#ifdef TPM_ALG_KEYEDHASH
  if (*target == TPM_ALG_KEYEDHASH) {
    return TPM_RC_SUCCESS;
  }
#endif
#ifdef TPM_ALG_XOR
  if (*target == TPM_ALG_XOR) {
    return TPM_RC_SUCCESS;
  }
#endif
#ifdef TPM_ALG_SHA256
  if (*target == TPM_ALG_SHA256) {
    return TPM_RC_SUCCESS;
  }
#endif
#ifdef TPM_ALG_SHA384
  if (*target == TPM_ALG_SHA384) {
    return TPM_RC_SUCCESS;
  }
#endif
#ifdef TPM_ALG_SHA512
  if (*target == TPM_ALG_SHA512) {
    return TPM_RC_SUCCESS;
  }
#endif
#ifdef TPM_ALG_NULL
  if (*target == TPM_ALG_NULL) {
    return TPM_RC_SUCCESS;
  }
#endif
#ifdef TPM_ALG_SM3_256
  if (*target == TPM_ALG_SM3_256) {
    return TPM_RC_SUCCESS;
  }
#endif
#ifdef TPM_ALG_SM4
  if (*target == TPM_ALG_SM4) {
    return TPM_RC_SUCCESS;
  }
#endif
#ifdef TPM_ALG_RSASSA
  if (*target == TPM_ALG_RSASSA) {
    return TPM_RC_SUCCESS;
  }
#endif
#ifdef TPM_ALG_RSAES
  if (*target == TPM_ALG_RSAES) {
    return TPM_RC_SUCCESS;
  }
#endif
#ifdef TPM_ALG_RSAPSS
  if (*target == TPM_ALG_RSAPSS) {
    return TPM_RC_SUCCESS;
  }
#endif
#ifdef TPM_ALG_OAEP
  if (*target == TPM_ALG_OAEP) {
    return TPM_RC_SUCCESS;
  }
#endif
#ifdef TPM_ALG_ECDSA
  if (*target == TPM_ALG_ECDSA) {
    return TPM_RC_SUCCESS;
  }
#endif
#ifdef TPM_ALG_ECDH
  if (*target == TPM_ALG_ECDH) {
    return TPM_RC_SUCCESS;
  }
#endif
#ifdef TPM_ALG_ECDAA
  if (*target == TPM_ALG_ECDAA) {
    return TPM_RC_SUCCESS;
  }
#endif
#ifdef TPM_ALG_SM2
  if (*target == TPM_ALG_SM2) {
    return TPM_RC_SUCCESS;
  }
#endif
#ifdef TPM_ALG_ECSCHNORR
  if (*target == TPM_ALG_ECSCHNORR) {
    return TPM_RC_SUCCESS;
  }
#endif
#ifdef TPM_ALG_ECMQV
  if (*target == TPM_ALG_ECMQV) {
    return TPM_RC_SUCCESS;
  }
#endif
#ifdef TPM_ALG_KDF1_SP800_56A
  if (*target == TPM_ALG_KDF1_SP800_56A) {
    return TPM_RC_SUCCESS;
  }
#endif
#ifdef TPM_ALG_KDF2
  if (*target == TPM_ALG_KDF2) {
    return TPM_RC_SUCCESS;
  }
#endif
#ifdef TPM_ALG_KDF1_SP800_108
  if (*target == TPM_ALG_KDF1_SP800_108) {
    return TPM_RC_SUCCESS;
  }
#endif
#ifdef TPM_ALG_ECC
  if (*target == TPM_ALG_ECC) {
    return TPM_RC_SUCCESS;
  }
#endif
#ifdef TPM_ALG_SYMCIPHER
  if (*target == TPM_ALG_SYMCIPHER) {
    return TPM_RC_SUCCESS;
  }
#endif
#ifdef TPM_ALG_CAMELLIA
  if (*target == TPM_ALG_CAMELLIA) {
    return TPM_RC_SUCCESS;
  }
#endif
#ifdef TPM_ALG_CTR
  if (*target == TPM_ALG_CTR) {
    return TPM_RC_SUCCESS;
  }
#endif
#ifdef TPM_ALG_OFB
  if (*target == TPM_ALG_OFB) {
    return TPM_RC_SUCCESS;
  }
#endif
#ifdef TPM_ALG_CBC
  if (*target == TPM_ALG_CBC) {
    return TPM_RC_SUCCESS;
  }
#endif
#ifdef TPM_ALG_CFB
  if (*target == TPM_ALG_CFB) {
    return TPM_RC_SUCCESS;
  }
#endif
#ifdef TPM_ALG_ECB
  if (*target == TPM_ALG_ECB) {
    return TPM_RC_SUCCESS;
  }
#endif
  return TPM_RC_VALUE;
}

UINT16 TPM2B_DATA_Marshal(TPM2B_DATA* source, BYTE** buffer, INT32* size) {
  UINT16 total_size = 0;
  INT32 i;
  total_size += UINT16_Marshal(&source->t.size, buffer, size);
  for (i = 0; i < source->t.size; ++i) {
    total_size += BYTE_Marshal(&source->t.buffer[i], buffer, size);
  }
  return total_size;
}

TPM_RC TPM2B_DATA_Unmarshal(TPM2B_DATA* target, BYTE** buffer, INT32* size) {
  TPM_RC result;
  INT32 i;
  result = UINT16_Unmarshal(&target->t.size, buffer, size);
  if (result != TPM_RC_SUCCESS) {
    return result;
  }
  if (target->t.size == 0) {
    return TPM_RC_SUCCESS;
  }
  if (target->t.size > sizeof(TPMT_HA)) {
    return TPM_RC_SIZE;
  }
  for (i = 0; i < target->t.size; ++i) {
    result = BYTE_Unmarshal(&target->t.buffer[i], buffer, size);
    if (result != TPM_RC_SUCCESS) {
      return result;
    }
  }
  return TPM_RC_SUCCESS;
}

UINT16 TPMA_LOCALITY_Marshal(TPMA_LOCALITY* source,
                             BYTE** buffer,
                             INT32* size) {
  return uint8_t_Marshal((uint8_t*)source, buffer, size);
}

TPM_RC TPMA_LOCALITY_Unmarshal(TPMA_LOCALITY* target,
                               BYTE** buffer,
                               INT32* size) {
  TPM_RC result;
  result = uint8_t_Unmarshal((uint8_t*)target, buffer, size);
  if (result != TPM_RC_SUCCESS) {
    return result;
  }
  return TPM_RC_SUCCESS;
}

UINT16 TPM2B_NAME_Marshal(TPM2B_NAME* source, BYTE** buffer, INT32* size) {
  UINT16 total_size = 0;
  INT32 i;
  total_size += UINT16_Marshal(&source->t.size, buffer, size);
  for (i = 0; i < source->t.size; ++i) {
    total_size += BYTE_Marshal(&source->t.name[i], buffer, size);
  }
  return total_size;
}

TPM_RC TPM2B_NAME_Unmarshal(TPM2B_NAME* target, BYTE** buffer, INT32* size) {
  TPM_RC result;
  INT32 i;
  result = UINT16_Unmarshal(&target->t.size, buffer, size);
  if (result != TPM_RC_SUCCESS) {
    return result;
  }
  if (target->t.size == 0) {
    return TPM_RC_SUCCESS;
  }
  if (target->t.size > sizeof(TPMU_NAME)) {
    return TPM_RC_SIZE;
  }
  for (i = 0; i < target->t.size; ++i) {
    result = BYTE_Unmarshal(&target->t.name[i], buffer, size);
    if (result != TPM_RC_SUCCESS) {
      return result;
    }
  }
  return TPM_RC_SUCCESS;
}

UINT16 TPMI_ALG_HASH_Marshal(TPMI_ALG_HASH* source,
                             BYTE** buffer,
                             INT32* size) {
  return uint16_t_Marshal(source, buffer, size);
}

TPM_RC TPMI_ALG_HASH_Unmarshal(TPMI_ALG_HASH* target,
                               BYTE** buffer,
                               INT32* size,
                               BOOL allow_conditional_value) {
  TPM_RC result;
  BOOL has_valid_value = FALSE;
  result = uint16_t_Unmarshal(target, buffer, size);
  if ((result != TPM_RC_VALUE) && (result != TPM_RC_SUCCESS)) {
    return result;
  }
  if (*target == TPM_ALG_NULL) {
    return allow_conditional_value ? TPM_RC_SUCCESS : TPM_RC_HASH;
  }
  switch (*target) {
#ifdef TPM_ALG_SHA
    case TPM_ALG_SHA:
#endif
#ifdef TPM_ALG_SHA1
    case TPM_ALG_SHA1:
#endif
#ifdef TPM_ALG_SHA256
    case TPM_ALG_SHA256:
#endif
#ifdef TPM_ALG_SHA384
    case TPM_ALG_SHA384:
#endif
#ifdef TPM_ALG_SHA512
    case TPM_ALG_SHA512:
#endif
#ifdef TPM_ALG_SM3_256
    case TPM_ALG_SM3_256:
#endif
      has_valid_value = TRUE;
      break;
  }
  if (!has_valid_value) {
    return TPM_RC_HASH;
  }
  return TPM_RC_SUCCESS;
}

UINT16 UINT8_Marshal(UINT8* source, BYTE** buffer, INT32* size) {
  return uint8_t_Marshal(source, buffer, size);
}

TPM_RC UINT8_Unmarshal(UINT8* target, BYTE** buffer, INT32* size) {
  return uint8_t_Unmarshal(target, buffer, size);
}

UINT16 TPMS_PCR_SELECTION_Marshal(TPMS_PCR_SELECTION* source,
                                  BYTE** buffer,
                                  INT32* size) {
  UINT16 total_size = 0;
  INT32 i;
  total_size += TPMI_ALG_HASH_Marshal(&source->hash, buffer, size);
  total_size += UINT8_Marshal(&source->sizeofSelect, buffer, size);
  for (i = 0; i < source->sizeofSelect; ++i) {
    total_size += BYTE_Marshal(&source->pcrSelect[i], buffer, size);
  }
  return total_size;
}

TPM_RC TPMS_PCR_SELECTION_Unmarshal(TPMS_PCR_SELECTION* target,
                                    BYTE** buffer,
                                    INT32* size) {
  TPM_RC result;
  INT32 i;
  result = TPMI_ALG_HASH_Unmarshal(&target->hash, buffer, size, FALSE);
  if (result != TPM_RC_SUCCESS) {
    return result;
  }
  result = UINT8_Unmarshal(&target->sizeofSelect, buffer, size);
  if (result != TPM_RC_SUCCESS) {
    return result;
  }
  if (target->sizeofSelect > PCR_SELECT_MAX) {
    return TPM_RC_VALUE;
  }
  if (target->sizeofSelect < PCR_SELECT_MIN) {
    return TPM_RC_VALUE;
  }
  for (i = 0; i < target->sizeofSelect; ++i) {
    result = BYTE_Unmarshal(&target->pcrSelect[i], buffer, size);
    if (result != TPM_RC_SUCCESS) {
      return result;
    }
  }
  return TPM_RC_SUCCESS;
}

UINT16 UINT32_Marshal(UINT32* source, BYTE** buffer, INT32* size) {
  return uint32_t_Marshal(source, buffer, size);
}

TPM_RC UINT32_Unmarshal(UINT32* target, BYTE** buffer, INT32* size) {
  return uint32_t_Unmarshal(target, buffer, size);
}

UINT16 TPML_PCR_SELECTION_Marshal(TPML_PCR_SELECTION* source,
                                  BYTE** buffer,
                                  INT32* size) {
  UINT16 total_size = 0;
  INT32 i;
  total_size += UINT32_Marshal(&source->count, buffer, size);
  for (i = 0; i < source->count; ++i) {
    total_size +=
        TPMS_PCR_SELECTION_Marshal(&source->pcrSelections[i], buffer, size);
  }
  return total_size;
}

TPM_RC TPML_PCR_SELECTION_Unmarshal(TPML_PCR_SELECTION* target,
                                    BYTE** buffer,
                                    INT32* size) {
  TPM_RC result;
  INT32 i;
  result = UINT32_Unmarshal(&target->count, buffer, size);
  if (result != TPM_RC_SUCCESS) {
    return result;
  }
  if (target->count > HASH_COUNT) {
    return TPM_RC_SIZE;
  }
  for (i = 0; i < target->count; ++i) {
    result =
        TPMS_PCR_SELECTION_Unmarshal(&target->pcrSelections[i], buffer, size);
    if (result != TPM_RC_SUCCESS) {
      return result;
    }
  }
  return TPM_RC_SUCCESS;
}

UINT16 TPMS_CREATION_DATA_Marshal(TPMS_CREATION_DATA* source,
                                  BYTE** buffer,
                                  INT32* size) {
  UINT16 total_size = 0;
  total_size += TPML_PCR_SELECTION_Marshal(&source->pcrSelect, buffer, size);
  total_size += TPM2B_DIGEST_Marshal(&source->pcrDigest, buffer, size);
  total_size += TPMA_LOCALITY_Marshal(&source->locality, buffer, size);
  total_size += TPM_ALG_ID_Marshal(&source->parentNameAlg, buffer, size);
  total_size += TPM2B_NAME_Marshal(&source->parentName, buffer, size);
  total_size += TPM2B_NAME_Marshal(&source->parentQualifiedName, buffer, size);
  total_size += TPM2B_DATA_Marshal(&source->outsideInfo, buffer, size);
  return total_size;
}

TPM_RC TPMS_CREATION_DATA_Unmarshal(TPMS_CREATION_DATA* target,
                                    BYTE** buffer,
                                    INT32* size) {
  TPM_RC result;
  result = TPML_PCR_SELECTION_Unmarshal(&target->pcrSelect, buffer, size);
  if (result != TPM_RC_SUCCESS) {
    return result;
  }
  result = TPM2B_DIGEST_Unmarshal(&target->pcrDigest, buffer, size);
  if (result != TPM_RC_SUCCESS) {
    return result;
  }
  result = TPMA_LOCALITY_Unmarshal(&target->locality, buffer, size);
  if (result != TPM_RC_SUCCESS) {
    return result;
  }
  result = TPM_ALG_ID_Unmarshal(&target->parentNameAlg, buffer, size);
  if (result != TPM_RC_SUCCESS) {
    return result;
  }
  result = TPM2B_NAME_Unmarshal(&target->parentName, buffer, size);
  if (result != TPM_RC_SUCCESS) {
    return result;
  }
  result = TPM2B_NAME_Unmarshal(&target->parentQualifiedName, buffer, size);
  if (result != TPM_RC_SUCCESS) {
    return result;
  }
  result = TPM2B_DATA_Unmarshal(&target->outsideInfo, buffer, size);
  if (result != TPM_RC_SUCCESS) {
    return result;
  }
  return TPM_RC_SUCCESS;
}

UINT16 TPM2B_CREATION_DATA_Marshal(TPM2B_CREATION_DATA* source,
                                   BYTE** buffer,
                                   INT32* size) {
  UINT16 total_size = 0;
  total_size += UINT16_Marshal(&source->t.size, buffer, size);
  total_size +=
      TPMS_CREATION_DATA_Marshal(&source->t.creationData, buffer, size);
  {
    BYTE* size_location = *buffer - total_size;
    INT32 size_field_size = sizeof(UINT16);
    UINT16 payload_size = total_size - (UINT16)size_field_size;
    UINT16_Marshal(&payload_size, &size_location, &size_field_size);
  }
  return total_size;
}

TPM_RC TPM2B_CREATION_DATA_Unmarshal(TPM2B_CREATION_DATA* target,
                                     BYTE** buffer,
                                     INT32* size) {
  TPM_RC result;
  UINT32 start_size = *size;
  UINT32 struct_size;
  result = UINT16_Unmarshal(&target->t.size, buffer, size);
  if (result != TPM_RC_SUCCESS) {
    return result;
  }
  if (target->t.size == 0) {
    return TPM_RC_SIZE;
  }
  result = TPMS_CREATION_DATA_Unmarshal(&target->t.creationData, buffer, size);
  if (result != TPM_RC_SUCCESS) {
    return result;
  }
  struct_size = start_size - *size - sizeof(target->t.size);
  if (struct_size != target->t.size) {
    return TPM_RC_SIZE;
  }
  return TPM_RC_SUCCESS;
}

UINT16 TPM2B_DIGEST_VALUES_Marshal(TPM2B_DIGEST_VALUES* source,
                                   BYTE** buffer,
                                   INT32* size) {
  UINT16 total_size = 0;
  INT32 i;
  total_size += UINT16_Marshal(&source->t.size, buffer, size);
  for (i = 0; i < source->t.size; ++i) {
    total_size += BYTE_Marshal(&source->t.buffer[i], buffer, size);
  }
  return total_size;
}

TPM_RC TPM2B_DIGEST_VALUES_Unmarshal(TPM2B_DIGEST_VALUES* target,
                                     BYTE** buffer,
                                     INT32* size) {
  TPM_RC result;
  INT32 i;
  result = UINT16_Unmarshal(&target->t.size, buffer, size);
  if (result != TPM_RC_SUCCESS) {
    return result;
  }
  if (target->t.size == 0) {
    return TPM_RC_SUCCESS;
  }
  if (target->t.size > sizeof(TPML_DIGEST_VALUES)) {
    return TPM_RC_SIZE;
  }
  for (i = 0; i < target->t.size; ++i) {
    result = BYTE_Unmarshal(&target->t.buffer[i], buffer, size);
    if (result != TPM_RC_SUCCESS) {
      return result;
    }
  }
  return TPM_RC_SUCCESS;
}

UINT16 TPM2B_ECC_PARAMETER_Marshal(TPM2B_ECC_PARAMETER* source,
                                   BYTE** buffer,
                                   INT32* size) {
  UINT16 total_size = 0;
  INT32 i;
  total_size += UINT16_Marshal(&source->t.size, buffer, size);
  for (i = 0; i < source->t.size; ++i) {
    total_size += BYTE_Marshal(&source->t.buffer[i], buffer, size);
  }
  return total_size;
}

TPM_RC TPM2B_ECC_PARAMETER_Unmarshal(TPM2B_ECC_PARAMETER* target,
                                     BYTE** buffer,
                                     INT32* size) {
  TPM_RC result;
  INT32 i;
  result = UINT16_Unmarshal(&target->t.size, buffer, size);
  if (result != TPM_RC_SUCCESS) {
    return result;
  }
  if (target->t.size == 0) {
    return TPM_RC_SUCCESS;
  }
  if (target->t.size > MAX_ECC_KEY_BYTES) {
    return TPM_RC_SIZE;
  }
  for (i = 0; i < target->t.size; ++i) {
    result = BYTE_Unmarshal(&target->t.buffer[i], buffer, size);
    if (result != TPM_RC_SUCCESS) {
      return result;
    }
  }
  return TPM_RC_SUCCESS;
}

UINT16 TPMS_ECC_POINT_Marshal(TPMS_ECC_POINT* source,
                              BYTE** buffer,
                              INT32* size) {
  UINT16 total_size = 0;
  total_size += TPM2B_ECC_PARAMETER_Marshal(&source->x, buffer, size);
  total_size += TPM2B_ECC_PARAMETER_Marshal(&source->y, buffer, size);
  return total_size;
}

TPM_RC TPMS_ECC_POINT_Unmarshal(TPMS_ECC_POINT* target,
                                BYTE** buffer,
                                INT32* size) {
  TPM_RC result;
  result = TPM2B_ECC_PARAMETER_Unmarshal(&target->x, buffer, size);
  if (result != TPM_RC_SUCCESS) {
    return result;
  }
  result = TPM2B_ECC_PARAMETER_Unmarshal(&target->y, buffer, size);
  if (result != TPM_RC_SUCCESS) {
    return result;
  }
  return TPM_RC_SUCCESS;
}

UINT16 TPM2B_ECC_POINT_Marshal(TPM2B_ECC_POINT* source,
                               BYTE** buffer,
                               INT32* size) {
  UINT16 total_size = 0;
  total_size += UINT16_Marshal(&source->t.size, buffer, size);
  total_size += TPMS_ECC_POINT_Marshal(&source->t.point, buffer, size);
  {
    BYTE* size_location = *buffer - total_size;
    INT32 size_field_size = sizeof(UINT16);
    UINT16 payload_size = total_size - (UINT16)size_field_size;
    UINT16_Marshal(&payload_size, &size_location, &size_field_size);
  }
  return total_size;
}

TPM_RC TPM2B_ECC_POINT_Unmarshal(TPM2B_ECC_POINT* target,
                                 BYTE** buffer,
                                 INT32* size) {
  TPM_RC result;
  UINT32 start_size = *size;
  UINT32 struct_size;
  result = UINT16_Unmarshal(&target->t.size, buffer, size);
  if (result != TPM_RC_SUCCESS) {
    return result;
  }
  if (target->t.size == 0) {
    return TPM_RC_SIZE;
  }
  result = TPMS_ECC_POINT_Unmarshal(&target->t.point, buffer, size);
  if (result != TPM_RC_SUCCESS) {
    return result;
  }
  struct_size = start_size - *size - sizeof(target->t.size);
  if (struct_size != target->t.size) {
    return TPM_RC_SIZE;
  }
  return TPM_RC_SUCCESS;
}

UINT16 TPM2B_ENCRYPTED_SECRET_Marshal(TPM2B_ENCRYPTED_SECRET* source,
                                      BYTE** buffer,
                                      INT32* size) {
  UINT16 total_size = 0;
  INT32 i;
  total_size += UINT16_Marshal(&source->t.size, buffer, size);
  for (i = 0; i < source->t.size; ++i) {
    total_size += BYTE_Marshal(&source->t.secret[i], buffer, size);
  }
  return total_size;
}

TPM_RC TPM2B_ENCRYPTED_SECRET_Unmarshal(TPM2B_ENCRYPTED_SECRET* target,
                                        BYTE** buffer,
                                        INT32* size) {
  TPM_RC result;
  INT32 i;
  result = UINT16_Unmarshal(&target->t.size, buffer, size);
  if (result != TPM_RC_SUCCESS) {
    return result;
  }
  if (target->t.size == 0) {
    return TPM_RC_SUCCESS;
  }
  if (target->t.size > sizeof(TPMU_ENCRYPTED_SECRET)) {
    return TPM_RC_SIZE;
  }
  for (i = 0; i < target->t.size; ++i) {
    result = BYTE_Unmarshal(&target->t.secret[i], buffer, size);
    if (result != TPM_RC_SUCCESS) {
      return result;
    }
  }
  return TPM_RC_SUCCESS;
}

UINT16 TPM2B_EVENT_Marshal(TPM2B_EVENT* source, BYTE** buffer, INT32* size) {
  UINT16 total_size = 0;
  INT32 i;
  total_size += UINT16_Marshal(&source->t.size, buffer, size);
  for (i = 0; i < source->t.size; ++i) {
    total_size += BYTE_Marshal(&source->t.buffer[i], buffer, size);
  }
  return total_size;
}

TPM_RC TPM2B_EVENT_Unmarshal(TPM2B_EVENT* target, BYTE** buffer, INT32* size) {
  TPM_RC result;
  INT32 i;
  result = UINT16_Unmarshal(&target->t.size, buffer, size);
  if (result != TPM_RC_SUCCESS) {
    return result;
  }
  if (target->t.size == 0) {
    return TPM_RC_SUCCESS;
  }
  if (target->t.size > 1024) {
    return TPM_RC_SIZE;
  }
  for (i = 0; i < target->t.size; ++i) {
    result = BYTE_Unmarshal(&target->t.buffer[i], buffer, size);
    if (result != TPM_RC_SUCCESS) {
      return result;
    }
  }
  return TPM_RC_SUCCESS;
}

UINT16 TPM2B_ID_OBJECT_Marshal(TPM2B_ID_OBJECT* source,
                               BYTE** buffer,
                               INT32* size) {
  UINT16 total_size = 0;
  INT32 i;
  total_size += UINT16_Marshal(&source->t.size, buffer, size);
  for (i = 0; i < source->t.size; ++i) {
    total_size += BYTE_Marshal(&source->t.credential[i], buffer, size);
  }
  return total_size;
}

TPM_RC TPM2B_ID_OBJECT_Unmarshal(TPM2B_ID_OBJECT* target,
                                 BYTE** buffer,
                                 INT32* size) {
  TPM_RC result;
  INT32 i;
  result = UINT16_Unmarshal(&target->t.size, buffer, size);
  if (result != TPM_RC_SUCCESS) {
    return result;
  }
  if (target->t.size == 0) {
    return TPM_RC_SUCCESS;
  }
  if (target->t.size > sizeof(_ID_OBJECT)) {
    return TPM_RC_SIZE;
  }
  for (i = 0; i < target->t.size; ++i) {
    result = BYTE_Unmarshal(&target->t.credential[i], buffer, size);
    if (result != TPM_RC_SUCCESS) {
      return result;
    }
  }
  return TPM_RC_SUCCESS;
}

UINT16 TPM2B_IV_Marshal(TPM2B_IV* source, BYTE** buffer, INT32* size) {
  UINT16 total_size = 0;
  INT32 i;
  total_size += UINT16_Marshal(&source->t.size, buffer, size);
  for (i = 0; i < source->t.size; ++i) {
    total_size += BYTE_Marshal(&source->t.buffer[i], buffer, size);
  }
  return total_size;
}

TPM_RC TPM2B_IV_Unmarshal(TPM2B_IV* target, BYTE** buffer, INT32* size) {
  TPM_RC result;
  INT32 i;
  result = UINT16_Unmarshal(&target->t.size, buffer, size);
  if (result != TPM_RC_SUCCESS) {
    return result;
  }
  if (target->t.size == 0) {
    return TPM_RC_SUCCESS;
  }
  if (target->t.size > MAX_SYM_BLOCK_SIZE) {
    return TPM_RC_SIZE;
  }
  for (i = 0; i < target->t.size; ++i) {
    result = BYTE_Unmarshal(&target->t.buffer[i], buffer, size);
    if (result != TPM_RC_SUCCESS) {
      return result;
    }
  }
  return TPM_RC_SUCCESS;
}

UINT16 TPM2B_MAX_BUFFER_Marshal(TPM2B_MAX_BUFFER* source,
                                BYTE** buffer,
                                INT32* size) {
  UINT16 total_size = 0;
  INT32 i;
  total_size += UINT16_Marshal(&source->t.size, buffer, size);
  for (i = 0; i < source->t.size; ++i) {
    total_size += BYTE_Marshal(&source->t.buffer[i], buffer, size);
  }
  return total_size;
}

TPM_RC TPM2B_MAX_BUFFER_Unmarshal(TPM2B_MAX_BUFFER* target,
                                  BYTE** buffer,
                                  INT32* size) {
  TPM_RC result;
  INT32 i;
  result = UINT16_Unmarshal(&target->t.size, buffer, size);
  if (result != TPM_RC_SUCCESS) {
    return result;
  }
  if (target->t.size == 0) {
    return TPM_RC_SUCCESS;
  }
  if (target->t.size > MAX_DIGEST_BUFFER) {
    return TPM_RC_SIZE;
  }
  for (i = 0; i < target->t.size; ++i) {
    result = BYTE_Unmarshal(&target->t.buffer[i], buffer, size);
    if (result != TPM_RC_SUCCESS) {
      return result;
    }
  }
  return TPM_RC_SUCCESS;
}

UINT16 TPM2B_MAX_NV_BUFFER_Marshal(TPM2B_MAX_NV_BUFFER* source,
                                   BYTE** buffer,
                                   INT32* size) {
  UINT16 total_size = 0;
  INT32 i;
  total_size += UINT16_Marshal(&source->t.size, buffer, size);
  for (i = 0; i < source->t.size; ++i) {
    total_size += BYTE_Marshal(&source->t.buffer[i], buffer, size);
  }
  return total_size;
}

TPM_RC TPM2B_MAX_NV_BUFFER_Unmarshal(TPM2B_MAX_NV_BUFFER* target,
                                     BYTE** buffer,
                                     INT32* size) {
  TPM_RC result;
  INT32 i;
  result = UINT16_Unmarshal(&target->t.size, buffer, size);
  if (result != TPM_RC_SUCCESS) {
    return result;
  }
  if (target->t.size == 0) {
    return TPM_RC_SUCCESS;
  }
  if (target->t.size > MAX_NV_BUFFER_SIZE) {
    return TPM_RC_SIZE;
  }
  for (i = 0; i < target->t.size; ++i) {
    result = BYTE_Unmarshal(&target->t.buffer[i], buffer, size);
    if (result != TPM_RC_SUCCESS) {
      return result;
    }
  }
  return TPM_RC_SUCCESS;
}

UINT16 TPM2B_NONCE_Marshal(TPM2B_NONCE* source, BYTE** buffer, INT32* size) {
  return TPM2B_DIGEST_Marshal(source, buffer, size);
}

TPM_RC TPM2B_NONCE_Unmarshal(TPM2B_NONCE* target, BYTE** buffer, INT32* size) {
  return TPM2B_DIGEST_Unmarshal(target, buffer, size);
}

UINT16 TPMI_RH_NV_INDEX_Marshal(TPMI_RH_NV_INDEX* source,
                                BYTE** buffer,
                                INT32* size) {
  return uint32_t_Marshal(source, buffer, size);
}

TPM_RC TPMI_RH_NV_INDEX_Unmarshal(TPMI_RH_NV_INDEX* target,
                                  BYTE** buffer,
                                  INT32* size) {
  TPM_RC result;
  BOOL has_valid_value = FALSE;
  result = uint32_t_Unmarshal(target, buffer, size);
  if ((result != TPM_RC_VALUE) && (result != TPM_RC_SUCCESS)) {
    return result;
  }
  if ((*target >= NV_INDEX_FIRST) && (*target <= NV_INDEX_LAST)) {
    has_valid_value = TRUE;
  }
  if (!has_valid_value) {
    return TPM_RC_VALUE;
  }
  return TPM_RC_SUCCESS;
}

UINT16 TPMA_NV_Marshal(TPMA_NV* source, BYTE** buffer, INT32* size) {
  return uint32_t_Marshal((uint32_t*)source, buffer, size);
}

TPM_RC TPMA_NV_Unmarshal(TPMA_NV* target, BYTE** buffer, INT32* size) {
  TPM_RC result;
  result = uint32_t_Unmarshal((uint32_t*)target, buffer, size);
  if (result != TPM_RC_SUCCESS) {
    return result;
  }
  if (target->reserved7_9 != 0) {
    return TPM_RC_RESERVED_BITS;
  }
  if (target->reserved20_24 != 0) {
    return TPM_RC_RESERVED_BITS;
  }
  return TPM_RC_SUCCESS;
}

UINT16 TPMS_NV_PUBLIC_Marshal(TPMS_NV_PUBLIC* source,
                              BYTE** buffer,
                              INT32* size) {
  UINT16 total_size = 0;
  total_size += TPMI_RH_NV_INDEX_Marshal(&source->nvIndex, buffer, size);
  total_size += TPMI_ALG_HASH_Marshal(&source->nameAlg, buffer, size);
  total_size += TPMA_NV_Marshal(&source->attributes, buffer, size);
  total_size += TPM2B_DIGEST_Marshal(&source->authPolicy, buffer, size);
  total_size += UINT16_Marshal(&source->dataSize, buffer, size);
  return total_size;
}

TPM_RC TPMS_NV_PUBLIC_Unmarshal(TPMS_NV_PUBLIC* target,
                                BYTE** buffer,
                                INT32* size) {
  TPM_RC result;
  result = TPMI_RH_NV_INDEX_Unmarshal(&target->nvIndex, buffer, size);
  if (result != TPM_RC_SUCCESS) {
    return result;
  }
  result = TPMI_ALG_HASH_Unmarshal(&target->nameAlg, buffer, size, FALSE);
  if (result != TPM_RC_SUCCESS) {
    return result;
  }
  result = TPMA_NV_Unmarshal(&target->attributes, buffer, size);
  if (result != TPM_RC_SUCCESS) {
    return result;
  }
  result = TPM2B_DIGEST_Unmarshal(&target->authPolicy, buffer, size);
  if (result != TPM_RC_SUCCESS) {
    return result;
  }
  result = UINT16_Unmarshal(&target->dataSize, buffer, size);
  if (result != TPM_RC_SUCCESS) {
    return result;
  }
  if (target->dataSize > MAX_NV_INDEX_SIZE) {
    return TPM_RC_SIZE;
  }
  return TPM_RC_SUCCESS;
}

UINT16 TPM2B_NV_PUBLIC_Marshal(TPM2B_NV_PUBLIC* source,
                               BYTE** buffer,
                               INT32* size) {
  UINT16 total_size = 0;
  total_size += UINT16_Marshal(&source->t.size, buffer, size);
  total_size += TPMS_NV_PUBLIC_Marshal(&source->t.nvPublic, buffer, size);
  {
    BYTE* size_location = *buffer - total_size;
    INT32 size_field_size = sizeof(UINT16);
    UINT16 payload_size = total_size - (UINT16)size_field_size;
    UINT16_Marshal(&payload_size, &size_location, &size_field_size);
  }
  return total_size;
}

TPM_RC TPM2B_NV_PUBLIC_Unmarshal(TPM2B_NV_PUBLIC* target,
                                 BYTE** buffer,
                                 INT32* size) {
  TPM_RC result;
  UINT32 start_size = *size;
  UINT32 struct_size;
  result = UINT16_Unmarshal(&target->t.size, buffer, size);
  if (result != TPM_RC_SUCCESS) {
    return result;
  }
  if (target->t.size == 0) {
    return TPM_RC_SIZE;
  }
  result = TPMS_NV_PUBLIC_Unmarshal(&target->t.nvPublic, buffer, size);
  if (result != TPM_RC_SUCCESS) {
    return result;
  }
  struct_size = start_size - *size - sizeof(target->t.size);
  if (struct_size != target->t.size) {
    return TPM_RC_SIZE;
  }
  return TPM_RC_SUCCESS;
}

UINT16 TPM2B_OPERAND_Marshal(TPM2B_OPERAND* source,
                             BYTE** buffer,
                             INT32* size) {
  return TPM2B_DIGEST_Marshal(source, buffer, size);
}

TPM_RC TPM2B_OPERAND_Unmarshal(TPM2B_OPERAND* target,
                               BYTE** buffer,
                               INT32* size) {
  return TPM2B_DIGEST_Unmarshal(target, buffer, size);
}

UINT16 TPM2B_PRIVATE_Marshal(TPM2B_PRIVATE* source,
                             BYTE** buffer,
                             INT32* size) {
  UINT16 total_size = 0;
  INT32 i;
  total_size += UINT16_Marshal(&source->t.size, buffer, size);
  for (i = 0; i < source->t.size; ++i) {
    total_size += BYTE_Marshal(&source->t.buffer[i], buffer, size);
  }
  return total_size;
}

TPM_RC TPM2B_PRIVATE_Unmarshal(TPM2B_PRIVATE* target,
                               BYTE** buffer,
                               INT32* size) {
  TPM_RC result;
  INT32 i;
  result = UINT16_Unmarshal(&target->t.size, buffer, size);
  if (result != TPM_RC_SUCCESS) {
    return result;
  }
  if (target->t.size == 0) {
    return TPM_RC_SUCCESS;
  }
  if (target->t.size > sizeof(_PRIVATE)) {
    return TPM_RC_SIZE;
  }
  for (i = 0; i < target->t.size; ++i) {
    result = BYTE_Unmarshal(&target->t.buffer[i], buffer, size);
    if (result != TPM_RC_SUCCESS) {
      return result;
    }
  }
  return TPM_RC_SUCCESS;
}

UINT16 TPM2B_PRIVATE_KEY_RSA_Marshal(TPM2B_PRIVATE_KEY_RSA* source,
                                     BYTE** buffer,
                                     INT32* size) {
  UINT16 total_size = 0;
  INT32 i;
  total_size += UINT16_Marshal(&source->t.size, buffer, size);
  for (i = 0; i < source->t.size; ++i) {
    total_size += BYTE_Marshal(&source->t.buffer[i], buffer, size);
  }
  return total_size;
}

TPM_RC TPM2B_PRIVATE_KEY_RSA_Unmarshal(TPM2B_PRIVATE_KEY_RSA* target,
                                       BYTE** buffer,
                                       INT32* size) {
  TPM_RC result;
  INT32 i;
  result = UINT16_Unmarshal(&target->t.size, buffer, size);
  if (result != TPM_RC_SUCCESS) {
    return result;
  }
  if (target->t.size == 0) {
    return TPM_RC_SUCCESS;
  }
  if (target->t.size > MAX_RSA_KEY_BYTES / 2) {
    return TPM_RC_SIZE;
  }
  for (i = 0; i < target->t.size; ++i) {
    result = BYTE_Unmarshal(&target->t.buffer[i], buffer, size);
    if (result != TPM_RC_SUCCESS) {
      return result;
    }
  }
  return TPM_RC_SUCCESS;
}

UINT16 TPM2B_PRIVATE_VENDOR_SPECIFIC_Marshal(
    TPM2B_PRIVATE_VENDOR_SPECIFIC* source,
    BYTE** buffer,
    INT32* size) {
  UINT16 total_size = 0;
  INT32 i;
  total_size += UINT16_Marshal(&source->t.size, buffer, size);
  for (i = 0; i < source->t.size; ++i) {
    total_size += BYTE_Marshal(&source->t.buffer[i], buffer, size);
  }
  return total_size;
}

TPM_RC TPM2B_PRIVATE_VENDOR_SPECIFIC_Unmarshal(
    TPM2B_PRIVATE_VENDOR_SPECIFIC* target,
    BYTE** buffer,
    INT32* size) {
  TPM_RC result;
  INT32 i;
  result = UINT16_Unmarshal(&target->t.size, buffer, size);
  if (result != TPM_RC_SUCCESS) {
    return result;
  }
  if (target->t.size == 0) {
    return TPM_RC_SUCCESS;
  }
  if (target->t.size > PRIVATE_VENDOR_SPECIFIC_BYTES) {
    return TPM_RC_SIZE;
  }
  for (i = 0; i < target->t.size; ++i) {
    result = BYTE_Unmarshal(&target->t.buffer[i], buffer, size);
    if (result != TPM_RC_SUCCESS) {
      return result;
    }
  }
  return TPM_RC_SUCCESS;
}

UINT16 TPMA_OBJECT_Marshal(TPMA_OBJECT* source, BYTE** buffer, INT32* size) {
  return uint32_t_Marshal((uint32_t*)source, buffer, size);
}

TPM_RC TPMA_OBJECT_Unmarshal(TPMA_OBJECT* target, BYTE** buffer, INT32* size) {
  TPM_RC result;
  result = uint32_t_Unmarshal((uint32_t*)target, buffer, size);
  if (result != TPM_RC_SUCCESS) {
    return result;
  }
  if (target->reserved0 != 0) {
    return TPM_RC_RESERVED_BITS;
  }
  if (target->reserved3 != 0) {
    return TPM_RC_RESERVED_BITS;
  }
  if (target->reserved8_9 != 0) {
    return TPM_RC_RESERVED_BITS;
  }
  if (target->reserved12_15 != 0) {
    return TPM_RC_RESERVED_BITS;
  }
  if (target->reserved19_31 != 0) {
    return TPM_RC_RESERVED_BITS;
  }
  return TPM_RC_SUCCESS;
}

UINT16 TPMI_ALG_PUBLIC_Marshal(TPMI_ALG_PUBLIC* source,
                               BYTE** buffer,
                               INT32* size) {
  return uint16_t_Marshal(source, buffer, size);
}

TPM_RC TPMI_ALG_PUBLIC_Unmarshal(TPMI_ALG_PUBLIC* target,
                                 BYTE** buffer,
                                 INT32* size) {
  TPM_RC result;
  BOOL has_valid_value = FALSE;
  result = uint16_t_Unmarshal(target, buffer, size);
  if ((result != TPM_RC_VALUE) && (result != TPM_RC_SUCCESS)) {
    return result;
  }
  switch (*target) {
#ifdef TPM_ALG_RSA
    case TPM_ALG_RSA:
#endif
#ifdef TPM_ALG_KEYEDHASH
    case TPM_ALG_KEYEDHASH:
#endif
#ifdef TPM_ALG_ECC
    case TPM_ALG_ECC:
#endif
#ifdef TPM_ALG_SYMCIPHER
    case TPM_ALG_SYMCIPHER:
#endif
      has_valid_value = TRUE;
      break;
  }
  if (!has_valid_value) {
    return TPM_RC_TYPE;
  }
  return TPM_RC_SUCCESS;
}

UINT16 TPM2B_PUBLIC_KEY_RSA_Marshal(TPM2B_PUBLIC_KEY_RSA* source,
                                    BYTE** buffer,
                                    INT32* size) {
  UINT16 total_size = 0;
  INT32 i;
  total_size += UINT16_Marshal(&source->t.size, buffer, size);
  for (i = 0; i < source->t.size; ++i) {
    total_size += BYTE_Marshal(&source->t.buffer[i], buffer, size);
  }
  return total_size;
}

TPM_RC TPM2B_PUBLIC_KEY_RSA_Unmarshal(TPM2B_PUBLIC_KEY_RSA* target,
                                      BYTE** buffer,
                                      INT32* size) {
  TPM_RC result;
  INT32 i;
  result = UINT16_Unmarshal(&target->t.size, buffer, size);
  if (result != TPM_RC_SUCCESS) {
    return result;
  }
  if (target->t.size == 0) {
    return TPM_RC_SUCCESS;
  }
  if (target->t.size > MAX_RSA_KEY_BYTES) {
    return TPM_RC_SIZE;
  }
  for (i = 0; i < target->t.size; ++i) {
    result = BYTE_Unmarshal(&target->t.buffer[i], buffer, size);
    if (result != TPM_RC_SUCCESS) {
      return result;
    }
  }
  return TPM_RC_SUCCESS;
}

UINT16 TPMU_PUBLIC_ID_Marshal(TPMU_PUBLIC_ID* source,
                              BYTE** buffer,
                              INT32* size,
                              UINT32 selector) {
  switch (selector) {
#ifdef TPM_ALG_KEYEDHASH
    case TPM_ALG_KEYEDHASH:
      return TPM2B_DIGEST_Marshal((TPM2B_DIGEST*)&source->keyedHash, buffer,
                                  size);
#endif
#ifdef TPM_ALG_SYMCIPHER
    case TPM_ALG_SYMCIPHER:
      return TPM2B_DIGEST_Marshal((TPM2B_DIGEST*)&source->sym, buffer, size);
#endif
#ifdef TPM_ALG_RSA
    case TPM_ALG_RSA:
      return TPM2B_PUBLIC_KEY_RSA_Marshal((TPM2B_PUBLIC_KEY_RSA*)&source->rsa,
                                          buffer, size);
#endif
#ifdef TPM_ALG_ECC
    case TPM_ALG_ECC:
      return TPMS_ECC_POINT_Marshal((TPMS_ECC_POINT*)&source->ecc, buffer,
                                    size);
#endif
  }
  return 0;
}

TPM_RC TPMU_PUBLIC_ID_Unmarshal(TPMU_PUBLIC_ID* target,
                                BYTE** buffer,
                                INT32* size,
                                UINT32 selector) {
  switch (selector) {
#ifdef TPM_ALG_KEYEDHASH
    case TPM_ALG_KEYEDHASH:
      return TPM2B_DIGEST_Unmarshal((TPM2B_DIGEST*)&target->keyedHash, buffer,
                                    size);
#endif
#ifdef TPM_ALG_SYMCIPHER
    case TPM_ALG_SYMCIPHER:
      return TPM2B_DIGEST_Unmarshal((TPM2B_DIGEST*)&target->sym, buffer, size);
#endif
#ifdef TPM_ALG_RSA
    case TPM_ALG_RSA:
      return TPM2B_PUBLIC_KEY_RSA_Unmarshal((TPM2B_PUBLIC_KEY_RSA*)&target->rsa,
                                            buffer, size);
#endif
#ifdef TPM_ALG_ECC
    case TPM_ALG_ECC:
      return TPMS_ECC_POINT_Unmarshal((TPMS_ECC_POINT*)&target->ecc, buffer,
                                      size);
#endif
  }
  return TPM_RC_SELECTOR;
}

UINT16 TPM_KEY_BITS_Marshal(TPM_KEY_BITS* source, BYTE** buffer, INT32* size) {
  return uint16_t_Marshal(source, buffer, size);
}

TPM_RC TPM_KEY_BITS_Unmarshal(TPM_KEY_BITS* target,
                              BYTE** buffer,
                              INT32* size) {
  return uint16_t_Unmarshal(target, buffer, size);
}

UINT16 TPMI_AES_KEY_BITS_Marshal(TPMI_AES_KEY_BITS* source,
                                 BYTE** buffer,
                                 INT32* size) {
  return TPM_KEY_BITS_Marshal(source, buffer, size);
}

TPM_RC TPMI_AES_KEY_BITS_Unmarshal(TPMI_AES_KEY_BITS* target,
                                   BYTE** buffer,
                                   INT32* size) {
  TPM_RC result;
  uint16_t supported_values[] = AES_KEY_SIZES_BITS;
  size_t length = sizeof(supported_values) / sizeof(supported_values[0]);
  size_t i;
  BOOL is_supported_value = FALSE;
  result = TPM_KEY_BITS_Unmarshal(target, buffer, size);
  if (result != TPM_RC_SUCCESS) {
    return result;
  }
  for (i = 0; i < length; ++i) {
    if (*target == supported_values[i]) {
      is_supported_value = TRUE;
      break;
    }
  }
  if (!is_supported_value) {
    return TPM_RC_VALUE;
  }
  return TPM_RC_SUCCESS;
}

UINT16 TPMI_SM4_KEY_BITS_Marshal(TPMI_SM4_KEY_BITS* source,
                                 BYTE** buffer,
                                 INT32* size) {
  return TPM_KEY_BITS_Marshal(source, buffer, size);
}

TPM_RC TPMI_SM4_KEY_BITS_Unmarshal(TPMI_SM4_KEY_BITS* target,
                                   BYTE** buffer,
                                   INT32* size) {
  TPM_RC result;
  uint16_t supported_values[] = SM4_KEY_SIZES_BITS;
  size_t length = sizeof(supported_values) / sizeof(supported_values[0]);
  size_t i;
  BOOL is_supported_value = FALSE;
  result = TPM_KEY_BITS_Unmarshal(target, buffer, size);
  if (result != TPM_RC_SUCCESS) {
    return result;
  }
  for (i = 0; i < length; ++i) {
    if (*target == supported_values[i]) {
      is_supported_value = TRUE;
      break;
    }
  }
  if (!is_supported_value) {
    return TPM_RC_VALUE;
  }
  return TPM_RC_SUCCESS;
}

UINT16 TPMI_CAMELLIA_KEY_BITS_Marshal(TPMI_CAMELLIA_KEY_BITS* source,
                                      BYTE** buffer,
                                      INT32* size) {
  return TPM_KEY_BITS_Marshal(source, buffer, size);
}

TPM_RC TPMI_CAMELLIA_KEY_BITS_Unmarshal(TPMI_CAMELLIA_KEY_BITS* target,
                                        BYTE** buffer,
                                        INT32* size) {
  TPM_RC result;
  uint16_t supported_values[] = CAMELLIA_KEY_SIZES_BITS;
  size_t length = sizeof(supported_values) / sizeof(supported_values[0]);
  size_t i;
  BOOL is_supported_value = FALSE;
  result = TPM_KEY_BITS_Unmarshal(target, buffer, size);
  if (result != TPM_RC_SUCCESS) {
    return result;
  }
  for (i = 0; i < length; ++i) {
    if (*target == supported_values[i]) {
      is_supported_value = TRUE;
      break;
    }
  }
  if (!is_supported_value) {
    return TPM_RC_VALUE;
  }
  return TPM_RC_SUCCESS;
}

UINT16 TPMU_SYM_KEY_BITS_Marshal(TPMU_SYM_KEY_BITS* source,
                                 BYTE** buffer,
                                 INT32* size,
                                 UINT32 selector) {
  switch (selector) {
#ifdef TPM_ALG_AES
    case TPM_ALG_AES:
      return TPMI_AES_KEY_BITS_Marshal((TPMI_AES_KEY_BITS*)&source->aes, buffer,
                                       size);
#endif
#ifdef TPM_ALG_SM4
    case TPM_ALG_SM4:
      return TPMI_SM4_KEY_BITS_Marshal((TPMI_SM4_KEY_BITS*)&source->sm4, buffer,
                                       size);
#endif
#ifdef TPM_ALG_CAMELLIA
    case TPM_ALG_CAMELLIA:
      return TPMI_CAMELLIA_KEY_BITS_Marshal(
          (TPMI_CAMELLIA_KEY_BITS*)&source->camellia, buffer, size);
#endif
#ifdef TPM_ALG_XOR
    case TPM_ALG_XOR:
      return TPMI_ALG_HASH_Marshal((TPMI_ALG_HASH*)&source->xor_, buffer, size);
#endif
#ifdef TPM_ALG_NULL
    case TPM_ALG_NULL:
      return 0;
#endif
  }
  return 0;
}

TPM_RC TPMU_SYM_KEY_BITS_Unmarshal(TPMU_SYM_KEY_BITS* target,
                                   BYTE** buffer,
                                   INT32* size,
                                   UINT32 selector) {
  switch (selector) {
#ifdef TPM_ALG_AES
    case TPM_ALG_AES:
      return TPMI_AES_KEY_BITS_Unmarshal((TPMI_AES_KEY_BITS*)&target->aes,
                                         buffer, size);
#endif
#ifdef TPM_ALG_SM4
    case TPM_ALG_SM4:
      return TPMI_SM4_KEY_BITS_Unmarshal((TPMI_SM4_KEY_BITS*)&target->sm4,
                                         buffer, size);
#endif
#ifdef TPM_ALG_CAMELLIA
    case TPM_ALG_CAMELLIA:
      return TPMI_CAMELLIA_KEY_BITS_Unmarshal(
          (TPMI_CAMELLIA_KEY_BITS*)&target->camellia, buffer, size);
#endif
#ifdef TPM_ALG_XOR
    case TPM_ALG_XOR:
      return TPMI_ALG_HASH_Unmarshal(&target->xor_, buffer, size, FALSE);
#endif
#ifdef TPM_ALG_NULL
    case TPM_ALG_NULL:
      return TPM_RC_SUCCESS;
#endif
  }
  return TPM_RC_SELECTOR;
}

UINT16 TPMI_ALG_SYM_MODE_Marshal(TPMI_ALG_SYM_MODE* source,
                                 BYTE** buffer,
                                 INT32* size) {
  return uint16_t_Marshal(source, buffer, size);
}

TPM_RC TPMI_ALG_SYM_MODE_Unmarshal(TPMI_ALG_SYM_MODE* target,
                                   BYTE** buffer,
                                   INT32* size,
                                   BOOL allow_conditional_value) {
  TPM_RC result;
  BOOL has_valid_value = FALSE;
  result = uint16_t_Unmarshal(target, buffer, size);
  if ((result != TPM_RC_VALUE) && (result != TPM_RC_SUCCESS)) {
    return result;
  }
  if (*target == TPM_ALG_NULL) {
    return allow_conditional_value ? TPM_RC_SUCCESS : TPM_RC_MODE;
  }
  switch (*target) {
#ifdef TPM_ALG_CTR
    case TPM_ALG_CTR:
#endif
#ifdef TPM_ALG_OFB
    case TPM_ALG_OFB:
#endif
#ifdef TPM_ALG_CBC
    case TPM_ALG_CBC:
#endif
#ifdef TPM_ALG_CFB
    case TPM_ALG_CFB:
#endif
#ifdef TPM_ALG_ECB
    case TPM_ALG_ECB:
#endif
      has_valid_value = TRUE;
      break;
  }
  if (!has_valid_value) {
    return TPM_RC_MODE;
  }
  return TPM_RC_SUCCESS;
}

UINT16 TPMU_SYM_MODE_Marshal(TPMU_SYM_MODE* source,
                             BYTE** buffer,
                             INT32* size,
                             UINT32 selector) {
  switch (selector) {
#ifdef TPM_ALG_AES
    case TPM_ALG_AES:
      return TPMI_ALG_SYM_MODE_Marshal((TPMI_ALG_SYM_MODE*)&source->aes, buffer,
                                       size);
#endif
#ifdef TPM_ALG_SM4
    case TPM_ALG_SM4:
      return TPMI_ALG_SYM_MODE_Marshal((TPMI_ALG_SYM_MODE*)&source->sm4, buffer,
                                       size);
#endif
#ifdef TPM_ALG_CAMELLIA
    case TPM_ALG_CAMELLIA:
      return TPMI_ALG_SYM_MODE_Marshal((TPMI_ALG_SYM_MODE*)&source->camellia,
                                       buffer, size);
#endif
#ifdef TPM_ALG_XOR
    case TPM_ALG_XOR:
      return 0;
#endif
#ifdef TPM_ALG_NULL
    case TPM_ALG_NULL:
      return 0;
#endif
  }
  return 0;
}

TPM_RC TPMU_SYM_MODE_Unmarshal(TPMU_SYM_MODE* target,
                               BYTE** buffer,
                               INT32* size,
                               UINT32 selector) {
  switch (selector) {
#ifdef TPM_ALG_AES
    case TPM_ALG_AES:
      return TPMI_ALG_SYM_MODE_Unmarshal(&target->aes, buffer, size, FALSE);
#endif
#ifdef TPM_ALG_SM4
    case TPM_ALG_SM4:
      return TPMI_ALG_SYM_MODE_Unmarshal(&target->sm4, buffer, size, FALSE);
#endif
#ifdef TPM_ALG_CAMELLIA
    case TPM_ALG_CAMELLIA:
      return TPMI_ALG_SYM_MODE_Unmarshal(&target->camellia, buffer, size,
                                         FALSE);
#endif
#ifdef TPM_ALG_XOR
    case TPM_ALG_XOR:
      return TPM_RC_SUCCESS;
#endif
#ifdef TPM_ALG_NULL
    case TPM_ALG_NULL:
      return TPM_RC_SUCCESS;
#endif
  }
  return TPM_RC_SELECTOR;
}

UINT16 TPMI_ALG_SYM_OBJECT_Marshal(TPMI_ALG_SYM_OBJECT* source,
                                   BYTE** buffer,
                                   INT32* size) {
  return uint16_t_Marshal(source, buffer, size);
}

TPM_RC TPMI_ALG_SYM_OBJECT_Unmarshal(TPMI_ALG_SYM_OBJECT* target,
                                     BYTE** buffer,
                                     INT32* size,
                                     BOOL allow_conditional_value) {
  TPM_RC result;
  BOOL has_valid_value = FALSE;
  result = uint16_t_Unmarshal(target, buffer, size);
  if ((result != TPM_RC_VALUE) && (result != TPM_RC_SUCCESS)) {
    return result;
  }
  if (*target == TPM_ALG_NULL) {
    return allow_conditional_value ? TPM_RC_SUCCESS : TPM_RC_SYMMETRIC;
  }
  switch (*target) {
#ifdef TPM_ALG_AES
    case TPM_ALG_AES:
#endif
#ifdef TPM_ALG_SM4
    case TPM_ALG_SM4:
#endif
#ifdef TPM_ALG_CAMELLIA
    case TPM_ALG_CAMELLIA:
#endif
      has_valid_value = TRUE;
      break;
  }
  if (!has_valid_value) {
    return TPM_RC_SYMMETRIC;
  }
  return TPM_RC_SUCCESS;
}

UINT16 TPMT_SYM_DEF_OBJECT_Marshal(TPMT_SYM_DEF_OBJECT* source,
                                   BYTE** buffer,
                                   INT32* size) {
  UINT16 total_size = 0;
  total_size += TPMI_ALG_SYM_OBJECT_Marshal(&source->algorithm, buffer, size);
  total_size += TPMU_SYM_KEY_BITS_Marshal(&source->keyBits, buffer, size,
                                          source->algorithm);
  total_size +=
      TPMU_SYM_MODE_Marshal(&source->mode, buffer, size, source->algorithm);
  return total_size;
}

TPM_RC TPMT_SYM_DEF_OBJECT_Unmarshal(TPMT_SYM_DEF_OBJECT* target,
                                     BYTE** buffer,
                                     INT32* size) {
  TPM_RC result;
  result =
      TPMI_ALG_SYM_OBJECT_Unmarshal(&target->algorithm, buffer, size, TRUE);
  if (result != TPM_RC_SUCCESS) {
    return result;
  }
  result = TPMU_SYM_KEY_BITS_Unmarshal(&target->keyBits, buffer, size,
                                       target->algorithm);
  if (result != TPM_RC_SUCCESS) {
    return result;
  }
  result =
      TPMU_SYM_MODE_Unmarshal(&target->mode, buffer, size, target->algorithm);
  if (result != TPM_RC_SUCCESS) {
    return result;
  }
  return TPM_RC_SUCCESS;
}

UINT16 TPMI_ALG_RSA_SCHEME_Marshal(TPMI_ALG_RSA_SCHEME* source,
                                   BYTE** buffer,
                                   INT32* size) {
  return uint16_t_Marshal(source, buffer, size);
}

TPM_RC TPMI_ALG_RSA_SCHEME_Unmarshal(TPMI_ALG_RSA_SCHEME* target,
                                     BYTE** buffer,
                                     INT32* size,
                                     BOOL allow_conditional_value) {
  TPM_RC result;
  BOOL has_valid_value = FALSE;
  result = uint16_t_Unmarshal(target, buffer, size);
  if ((result != TPM_RC_VALUE) && (result != TPM_RC_SUCCESS)) {
    return result;
  }
  if (*target == TPM_ALG_NULL) {
    return allow_conditional_value ? TPM_RC_SUCCESS : TPM_RC_VALUE;
  }
  switch (*target) {
#ifdef TPM_ALG_RSAES
    case TPM_ALG_RSAES:
#endif
#ifdef TPM_ALG_OAEP
    case TPM_ALG_OAEP:
#endif
#ifdef TPM_ALG_RSASSA
    case TPM_ALG_RSASSA:
#endif
#ifdef TPM_ALG_RSAPSS
    case TPM_ALG_RSAPSS:
#endif
      has_valid_value = TRUE;
      break;
  }
  if (!has_valid_value) {
    return TPM_RC_VALUE;
  }
  return TPM_RC_SUCCESS;
}

UINT16 TPMS_SCHEME_HASH_Marshal(TPMS_SCHEME_HASH* source,
                                BYTE** buffer,
                                INT32* size) {
  UINT16 total_size = 0;
  total_size += TPMI_ALG_HASH_Marshal(&source->hashAlg, buffer, size);
  return total_size;
}

TPM_RC TPMS_SCHEME_HASH_Unmarshal(TPMS_SCHEME_HASH* target,
                                  BYTE** buffer,
                                  INT32* size) {
  TPM_RC result;
  result = TPMI_ALG_HASH_Unmarshal(&target->hashAlg, buffer, size, FALSE);
  if (result != TPM_RC_SUCCESS) {
    return result;
  }
  return TPM_RC_SUCCESS;
}

UINT16 TPMS_SIG_SCHEME_RSAPSS_Marshal(TPMS_SIG_SCHEME_RSAPSS* source,
                                      BYTE** buffer,
                                      INT32* size) {
  return TPMS_SCHEME_HASH_Marshal(source, buffer, size);
}

TPM_RC TPMS_SIG_SCHEME_RSAPSS_Unmarshal(TPMS_SIG_SCHEME_RSAPSS* target,
                                        BYTE** buffer,
                                        INT32* size) {
  return TPMS_SCHEME_HASH_Unmarshal(target, buffer, size);
}

UINT16 TPMS_SIG_SCHEME_SM2_Marshal(TPMS_SIG_SCHEME_SM2* source,
                                   BYTE** buffer,
                                   INT32* size) {
  return TPMS_SCHEME_HASH_Marshal(source, buffer, size);
}

TPM_RC TPMS_SIG_SCHEME_SM2_Unmarshal(TPMS_SIG_SCHEME_SM2* target,
                                     BYTE** buffer,
                                     INT32* size) {
  return TPMS_SCHEME_HASH_Unmarshal(target, buffer, size);
}

UINT16 TPMS_SIG_SCHEME_ECSCHNORR_Marshal(TPMS_SIG_SCHEME_ECSCHNORR* source,
                                         BYTE** buffer,
                                         INT32* size) {
  return TPMS_SCHEME_HASH_Marshal(source, buffer, size);
}

TPM_RC TPMS_SIG_SCHEME_ECSCHNORR_Unmarshal(TPMS_SIG_SCHEME_ECSCHNORR* target,
                                           BYTE** buffer,
                                           INT32* size) {
  return TPMS_SCHEME_HASH_Unmarshal(target, buffer, size);
}

UINT16 TPMS_SCHEME_ECDAA_Marshal(TPMS_SCHEME_ECDAA* source,
                                 BYTE** buffer,
                                 INT32* size) {
  UINT16 total_size = 0;
  total_size += TPMI_ALG_HASH_Marshal(&source->hashAlg, buffer, size);
  total_size += UINT16_Marshal(&source->count, buffer, size);
  return total_size;
}

TPM_RC TPMS_SCHEME_ECDAA_Unmarshal(TPMS_SCHEME_ECDAA* target,
                                   BYTE** buffer,
                                   INT32* size) {
  TPM_RC result;
  result = TPMI_ALG_HASH_Unmarshal(&target->hashAlg, buffer, size, FALSE);
  if (result != TPM_RC_SUCCESS) {
    return result;
  }
  result = UINT16_Unmarshal(&target->count, buffer, size);
  if (result != TPM_RC_SUCCESS) {
    return result;
  }
  return TPM_RC_SUCCESS;
}

UINT16 TPMS_SIG_SCHEME_ECDAA_Marshal(TPMS_SIG_SCHEME_ECDAA* source,
                                     BYTE** buffer,
                                     INT32* size) {
  return TPMS_SCHEME_ECDAA_Marshal(source, buffer, size);
}

TPM_RC TPMS_SIG_SCHEME_ECDAA_Unmarshal(TPMS_SIG_SCHEME_ECDAA* target,
                                       BYTE** buffer,
                                       INT32* size) {
  return TPMS_SCHEME_ECDAA_Unmarshal(target, buffer, size);
}

UINT16 TPMS_KEY_SCHEME_ECDH_Marshal(TPMS_KEY_SCHEME_ECDH* source,
                                    BYTE** buffer,
                                    INT32* size) {
  return TPMS_SCHEME_HASH_Marshal(source, buffer, size);
}

TPM_RC TPMS_KEY_SCHEME_ECDH_Unmarshal(TPMS_KEY_SCHEME_ECDH* target,
                                      BYTE** buffer,
                                      INT32* size) {
  return TPMS_SCHEME_HASH_Unmarshal(target, buffer, size);
}

UINT16 TPMS_KEY_SCHEME_ECMQV_Marshal(TPMS_KEY_SCHEME_ECMQV* source,
                                     BYTE** buffer,
                                     INT32* size) {
  return TPMS_SCHEME_HASH_Marshal(source, buffer, size);
}

TPM_RC TPMS_KEY_SCHEME_ECMQV_Unmarshal(TPMS_KEY_SCHEME_ECMQV* target,
                                       BYTE** buffer,
                                       INT32* size) {
  return TPMS_SCHEME_HASH_Unmarshal(target, buffer, size);
}

UINT16 TPMS_SIG_SCHEME_RSASSA_Marshal(TPMS_SIG_SCHEME_RSASSA* source,
                                      BYTE** buffer,
                                      INT32* size) {
  return TPMS_SCHEME_HASH_Marshal(source, buffer, size);
}

TPM_RC TPMS_SIG_SCHEME_RSASSA_Unmarshal(TPMS_SIG_SCHEME_RSASSA* target,
                                        BYTE** buffer,
                                        INT32* size) {
  return TPMS_SCHEME_HASH_Unmarshal(target, buffer, size);
}

UINT16 TPMS_ENC_SCHEME_OAEP_Marshal(TPMS_ENC_SCHEME_OAEP* source,
                                    BYTE** buffer,
                                    INT32* size) {
  return TPMS_SCHEME_HASH_Marshal(source, buffer, size);
}

TPM_RC TPMS_ENC_SCHEME_OAEP_Unmarshal(TPMS_ENC_SCHEME_OAEP* target,
                                      BYTE** buffer,
                                      INT32* size) {
  return TPMS_SCHEME_HASH_Unmarshal(target, buffer, size);
}

UINT16 TPMS_EMPTY_Marshal(TPMS_EMPTY* source, BYTE** buffer, INT32* size) {
  UINT16 total_size = 0;
  return total_size;
}

TPM_RC TPMS_EMPTY_Unmarshal(TPMS_EMPTY* target, BYTE** buffer, INT32* size) {
  TPM_RC result;
  (void)result;

  return TPM_RC_SUCCESS;
}

UINT16 TPMS_ENC_SCHEME_RSAES_Marshal(TPMS_ENC_SCHEME_RSAES* source,
                                     BYTE** buffer,
                                     INT32* size) {
  return TPMS_EMPTY_Marshal(source, buffer, size);
}

TPM_RC TPMS_ENC_SCHEME_RSAES_Unmarshal(TPMS_ENC_SCHEME_RSAES* target,
                                       BYTE** buffer,
                                       INT32* size) {
  return TPMS_EMPTY_Unmarshal(target, buffer, size);
}

UINT16 TPMS_SIG_SCHEME_ECDSA_Marshal(TPMS_SIG_SCHEME_ECDSA* source,
                                     BYTE** buffer,
                                     INT32* size) {
  return TPMS_SCHEME_HASH_Marshal(source, buffer, size);
}

TPM_RC TPMS_SIG_SCHEME_ECDSA_Unmarshal(TPMS_SIG_SCHEME_ECDSA* target,
                                       BYTE** buffer,
                                       INT32* size) {
  return TPMS_SCHEME_HASH_Unmarshal(target, buffer, size);
}

UINT16 TPMU_ASYM_SCHEME_Marshal(TPMU_ASYM_SCHEME* source,
                                BYTE** buffer,
                                INT32* size,
                                UINT32 selector) {
  switch (selector) {
#ifdef TPM_ALG_ECDH
    case TPM_ALG_ECDH:
      return TPMS_KEY_SCHEME_ECDH_Marshal((TPMS_KEY_SCHEME_ECDH*)&source->ecdh,
                                          buffer, size);
#endif
#ifdef TPM_ALG_ECMQV
    case TPM_ALG_ECMQV:
      return TPMS_KEY_SCHEME_ECMQV_Marshal(
          (TPMS_KEY_SCHEME_ECMQV*)&source->ecmqv, buffer, size);
#endif
#ifdef TPM_ALG_RSASSA
    case TPM_ALG_RSASSA:
      return TPMS_SIG_SCHEME_RSASSA_Marshal(
          (TPMS_SIG_SCHEME_RSASSA*)&source->rsassa, buffer, size);
#endif
#ifdef TPM_ALG_RSAPSS
    case TPM_ALG_RSAPSS:
      return TPMS_SIG_SCHEME_RSAPSS_Marshal(
          (TPMS_SIG_SCHEME_RSAPSS*)&source->rsapss, buffer, size);
#endif
#ifdef TPM_ALG_ECDSA
    case TPM_ALG_ECDSA:
      return TPMS_SIG_SCHEME_ECDSA_Marshal(
          (TPMS_SIG_SCHEME_ECDSA*)&source->ecdsa, buffer, size);
#endif
#ifdef TPM_ALG_ECDAA
    case TPM_ALG_ECDAA:
      return TPMS_SIG_SCHEME_ECDAA_Marshal(
          (TPMS_SIG_SCHEME_ECDAA*)&source->ecdaa, buffer, size);
#endif
#ifdef TPM_ALG_SM2
    case TPM_ALG_SM2:
      return TPMS_SIG_SCHEME_SM2_Marshal((TPMS_SIG_SCHEME_SM2*)&source->sm2,
                                         buffer, size);
#endif
#ifdef TPM_ALG_ECSCHNORR
    case TPM_ALG_ECSCHNORR:
      return TPMS_SIG_SCHEME_ECSCHNORR_Marshal(
          (TPMS_SIG_SCHEME_ECSCHNORR*)&source->ecschnorr, buffer, size);
#endif
#ifdef TPM_ALG_RSAES
    case TPM_ALG_RSAES:
      return TPMS_ENC_SCHEME_RSAES_Marshal(
          (TPMS_ENC_SCHEME_RSAES*)&source->rsaes, buffer, size);
#endif
#ifdef TPM_ALG_OAEP
    case TPM_ALG_OAEP:
      return TPMS_ENC_SCHEME_OAEP_Marshal((TPMS_ENC_SCHEME_OAEP*)&source->oaep,
                                          buffer, size);
#endif
#ifdef TPM_ALG_NULL
    case TPM_ALG_NULL:
      return 0;
#endif
  }
  return 0;
}

TPM_RC TPMU_ASYM_SCHEME_Unmarshal(TPMU_ASYM_SCHEME* target,
                                  BYTE** buffer,
                                  INT32* size,
                                  UINT32 selector) {
  switch (selector) {
#ifdef TPM_ALG_ECDH
    case TPM_ALG_ECDH:
      return TPMS_KEY_SCHEME_ECDH_Unmarshal(
          (TPMS_KEY_SCHEME_ECDH*)&target->ecdh, buffer, size);
#endif
#ifdef TPM_ALG_ECMQV
    case TPM_ALG_ECMQV:
      return TPMS_KEY_SCHEME_ECMQV_Unmarshal(
          (TPMS_KEY_SCHEME_ECMQV*)&target->ecmqv, buffer, size);
#endif
#ifdef TPM_ALG_RSASSA
    case TPM_ALG_RSASSA:
      return TPMS_SIG_SCHEME_RSASSA_Unmarshal(
          (TPMS_SIG_SCHEME_RSASSA*)&target->rsassa, buffer, size);
#endif
#ifdef TPM_ALG_RSAPSS
    case TPM_ALG_RSAPSS:
      return TPMS_SIG_SCHEME_RSAPSS_Unmarshal(
          (TPMS_SIG_SCHEME_RSAPSS*)&target->rsapss, buffer, size);
#endif
#ifdef TPM_ALG_ECDSA
    case TPM_ALG_ECDSA:
      return TPMS_SIG_SCHEME_ECDSA_Unmarshal(
          (TPMS_SIG_SCHEME_ECDSA*)&target->ecdsa, buffer, size);
#endif
#ifdef TPM_ALG_ECDAA
    case TPM_ALG_ECDAA:
      return TPMS_SIG_SCHEME_ECDAA_Unmarshal(
          (TPMS_SIG_SCHEME_ECDAA*)&target->ecdaa, buffer, size);
#endif
#ifdef TPM_ALG_SM2
    case TPM_ALG_SM2:
      return TPMS_SIG_SCHEME_SM2_Unmarshal((TPMS_SIG_SCHEME_SM2*)&target->sm2,
                                           buffer, size);
#endif
#ifdef TPM_ALG_ECSCHNORR
    case TPM_ALG_ECSCHNORR:
      return TPMS_SIG_SCHEME_ECSCHNORR_Unmarshal(
          (TPMS_SIG_SCHEME_ECSCHNORR*)&target->ecschnorr, buffer, size);
#endif
#ifdef TPM_ALG_RSAES
    case TPM_ALG_RSAES:
      return TPMS_ENC_SCHEME_RSAES_Unmarshal(
          (TPMS_ENC_SCHEME_RSAES*)&target->rsaes, buffer, size);
#endif
#ifdef TPM_ALG_OAEP
    case TPM_ALG_OAEP:
      return TPMS_ENC_SCHEME_OAEP_Unmarshal(
          (TPMS_ENC_SCHEME_OAEP*)&target->oaep, buffer, size);
#endif
#ifdef TPM_ALG_NULL
    case TPM_ALG_NULL:
      return TPM_RC_SUCCESS;
#endif
  }
  return TPM_RC_SELECTOR;
}

UINT16 TPMT_RSA_SCHEME_Marshal(TPMT_RSA_SCHEME* source,
                               BYTE** buffer,
                               INT32* size) {
  UINT16 total_size = 0;
  total_size += TPMI_ALG_RSA_SCHEME_Marshal(&source->scheme, buffer, size);
  total_size +=
      TPMU_ASYM_SCHEME_Marshal(&source->details, buffer, size, source->scheme);
  return total_size;
}

TPM_RC TPMT_RSA_SCHEME_Unmarshal(TPMT_RSA_SCHEME* target,
                                 BYTE** buffer,
                                 INT32* size) {
  TPM_RC result;
  result = TPMI_ALG_RSA_SCHEME_Unmarshal(&target->scheme, buffer, size, TRUE);
  if (result != TPM_RC_SUCCESS) {
    return result;
  }
  result = TPMU_ASYM_SCHEME_Unmarshal(&target->details, buffer, size,
                                      target->scheme);
  if (result != TPM_RC_SUCCESS) {
    return result;
  }
  return TPM_RC_SUCCESS;
}

UINT16 TPMI_RSA_KEY_BITS_Marshal(TPMI_RSA_KEY_BITS* source,
                                 BYTE** buffer,
                                 INT32* size) {
  return uint16_t_Marshal(source, buffer, size);
}

TPM_RC TPMI_RSA_KEY_BITS_Unmarshal(TPMI_RSA_KEY_BITS* target,
                                   BYTE** buffer,
                                   INT32* size) {
  TPM_RC result;
  uint16_t supported_values[] = RSA_KEY_SIZES_BITS;
  size_t length = sizeof(supported_values) / sizeof(supported_values[0]);
  size_t i;
  BOOL is_supported_value = FALSE;
  result = uint16_t_Unmarshal(target, buffer, size);
  if (result != TPM_RC_SUCCESS) {
    return result;
  }
  for (i = 0; i < length; ++i) {
    if (*target == supported_values[i]) {
      is_supported_value = TRUE;
      break;
    }
  }
  if (!is_supported_value) {
    return TPM_RC_VALUE;
  }
  return TPM_RC_SUCCESS;
}

UINT16 TPMS_RSA_PARMS_Marshal(TPMS_RSA_PARMS* source,
                              BYTE** buffer,
                              INT32* size) {
  UINT16 total_size = 0;
  total_size += TPMT_SYM_DEF_OBJECT_Marshal(&source->symmetric, buffer, size);
  total_size += TPMT_RSA_SCHEME_Marshal(&source->scheme, buffer, size);
  total_size += TPMI_RSA_KEY_BITS_Marshal(&source->keyBits, buffer, size);
  total_size += UINT32_Marshal(&source->exponent, buffer, size);
  return total_size;
}

TPM_RC TPMS_RSA_PARMS_Unmarshal(TPMS_RSA_PARMS* target,
                                BYTE** buffer,
                                INT32* size) {
  TPM_RC result;
  result = TPMT_SYM_DEF_OBJECT_Unmarshal(&target->symmetric, buffer, size);
  if (result != TPM_RC_SUCCESS) {
    return result;
  }
  result = TPMT_RSA_SCHEME_Unmarshal(&target->scheme, buffer, size);
  if (result != TPM_RC_SUCCESS) {
    return result;
  }
  result = TPMI_RSA_KEY_BITS_Unmarshal(&target->keyBits, buffer, size);
  if (result != TPM_RC_SUCCESS) {
    return result;
  }
  result = UINT32_Unmarshal(&target->exponent, buffer, size);
  if (result != TPM_RC_SUCCESS) {
    return result;
  }
  return TPM_RC_SUCCESS;
}

UINT16 TPMS_SYMCIPHER_PARMS_Marshal(TPMS_SYMCIPHER_PARMS* source,
                                    BYTE** buffer,
                                    INT32* size) {
  UINT16 total_size = 0;
  total_size += TPMT_SYM_DEF_OBJECT_Marshal(&source->sym, buffer, size);
  return total_size;
}

TPM_RC TPMS_SYMCIPHER_PARMS_Unmarshal(TPMS_SYMCIPHER_PARMS* target,
                                      BYTE** buffer,
                                      INT32* size) {
  TPM_RC result;
  result = TPMT_SYM_DEF_OBJECT_Unmarshal(&target->sym, buffer, size);
  if (result != TPM_RC_SUCCESS) {
    return result;
  }
  return TPM_RC_SUCCESS;
}

UINT16 TPMI_ALG_ASYM_SCHEME_Marshal(TPMI_ALG_ASYM_SCHEME* source,
                                    BYTE** buffer,
                                    INT32* size) {
  return uint16_t_Marshal(source, buffer, size);
}

TPM_RC TPMI_ALG_ASYM_SCHEME_Unmarshal(TPMI_ALG_ASYM_SCHEME* target,
                                      BYTE** buffer,
                                      INT32* size,
                                      BOOL allow_conditional_value) {
  TPM_RC result;
  BOOL has_valid_value = FALSE;
  result = uint16_t_Unmarshal(target, buffer, size);
  if ((result != TPM_RC_VALUE) && (result != TPM_RC_SUCCESS)) {
    return result;
  }
  if (*target == TPM_ALG_NULL) {
    return allow_conditional_value ? TPM_RC_SUCCESS : TPM_RC_VALUE;
  }
  switch (*target) {
#ifdef TPM_ALG_ECDH
    case TPM_ALG_ECDH:
#endif
#ifdef TPM_ALG_ECMQV
    case TPM_ALG_ECMQV:
#endif
#ifdef TPM_ALG_RSASSA
    case TPM_ALG_RSASSA:
#endif
#ifdef TPM_ALG_RSAPSS
    case TPM_ALG_RSAPSS:
#endif
#ifdef TPM_ALG_ECDSA
    case TPM_ALG_ECDSA:
#endif
#ifdef TPM_ALG_ECDAA
    case TPM_ALG_ECDAA:
#endif
#ifdef TPM_ALG_SM2
    case TPM_ALG_SM2:
#endif
#ifdef TPM_ALG_ECSCHNORR
    case TPM_ALG_ECSCHNORR:
#endif
#ifdef TPM_ALG_RSAES
    case TPM_ALG_RSAES:
#endif
#ifdef TPM_ALG_OAEP
    case TPM_ALG_OAEP:
#endif
      has_valid_value = TRUE;
      break;
  }
  if (!has_valid_value) {
    return TPM_RC_VALUE;
  }
  return TPM_RC_SUCCESS;
}

UINT16 TPMT_ASYM_SCHEME_Marshal(TPMT_ASYM_SCHEME* source,
                                BYTE** buffer,
                                INT32* size) {
  UINT16 total_size = 0;
  total_size += TPMI_ALG_ASYM_SCHEME_Marshal(&source->scheme, buffer, size);
  total_size +=
      TPMU_ASYM_SCHEME_Marshal(&source->details, buffer, size, source->scheme);
  return total_size;
}

TPM_RC TPMT_ASYM_SCHEME_Unmarshal(TPMT_ASYM_SCHEME* target,
                                  BYTE** buffer,
                                  INT32* size) {
  TPM_RC result;
  result = TPMI_ALG_ASYM_SCHEME_Unmarshal(&target->scheme, buffer, size, TRUE);
  if (result != TPM_RC_SUCCESS) {
    return result;
  }
  result = TPMU_ASYM_SCHEME_Unmarshal(&target->details, buffer, size,
                                      target->scheme);
  if (result != TPM_RC_SUCCESS) {
    return result;
  }
  return TPM_RC_SUCCESS;
}

UINT16 TPMS_ASYM_PARMS_Marshal(TPMS_ASYM_PARMS* source,
                               BYTE** buffer,
                               INT32* size) {
  UINT16 total_size = 0;
  total_size += TPMT_SYM_DEF_OBJECT_Marshal(&source->symmetric, buffer, size);
  total_size += TPMT_ASYM_SCHEME_Marshal(&source->scheme, buffer, size);
  return total_size;
}

TPM_RC TPMS_ASYM_PARMS_Unmarshal(TPMS_ASYM_PARMS* target,
                                 BYTE** buffer,
                                 INT32* size) {
  TPM_RC result;
  result = TPMT_SYM_DEF_OBJECT_Unmarshal(&target->symmetric, buffer, size);
  if (result != TPM_RC_SUCCESS) {
    return result;
  }
  result = TPMT_ASYM_SCHEME_Unmarshal(&target->scheme, buffer, size);
  if (result != TPM_RC_SUCCESS) {
    return result;
  }
  return TPM_RC_SUCCESS;
}

UINT16 TPMI_ALG_KDF_Marshal(TPMI_ALG_KDF* source, BYTE** buffer, INT32* size) {
  return uint16_t_Marshal(source, buffer, size);
}

TPM_RC TPMI_ALG_KDF_Unmarshal(TPMI_ALG_KDF* target,
                              BYTE** buffer,
                              INT32* size,
                              BOOL allow_conditional_value) {
  TPM_RC result;
  BOOL has_valid_value = FALSE;
  result = uint16_t_Unmarshal(target, buffer, size);
  if ((result != TPM_RC_VALUE) && (result != TPM_RC_SUCCESS)) {
    return result;
  }
  if (*target == TPM_ALG_NULL) {
    return allow_conditional_value ? TPM_RC_SUCCESS : TPM_RC_KDF;
  }
  switch (*target) {
#ifdef TPM_ALG_MGF1
    case TPM_ALG_MGF1:
#endif
#ifdef TPM_ALG_KDF1_SP800_56A
    case TPM_ALG_KDF1_SP800_56A:
#endif
#ifdef TPM_ALG_KDF2
    case TPM_ALG_KDF2:
#endif
#ifdef TPM_ALG_KDF1_SP800_108
    case TPM_ALG_KDF1_SP800_108:
#endif
      has_valid_value = TRUE;
      break;
  }
  if (!has_valid_value) {
    return TPM_RC_KDF;
  }
  return TPM_RC_SUCCESS;
}

UINT16 TPMS_SCHEME_KDF1_SP800_108_Marshal(TPMS_SCHEME_KDF1_SP800_108* source,
                                          BYTE** buffer,
                                          INT32* size) {
  return TPMS_SCHEME_HASH_Marshal(source, buffer, size);
}

TPM_RC TPMS_SCHEME_KDF1_SP800_108_Unmarshal(TPMS_SCHEME_KDF1_SP800_108* target,
                                            BYTE** buffer,
                                            INT32* size) {
  return TPMS_SCHEME_HASH_Unmarshal(target, buffer, size);
}

UINT16 TPMS_SCHEME_KDF2_Marshal(TPMS_SCHEME_KDF2* source,
                                BYTE** buffer,
                                INT32* size) {
  return TPMS_SCHEME_HASH_Marshal(source, buffer, size);
}

TPM_RC TPMS_SCHEME_KDF2_Unmarshal(TPMS_SCHEME_KDF2* target,
                                  BYTE** buffer,
                                  INT32* size) {
  return TPMS_SCHEME_HASH_Unmarshal(target, buffer, size);
}

UINT16 TPMS_SCHEME_KDF1_SP800_56A_Marshal(TPMS_SCHEME_KDF1_SP800_56A* source,
                                          BYTE** buffer,
                                          INT32* size) {
  return TPMS_SCHEME_HASH_Marshal(source, buffer, size);
}

TPM_RC TPMS_SCHEME_KDF1_SP800_56A_Unmarshal(TPMS_SCHEME_KDF1_SP800_56A* target,
                                            BYTE** buffer,
                                            INT32* size) {
  return TPMS_SCHEME_HASH_Unmarshal(target, buffer, size);
}

UINT16 TPMS_SCHEME_MGF1_Marshal(TPMS_SCHEME_MGF1* source,
                                BYTE** buffer,
                                INT32* size) {
  return TPMS_SCHEME_HASH_Marshal(source, buffer, size);
}

TPM_RC TPMS_SCHEME_MGF1_Unmarshal(TPMS_SCHEME_MGF1* target,
                                  BYTE** buffer,
                                  INT32* size) {
  return TPMS_SCHEME_HASH_Unmarshal(target, buffer, size);
}

UINT16 TPMU_KDF_SCHEME_Marshal(TPMU_KDF_SCHEME* source,
                               BYTE** buffer,
                               INT32* size,
                               UINT32 selector) {
  switch (selector) {
#ifdef TPM_ALG_MGF1
    case TPM_ALG_MGF1:
      return TPMS_SCHEME_MGF1_Marshal((TPMS_SCHEME_MGF1*)&source->mgf1, buffer,
                                      size);
#endif
#ifdef TPM_ALG_KDF1_SP800_56A
    case TPM_ALG_KDF1_SP800_56A:
      return TPMS_SCHEME_KDF1_SP800_56A_Marshal(
          (TPMS_SCHEME_KDF1_SP800_56A*)&source->kdf1_sp800_56a, buffer, size);
#endif
#ifdef TPM_ALG_KDF2
    case TPM_ALG_KDF2:
      return TPMS_SCHEME_KDF2_Marshal((TPMS_SCHEME_KDF2*)&source->kdf2, buffer,
                                      size);
#endif
#ifdef TPM_ALG_KDF1_SP800_108
    case TPM_ALG_KDF1_SP800_108:
      return TPMS_SCHEME_KDF1_SP800_108_Marshal(
          (TPMS_SCHEME_KDF1_SP800_108*)&source->kdf1_sp800_108, buffer, size);
#endif
#ifdef TPM_ALG_NULL
    case TPM_ALG_NULL:
      return 0;
#endif
  }
  return 0;
}

TPM_RC TPMU_KDF_SCHEME_Unmarshal(TPMU_KDF_SCHEME* target,
                                 BYTE** buffer,
                                 INT32* size,
                                 UINT32 selector) {
  switch (selector) {
#ifdef TPM_ALG_MGF1
    case TPM_ALG_MGF1:
      return TPMS_SCHEME_MGF1_Unmarshal((TPMS_SCHEME_MGF1*)&target->mgf1,
                                        buffer, size);
#endif
#ifdef TPM_ALG_KDF1_SP800_56A
    case TPM_ALG_KDF1_SP800_56A:
      return TPMS_SCHEME_KDF1_SP800_56A_Unmarshal(
          (TPMS_SCHEME_KDF1_SP800_56A*)&target->kdf1_sp800_56a, buffer, size);
#endif
#ifdef TPM_ALG_KDF2
    case TPM_ALG_KDF2:
      return TPMS_SCHEME_KDF2_Unmarshal((TPMS_SCHEME_KDF2*)&target->kdf2,
                                        buffer, size);
#endif
#ifdef TPM_ALG_KDF1_SP800_108
    case TPM_ALG_KDF1_SP800_108:
      return TPMS_SCHEME_KDF1_SP800_108_Unmarshal(
          (TPMS_SCHEME_KDF1_SP800_108*)&target->kdf1_sp800_108, buffer, size);
#endif
#ifdef TPM_ALG_NULL
    case TPM_ALG_NULL:
      return TPM_RC_SUCCESS;
#endif
  }
  return TPM_RC_SELECTOR;
}

UINT16 TPMT_KDF_SCHEME_Marshal(TPMT_KDF_SCHEME* source,
                               BYTE** buffer,
                               INT32* size) {
  UINT16 total_size = 0;
  total_size += TPMI_ALG_KDF_Marshal(&source->scheme, buffer, size);
  total_size +=
      TPMU_KDF_SCHEME_Marshal(&source->details, buffer, size, source->scheme);
  return total_size;
}

TPM_RC TPMT_KDF_SCHEME_Unmarshal(TPMT_KDF_SCHEME* target,
                                 BYTE** buffer,
                                 INT32* size) {
  TPM_RC result;
  result = TPMI_ALG_KDF_Unmarshal(&target->scheme, buffer, size, TRUE);
  if (result != TPM_RC_SUCCESS) {
    return result;
  }
  result =
      TPMU_KDF_SCHEME_Unmarshal(&target->details, buffer, size, target->scheme);
  if (result != TPM_RC_SUCCESS) {
    return result;
  }
  return TPM_RC_SUCCESS;
}

UINT16 TPMI_ALG_ECC_SCHEME_Marshal(TPMI_ALG_ECC_SCHEME* source,
                                   BYTE** buffer,
                                   INT32* size) {
  return uint16_t_Marshal(source, buffer, size);
}

TPM_RC TPMI_ALG_ECC_SCHEME_Unmarshal(TPMI_ALG_ECC_SCHEME* target,
                                     BYTE** buffer,
                                     INT32* size,
                                     BOOL allow_conditional_value) {
  TPM_RC result;
  BOOL has_valid_value = FALSE;
  result = uint16_t_Unmarshal(target, buffer, size);
  if ((result != TPM_RC_VALUE) && (result != TPM_RC_SUCCESS)) {
    return result;
  }
  if (*target == TPM_ALG_NULL) {
    return allow_conditional_value ? TPM_RC_SUCCESS : TPM_RC_SCHEME;
  }
  switch (*target) {
#ifdef TPM_ALG_ECDSA
    case TPM_ALG_ECDSA:
#endif
#ifdef TPM_ALG_ECDAA
    case TPM_ALG_ECDAA:
#endif
#ifdef TPM_ALG_SM2
    case TPM_ALG_SM2:
#endif
#ifdef TPM_ALG_ECSCHNORR
    case TPM_ALG_ECSCHNORR:
#endif
#ifdef TPM_ALG_ECDH
    case TPM_ALG_ECDH:
#endif
#ifdef TPM_ALG_ECMQV
    case TPM_ALG_ECMQV:
#endif
      has_valid_value = TRUE;
      break;
  }
  if (!has_valid_value) {
    return TPM_RC_SCHEME;
  }
  return TPM_RC_SUCCESS;
}

UINT16 TPMT_ECC_SCHEME_Marshal(TPMT_ECC_SCHEME* source,
                               BYTE** buffer,
                               INT32* size) {
  UINT16 total_size = 0;
  total_size += TPMI_ALG_ECC_SCHEME_Marshal(&source->scheme, buffer, size);
  total_size +=
      TPMU_ASYM_SCHEME_Marshal(&source->details, buffer, size, source->scheme);
  return total_size;
}

TPM_RC TPMT_ECC_SCHEME_Unmarshal(TPMT_ECC_SCHEME* target,
                                 BYTE** buffer,
                                 INT32* size) {
  TPM_RC result;
  result = TPMI_ALG_ECC_SCHEME_Unmarshal(&target->scheme, buffer, size, TRUE);
  if (result != TPM_RC_SUCCESS) {
    return result;
  }
  result = TPMU_ASYM_SCHEME_Unmarshal(&target->details, buffer, size,
                                      target->scheme);
  if (result != TPM_RC_SUCCESS) {
    return result;
  }
  return TPM_RC_SUCCESS;
}

UINT16 TPMI_ECC_CURVE_Marshal(TPMI_ECC_CURVE* source,
                              BYTE** buffer,
                              INT32* size) {
  return uint16_t_Marshal(source, buffer, size);
}

TPM_RC TPMI_ECC_CURVE_Unmarshal(TPMI_ECC_CURVE* target,
                                BYTE** buffer,
                                INT32* size) {
  TPM_RC result;
  uint16_t supported_values[] = ECC_CURVES;
  size_t length = sizeof(supported_values) / sizeof(supported_values[0]);
  size_t i;
  BOOL is_supported_value = FALSE;
  result = uint16_t_Unmarshal(target, buffer, size);
  if (result != TPM_RC_SUCCESS) {
    return result;
  }
  for (i = 0; i < length; ++i) {
    if (*target == supported_values[i]) {
      is_supported_value = TRUE;
      break;
    }
  }
  if (!is_supported_value) {
    return TPM_RC_CURVE;
  }
  return TPM_RC_SUCCESS;
}

UINT16 TPMS_ECC_PARMS_Marshal(TPMS_ECC_PARMS* source,
                              BYTE** buffer,
                              INT32* size) {
  UINT16 total_size = 0;
  total_size += TPMT_SYM_DEF_OBJECT_Marshal(&source->symmetric, buffer, size);
  total_size += TPMT_ECC_SCHEME_Marshal(&source->scheme, buffer, size);
  total_size += TPMI_ECC_CURVE_Marshal(&source->curveID, buffer, size);
  total_size += TPMT_KDF_SCHEME_Marshal(&source->kdf, buffer, size);
  return total_size;
}

TPM_RC TPMS_ECC_PARMS_Unmarshal(TPMS_ECC_PARMS* target,
                                BYTE** buffer,
                                INT32* size) {
  TPM_RC result;
  result = TPMT_SYM_DEF_OBJECT_Unmarshal(&target->symmetric, buffer, size);
  if (result != TPM_RC_SUCCESS) {
    return result;
  }
  result = TPMT_ECC_SCHEME_Unmarshal(&target->scheme, buffer, size);
  if (result != TPM_RC_SUCCESS) {
    return result;
  }
  result = TPMI_ECC_CURVE_Unmarshal(&target->curveID, buffer, size);
  if (result != TPM_RC_SUCCESS) {
    return result;
  }
  result = TPMT_KDF_SCHEME_Unmarshal(&target->kdf, buffer, size);
  if (result != TPM_RC_SUCCESS) {
    return result;
  }
  return TPM_RC_SUCCESS;
}

UINT16 TPMI_ALG_KEYEDHASH_SCHEME_Marshal(TPMI_ALG_KEYEDHASH_SCHEME* source,
                                         BYTE** buffer,
                                         INT32* size) {
  return uint16_t_Marshal(source, buffer, size);
}

TPM_RC TPMI_ALG_KEYEDHASH_SCHEME_Unmarshal(TPMI_ALG_KEYEDHASH_SCHEME* target,
                                           BYTE** buffer,
                                           INT32* size,
                                           BOOL allow_conditional_value) {
  TPM_RC result;
  BOOL has_valid_value = FALSE;
  result = uint16_t_Unmarshal(target, buffer, size);
  if ((result != TPM_RC_VALUE) && (result != TPM_RC_SUCCESS)) {
    return result;
  }
  if (*target == TPM_ALG_NULL) {
    return allow_conditional_value ? TPM_RC_SUCCESS : TPM_RC_VALUE;
  }
  switch (*target) {
#ifdef TPM_ALG_HMAC
    case TPM_ALG_HMAC:
#endif
#ifdef TPM_ALG_XOR
    case TPM_ALG_XOR:
#endif
      has_valid_value = TRUE;
      break;
  }
  if (!has_valid_value) {
    return TPM_RC_VALUE;
  }
  return TPM_RC_SUCCESS;
}

UINT16 TPMS_SCHEME_HMAC_Marshal(TPMS_SCHEME_HMAC* source,
                                BYTE** buffer,
                                INT32* size) {
  return TPMS_SCHEME_HASH_Marshal(source, buffer, size);
}

TPM_RC TPMS_SCHEME_HMAC_Unmarshal(TPMS_SCHEME_HMAC* target,
                                  BYTE** buffer,
                                  INT32* size) {
  return TPMS_SCHEME_HASH_Unmarshal(target, buffer, size);
}

UINT16 TPMS_SCHEME_XOR_Marshal(TPMS_SCHEME_XOR* source,
                               BYTE** buffer,
                               INT32* size) {
  UINT16 total_size = 0;
  total_size += TPMI_ALG_HASH_Marshal(&source->hashAlg, buffer, size);
  total_size += TPMI_ALG_KDF_Marshal(&source->kdf, buffer, size);
  return total_size;
}

TPM_RC TPMS_SCHEME_XOR_Unmarshal(TPMS_SCHEME_XOR* target,
                                 BYTE** buffer,
                                 INT32* size) {
  TPM_RC result;
  result = TPMI_ALG_HASH_Unmarshal(&target->hashAlg, buffer, size, TRUE);
  if (result != TPM_RC_SUCCESS) {
    return result;
  }
  result = TPMI_ALG_KDF_Unmarshal(&target->kdf, buffer, size, FALSE);
  if (result != TPM_RC_SUCCESS) {
    return result;
  }
  return TPM_RC_SUCCESS;
}

UINT16 TPMU_SCHEME_KEYEDHASH_Marshal(TPMU_SCHEME_KEYEDHASH* source,
                                     BYTE** buffer,
                                     INT32* size,
                                     UINT32 selector) {
  switch (selector) {
#ifdef TPM_ALG_HMAC
    case TPM_ALG_HMAC:
      return TPMS_SCHEME_HMAC_Marshal((TPMS_SCHEME_HMAC*)&source->hmac, buffer,
                                      size);
#endif
#ifdef TPM_ALG_XOR
    case TPM_ALG_XOR:
      return TPMS_SCHEME_XOR_Marshal((TPMS_SCHEME_XOR*)&source->xor_, buffer,
                                     size);
#endif
#ifdef TPM_ALG_NULL
    case TPM_ALG_NULL:
      return 0;
#endif
  }
  return 0;
}

TPM_RC TPMU_SCHEME_KEYEDHASH_Unmarshal(TPMU_SCHEME_KEYEDHASH* target,
                                       BYTE** buffer,
                                       INT32* size,
                                       UINT32 selector) {
  switch (selector) {
#ifdef TPM_ALG_HMAC
    case TPM_ALG_HMAC:
      return TPMS_SCHEME_HMAC_Unmarshal((TPMS_SCHEME_HMAC*)&target->hmac,
                                        buffer, size);
#endif
#ifdef TPM_ALG_XOR
    case TPM_ALG_XOR:
      return TPMS_SCHEME_XOR_Unmarshal((TPMS_SCHEME_XOR*)&target->xor_, buffer,
                                       size);
#endif
#ifdef TPM_ALG_NULL
    case TPM_ALG_NULL:
      return TPM_RC_SUCCESS;
#endif
  }
  return TPM_RC_SELECTOR;
}

UINT16 TPMT_KEYEDHASH_SCHEME_Marshal(TPMT_KEYEDHASH_SCHEME* source,
                                     BYTE** buffer,
                                     INT32* size) {
  UINT16 total_size = 0;
  total_size +=
      TPMI_ALG_KEYEDHASH_SCHEME_Marshal(&source->scheme, buffer, size);
  total_size += TPMU_SCHEME_KEYEDHASH_Marshal(&source->details, buffer, size,
                                              source->scheme);
  return total_size;
}

TPM_RC TPMT_KEYEDHASH_SCHEME_Unmarshal(TPMT_KEYEDHASH_SCHEME* target,
                                       BYTE** buffer,
                                       INT32* size) {
  TPM_RC result;
  result =
      TPMI_ALG_KEYEDHASH_SCHEME_Unmarshal(&target->scheme, buffer, size, TRUE);
  if (result != TPM_RC_SUCCESS) {
    return result;
  }
  result = TPMU_SCHEME_KEYEDHASH_Unmarshal(&target->details, buffer, size,
                                           target->scheme);
  if (result != TPM_RC_SUCCESS) {
    return result;
  }
  return TPM_RC_SUCCESS;
}

UINT16 TPMS_KEYEDHASH_PARMS_Marshal(TPMS_KEYEDHASH_PARMS* source,
                                    BYTE** buffer,
                                    INT32* size) {
  UINT16 total_size = 0;
  total_size += TPMT_KEYEDHASH_SCHEME_Marshal(&source->scheme, buffer, size);
  return total_size;
}

TPM_RC TPMS_KEYEDHASH_PARMS_Unmarshal(TPMS_KEYEDHASH_PARMS* target,
                                      BYTE** buffer,
                                      INT32* size) {
  TPM_RC result;
  result = TPMT_KEYEDHASH_SCHEME_Unmarshal(&target->scheme, buffer, size);
  if (result != TPM_RC_SUCCESS) {
    return result;
  }
  return TPM_RC_SUCCESS;
}

UINT16 TPMU_PUBLIC_PARMS_Marshal(TPMU_PUBLIC_PARMS* source,
                                 BYTE** buffer,
                                 INT32* size,
                                 UINT32 selector) {
  switch (selector) {
#ifdef TPM_ALG_KEYEDHASH
    case TPM_ALG_KEYEDHASH:
      return TPMS_KEYEDHASH_PARMS_Marshal(
          (TPMS_KEYEDHASH_PARMS*)&source->keyedHashDetail, buffer, size);
#endif
#ifdef TPM_ALG_SYMCIPHER
    case TPM_ALG_SYMCIPHER:
      return TPMS_SYMCIPHER_PARMS_Marshal(
          (TPMS_SYMCIPHER_PARMS*)&source->symDetail, buffer, size);
#endif
#ifdef TPM_ALG_RSA
    case TPM_ALG_RSA:
      return TPMS_RSA_PARMS_Marshal((TPMS_RSA_PARMS*)&source->rsaDetail, buffer,
                                    size);
#endif
#ifdef TPM_ALG_ECC
    case TPM_ALG_ECC:
      return TPMS_ECC_PARMS_Marshal((TPMS_ECC_PARMS*)&source->eccDetail, buffer,
                                    size);
#endif
  }
  return 0;
}

TPM_RC TPMU_PUBLIC_PARMS_Unmarshal(TPMU_PUBLIC_PARMS* target,
                                   BYTE** buffer,
                                   INT32* size,
                                   UINT32 selector) {
  switch (selector) {
#ifdef TPM_ALG_KEYEDHASH
    case TPM_ALG_KEYEDHASH:
      return TPMS_KEYEDHASH_PARMS_Unmarshal(
          (TPMS_KEYEDHASH_PARMS*)&target->keyedHashDetail, buffer, size);
#endif
#ifdef TPM_ALG_SYMCIPHER
    case TPM_ALG_SYMCIPHER:
      return TPMS_SYMCIPHER_PARMS_Unmarshal(
          (TPMS_SYMCIPHER_PARMS*)&target->symDetail, buffer, size);
#endif
#ifdef TPM_ALG_RSA
    case TPM_ALG_RSA:
      return TPMS_RSA_PARMS_Unmarshal((TPMS_RSA_PARMS*)&target->rsaDetail,
                                      buffer, size);
#endif
#ifdef TPM_ALG_ECC
    case TPM_ALG_ECC:
      return TPMS_ECC_PARMS_Unmarshal((TPMS_ECC_PARMS*)&target->eccDetail,
                                      buffer, size);
#endif
  }
  return TPM_RC_SELECTOR;
}

UINT16 TPMT_PUBLIC_Marshal(TPMT_PUBLIC* source, BYTE** buffer, INT32* size) {
  UINT16 total_size = 0;
  total_size += TPMI_ALG_PUBLIC_Marshal(&source->type, buffer, size);
  total_size += TPMI_ALG_HASH_Marshal(&source->nameAlg, buffer, size);
  total_size += TPMA_OBJECT_Marshal(&source->objectAttributes, buffer, size);
  total_size += TPM2B_DIGEST_Marshal(&source->authPolicy, buffer, size);
  total_size += TPMU_PUBLIC_PARMS_Marshal(&source->parameters, buffer, size,
                                          source->type);
  total_size +=
      TPMU_PUBLIC_ID_Marshal(&source->unique, buffer, size, source->type);
  return total_size;
}

TPM_RC TPMT_PUBLIC_Unmarshal(TPMT_PUBLIC* target, BYTE** buffer, INT32* size) {
  TPM_RC result;
  result = TPMI_ALG_PUBLIC_Unmarshal(&target->type, buffer, size);
  if (result != TPM_RC_SUCCESS) {
    return result;
  }
  result = TPMI_ALG_HASH_Unmarshal(&target->nameAlg, buffer, size, TRUE);
  if (result != TPM_RC_SUCCESS) {
    return result;
  }
  result = TPMA_OBJECT_Unmarshal(&target->objectAttributes, buffer, size);
  if (result != TPM_RC_SUCCESS) {
    return result;
  }
  result = TPM2B_DIGEST_Unmarshal(&target->authPolicy, buffer, size);
  if (result != TPM_RC_SUCCESS) {
    return result;
  }
  result = TPMU_PUBLIC_PARMS_Unmarshal(&target->parameters, buffer, size,
                                       target->type);
  if (result != TPM_RC_SUCCESS) {
    return result;
  }
  result =
      TPMU_PUBLIC_ID_Unmarshal(&target->unique, buffer, size, target->type);
  if (result != TPM_RC_SUCCESS) {
    return result;
  }
  return TPM_RC_SUCCESS;
}

UINT16 TPM2B_PUBLIC_Marshal(TPM2B_PUBLIC* source, BYTE** buffer, INT32* size) {
  UINT16 total_size = 0;
  total_size += UINT16_Marshal(&source->t.size, buffer, size);
  total_size += TPMT_PUBLIC_Marshal(&source->t.publicArea, buffer, size);
  {
    BYTE* size_location = *buffer - total_size;
    INT32 size_field_size = sizeof(UINT16);
    UINT16 payload_size = total_size - (UINT16)size_field_size;
    UINT16_Marshal(&payload_size, &size_location, &size_field_size);
  }
  return total_size;
}

TPM_RC TPM2B_PUBLIC_Unmarshal(TPM2B_PUBLIC* target,
                              BYTE** buffer,
                              INT32* size) {
  TPM_RC result;
  UINT32 start_size = *size;
  UINT32 struct_size;
  result = UINT16_Unmarshal(&target->t.size, buffer, size);
  if (result != TPM_RC_SUCCESS) {
    return result;
  }
  if (target->t.size == 0) {
    return TPM_RC_SIZE;
  }
  result = TPMT_PUBLIC_Unmarshal(&target->t.publicArea, buffer, size);
  if (result != TPM_RC_SUCCESS) {
    return result;
  }
  struct_size = start_size - *size - sizeof(target->t.size);
  if (struct_size != target->t.size) {
    return TPM_RC_SIZE;
  }
  return TPM_RC_SUCCESS;
}

UINT16 TPM2B_SENSITIVE_DATA_Marshal(TPM2B_SENSITIVE_DATA* source,
                                    BYTE** buffer,
                                    INT32* size) {
  UINT16 total_size = 0;
  INT32 i;
  total_size += UINT16_Marshal(&source->t.size, buffer, size);
  for (i = 0; i < source->t.size; ++i) {
    total_size += BYTE_Marshal(&source->t.buffer[i], buffer, size);
  }
  return total_size;
}

TPM_RC TPM2B_SENSITIVE_DATA_Unmarshal(TPM2B_SENSITIVE_DATA* target,
                                      BYTE** buffer,
                                      INT32* size) {
  TPM_RC result;
  INT32 i;
  result = UINT16_Unmarshal(&target->t.size, buffer, size);
  if (result != TPM_RC_SUCCESS) {
    return result;
  }
  if (target->t.size == 0) {
    return TPM_RC_SUCCESS;
  }
  if (target->t.size > MAX_SYM_DATA) {
    return TPM_RC_SIZE;
  }
  for (i = 0; i < target->t.size; ++i) {
    result = BYTE_Unmarshal(&target->t.buffer[i], buffer, size);
    if (result != TPM_RC_SUCCESS) {
      return result;
    }
  }
  return TPM_RC_SUCCESS;
}

UINT16 TPM2B_SYM_KEY_Marshal(TPM2B_SYM_KEY* source,
                             BYTE** buffer,
                             INT32* size) {
  UINT16 total_size = 0;
  INT32 i;
  total_size += UINT16_Marshal(&source->t.size, buffer, size);
  for (i = 0; i < source->t.size; ++i) {
    total_size += BYTE_Marshal(&source->t.buffer[i], buffer, size);
  }
  return total_size;
}

TPM_RC TPM2B_SYM_KEY_Unmarshal(TPM2B_SYM_KEY* target,
                               BYTE** buffer,
                               INT32* size) {
  TPM_RC result;
  INT32 i;
  result = UINT16_Unmarshal(&target->t.size, buffer, size);
  if (result != TPM_RC_SUCCESS) {
    return result;
  }
  if (target->t.size == 0) {
    return TPM_RC_SUCCESS;
  }
  if (target->t.size > MAX_SYM_KEY_BYTES) {
    return TPM_RC_SIZE;
  }
  for (i = 0; i < target->t.size; ++i) {
    result = BYTE_Unmarshal(&target->t.buffer[i], buffer, size);
    if (result != TPM_RC_SUCCESS) {
      return result;
    }
  }
  return TPM_RC_SUCCESS;
}

UINT16 TPMU_SENSITIVE_COMPOSITE_Marshal(TPMU_SENSITIVE_COMPOSITE* source,
                                        BYTE** buffer,
                                        INT32* size,
                                        UINT32 selector) {
  switch (selector) {
#ifdef TPM_ALG_RSA
    case TPM_ALG_RSA:
      return TPM2B_PRIVATE_KEY_RSA_Marshal((TPM2B_PRIVATE_KEY_RSA*)&source->rsa,
                                           buffer, size);
#endif
#ifdef TPM_ALG_ECC
    case TPM_ALG_ECC:
      return TPM2B_ECC_PARAMETER_Marshal((TPM2B_ECC_PARAMETER*)&source->ecc,
                                         buffer, size);
#endif
#ifdef TPM_ALG_KEYEDHASH
    case TPM_ALG_KEYEDHASH:
      return TPM2B_SENSITIVE_DATA_Marshal((TPM2B_SENSITIVE_DATA*)&source->bits,
                                          buffer, size);
#endif
#ifdef TPM_ALG_SYMCIPHER
    case TPM_ALG_SYMCIPHER:
      return TPM2B_SYM_KEY_Marshal((TPM2B_SYM_KEY*)&source->sym, buffer, size);
#endif
  }
  return 0;
}

TPM_RC TPMU_SENSITIVE_COMPOSITE_Unmarshal(TPMU_SENSITIVE_COMPOSITE* target,
                                          BYTE** buffer,
                                          INT32* size,
                                          UINT32 selector) {
  switch (selector) {
#ifdef TPM_ALG_RSA
    case TPM_ALG_RSA:
      return TPM2B_PRIVATE_KEY_RSA_Unmarshal(
          (TPM2B_PRIVATE_KEY_RSA*)&target->rsa, buffer, size);
#endif
#ifdef TPM_ALG_ECC
    case TPM_ALG_ECC:
      return TPM2B_ECC_PARAMETER_Unmarshal((TPM2B_ECC_PARAMETER*)&target->ecc,
                                           buffer, size);
#endif
#ifdef TPM_ALG_KEYEDHASH
    case TPM_ALG_KEYEDHASH:
      return TPM2B_SENSITIVE_DATA_Unmarshal(
          (TPM2B_SENSITIVE_DATA*)&target->bits, buffer, size);
#endif
#ifdef TPM_ALG_SYMCIPHER
    case TPM_ALG_SYMCIPHER:
      return TPM2B_SYM_KEY_Unmarshal((TPM2B_SYM_KEY*)&target->sym, buffer,
                                     size);
#endif
  }
  return TPM_RC_SELECTOR;
}

UINT16 TPMT_SENSITIVE_Marshal(TPMT_SENSITIVE* source,
                              BYTE** buffer,
                              INT32* size) {
  UINT16 total_size = 0;
  total_size += TPMI_ALG_PUBLIC_Marshal(&source->sensitiveType, buffer, size);
  total_size += TPM2B_AUTH_Marshal(&source->authValue, buffer, size);
  total_size += TPM2B_DIGEST_Marshal(&source->seedValue, buffer, size);
  total_size += TPMU_SENSITIVE_COMPOSITE_Marshal(&source->sensitive, buffer,
                                                 size, source->sensitiveType);
  return total_size;
}

TPM_RC TPMT_SENSITIVE_Unmarshal(TPMT_SENSITIVE* target,
                                BYTE** buffer,
                                INT32* size) {
  TPM_RC result;
  result = TPMI_ALG_PUBLIC_Unmarshal(&target->sensitiveType, buffer, size);
  if (result != TPM_RC_SUCCESS) {
    return result;
  }
  result = TPM2B_AUTH_Unmarshal(&target->authValue, buffer, size);
  if (result != TPM_RC_SUCCESS) {
    return result;
  }
  result = TPM2B_DIGEST_Unmarshal(&target->seedValue, buffer, size);
  if (result != TPM_RC_SUCCESS) {
    return result;
  }
  result = TPMU_SENSITIVE_COMPOSITE_Unmarshal(&target->sensitive, buffer, size,
                                              target->sensitiveType);
  if (result != TPM_RC_SUCCESS) {
    return result;
  }
  return TPM_RC_SUCCESS;
}

UINT16 TPM2B_SENSITIVE_Marshal(TPM2B_SENSITIVE* source,
                               BYTE** buffer,
                               INT32* size) {
  UINT16 total_size = 0;
  total_size += UINT16_Marshal(&source->t.size, buffer, size);
  total_size += TPMT_SENSITIVE_Marshal(&source->t.sensitiveArea, buffer, size);
  return total_size;
}

TPM_RC TPM2B_SENSITIVE_Unmarshal(TPM2B_SENSITIVE* target,
                                 BYTE** buffer,
                                 INT32* size) {
  TPM_RC result;
  result = UINT16_Unmarshal(&target->t.size, buffer, size);
  if (result != TPM_RC_SUCCESS) {
    return result;
  }
  if (target->t.size == 0) {
    return TPM_RC_SUCCESS;
  }
  result = TPMT_SENSITIVE_Unmarshal(&target->t.sensitiveArea, buffer, size);
  if (result != TPM_RC_SUCCESS) {
    return result;
  }
  return TPM_RC_SUCCESS;
}

UINT16 TPMS_SENSITIVE_CREATE_Marshal(TPMS_SENSITIVE_CREATE* source,
                                     BYTE** buffer,
                                     INT32* size) {
  UINT16 total_size = 0;
  total_size += TPM2B_AUTH_Marshal(&source->userAuth, buffer, size);
  total_size += TPM2B_SENSITIVE_DATA_Marshal(&source->data, buffer, size);
  return total_size;
}

TPM_RC TPMS_SENSITIVE_CREATE_Unmarshal(TPMS_SENSITIVE_CREATE* target,
                                       BYTE** buffer,
                                       INT32* size) {
  TPM_RC result;
  result = TPM2B_AUTH_Unmarshal(&target->userAuth, buffer, size);
  if (result != TPM_RC_SUCCESS) {
    return result;
  }
  result = TPM2B_SENSITIVE_DATA_Unmarshal(&target->data, buffer, size);
  if (result != TPM_RC_SUCCESS) {
    return result;
  }
  return TPM_RC_SUCCESS;
}

UINT16 TPM2B_SENSITIVE_CREATE_Marshal(TPM2B_SENSITIVE_CREATE* source,
                                      BYTE** buffer,
                                      INT32* size) {
  UINT16 total_size = 0;
  total_size += UINT16_Marshal(&source->t.size, buffer, size);
  total_size +=
      TPMS_SENSITIVE_CREATE_Marshal(&source->t.sensitive, buffer, size);
  {
    BYTE* size_location = *buffer - total_size;
    INT32 size_field_size = sizeof(UINT16);
    UINT16 payload_size = total_size - (UINT16)size_field_size;
    UINT16_Marshal(&payload_size, &size_location, &size_field_size);
  }
  return total_size;
}

TPM_RC TPM2B_SENSITIVE_CREATE_Unmarshal(TPM2B_SENSITIVE_CREATE* target,
                                        BYTE** buffer,
                                        INT32* size) {
  TPM_RC result;
  UINT32 start_size = *size;
  UINT32 struct_size;
  result = UINT16_Unmarshal(&target->t.size, buffer, size);
  if (result != TPM_RC_SUCCESS) {
    return result;
  }
  if (target->t.size == 0) {
    return TPM_RC_SIZE;
  }
  result = TPMS_SENSITIVE_CREATE_Unmarshal(&target->t.sensitive, buffer, size);
  if (result != TPM_RC_SUCCESS) {
    return result;
  }
  struct_size = start_size - *size - sizeof(target->t.size);
  if (struct_size != target->t.size) {
    return TPM_RC_SIZE;
  }
  return TPM_RC_SUCCESS;
}

UINT16 TPM2B_TIMEOUT_Marshal(TPM2B_TIMEOUT* source,
                             BYTE** buffer,
                             INT32* size) {
  UINT16 total_size = 0;
  INT32 i;
  total_size += UINT16_Marshal(&source->t.size, buffer, size);
  for (i = 0; i < source->t.size; ++i) {
    total_size += BYTE_Marshal(&source->t.buffer[i], buffer, size);
  }
  return total_size;
}

TPM_RC TPM2B_TIMEOUT_Unmarshal(TPM2B_TIMEOUT* target,
                               BYTE** buffer,
                               INT32* size) {
  TPM_RC result;
  INT32 i;
  result = UINT16_Unmarshal(&target->t.size, buffer, size);
  if (result != TPM_RC_SUCCESS) {
    return result;
  }
  if (target->t.size == 0) {
    return TPM_RC_SUCCESS;
  }
  if (target->t.size > sizeof(UINT64)) {
    return TPM_RC_SIZE;
  }
  for (i = 0; i < target->t.size; ++i) {
    result = BYTE_Unmarshal(&target->t.buffer[i], buffer, size);
    if (result != TPM_RC_SUCCESS) {
      return result;
    }
  }
  return TPM_RC_SUCCESS;
}

UINT16 TPMA_ALGORITHM_Marshal(TPMA_ALGORITHM* source,
                              BYTE** buffer,
                              INT32* size) {
  return uint32_t_Marshal((uint32_t*)source, buffer, size);
}

TPM_RC TPMA_ALGORITHM_Unmarshal(TPMA_ALGORITHM* target,
                                BYTE** buffer,
                                INT32* size) {
  TPM_RC result;
  result = uint32_t_Unmarshal((uint32_t*)target, buffer, size);
  if (result != TPM_RC_SUCCESS) {
    return result;
  }
  if (target->reserved4_7 != 0) {
    return TPM_RC_RESERVED_BITS;
  }
  if (target->reserved11_31 != 0) {
    return TPM_RC_RESERVED_BITS;
  }
  return TPM_RC_SUCCESS;
}

UINT16 TPMA_CC_Marshal(TPMA_CC* source, BYTE** buffer, INT32* size) {
  return uint32_t_Marshal((uint32_t*)source, buffer, size);
}

TPM_RC TPMA_CC_Unmarshal(TPMA_CC* target, BYTE** buffer, INT32* size) {
  TPM_RC result;
  result = uint32_t_Unmarshal((uint32_t*)target, buffer, size);
  if (result != TPM_RC_SUCCESS) {
    return result;
  }
  if (target->reserved16_21 != 0) {
    return TPM_RC_RESERVED_BITS;
  }
  return TPM_RC_SUCCESS;
}

UINT16 TPMA_MEMORY_Marshal(TPMA_MEMORY* source, BYTE** buffer, INT32* size) {
  return uint32_t_Marshal((uint32_t*)source, buffer, size);
}

TPM_RC TPMA_MEMORY_Unmarshal(TPMA_MEMORY* target, BYTE** buffer, INT32* size) {
  TPM_RC result;
  result = uint32_t_Unmarshal((uint32_t*)target, buffer, size);
  if (result != TPM_RC_SUCCESS) {
    return result;
  }
  if (target->reserved3_31 != 0) {
    return TPM_RC_RESERVED_BITS;
  }
  return TPM_RC_SUCCESS;
}

UINT16 TPMA_PERMANENT_Marshal(TPMA_PERMANENT* source,
                              BYTE** buffer,
                              INT32* size) {
  return uint32_t_Marshal((uint32_t*)source, buffer, size);
}

TPM_RC TPMA_PERMANENT_Unmarshal(TPMA_PERMANENT* target,
                                BYTE** buffer,
                                INT32* size) {
  TPM_RC result;
  result = uint32_t_Unmarshal((uint32_t*)target, buffer, size);
  if (result != TPM_RC_SUCCESS) {
    return result;
  }
  if (target->reserved3_7 != 0) {
    return TPM_RC_RESERVED_BITS;
  }
  if (target->reserved11_31 != 0) {
    return TPM_RC_RESERVED_BITS;
  }
  return TPM_RC_SUCCESS;
}

UINT16 TPMA_SESSION_Marshal(TPMA_SESSION* source, BYTE** buffer, INT32* size) {
  return uint8_t_Marshal((uint8_t*)source, buffer, size);
}

TPM_RC TPMA_SESSION_Unmarshal(TPMA_SESSION* target,
                              BYTE** buffer,
                              INT32* size) {
  TPM_RC result;
  result = uint8_t_Unmarshal((uint8_t*)target, buffer, size);
  if (result != TPM_RC_SUCCESS) {
    return result;
  }
  if (target->reserved3_4 != 0) {
    return TPM_RC_RESERVED_BITS;
  }
  return TPM_RC_SUCCESS;
}

UINT16 TPMA_STARTUP_CLEAR_Marshal(TPMA_STARTUP_CLEAR* source,
                                  BYTE** buffer,
                                  INT32* size) {
  return uint32_t_Marshal((uint32_t*)source, buffer, size);
}

TPM_RC TPMA_STARTUP_CLEAR_Unmarshal(TPMA_STARTUP_CLEAR* target,
                                    BYTE** buffer,
                                    INT32* size) {
  TPM_RC result;
  result = uint32_t_Unmarshal((uint32_t*)target, buffer, size);
  if (result != TPM_RC_SUCCESS) {
    return result;
  }
  if (target->reserved4_30 != 0) {
    return TPM_RC_RESERVED_BITS;
  }
  return TPM_RC_SUCCESS;
}

UINT16 TPMI_ALG_ASYM_Marshal(TPMI_ALG_ASYM* source,
                             BYTE** buffer,
                             INT32* size) {
  return uint16_t_Marshal(source, buffer, size);
}

TPM_RC TPMI_ALG_ASYM_Unmarshal(TPMI_ALG_ASYM* target,
                               BYTE** buffer,
                               INT32* size,
                               BOOL allow_conditional_value) {
  TPM_RC result;
  BOOL has_valid_value = FALSE;
  result = uint16_t_Unmarshal(target, buffer, size);
  if ((result != TPM_RC_VALUE) && (result != TPM_RC_SUCCESS)) {
    return result;
  }
  if (*target == TPM_ALG_NULL) {
    return allow_conditional_value ? TPM_RC_SUCCESS : TPM_RC_ASYMMETRIC;
  }
  switch (*target) {
#ifdef TPM_ALG_RSA
    case TPM_ALG_RSA:
#endif
#ifdef TPM_ALG_ECC
    case TPM_ALG_ECC:
#endif
      has_valid_value = TRUE;
      break;
  }
  if (!has_valid_value) {
    return TPM_RC_ASYMMETRIC;
  }
  return TPM_RC_SUCCESS;
}

UINT16 TPMI_ALG_RSA_DECRYPT_Marshal(TPMI_ALG_RSA_DECRYPT* source,
                                    BYTE** buffer,
                                    INT32* size) {
  return uint16_t_Marshal(source, buffer, size);
}

TPM_RC TPMI_ALG_RSA_DECRYPT_Unmarshal(TPMI_ALG_RSA_DECRYPT* target,
                                      BYTE** buffer,
                                      INT32* size,
                                      BOOL allow_conditional_value) {
  TPM_RC result;
  BOOL has_valid_value = FALSE;
  result = uint16_t_Unmarshal(target, buffer, size);
  if ((result != TPM_RC_VALUE) && (result != TPM_RC_SUCCESS)) {
    return result;
  }
  if (*target == TPM_ALG_NULL) {
    return allow_conditional_value ? TPM_RC_SUCCESS : TPM_RC_VALUE;
  }
  switch (*target) {
#ifdef TPM_ALG_RSAES
    case TPM_ALG_RSAES:
#endif
#ifdef TPM_ALG_OAEP
    case TPM_ALG_OAEP:
#endif
      has_valid_value = TRUE;
      break;
  }
  if (!has_valid_value) {
    return TPM_RC_VALUE;
  }
  return TPM_RC_SUCCESS;
}

UINT16 TPMI_ALG_SIG_SCHEME_Marshal(TPMI_ALG_SIG_SCHEME* source,
                                   BYTE** buffer,
                                   INT32* size) {
  return uint16_t_Marshal(source, buffer, size);
}

TPM_RC TPMI_ALG_SIG_SCHEME_Unmarshal(TPMI_ALG_SIG_SCHEME* target,
                                     BYTE** buffer,
                                     INT32* size,
                                     BOOL allow_conditional_value) {
  TPM_RC result;
  BOOL has_valid_value = FALSE;
  result = uint16_t_Unmarshal(target, buffer, size);
  if ((result != TPM_RC_VALUE) && (result != TPM_RC_SUCCESS)) {
    return result;
  }
  if (*target == TPM_ALG_NULL) {
    return allow_conditional_value ? TPM_RC_SUCCESS : TPM_RC_SCHEME;
  }
  switch (*target) {
#ifdef TPM_ALG_RSASSA
    case TPM_ALG_RSASSA:
#endif
#ifdef TPM_ALG_RSAPSS
    case TPM_ALG_RSAPSS:
#endif
#ifdef TPM_ALG_ECDSA
    case TPM_ALG_ECDSA:
#endif
#ifdef TPM_ALG_ECDAA
    case TPM_ALG_ECDAA:
#endif
#ifdef TPM_ALG_SM2
    case TPM_ALG_SM2:
#endif
#ifdef TPM_ALG_ECSCHNORR
    case TPM_ALG_ECSCHNORR:
#endif
#ifdef TPM_ALG_HMAC
    case TPM_ALG_HMAC:
#endif
      has_valid_value = TRUE;
      break;
  }
  if (!has_valid_value) {
    return TPM_RC_SCHEME;
  }
  return TPM_RC_SUCCESS;
}

UINT16 TPMI_ALG_SYM_Marshal(TPMI_ALG_SYM* source, BYTE** buffer, INT32* size) {
  return uint16_t_Marshal(source, buffer, size);
}

TPM_RC TPMI_ALG_SYM_Unmarshal(TPMI_ALG_SYM* target,
                              BYTE** buffer,
                              INT32* size,
                              BOOL allow_conditional_value) {
  TPM_RC result;
  BOOL has_valid_value = FALSE;
  result = uint16_t_Unmarshal(target, buffer, size);
  if ((result != TPM_RC_VALUE) && (result != TPM_RC_SUCCESS)) {
    return result;
  }
  if (*target == TPM_ALG_NULL) {
    return allow_conditional_value ? TPM_RC_SUCCESS : TPM_RC_SYMMETRIC;
  }
  switch (*target) {
#ifdef TPM_ALG_AES
    case TPM_ALG_AES:
#endif
#ifdef TPM_ALG_SM4
    case TPM_ALG_SM4:
#endif
#ifdef TPM_ALG_CAMELLIA
    case TPM_ALG_CAMELLIA:
#endif
#ifdef TPM_ALG_XOR
    case TPM_ALG_XOR:
#endif
      has_valid_value = TRUE;
      break;
  }
  if (!has_valid_value) {
    return TPM_RC_SYMMETRIC;
  }
  return TPM_RC_SUCCESS;
}

UINT16 TPMI_DH_CONTEXT_Marshal(TPMI_DH_CONTEXT* source,
                               BYTE** buffer,
                               INT32* size) {
  return uint32_t_Marshal(source, buffer, size);
}

TPM_RC TPMI_DH_CONTEXT_Unmarshal(TPMI_DH_CONTEXT* target,
                                 BYTE** buffer,
                                 INT32* size) {
  TPM_RC result;
  BOOL has_valid_value = FALSE;
  result = uint32_t_Unmarshal(target, buffer, size);
  if ((result != TPM_RC_VALUE) && (result != TPM_RC_SUCCESS)) {
    return result;
  }
  if ((*target >= HMAC_SESSION_FIRST) && (*target <= HMAC_SESSION_LAST)) {
    has_valid_value = TRUE;
  }
  if ((*target >= POLICY_SESSION_FIRST) && (*target <= POLICY_SESSION_LAST)) {
    has_valid_value = TRUE;
  }
  if ((*target >= TRANSIENT_FIRST) && (*target <= TRANSIENT_LAST)) {
    has_valid_value = TRUE;
  }
  if (!has_valid_value) {
    return TPM_RC_VALUE;
  }
  return TPM_RC_SUCCESS;
}

UINT16 TPMI_DH_ENTITY_Marshal(TPMI_DH_ENTITY* source,
                              BYTE** buffer,
                              INT32* size) {
  return uint32_t_Marshal(source, buffer, size);
}

TPM_RC TPMI_DH_ENTITY_Unmarshal(TPMI_DH_ENTITY* target,
                                BYTE** buffer,
                                INT32* size,
                                BOOL allow_conditional_value) {
  TPM_RC result;
  BOOL has_valid_value = FALSE;
  result = uint32_t_Unmarshal(target, buffer, size);
  if ((result != TPM_RC_VALUE) && (result != TPM_RC_SUCCESS)) {
    return result;
  }
  if (*target == TPM_RH_NULL) {
    return allow_conditional_value ? TPM_RC_SUCCESS : TPM_RC_VALUE;
  }
  switch (*target) {
    case TPM_RH_OWNER:
    case TPM_RH_ENDORSEMENT:
    case TPM_RH_PLATFORM:
    case TPM_RH_LOCKOUT:
      has_valid_value = TRUE;
      break;
  }
  if ((*target >= TRANSIENT_FIRST) && (*target <= TRANSIENT_LAST)) {
    has_valid_value = TRUE;
  }
  if ((*target >= PERSISTENT_FIRST) && (*target <= PERSISTENT_LAST)) {
    has_valid_value = TRUE;
  }
  if ((*target >= NV_INDEX_FIRST) && (*target <= NV_INDEX_LAST)) {
    has_valid_value = TRUE;
  }
  if ((*target >= PCR_FIRST) && (*target <= PCR_LAST)) {
    has_valid_value = TRUE;
  }
  if ((*target >= TPM_RH_AUTH_00) && (*target <= TPM_RH_AUTH_FF)) {
    has_valid_value = TRUE;
  }
  if (!has_valid_value) {
    return TPM_RC_VALUE;
  }
  return TPM_RC_SUCCESS;
}

UINT16 TPMI_DH_OBJECT_Marshal(TPMI_DH_OBJECT* source,
                              BYTE** buffer,
                              INT32* size) {
  return uint32_t_Marshal(source, buffer, size);
}

TPM_RC TPMI_DH_OBJECT_Unmarshal(TPMI_DH_OBJECT* target,
                                BYTE** buffer,
                                INT32* size,
                                BOOL allow_conditional_value) {
  TPM_RC result;
  BOOL has_valid_value = FALSE;
  result = uint32_t_Unmarshal(target, buffer, size);
  if ((result != TPM_RC_VALUE) && (result != TPM_RC_SUCCESS)) {
    return result;
  }
  if (*target == TPM_RH_NULL) {
    return allow_conditional_value ? TPM_RC_SUCCESS : TPM_RC_VALUE;
  }
  if ((*target >= TRANSIENT_FIRST) && (*target <= TRANSIENT_LAST)) {
    has_valid_value = TRUE;
  }
  if ((*target >= PERSISTENT_FIRST) && (*target <= PERSISTENT_LAST)) {
    has_valid_value = TRUE;
  }
  if (!has_valid_value) {
    return TPM_RC_VALUE;
  }
  return TPM_RC_SUCCESS;
}

UINT16 TPMI_DH_PCR_Marshal(TPMI_DH_PCR* source, BYTE** buffer, INT32* size) {
  return uint32_t_Marshal(source, buffer, size);
}

TPM_RC TPMI_DH_PCR_Unmarshal(TPMI_DH_PCR* target,
                             BYTE** buffer,
                             INT32* size,
                             BOOL allow_conditional_value) {
  TPM_RC result;
  BOOL has_valid_value = FALSE;
  result = uint32_t_Unmarshal(target, buffer, size);
  if ((result != TPM_RC_VALUE) && (result != TPM_RC_SUCCESS)) {
    return result;
  }
  if (*target == TPM_RH_NULL) {
    return allow_conditional_value ? TPM_RC_SUCCESS : TPM_RC_VALUE;
  }
  if ((*target >= PCR_FIRST) && (*target <= PCR_LAST)) {
    has_valid_value = TRUE;
  }
  if (!has_valid_value) {
    return TPM_RC_VALUE;
  }
  return TPM_RC_SUCCESS;
}

UINT16 TPMI_DH_PERSISTENT_Marshal(TPMI_DH_PERSISTENT* source,
                                  BYTE** buffer,
                                  INT32* size) {
  return uint32_t_Marshal(source, buffer, size);
}

TPM_RC TPMI_DH_PERSISTENT_Unmarshal(TPMI_DH_PERSISTENT* target,
                                    BYTE** buffer,
                                    INT32* size) {
  TPM_RC result;
  BOOL has_valid_value = FALSE;
  result = uint32_t_Unmarshal(target, buffer, size);
  if ((result != TPM_RC_VALUE) && (result != TPM_RC_SUCCESS)) {
    return result;
  }
  if ((*target >= PERSISTENT_FIRST) && (*target <= PERSISTENT_LAST)) {
    has_valid_value = TRUE;
  }
  if (!has_valid_value) {
    return TPM_RC_VALUE;
  }
  return TPM_RC_SUCCESS;
}

UINT16 TPMI_ECC_KEY_EXCHANGE_Marshal(TPMI_ECC_KEY_EXCHANGE* source,
                                     BYTE** buffer,
                                     INT32* size) {
  return uint16_t_Marshal(source, buffer, size);
}

TPM_RC TPMI_ECC_KEY_EXCHANGE_Unmarshal(TPMI_ECC_KEY_EXCHANGE* target,
                                       BYTE** buffer,
                                       INT32* size,
                                       BOOL allow_conditional_value) {
  TPM_RC result;
  BOOL has_valid_value = FALSE;
  result = uint16_t_Unmarshal(target, buffer, size);
  if ((result != TPM_RC_VALUE) && (result != TPM_RC_SUCCESS)) {
    return result;
  }
  if (*target == TPM_ALG_NULL) {
    return allow_conditional_value ? TPM_RC_SUCCESS : TPM_RC_SCHEME;
  }
  switch (*target) {
#ifdef TPM_ALG_ECDH
    case TPM_ALG_ECDH:
#endif
#ifdef TPM_ALG_ECMQV
    case TPM_ALG_ECMQV:
#endif
#ifdef TPM_ALG_SM2
    case TPM_ALG_SM2:
#endif
      has_valid_value = TRUE;
      break;
  }
  if (!has_valid_value) {
    return TPM_RC_SCHEME;
  }
  return TPM_RC_SUCCESS;
}

UINT16 TPMI_RH_CLEAR_Marshal(TPMI_RH_CLEAR* source,
                             BYTE** buffer,
                             INT32* size) {
  return uint32_t_Marshal(source, buffer, size);
}

TPM_RC TPMI_RH_CLEAR_Unmarshal(TPMI_RH_CLEAR* target,
                               BYTE** buffer,
                               INT32* size) {
  TPM_RC result;
  BOOL has_valid_value = FALSE;
  result = uint32_t_Unmarshal(target, buffer, size);
  if ((result != TPM_RC_VALUE) && (result != TPM_RC_SUCCESS)) {
    return result;
  }
  switch (*target) {
    case TPM_RH_LOCKOUT:
    case TPM_RH_PLATFORM:
      has_valid_value = TRUE;
      break;
  }
  if (!has_valid_value) {
    return TPM_RC_VALUE;
  }
  return TPM_RC_SUCCESS;
}

UINT16 TPMI_RH_ENABLES_Marshal(TPMI_RH_ENABLES* source,
                               BYTE** buffer,
                               INT32* size) {
  return uint32_t_Marshal(source, buffer, size);
}

TPM_RC TPMI_RH_ENABLES_Unmarshal(TPMI_RH_ENABLES* target,
                                 BYTE** buffer,
                                 INT32* size,
                                 BOOL allow_conditional_value) {
  TPM_RC result;
  BOOL has_valid_value = FALSE;
  result = uint32_t_Unmarshal(target, buffer, size);
  if ((result != TPM_RC_VALUE) && (result != TPM_RC_SUCCESS)) {
    return result;
  }
  if (*target == TPM_RH_NULL) {
    return allow_conditional_value ? TPM_RC_SUCCESS : TPM_RC_VALUE;
  }
  switch (*target) {
    case TPM_RH_OWNER:
    case TPM_RH_PLATFORM:
    case TPM_RH_ENDORSEMENT:
    case TPM_RH_PLATFORM_NV:
      has_valid_value = TRUE;
      break;
  }
  if (!has_valid_value) {
    return TPM_RC_VALUE;
  }
  return TPM_RC_SUCCESS;
}

UINT16 TPMI_RH_ENDORSEMENT_Marshal(TPMI_RH_ENDORSEMENT* source,
                                   BYTE** buffer,
                                   INT32* size) {
  return uint32_t_Marshal(source, buffer, size);
}

TPM_RC TPMI_RH_ENDORSEMENT_Unmarshal(TPMI_RH_ENDORSEMENT* target,
                                     BYTE** buffer,
                                     INT32* size,
                                     BOOL allow_conditional_value) {
  TPM_RC result;
  BOOL has_valid_value = FALSE;
  result = uint32_t_Unmarshal(target, buffer, size);
  if ((result != TPM_RC_VALUE) && (result != TPM_RC_SUCCESS)) {
    return result;
  }
  if (*target == TPM_RH_NULL) {
    return allow_conditional_value ? TPM_RC_SUCCESS : TPM_RC_VALUE;
  }
  switch (*target) {
    case TPM_RH_ENDORSEMENT:
      has_valid_value = TRUE;
      break;
  }
  if (!has_valid_value) {
    return TPM_RC_VALUE;
  }
  return TPM_RC_SUCCESS;
}

UINT16 TPMI_RH_HIERARCHY_Marshal(TPMI_RH_HIERARCHY* source,
                                 BYTE** buffer,
                                 INT32* size) {
  return uint32_t_Marshal(source, buffer, size);
}

TPM_RC TPMI_RH_HIERARCHY_Unmarshal(TPMI_RH_HIERARCHY* target,
                                   BYTE** buffer,
                                   INT32* size,
                                   BOOL allow_conditional_value) {
  TPM_RC result;
  BOOL has_valid_value = FALSE;
  result = uint32_t_Unmarshal(target, buffer, size);
  if ((result != TPM_RC_VALUE) && (result != TPM_RC_SUCCESS)) {
    return result;
  }
  if (*target == TPM_RH_NULL) {
    return allow_conditional_value ? TPM_RC_SUCCESS : TPM_RC_VALUE;
  }
  switch (*target) {
    case TPM_RH_OWNER:
    case TPM_RH_PLATFORM:
    case TPM_RH_ENDORSEMENT:
      has_valid_value = TRUE;
      break;
  }
  if (!has_valid_value) {
    return TPM_RC_VALUE;
  }
  return TPM_RC_SUCCESS;
}

UINT16 TPMI_RH_HIERARCHY_AUTH_Marshal(TPMI_RH_HIERARCHY_AUTH* source,
                                      BYTE** buffer,
                                      INT32* size) {
  return uint32_t_Marshal(source, buffer, size);
}

TPM_RC TPMI_RH_HIERARCHY_AUTH_Unmarshal(TPMI_RH_HIERARCHY_AUTH* target,
                                        BYTE** buffer,
                                        INT32* size) {
  TPM_RC result;
  BOOL has_valid_value = FALSE;
  result = uint32_t_Unmarshal(target, buffer, size);
  if ((result != TPM_RC_VALUE) && (result != TPM_RC_SUCCESS)) {
    return result;
  }
  switch (*target) {
    case TPM_RH_OWNER:
    case TPM_RH_PLATFORM:
    case TPM_RH_ENDORSEMENT:
    case TPM_RH_LOCKOUT:
      has_valid_value = TRUE;
      break;
  }
  if (!has_valid_value) {
    return TPM_RC_VALUE;
  }
  return TPM_RC_SUCCESS;
}

UINT16 TPMI_RH_LOCKOUT_Marshal(TPMI_RH_LOCKOUT* source,
                               BYTE** buffer,
                               INT32* size) {
  return uint32_t_Marshal(source, buffer, size);
}

TPM_RC TPMI_RH_LOCKOUT_Unmarshal(TPMI_RH_LOCKOUT* target,
                                 BYTE** buffer,
                                 INT32* size) {
  TPM_RC result;
  BOOL has_valid_value = FALSE;
  result = uint32_t_Unmarshal(target, buffer, size);
  if ((result != TPM_RC_VALUE) && (result != TPM_RC_SUCCESS)) {
    return result;
  }
  switch (*target) {
    case TPM_RH_LOCKOUT:
      has_valid_value = TRUE;
      break;
  }
  if (!has_valid_value) {
    return TPM_RC_VALUE;
  }
  return TPM_RC_SUCCESS;
}

UINT16 TPMI_RH_NV_AUTH_Marshal(TPMI_RH_NV_AUTH* source,
                               BYTE** buffer,
                               INT32* size) {
  return uint32_t_Marshal(source, buffer, size);
}

TPM_RC TPMI_RH_NV_AUTH_Unmarshal(TPMI_RH_NV_AUTH* target,
                                 BYTE** buffer,
                                 INT32* size) {
  TPM_RC result;
  BOOL has_valid_value = FALSE;
  result = uint32_t_Unmarshal(target, buffer, size);
  if ((result != TPM_RC_VALUE) && (result != TPM_RC_SUCCESS)) {
    return result;
  }
  switch (*target) {
    case TPM_RH_PLATFORM:
    case TPM_RH_OWNER:
      has_valid_value = TRUE;
      break;
  }
  if ((*target >= NV_INDEX_FIRST) && (*target <= NV_INDEX_LAST)) {
    has_valid_value = TRUE;
  }
  if (!has_valid_value) {
    return TPM_RC_VALUE;
  }
  return TPM_RC_SUCCESS;
}

UINT16 TPMI_RH_OWNER_Marshal(TPMI_RH_OWNER* source,
                             BYTE** buffer,
                             INT32* size) {
  return uint32_t_Marshal(source, buffer, size);
}

TPM_RC TPMI_RH_OWNER_Unmarshal(TPMI_RH_OWNER* target,
                               BYTE** buffer,
                               INT32* size,
                               BOOL allow_conditional_value) {
  TPM_RC result;
  BOOL has_valid_value = FALSE;
  result = uint32_t_Unmarshal(target, buffer, size);
  if ((result != TPM_RC_VALUE) && (result != TPM_RC_SUCCESS)) {
    return result;
  }
  if (*target == TPM_RH_NULL) {
    return allow_conditional_value ? TPM_RC_SUCCESS : TPM_RC_VALUE;
  }
  switch (*target) {
    case TPM_RH_OWNER:
      has_valid_value = TRUE;
      break;
  }
  if (!has_valid_value) {
    return TPM_RC_VALUE;
  }
  return TPM_RC_SUCCESS;
}

UINT16 TPMI_RH_PLATFORM_Marshal(TPMI_RH_PLATFORM* source,
                                BYTE** buffer,
                                INT32* size) {
  return uint32_t_Marshal(source, buffer, size);
}

TPM_RC TPMI_RH_PLATFORM_Unmarshal(TPMI_RH_PLATFORM* target,
                                  BYTE** buffer,
                                  INT32* size) {
  TPM_RC result;
  BOOL has_valid_value = FALSE;
  result = uint32_t_Unmarshal(target, buffer, size);
  if ((result != TPM_RC_VALUE) && (result != TPM_RC_SUCCESS)) {
    return result;
  }
  switch (*target) {
    case TPM_RH_PLATFORM:
      has_valid_value = TRUE;
      break;
  }
  if (!has_valid_value) {
    return TPM_RC_VALUE;
  }
  return TPM_RC_SUCCESS;
}

UINT16 TPMI_RH_PROVISION_Marshal(TPMI_RH_PROVISION* source,
                                 BYTE** buffer,
                                 INT32* size) {
  return uint32_t_Marshal(source, buffer, size);
}

TPM_RC TPMI_RH_PROVISION_Unmarshal(TPMI_RH_PROVISION* target,
                                   BYTE** buffer,
                                   INT32* size) {
  TPM_RC result;
  BOOL has_valid_value = FALSE;
  result = uint32_t_Unmarshal(target, buffer, size);
  if ((result != TPM_RC_VALUE) && (result != TPM_RC_SUCCESS)) {
    return result;
  }
  switch (*target) {
    case TPM_RH_OWNER:
    case TPM_RH_PLATFORM:
      has_valid_value = TRUE;
      break;
  }
  if (!has_valid_value) {
    return TPM_RC_VALUE;
  }
  return TPM_RC_SUCCESS;
}

UINT16 TPMI_SH_AUTH_SESSION_Marshal(TPMI_SH_AUTH_SESSION* source,
                                    BYTE** buffer,
                                    INT32* size) {
  return uint32_t_Marshal(source, buffer, size);
}

TPM_RC TPMI_SH_AUTH_SESSION_Unmarshal(TPMI_SH_AUTH_SESSION* target,
                                      BYTE** buffer,
                                      INT32* size,
                                      BOOL allow_conditional_value) {
  TPM_RC result;
  BOOL has_valid_value = FALSE;
  result = uint32_t_Unmarshal(target, buffer, size);
  if ((result != TPM_RC_VALUE) && (result != TPM_RC_SUCCESS)) {
    return result;
  }
  if (*target == TPM_RS_PW) {
    return allow_conditional_value ? TPM_RC_SUCCESS : TPM_RC_VALUE;
  }
  if ((*target >= HMAC_SESSION_FIRST) && (*target <= HMAC_SESSION_LAST)) {
    has_valid_value = TRUE;
  }
  if ((*target >= POLICY_SESSION_FIRST) && (*target <= POLICY_SESSION_LAST)) {
    has_valid_value = TRUE;
  }
  if (!has_valid_value) {
    return TPM_RC_VALUE;
  }
  return TPM_RC_SUCCESS;
}

UINT16 TPMI_SH_HMAC_Marshal(TPMI_SH_HMAC* source, BYTE** buffer, INT32* size) {
  return uint32_t_Marshal(source, buffer, size);
}

TPM_RC TPMI_SH_HMAC_Unmarshal(TPMI_SH_HMAC* target,
                              BYTE** buffer,
                              INT32* size) {
  TPM_RC result;
  BOOL has_valid_value = FALSE;
  result = uint32_t_Unmarshal(target, buffer, size);
  if ((result != TPM_RC_VALUE) && (result != TPM_RC_SUCCESS)) {
    return result;
  }
  if ((*target >= HMAC_SESSION_FIRST) && (*target <= HMAC_SESSION_LAST)) {
    has_valid_value = TRUE;
  }
  if (!has_valid_value) {
    return TPM_RC_VALUE;
  }
  return TPM_RC_SUCCESS;
}

UINT16 TPMI_SH_POLICY_Marshal(TPMI_SH_POLICY* source,
                              BYTE** buffer,
                              INT32* size) {
  return uint32_t_Marshal(source, buffer, size);
}

TPM_RC TPMI_SH_POLICY_Unmarshal(TPMI_SH_POLICY* target,
                                BYTE** buffer,
                                INT32* size) {
  TPM_RC result;
  BOOL has_valid_value = FALSE;
  result = uint32_t_Unmarshal(target, buffer, size);
  if ((result != TPM_RC_VALUE) && (result != TPM_RC_SUCCESS)) {
    return result;
  }
  if ((*target >= POLICY_SESSION_FIRST) && (*target <= POLICY_SESSION_LAST)) {
    has_valid_value = TRUE;
  }
  if (!has_valid_value) {
    return TPM_RC_VALUE;
  }
  return TPM_RC_SUCCESS;
}

UINT16 TPMI_ST_ATTEST_Marshal(TPMI_ST_ATTEST* source,
                              BYTE** buffer,
                              INT32* size) {
  return uint16_t_Marshal(source, buffer, size);
}

TPM_RC TPMI_ST_ATTEST_Unmarshal(TPMI_ST_ATTEST* target,
                                BYTE** buffer,
                                INT32* size) {
  TPM_RC result;
  BOOL has_valid_value = FALSE;
  result = uint16_t_Unmarshal(target, buffer, size);
  if ((result != TPM_RC_VALUE) && (result != TPM_RC_SUCCESS)) {
    return result;
  }
  switch (*target) {
    case TPM_ST_ATTEST_CERTIFY:
    case TPM_ST_ATTEST_QUOTE:
    case TPM_ST_ATTEST_SESSION_AUDIT:
    case TPM_ST_ATTEST_COMMAND_AUDIT:
    case TPM_ST_ATTEST_TIME:
    case TPM_ST_ATTEST_CREATION:
    case TPM_ST_ATTEST_NV:
      has_valid_value = TRUE;
      break;
  }
  if (!has_valid_value) {
    return TPM_RC_VALUE;
  }
  return TPM_RC_SUCCESS;
}

UINT16 TPMI_ST_COMMAND_TAG_Marshal(TPMI_ST_COMMAND_TAG* source,
                                   BYTE** buffer,
                                   INT32* size) {
  return uint16_t_Marshal(source, buffer, size);
}

TPM_RC TPMI_ST_COMMAND_TAG_Unmarshal(TPMI_ST_COMMAND_TAG* target,
                                     BYTE** buffer,
                                     INT32* size) {
  TPM_RC result;
  BOOL has_valid_value = FALSE;
  result = uint16_t_Unmarshal(target, buffer, size);
  if ((result != TPM_RC_VALUE) && (result != TPM_RC_SUCCESS)) {
    return result;
  }
  switch (*target) {
    case TPM_ST_NO_SESSIONS:
    case TPM_ST_SESSIONS:
      has_valid_value = TRUE;
      break;
  }
  if (!has_valid_value) {
    return TPM_RC_BAD_TAG;
  }
  return TPM_RC_SUCCESS;
}

UINT16 TPMI_YES_NO_Marshal(TPMI_YES_NO* source, BYTE** buffer, INT32* size) {
  return uint8_t_Marshal(source, buffer, size);
}

TPM_RC TPMI_YES_NO_Unmarshal(TPMI_YES_NO* target, BYTE** buffer, INT32* size) {
  TPM_RC result;
  BOOL has_valid_value = FALSE;
  result = uint8_t_Unmarshal(target, buffer, size);
  if ((result != TPM_RC_VALUE) && (result != TPM_RC_SUCCESS)) {
    return result;
  }
  switch (*target) {
    case NO:
    case YES:
      has_valid_value = TRUE;
      break;
  }
  if (!has_valid_value) {
    return TPM_RC_VALUE;
  }
  return TPM_RC_SUCCESS;
}

UINT16 TPML_ALG_Marshal(TPML_ALG* source, BYTE** buffer, INT32* size) {
  UINT16 total_size = 0;
  INT32 i;
  total_size += UINT32_Marshal(&source->count, buffer, size);
  for (i = 0; i < source->count; ++i) {
    total_size += TPM_ALG_ID_Marshal(&source->algorithms[i], buffer, size);
  }
  return total_size;
}

TPM_RC TPML_ALG_Unmarshal(TPML_ALG* target, BYTE** buffer, INT32* size) {
  TPM_RC result;
  INT32 i;
  result = UINT32_Unmarshal(&target->count, buffer, size);
  if (result != TPM_RC_SUCCESS) {
    return result;
  }
  if (target->count > MAX_ALG_LIST_SIZE) {
    return TPM_RC_SIZE;
  }
  for (i = 0; i < target->count; ++i) {
    result = TPM_ALG_ID_Unmarshal(&target->algorithms[i], buffer, size);
    if (result != TPM_RC_SUCCESS) {
      return result;
    }
  }
  return TPM_RC_SUCCESS;
}

UINT16 TPMS_ALG_PROPERTY_Marshal(TPMS_ALG_PROPERTY* source,
                                 BYTE** buffer,
                                 INT32* size) {
  UINT16 total_size = 0;
  total_size += TPM_ALG_ID_Marshal(&source->alg, buffer, size);
  total_size += TPMA_ALGORITHM_Marshal(&source->algProperties, buffer, size);
  return total_size;
}

TPM_RC TPMS_ALG_PROPERTY_Unmarshal(TPMS_ALG_PROPERTY* target,
                                   BYTE** buffer,
                                   INT32* size) {
  TPM_RC result;
  result = TPM_ALG_ID_Unmarshal(&target->alg, buffer, size);
  if (result != TPM_RC_SUCCESS) {
    return result;
  }
  result = TPMA_ALGORITHM_Unmarshal(&target->algProperties, buffer, size);
  if (result != TPM_RC_SUCCESS) {
    return result;
  }
  return TPM_RC_SUCCESS;
}

UINT16 TPML_ALG_PROPERTY_Marshal(TPML_ALG_PROPERTY* source,
                                 BYTE** buffer,
                                 INT32* size) {
  UINT16 total_size = 0;
  INT32 i;
  total_size += UINT32_Marshal(&source->count, buffer, size);
  for (i = 0; i < source->count; ++i) {
    total_size +=
        TPMS_ALG_PROPERTY_Marshal(&source->algProperties[i], buffer, size);
  }
  return total_size;
}

TPM_RC TPML_ALG_PROPERTY_Unmarshal(TPML_ALG_PROPERTY* target,
                                   BYTE** buffer,
                                   INT32* size) {
  TPM_RC result;
  INT32 i;
  result = UINT32_Unmarshal(&target->count, buffer, size);
  if (result != TPM_RC_SUCCESS) {
    return result;
  }
  if (target->count > MAX_CAP_ALGS) {
    return TPM_RC_SIZE;
  }
  for (i = 0; i < target->count; ++i) {
    result =
        TPMS_ALG_PROPERTY_Unmarshal(&target->algProperties[i], buffer, size);
    if (result != TPM_RC_SUCCESS) {
      return result;
    }
  }
  return TPM_RC_SUCCESS;
}

UINT16 TPM_CC_Marshal(TPM_CC* source, BYTE** buffer, INT32* size) {
  return uint32_t_Marshal(source, buffer, size);
}

TPM_RC TPM_CC_Unmarshal(TPM_CC* target, BYTE** buffer, INT32* size) {
  TPM_RC result;
  result = uint32_t_Unmarshal(target, buffer, size);
  if (result != TPM_RC_SUCCESS) {
    return result;
  }
#ifdef TPM_CC_FIRST
  if (*target == TPM_CC_FIRST) {
    return TPM_RC_SUCCESS;
  }
#endif
#ifdef TPM_CC_PP_FIRST
  if (*target == TPM_CC_PP_FIRST) {
    return TPM_RC_SUCCESS;
  }
#endif
#ifdef TPM_CC_NV_UndefineSpaceSpecial
  if (*target == TPM_CC_NV_UndefineSpaceSpecial) {
    return TPM_RC_SUCCESS;
  }
#endif
#ifdef TPM_CC_EvictControl
  if (*target == TPM_CC_EvictControl) {
    return TPM_RC_SUCCESS;
  }
#endif
#ifdef TPM_CC_HierarchyControl
  if (*target == TPM_CC_HierarchyControl) {
    return TPM_RC_SUCCESS;
  }
#endif
#ifdef TPM_CC_NV_UndefineSpace
  if (*target == TPM_CC_NV_UndefineSpace) {
    return TPM_RC_SUCCESS;
  }
#endif
#ifdef TPM_CC_ChangeEPS
  if (*target == TPM_CC_ChangeEPS) {
    return TPM_RC_SUCCESS;
  }
#endif
#ifdef TPM_CC_ChangePPS
  if (*target == TPM_CC_ChangePPS) {
    return TPM_RC_SUCCESS;
  }
#endif
#ifdef TPM_CC_Clear
  if (*target == TPM_CC_Clear) {
    return TPM_RC_SUCCESS;
  }
#endif
#ifdef TPM_CC_ClearControl
  if (*target == TPM_CC_ClearControl) {
    return TPM_RC_SUCCESS;
  }
#endif
#ifdef TPM_CC_ClockSet
  if (*target == TPM_CC_ClockSet) {
    return TPM_RC_SUCCESS;
  }
#endif
#ifdef TPM_CC_HierarchyChangeAuth
  if (*target == TPM_CC_HierarchyChangeAuth) {
    return TPM_RC_SUCCESS;
  }
#endif
#ifdef TPM_CC_NV_DefineSpace
  if (*target == TPM_CC_NV_DefineSpace) {
    return TPM_RC_SUCCESS;
  }
#endif
#ifdef TPM_CC_PCR_Allocate
  if (*target == TPM_CC_PCR_Allocate) {
    return TPM_RC_SUCCESS;
  }
#endif
#ifdef TPM_CC_PCR_SetAuthPolicy
  if (*target == TPM_CC_PCR_SetAuthPolicy) {
    return TPM_RC_SUCCESS;
  }
#endif
#ifdef TPM_CC_PP_Commands
  if (*target == TPM_CC_PP_Commands) {
    return TPM_RC_SUCCESS;
  }
#endif
#ifdef TPM_CC_SetPrimaryPolicy
  if (*target == TPM_CC_SetPrimaryPolicy) {
    return TPM_RC_SUCCESS;
  }
#endif
#ifdef TPM_CC_FieldUpgradeStart
  if (*target == TPM_CC_FieldUpgradeStart) {
    return TPM_RC_SUCCESS;
  }
#endif
#ifdef TPM_CC_ClockRateAdjust
  if (*target == TPM_CC_ClockRateAdjust) {
    return TPM_RC_SUCCESS;
  }
#endif
#ifdef TPM_CC_CreatePrimary
  if (*target == TPM_CC_CreatePrimary) {
    return TPM_RC_SUCCESS;
  }
#endif
#ifdef TPM_CC_NV_GlobalWriteLock
  if (*target == TPM_CC_NV_GlobalWriteLock) {
    return TPM_RC_SUCCESS;
  }
#endif
#ifdef TPM_CC_PP_LAST
  if (*target == TPM_CC_PP_LAST) {
    return TPM_RC_SUCCESS;
  }
#endif
#ifdef TPM_CC_GetCommandAuditDigest
  if (*target == TPM_CC_GetCommandAuditDigest) {
    return TPM_RC_SUCCESS;
  }
#endif
#ifdef TPM_CC_NV_Increment
  if (*target == TPM_CC_NV_Increment) {
    return TPM_RC_SUCCESS;
  }
#endif
#ifdef TPM_CC_NV_SetBits
  if (*target == TPM_CC_NV_SetBits) {
    return TPM_RC_SUCCESS;
  }
#endif
#ifdef TPM_CC_NV_Extend
  if (*target == TPM_CC_NV_Extend) {
    return TPM_RC_SUCCESS;
  }
#endif
#ifdef TPM_CC_NV_Write
  if (*target == TPM_CC_NV_Write) {
    return TPM_RC_SUCCESS;
  }
#endif
#ifdef TPM_CC_NV_WriteLock
  if (*target == TPM_CC_NV_WriteLock) {
    return TPM_RC_SUCCESS;
  }
#endif
#ifdef TPM_CC_DictionaryAttackLockReset
  if (*target == TPM_CC_DictionaryAttackLockReset) {
    return TPM_RC_SUCCESS;
  }
#endif
#ifdef TPM_CC_DictionaryAttackParameters
  if (*target == TPM_CC_DictionaryAttackParameters) {
    return TPM_RC_SUCCESS;
  }
#endif
#ifdef TPM_CC_NV_ChangeAuth
  if (*target == TPM_CC_NV_ChangeAuth) {
    return TPM_RC_SUCCESS;
  }
#endif
#ifdef TPM_CC_PCR_Event
  if (*target == TPM_CC_PCR_Event) {
    return TPM_RC_SUCCESS;
  }
#endif
#ifdef TPM_CC_PCR_Reset
  if (*target == TPM_CC_PCR_Reset) {
    return TPM_RC_SUCCESS;
  }
#endif
#ifdef TPM_CC_SequenceComplete
  if (*target == TPM_CC_SequenceComplete) {
    return TPM_RC_SUCCESS;
  }
#endif
#ifdef TPM_CC_SetAlgorithmSet
  if (*target == TPM_CC_SetAlgorithmSet) {
    return TPM_RC_SUCCESS;
  }
#endif
#ifdef TPM_CC_SetCommandCodeAuditStatus
  if (*target == TPM_CC_SetCommandCodeAuditStatus) {
    return TPM_RC_SUCCESS;
  }
#endif
#ifdef TPM_CC_FieldUpgradeData
  if (*target == TPM_CC_FieldUpgradeData) {
    return TPM_RC_SUCCESS;
  }
#endif
#ifdef TPM_CC_IncrementalSelfTest
  if (*target == TPM_CC_IncrementalSelfTest) {
    return TPM_RC_SUCCESS;
  }
#endif
#ifdef TPM_CC_SelfTest
  if (*target == TPM_CC_SelfTest) {
    return TPM_RC_SUCCESS;
  }
#endif
#ifdef TPM_CC_Startup
  if (*target == TPM_CC_Startup) {
    return TPM_RC_SUCCESS;
  }
#endif
#ifdef TPM_CC_Shutdown
  if (*target == TPM_CC_Shutdown) {
    return TPM_RC_SUCCESS;
  }
#endif
#ifdef TPM_CC_StirRandom
  if (*target == TPM_CC_StirRandom) {
    return TPM_RC_SUCCESS;
  }
#endif
#ifdef TPM_CC_ActivateCredential
  if (*target == TPM_CC_ActivateCredential) {
    return TPM_RC_SUCCESS;
  }
#endif
#ifdef TPM_CC_Certify
  if (*target == TPM_CC_Certify) {
    return TPM_RC_SUCCESS;
  }
#endif
#ifdef TPM_CC_PolicyNV
  if (*target == TPM_CC_PolicyNV) {
    return TPM_RC_SUCCESS;
  }
#endif
#ifdef TPM_CC_CertifyCreation
  if (*target == TPM_CC_CertifyCreation) {
    return TPM_RC_SUCCESS;
  }
#endif
#ifdef TPM_CC_Duplicate
  if (*target == TPM_CC_Duplicate) {
    return TPM_RC_SUCCESS;
  }
#endif
#ifdef TPM_CC_GetTime
  if (*target == TPM_CC_GetTime) {
    return TPM_RC_SUCCESS;
  }
#endif
#ifdef TPM_CC_GetSessionAuditDigest
  if (*target == TPM_CC_GetSessionAuditDigest) {
    return TPM_RC_SUCCESS;
  }
#endif
#ifdef TPM_CC_NV_Read
  if (*target == TPM_CC_NV_Read) {
    return TPM_RC_SUCCESS;
  }
#endif
#ifdef TPM_CC_NV_ReadLock
  if (*target == TPM_CC_NV_ReadLock) {
    return TPM_RC_SUCCESS;
  }
#endif
#ifdef TPM_CC_ObjectChangeAuth
  if (*target == TPM_CC_ObjectChangeAuth) {
    return TPM_RC_SUCCESS;
  }
#endif
#ifdef TPM_CC_PolicySecret
  if (*target == TPM_CC_PolicySecret) {
    return TPM_RC_SUCCESS;
  }
#endif
#ifdef TPM_CC_Rewrap
  if (*target == TPM_CC_Rewrap) {
    return TPM_RC_SUCCESS;
  }
#endif
#ifdef TPM_CC_Create
  if (*target == TPM_CC_Create) {
    return TPM_RC_SUCCESS;
  }
#endif
#ifdef TPM_CC_ECDH_ZGen
  if (*target == TPM_CC_ECDH_ZGen) {
    return TPM_RC_SUCCESS;
  }
#endif
#ifdef TPM_CC_HMAC
  if (*target == TPM_CC_HMAC) {
    return TPM_RC_SUCCESS;
  }
#endif
#ifdef TPM_CC_Import
  if (*target == TPM_CC_Import) {
    return TPM_RC_SUCCESS;
  }
#endif
#ifdef TPM_CC_Load
  if (*target == TPM_CC_Load) {
    return TPM_RC_SUCCESS;
  }
#endif
#ifdef TPM_CC_Quote
  if (*target == TPM_CC_Quote) {
    return TPM_RC_SUCCESS;
  }
#endif
#ifdef TPM_CC_RSA_Decrypt
  if (*target == TPM_CC_RSA_Decrypt) {
    return TPM_RC_SUCCESS;
  }
#endif
#ifdef TPM_CC_HMAC_Start
  if (*target == TPM_CC_HMAC_Start) {
    return TPM_RC_SUCCESS;
  }
#endif
#ifdef TPM_CC_SequenceUpdate
  if (*target == TPM_CC_SequenceUpdate) {
    return TPM_RC_SUCCESS;
  }
#endif
#ifdef TPM_CC_Sign
  if (*target == TPM_CC_Sign) {
    return TPM_RC_SUCCESS;
  }
#endif
#ifdef TPM_CC_Unseal
  if (*target == TPM_CC_Unseal) {
    return TPM_RC_SUCCESS;
  }
#endif
#ifdef TPM_CC_PolicySigned
  if (*target == TPM_CC_PolicySigned) {
    return TPM_RC_SUCCESS;
  }
#endif
#ifdef TPM_CC_ContextLoad
  if (*target == TPM_CC_ContextLoad) {
    return TPM_RC_SUCCESS;
  }
#endif
#ifdef TPM_CC_ContextSave
  if (*target == TPM_CC_ContextSave) {
    return TPM_RC_SUCCESS;
  }
#endif
#ifdef TPM_CC_ECDH_KeyGen
  if (*target == TPM_CC_ECDH_KeyGen) {
    return TPM_RC_SUCCESS;
  }
#endif
#ifdef TPM_CC_EncryptDecrypt
  if (*target == TPM_CC_EncryptDecrypt) {
    return TPM_RC_SUCCESS;
  }
#endif
#ifdef TPM_CC_FlushContext
  if (*target == TPM_CC_FlushContext) {
    return TPM_RC_SUCCESS;
  }
#endif
#ifdef TPM_CC_LoadExternal
  if (*target == TPM_CC_LoadExternal) {
    return TPM_RC_SUCCESS;
  }
#endif
#ifdef TPM_CC_MakeCredential
  if (*target == TPM_CC_MakeCredential) {
    return TPM_RC_SUCCESS;
  }
#endif
#ifdef TPM_CC_NV_ReadPublic
  if (*target == TPM_CC_NV_ReadPublic) {
    return TPM_RC_SUCCESS;
  }
#endif
#ifdef TPM_CC_PolicyAuthorize
  if (*target == TPM_CC_PolicyAuthorize) {
    return TPM_RC_SUCCESS;
  }
#endif
#ifdef TPM_CC_PolicyAuthValue
  if (*target == TPM_CC_PolicyAuthValue) {
    return TPM_RC_SUCCESS;
  }
#endif
#ifdef TPM_CC_PolicyCommandCode
  if (*target == TPM_CC_PolicyCommandCode) {
    return TPM_RC_SUCCESS;
  }
#endif
#ifdef TPM_CC_PolicyCounterTimer
  if (*target == TPM_CC_PolicyCounterTimer) {
    return TPM_RC_SUCCESS;
  }
#endif
#ifdef TPM_CC_PolicyCpHash
  if (*target == TPM_CC_PolicyCpHash) {
    return TPM_RC_SUCCESS;
  }
#endif
#ifdef TPM_CC_PolicyLocality
  if (*target == TPM_CC_PolicyLocality) {
    return TPM_RC_SUCCESS;
  }
#endif
#ifdef TPM_CC_PolicyNameHash
  if (*target == TPM_CC_PolicyNameHash) {
    return TPM_RC_SUCCESS;
  }
#endif
#ifdef TPM_CC_PolicyOR
  if (*target == TPM_CC_PolicyOR) {
    return TPM_RC_SUCCESS;
  }
#endif
#ifdef TPM_CC_PolicyTicket
  if (*target == TPM_CC_PolicyTicket) {
    return TPM_RC_SUCCESS;
  }
#endif
#ifdef TPM_CC_ReadPublic
  if (*target == TPM_CC_ReadPublic) {
    return TPM_RC_SUCCESS;
  }
#endif
#ifdef TPM_CC_RSA_Encrypt
  if (*target == TPM_CC_RSA_Encrypt) {
    return TPM_RC_SUCCESS;
  }
#endif
#ifdef TPM_CC_StartAuthSession
  if (*target == TPM_CC_StartAuthSession) {
    return TPM_RC_SUCCESS;
  }
#endif
#ifdef TPM_CC_VerifySignature
  if (*target == TPM_CC_VerifySignature) {
    return TPM_RC_SUCCESS;
  }
#endif
#ifdef TPM_CC_ECC_Parameters
  if (*target == TPM_CC_ECC_Parameters) {
    return TPM_RC_SUCCESS;
  }
#endif
#ifdef TPM_CC_FirmwareRead
  if (*target == TPM_CC_FirmwareRead) {
    return TPM_RC_SUCCESS;
  }
#endif
#ifdef TPM_CC_GetCapability
  if (*target == TPM_CC_GetCapability) {
    return TPM_RC_SUCCESS;
  }
#endif
#ifdef TPM_CC_GetRandom
  if (*target == TPM_CC_GetRandom) {
    return TPM_RC_SUCCESS;
  }
#endif
#ifdef TPM_CC_GetTestResult
  if (*target == TPM_CC_GetTestResult) {
    return TPM_RC_SUCCESS;
  }
#endif
#ifdef TPM_CC_Hash
  if (*target == TPM_CC_Hash) {
    return TPM_RC_SUCCESS;
  }
#endif
#ifdef TPM_CC_PCR_Read
  if (*target == TPM_CC_PCR_Read) {
    return TPM_RC_SUCCESS;
  }
#endif
#ifdef TPM_CC_PolicyPCR
  if (*target == TPM_CC_PolicyPCR) {
    return TPM_RC_SUCCESS;
  }
#endif
#ifdef TPM_CC_PolicyRestart
  if (*target == TPM_CC_PolicyRestart) {
    return TPM_RC_SUCCESS;
  }
#endif
#ifdef TPM_CC_ReadClock
  if (*target == TPM_CC_ReadClock) {
    return TPM_RC_SUCCESS;
  }
#endif
#ifdef TPM_CC_PCR_Extend
  if (*target == TPM_CC_PCR_Extend) {
    return TPM_RC_SUCCESS;
  }
#endif
#ifdef TPM_CC_PCR_SetAuthValue
  if (*target == TPM_CC_PCR_SetAuthValue) {
    return TPM_RC_SUCCESS;
  }
#endif
#ifdef TPM_CC_NV_Certify
  if (*target == TPM_CC_NV_Certify) {
    return TPM_RC_SUCCESS;
  }
#endif
#ifdef TPM_CC_EventSequenceComplete
  if (*target == TPM_CC_EventSequenceComplete) {
    return TPM_RC_SUCCESS;
  }
#endif
#ifdef TPM_CC_HashSequenceStart
  if (*target == TPM_CC_HashSequenceStart) {
    return TPM_RC_SUCCESS;
  }
#endif
#ifdef TPM_CC_PolicyPhysicalPresence
  if (*target == TPM_CC_PolicyPhysicalPresence) {
    return TPM_RC_SUCCESS;
  }
#endif
#ifdef TPM_CC_PolicyDuplicationSelect
  if (*target == TPM_CC_PolicyDuplicationSelect) {
    return TPM_RC_SUCCESS;
  }
#endif
#ifdef TPM_CC_PolicyGetDigest
  if (*target == TPM_CC_PolicyGetDigest) {
    return TPM_RC_SUCCESS;
  }
#endif
#ifdef TPM_CC_TestParms
  if (*target == TPM_CC_TestParms) {
    return TPM_RC_SUCCESS;
  }
#endif
#ifdef TPM_CC_Commit
  if (*target == TPM_CC_Commit) {
    return TPM_RC_SUCCESS;
  }
#endif
#ifdef TPM_CC_PolicyPassword
  if (*target == TPM_CC_PolicyPassword) {
    return TPM_RC_SUCCESS;
  }
#endif
#ifdef TPM_CC_ZGen_2Phase
  if (*target == TPM_CC_ZGen_2Phase) {
    return TPM_RC_SUCCESS;
  }
#endif
#ifdef TPM_CC_EC_Ephemeral
  if (*target == TPM_CC_EC_Ephemeral) {
    return TPM_RC_SUCCESS;
  }
#endif
#ifdef TPM_CC_PolicyNvWritten
  if (*target == TPM_CC_PolicyNvWritten) {
    return TPM_RC_SUCCESS;
  }
#endif
#ifdef TPM_CC_LAST
  if (*target == TPM_CC_LAST) {
    return TPM_RC_SUCCESS;
  }
#endif
  return TPM_RC_COMMAND_CODE;
}

UINT16 TPML_CC_Marshal(TPML_CC* source, BYTE** buffer, INT32* size) {
  UINT16 total_size = 0;
  INT32 i;
  total_size += UINT32_Marshal(&source->count, buffer, size);
  for (i = 0; i < source->count; ++i) {
    total_size += TPM_CC_Marshal(&source->commandCodes[i], buffer, size);
  }
  return total_size;
}

TPM_RC TPML_CC_Unmarshal(TPML_CC* target, BYTE** buffer, INT32* size) {
  TPM_RC result;
  INT32 i;
  result = UINT32_Unmarshal(&target->count, buffer, size);
  if (result != TPM_RC_SUCCESS) {
    return result;
  }
  if (target->count > MAX_CAP_CC) {
    return TPM_RC_SIZE;
  }
  for (i = 0; i < target->count; ++i) {
    result = TPM_CC_Unmarshal(&target->commandCodes[i], buffer, size);
    if (result != TPM_RC_SUCCESS) {
      return result;
    }
  }
  return TPM_RC_SUCCESS;
}

UINT16 TPML_CCA_Marshal(TPML_CCA* source, BYTE** buffer, INT32* size) {
  UINT16 total_size = 0;
  INT32 i;
  total_size += UINT32_Marshal(&source->count, buffer, size);
  for (i = 0; i < source->count; ++i) {
    total_size += TPMA_CC_Marshal(&source->commandAttributes[i], buffer, size);
  }
  return total_size;
}

TPM_RC TPML_CCA_Unmarshal(TPML_CCA* target, BYTE** buffer, INT32* size) {
  TPM_RC result;
  INT32 i;
  result = UINT32_Unmarshal(&target->count, buffer, size);
  if (result != TPM_RC_SUCCESS) {
    return result;
  }
  if (target->count > MAX_CAP_CC) {
    return TPM_RC_SIZE;
  }
  for (i = 0; i < target->count; ++i) {
    result = TPMA_CC_Unmarshal(&target->commandAttributes[i], buffer, size);
    if (result != TPM_RC_SUCCESS) {
      return result;
    }
  }
  return TPM_RC_SUCCESS;
}

UINT16 TPML_DIGEST_Marshal(TPML_DIGEST* source, BYTE** buffer, INT32* size) {
  UINT16 total_size = 0;
  INT32 i;
  total_size += UINT32_Marshal(&source->count, buffer, size);
  for (i = 0; i < source->count; ++i) {
    total_size += TPM2B_DIGEST_Marshal(&source->digests[i], buffer, size);
  }
  return total_size;
}

TPM_RC TPML_DIGEST_Unmarshal(TPML_DIGEST* target, BYTE** buffer, INT32* size) {
  TPM_RC result;
  INT32 i;
  result = UINT32_Unmarshal(&target->count, buffer, size);
  if (result != TPM_RC_SUCCESS) {
    return result;
  }
  if (target->count > 8) {
    return TPM_RC_SIZE;
  }
  if (target->count < 2) {
    return TPM_RC_SIZE;
  }
  for (i = 0; i < target->count; ++i) {
    result = TPM2B_DIGEST_Unmarshal(&target->digests[i], buffer, size);
    if (result != TPM_RC_SUCCESS) {
      return result;
    }
  }
  return TPM_RC_SUCCESS;
}

UINT16 TPMU_HA_Marshal(TPMU_HA* source,
                       BYTE** buffer,
                       INT32* size,
                       UINT32 selector) {
  INT32 i;
  UINT16 total_size = 0;
  switch (selector) {
#ifdef TPM_ALG_SHA
    case TPM_ALG_SHA:
      for (i = 0; i < SHA_DIGEST_SIZE; ++i) {
        total_size += BYTE_Marshal(&source->sha[i], buffer, size);
      }
      return total_size;
#endif
#ifdef TPM_ALG_SHA1
    case TPM_ALG_SHA1:
      for (i = 0; i < SHA1_DIGEST_SIZE; ++i) {
        total_size += BYTE_Marshal(&source->sha1[i], buffer, size);
      }
      return total_size;
#endif
#ifdef TPM_ALG_SHA256
    case TPM_ALG_SHA256:
      for (i = 0; i < SHA256_DIGEST_SIZE; ++i) {
        total_size += BYTE_Marshal(&source->sha256[i], buffer, size);
      }
      return total_size;
#endif
#ifdef TPM_ALG_SHA384
    case TPM_ALG_SHA384:
      for (i = 0; i < SHA384_DIGEST_SIZE; ++i) {
        total_size += BYTE_Marshal(&source->sha384[i], buffer, size);
      }
      return total_size;
#endif
#ifdef TPM_ALG_SHA512
    case TPM_ALG_SHA512:
      for (i = 0; i < SHA512_DIGEST_SIZE; ++i) {
        total_size += BYTE_Marshal(&source->sha512[i], buffer, size);
      }
      return total_size;
#endif
#ifdef TPM_ALG_SM3_256
    case TPM_ALG_SM3_256:
      for (i = 0; i < SM3_256_DIGEST_SIZE; ++i) {
        total_size += BYTE_Marshal(&source->sm3_256[i], buffer, size);
      }
      return total_size;
#endif
#ifdef TPM_ALG_NULL
    case TPM_ALG_NULL:
      return 0;
#endif
  }
  return 0;
}

TPM_RC TPMU_HA_Unmarshal(TPMU_HA* target,
                         BYTE** buffer,
                         INT32* size,
                         UINT32 selector) {
  switch (selector) {
    INT32 i;
    TPM_RC result = TPM_RC_SUCCESS;
#ifdef TPM_ALG_SHA
    case TPM_ALG_SHA:
      for (i = 0; i < SHA_DIGEST_SIZE; ++i) {
        result = BYTE_Unmarshal(&target->sha[i], buffer, size);
        if (result != TPM_RC_SUCCESS) {
          return result;
        }
      }
      return TPM_RC_SUCCESS;
#endif
#ifdef TPM_ALG_SHA1
    case TPM_ALG_SHA1:
      for (i = 0; i < SHA1_DIGEST_SIZE; ++i) {
        result = BYTE_Unmarshal(&target->sha1[i], buffer, size);
        if (result != TPM_RC_SUCCESS) {
          return result;
        }
      }
      return TPM_RC_SUCCESS;
#endif
#ifdef TPM_ALG_SHA256
    case TPM_ALG_SHA256:
      for (i = 0; i < SHA256_DIGEST_SIZE; ++i) {
        result = BYTE_Unmarshal(&target->sha256[i], buffer, size);
        if (result != TPM_RC_SUCCESS) {
          return result;
        }
      }
      return TPM_RC_SUCCESS;
#endif
#ifdef TPM_ALG_SHA384
    case TPM_ALG_SHA384:
      for (i = 0; i < SHA384_DIGEST_SIZE; ++i) {
        result = BYTE_Unmarshal(&target->sha384[i], buffer, size);
        if (result != TPM_RC_SUCCESS) {
          return result;
        }
      }
      return TPM_RC_SUCCESS;
#endif
#ifdef TPM_ALG_SHA512
    case TPM_ALG_SHA512:
      for (i = 0; i < SHA512_DIGEST_SIZE; ++i) {
        result = BYTE_Unmarshal(&target->sha512[i], buffer, size);
        if (result != TPM_RC_SUCCESS) {
          return result;
        }
      }
      return TPM_RC_SUCCESS;
#endif
#ifdef TPM_ALG_SM3_256
    case TPM_ALG_SM3_256:
      for (i = 0; i < SM3_256_DIGEST_SIZE; ++i) {
        result = BYTE_Unmarshal(&target->sm3_256[i], buffer, size);
        if (result != TPM_RC_SUCCESS) {
          return result;
        }
      }
      return TPM_RC_SUCCESS;
#endif
#ifdef TPM_ALG_NULL
    case TPM_ALG_NULL:
      return TPM_RC_SUCCESS;
#endif
  }
  return TPM_RC_SELECTOR;
}

UINT16 TPMT_HA_Marshal(TPMT_HA* source, BYTE** buffer, INT32* size) {
  UINT16 total_size = 0;
  total_size += TPMI_ALG_HASH_Marshal(&source->hashAlg, buffer, size);
  total_size += TPMU_HA_Marshal(&source->digest, buffer, size, source->hashAlg);
  return total_size;
}

TPM_RC TPMT_HA_Unmarshal(TPMT_HA* target, BYTE** buffer, INT32* size) {
  TPM_RC result;
  result = TPMI_ALG_HASH_Unmarshal(&target->hashAlg, buffer, size, TRUE);
  if (result != TPM_RC_SUCCESS) {
    return result;
  }
  result = TPMU_HA_Unmarshal(&target->digest, buffer, size, target->hashAlg);
  if (result != TPM_RC_SUCCESS) {
    return result;
  }
  return TPM_RC_SUCCESS;
}

UINT16 TPML_DIGEST_VALUES_Marshal(TPML_DIGEST_VALUES* source,
                                  BYTE** buffer,
                                  INT32* size) {
  UINT16 total_size = 0;
  INT32 i;
  total_size += UINT32_Marshal(&source->count, buffer, size);
  for (i = 0; i < source->count; ++i) {
    total_size += TPMT_HA_Marshal(&source->digests[i], buffer, size);
  }
  return total_size;
}

TPM_RC TPML_DIGEST_VALUES_Unmarshal(TPML_DIGEST_VALUES* target,
                                    BYTE** buffer,
                                    INT32* size) {
  TPM_RC result;
  INT32 i;
  result = UINT32_Unmarshal(&target->count, buffer, size);
  if (result != TPM_RC_SUCCESS) {
    return result;
  }
  if (target->count > HASH_COUNT) {
    return TPM_RC_SIZE;
  }
  for (i = 0; i < target->count; ++i) {
    result = TPMT_HA_Unmarshal(&target->digests[i], buffer, size);
    if (result != TPM_RC_SUCCESS) {
      return result;
    }
  }
  return TPM_RC_SUCCESS;
}

UINT16 TPM_ECC_CURVE_Marshal(TPM_ECC_CURVE* source,
                             BYTE** buffer,
                             INT32* size) {
  return uint16_t_Marshal(source, buffer, size);
}

TPM_RC TPM_ECC_CURVE_Unmarshal(TPM_ECC_CURVE* target,
                               BYTE** buffer,
                               INT32* size) {
  TPM_RC result;
  result = uint16_t_Unmarshal(target, buffer, size);
  if (result != TPM_RC_SUCCESS) {
    return result;
  }
  if (*target == TPM_ECC_NONE) {
    return TPM_RC_SUCCESS;
  }
  if (*target == TPM_ECC_NIST_P192) {
    return TPM_RC_SUCCESS;
  }
  if (*target == TPM_ECC_NIST_P224) {
    return TPM_RC_SUCCESS;
  }
  if (*target == TPM_ECC_NIST_P256) {
    return TPM_RC_SUCCESS;
  }
  if (*target == TPM_ECC_NIST_P384) {
    return TPM_RC_SUCCESS;
  }
  if (*target == TPM_ECC_NIST_P521) {
    return TPM_RC_SUCCESS;
  }
  if (*target == TPM_ECC_BN_P256) {
    return TPM_RC_SUCCESS;
  }
  if (*target == TPM_ECC_BN_P638) {
    return TPM_RC_SUCCESS;
  }
  if (*target == TPM_ECC_SM2_P256) {
    return TPM_RC_SUCCESS;
  }
  return TPM_RC_CURVE;
}

UINT16 TPML_ECC_CURVE_Marshal(TPML_ECC_CURVE* source,
                              BYTE** buffer,
                              INT32* size) {
  UINT16 total_size = 0;
  INT32 i;
  total_size += UINT32_Marshal(&source->count, buffer, size);
  for (i = 0; i < source->count; ++i) {
    total_size += TPM_ECC_CURVE_Marshal(&source->eccCurves[i], buffer, size);
  }
  return total_size;
}

TPM_RC TPML_ECC_CURVE_Unmarshal(TPML_ECC_CURVE* target,
                                BYTE** buffer,
                                INT32* size) {
  TPM_RC result;
  INT32 i;
  result = UINT32_Unmarshal(&target->count, buffer, size);
  if (result != TPM_RC_SUCCESS) {
    return result;
  }
  if (target->count > MAX_ECC_CURVES) {
    return TPM_RC_SIZE;
  }
  for (i = 0; i < target->count; ++i) {
    result = TPM_ECC_CURVE_Unmarshal(&target->eccCurves[i], buffer, size);
    if (result != TPM_RC_SUCCESS) {
      return result;
    }
  }
  return TPM_RC_SUCCESS;
}

UINT16 TPM_HANDLE_Marshal(TPM_HANDLE* source, BYTE** buffer, INT32* size) {
  return uint32_t_Marshal(source, buffer, size);
}

TPM_RC TPM_HANDLE_Unmarshal(TPM_HANDLE* target, BYTE** buffer, INT32* size) {
  return uint32_t_Unmarshal(target, buffer, size);
}

UINT16 TPML_HANDLE_Marshal(TPML_HANDLE* source, BYTE** buffer, INT32* size) {
  UINT16 total_size = 0;
  INT32 i;
  total_size += UINT32_Marshal(&source->count, buffer, size);
  for (i = 0; i < source->count; ++i) {
    total_size += TPM_HANDLE_Marshal(&source->handle[i], buffer, size);
  }
  return total_size;
}

TPM_RC TPML_HANDLE_Unmarshal(TPML_HANDLE* target, BYTE** buffer, INT32* size) {
  TPM_RC result;
  INT32 i;
  result = UINT32_Unmarshal(&target->count, buffer, size);
  if (result != TPM_RC_SUCCESS) {
    return result;
  }
  if (target->count > MAX_CAP_HANDLES) {
    return TPM_RC_SIZE;
  }
  for (i = 0; i < target->count; ++i) {
    result = TPM_HANDLE_Unmarshal(&target->handle[i], buffer, size);
    if (result != TPM_RC_SUCCESS) {
      return result;
    }
  }
  return TPM_RC_SUCCESS;
}

UINT16 TPM_PT_Marshal(TPM_PT* source, BYTE** buffer, INT32* size) {
  return uint32_t_Marshal(source, buffer, size);
}

TPM_RC TPM_PT_Unmarshal(TPM_PT* target, BYTE** buffer, INT32* size) {
  TPM_RC result;
  result = uint32_t_Unmarshal(target, buffer, size);
  if (result != TPM_RC_SUCCESS) {
    return result;
  }
  if (*target == TPM_PT_NONE) {
    return TPM_RC_SUCCESS;
  }
  if (*target == PT_GROUP) {
    return TPM_RC_SUCCESS;
  }
  if (*target == PT_FIXED) {
    return TPM_RC_SUCCESS;
  }
  if (*target == TPM_PT_FAMILY_INDICATOR) {
    return TPM_RC_SUCCESS;
  }
  if (*target == TPM_PT_LEVEL) {
    return TPM_RC_SUCCESS;
  }
  if (*target == TPM_PT_REVISION) {
    return TPM_RC_SUCCESS;
  }
  if (*target == TPM_PT_DAY_OF_YEAR) {
    return TPM_RC_SUCCESS;
  }
  if (*target == TPM_PT_YEAR) {
    return TPM_RC_SUCCESS;
  }
  if (*target == TPM_PT_MANUFACTURER) {
    return TPM_RC_SUCCESS;
  }
  if (*target == TPM_PT_VENDOR_STRING_1) {
    return TPM_RC_SUCCESS;
  }
  if (*target == TPM_PT_VENDOR_STRING_2) {
    return TPM_RC_SUCCESS;
  }
  if (*target == TPM_PT_VENDOR_STRING_3) {
    return TPM_RC_SUCCESS;
  }
  if (*target == TPM_PT_VENDOR_STRING_4) {
    return TPM_RC_SUCCESS;
  }
  if (*target == TPM_PT_VENDOR_TPM_TYPE) {
    return TPM_RC_SUCCESS;
  }
  if (*target == TPM_PT_FIRMWARE_VERSION_1) {
    return TPM_RC_SUCCESS;
  }
  if (*target == TPM_PT_FIRMWARE_VERSION_2) {
    return TPM_RC_SUCCESS;
  }
  if (*target == TPM_PT_INPUT_BUFFER) {
    return TPM_RC_SUCCESS;
  }
  if (*target == TPM_PT_HR_TRANSIENT_MIN) {
    return TPM_RC_SUCCESS;
  }
  if (*target == TPM_PT_HR_PERSISTENT_MIN) {
    return TPM_RC_SUCCESS;
  }
  if (*target == TPM_PT_HR_LOADED_MIN) {
    return TPM_RC_SUCCESS;
  }
  if (*target == TPM_PT_ACTIVE_SESSIONS_MAX) {
    return TPM_RC_SUCCESS;
  }
  if (*target == TPM_PT_PCR_COUNT) {
    return TPM_RC_SUCCESS;
  }
  if (*target == TPM_PT_PCR_SELECT_MIN) {
    return TPM_RC_SUCCESS;
  }
  if (*target == TPM_PT_CONTEXT_GAP_MAX) {
    return TPM_RC_SUCCESS;
  }
  if (*target == TPM_PT_NV_COUNTERS_MAX) {
    return TPM_RC_SUCCESS;
  }
  if (*target == TPM_PT_NV_INDEX_MAX) {
    return TPM_RC_SUCCESS;
  }
  if (*target == TPM_PT_MEMORY) {
    return TPM_RC_SUCCESS;
  }
  if (*target == TPM_PT_CLOCK_UPDATE) {
    return TPM_RC_SUCCESS;
  }
  if (*target == TPM_PT_CONTEXT_HASH) {
    return TPM_RC_SUCCESS;
  }
  if (*target == TPM_PT_CONTEXT_SYM) {
    return TPM_RC_SUCCESS;
  }
  if (*target == TPM_PT_CONTEXT_SYM_SIZE) {
    return TPM_RC_SUCCESS;
  }
  if (*target == TPM_PT_ORDERLY_COUNT) {
    return TPM_RC_SUCCESS;
  }
  if (*target == TPM_PT_MAX_COMMAND_SIZE) {
    return TPM_RC_SUCCESS;
  }
  if (*target == TPM_PT_MAX_RESPONSE_SIZE) {
    return TPM_RC_SUCCESS;
  }
  if (*target == TPM_PT_MAX_DIGEST) {
    return TPM_RC_SUCCESS;
  }
  if (*target == TPM_PT_MAX_OBJECT_CONTEXT) {
    return TPM_RC_SUCCESS;
  }
  if (*target == TPM_PT_MAX_SESSION_CONTEXT) {
    return TPM_RC_SUCCESS;
  }
  if (*target == TPM_PT_PS_FAMILY_INDICATOR) {
    return TPM_RC_SUCCESS;
  }
  if (*target == TPM_PT_PS_LEVEL) {
    return TPM_RC_SUCCESS;
  }
  if (*target == TPM_PT_PS_REVISION) {
    return TPM_RC_SUCCESS;
  }
  if (*target == TPM_PT_PS_DAY_OF_YEAR) {
    return TPM_RC_SUCCESS;
  }
  if (*target == TPM_PT_PS_YEAR) {
    return TPM_RC_SUCCESS;
  }
  if (*target == TPM_PT_SPLIT_MAX) {
    return TPM_RC_SUCCESS;
  }
  if (*target == TPM_PT_TOTAL_COMMANDS) {
    return TPM_RC_SUCCESS;
  }
  if (*target == TPM_PT_LIBRARY_COMMANDS) {
    return TPM_RC_SUCCESS;
  }
  if (*target == TPM_PT_VENDOR_COMMANDS) {
    return TPM_RC_SUCCESS;
  }
  if (*target == TPM_PT_NV_BUFFER_MAX) {
    return TPM_RC_SUCCESS;
  }
  if (*target == PT_VAR) {
    return TPM_RC_SUCCESS;
  }
  if (*target == TPM_PT_PERMANENT) {
    return TPM_RC_SUCCESS;
  }
  if (*target == TPM_PT_STARTUP_CLEAR) {
    return TPM_RC_SUCCESS;
  }
  if (*target == TPM_PT_HR_NV_INDEX) {
    return TPM_RC_SUCCESS;
  }
  if (*target == TPM_PT_HR_LOADED) {
    return TPM_RC_SUCCESS;
  }
  if (*target == TPM_PT_HR_LOADED_AVAIL) {
    return TPM_RC_SUCCESS;
  }
  if (*target == TPM_PT_HR_ACTIVE) {
    return TPM_RC_SUCCESS;
  }
  if (*target == TPM_PT_HR_ACTIVE_AVAIL) {
    return TPM_RC_SUCCESS;
  }
  if (*target == TPM_PT_HR_TRANSIENT_AVAIL) {
    return TPM_RC_SUCCESS;
  }
  if (*target == TPM_PT_HR_PERSISTENT) {
    return TPM_RC_SUCCESS;
  }
  if (*target == TPM_PT_HR_PERSISTENT_AVAIL) {
    return TPM_RC_SUCCESS;
  }
  if (*target == TPM_PT_NV_COUNTERS) {
    return TPM_RC_SUCCESS;
  }
  if (*target == TPM_PT_NV_COUNTERS_AVAIL) {
    return TPM_RC_SUCCESS;
  }
  if (*target == TPM_PT_ALGORITHM_SET) {
    return TPM_RC_SUCCESS;
  }
  if (*target == TPM_PT_LOADED_CURVES) {
    return TPM_RC_SUCCESS;
  }
  if (*target == TPM_PT_LOCKOUT_COUNTER) {
    return TPM_RC_SUCCESS;
  }
  if (*target == TPM_PT_MAX_AUTH_FAIL) {
    return TPM_RC_SUCCESS;
  }
  if (*target == TPM_PT_LOCKOUT_INTERVAL) {
    return TPM_RC_SUCCESS;
  }
  if (*target == TPM_PT_LOCKOUT_RECOVERY) {
    return TPM_RC_SUCCESS;
  }
  if (*target == TPM_PT_NV_WRITE_RECOVERY) {
    return TPM_RC_SUCCESS;
  }
  if (*target == TPM_PT_AUDIT_COUNTER_0) {
    return TPM_RC_SUCCESS;
  }
  if (*target == TPM_PT_AUDIT_COUNTER_1) {
    return TPM_RC_SUCCESS;
  }
  return TPM_RC_VALUE;
}

UINT16 TPMS_TAGGED_PCR_SELECT_Marshal(TPMS_TAGGED_PCR_SELECT* source,
                                      BYTE** buffer,
                                      INT32* size) {
  UINT16 total_size = 0;
  INT32 i;
  total_size += TPM_PT_Marshal(&source->tag, buffer, size);
  total_size += UINT8_Marshal(&source->sizeofSelect, buffer, size);
  for (i = 0; i < source->sizeofSelect; ++i) {
    total_size += BYTE_Marshal(&source->pcrSelect[i], buffer, size);
  }
  return total_size;
}

TPM_RC TPMS_TAGGED_PCR_SELECT_Unmarshal(TPMS_TAGGED_PCR_SELECT* target,
                                        BYTE** buffer,
                                        INT32* size) {
  TPM_RC result;
  INT32 i;
  result = TPM_PT_Unmarshal(&target->tag, buffer, size);
  if (result != TPM_RC_SUCCESS) {
    return result;
  }
  result = UINT8_Unmarshal(&target->sizeofSelect, buffer, size);
  if (result != TPM_RC_SUCCESS) {
    return result;
  }
  if (target->sizeofSelect > PCR_SELECT_MAX) {
    return TPM_RC_VALUE;
  }
  if (target->sizeofSelect < PCR_SELECT_MIN) {
    return TPM_RC_VALUE;
  }
  for (i = 0; i < target->sizeofSelect; ++i) {
    result = BYTE_Unmarshal(&target->pcrSelect[i], buffer, size);
    if (result != TPM_RC_SUCCESS) {
      return result;
    }
  }
  return TPM_RC_SUCCESS;
}

UINT16 TPML_TAGGED_PCR_PROPERTY_Marshal(TPML_TAGGED_PCR_PROPERTY* source,
                                        BYTE** buffer,
                                        INT32* size) {
  UINT16 total_size = 0;
  INT32 i;
  total_size += UINT32_Marshal(&source->count, buffer, size);
  for (i = 0; i < source->count; ++i) {
    total_size +=
        TPMS_TAGGED_PCR_SELECT_Marshal(&source->pcrProperty[i], buffer, size);
  }
  return total_size;
}

TPM_RC TPML_TAGGED_PCR_PROPERTY_Unmarshal(TPML_TAGGED_PCR_PROPERTY* target,
                                          BYTE** buffer,
                                          INT32* size) {
  TPM_RC result;
  INT32 i;
  result = UINT32_Unmarshal(&target->count, buffer, size);
  if (result != TPM_RC_SUCCESS) {
    return result;
  }
  if (target->count > MAX_PCR_PROPERTIES) {
    return TPM_RC_SIZE;
  }
  for (i = 0; i < target->count; ++i) {
    result =
        TPMS_TAGGED_PCR_SELECT_Unmarshal(&target->pcrProperty[i], buffer, size);
    if (result != TPM_RC_SUCCESS) {
      return result;
    }
  }
  return TPM_RC_SUCCESS;
}

UINT16 TPMS_TAGGED_PROPERTY_Marshal(TPMS_TAGGED_PROPERTY* source,
                                    BYTE** buffer,
                                    INT32* size) {
  UINT16 total_size = 0;
  total_size += TPM_PT_Marshal(&source->property, buffer, size);
  total_size += UINT32_Marshal(&source->value, buffer, size);
  return total_size;
}

TPM_RC TPMS_TAGGED_PROPERTY_Unmarshal(TPMS_TAGGED_PROPERTY* target,
                                      BYTE** buffer,
                                      INT32* size) {
  TPM_RC result;
  result = TPM_PT_Unmarshal(&target->property, buffer, size);
  if (result != TPM_RC_SUCCESS) {
    return result;
  }
  result = UINT32_Unmarshal(&target->value, buffer, size);
  if (result != TPM_RC_SUCCESS) {
    return result;
  }
  return TPM_RC_SUCCESS;
}

UINT16 TPML_TAGGED_TPM_PROPERTY_Marshal(TPML_TAGGED_TPM_PROPERTY* source,
                                        BYTE** buffer,
                                        INT32* size) {
  UINT16 total_size = 0;
  INT32 i;
  total_size += UINT32_Marshal(&source->count, buffer, size);
  for (i = 0; i < source->count; ++i) {
    total_size +=
        TPMS_TAGGED_PROPERTY_Marshal(&source->tpmProperty[i], buffer, size);
  }
  return total_size;
}

TPM_RC TPML_TAGGED_TPM_PROPERTY_Unmarshal(TPML_TAGGED_TPM_PROPERTY* target,
                                          BYTE** buffer,
                                          INT32* size) {
  TPM_RC result;
  INT32 i;
  result = UINT32_Unmarshal(&target->count, buffer, size);
  if (result != TPM_RC_SUCCESS) {
    return result;
  }
  if (target->count > MAX_TPM_PROPERTIES) {
    return TPM_RC_SIZE;
  }
  for (i = 0; i < target->count; ++i) {
    result =
        TPMS_TAGGED_PROPERTY_Unmarshal(&target->tpmProperty[i], buffer, size);
    if (result != TPM_RC_SUCCESS) {
      return result;
    }
  }
  return TPM_RC_SUCCESS;
}

UINT16 TPMS_ALGORITHM_DESCRIPTION_Marshal(TPMS_ALGORITHM_DESCRIPTION* source,
                                          BYTE** buffer,
                                          INT32* size) {
  UINT16 total_size = 0;
  total_size += TPM_ALG_ID_Marshal(&source->alg, buffer, size);
  total_size += TPMA_ALGORITHM_Marshal(&source->attributes, buffer, size);
  return total_size;
}

TPM_RC TPMS_ALGORITHM_DESCRIPTION_Unmarshal(TPMS_ALGORITHM_DESCRIPTION* target,
                                            BYTE** buffer,
                                            INT32* size) {
  TPM_RC result;
  result = TPM_ALG_ID_Unmarshal(&target->alg, buffer, size);
  if (result != TPM_RC_SUCCESS) {
    return result;
  }
  result = TPMA_ALGORITHM_Unmarshal(&target->attributes, buffer, size);
  if (result != TPM_RC_SUCCESS) {
    return result;
  }
  return TPM_RC_SUCCESS;
}

UINT16 TPMS_ALGORITHM_DETAIL_ECC_Marshal(TPMS_ALGORITHM_DETAIL_ECC* source,
                                         BYTE** buffer,
                                         INT32* size) {
  UINT16 total_size = 0;
  total_size += TPM_ECC_CURVE_Marshal(&source->curveID, buffer, size);
  total_size += UINT16_Marshal(&source->keySize, buffer, size);
  total_size += TPMT_KDF_SCHEME_Marshal(&source->kdf, buffer, size);
  total_size += TPMT_ECC_SCHEME_Marshal(&source->sign, buffer, size);
  total_size += TPM2B_ECC_PARAMETER_Marshal(&source->p, buffer, size);
  total_size += TPM2B_ECC_PARAMETER_Marshal(&source->a, buffer, size);
  total_size += TPM2B_ECC_PARAMETER_Marshal(&source->b, buffer, size);
  total_size += TPM2B_ECC_PARAMETER_Marshal(&source->gX, buffer, size);
  total_size += TPM2B_ECC_PARAMETER_Marshal(&source->gY, buffer, size);
  total_size += TPM2B_ECC_PARAMETER_Marshal(&source->n, buffer, size);
  total_size += TPM2B_ECC_PARAMETER_Marshal(&source->h, buffer, size);
  return total_size;
}

TPM_RC TPMS_ALGORITHM_DETAIL_ECC_Unmarshal(TPMS_ALGORITHM_DETAIL_ECC* target,
                                           BYTE** buffer,
                                           INT32* size) {
  TPM_RC result;
  result = TPM_ECC_CURVE_Unmarshal(&target->curveID, buffer, size);
  if (result != TPM_RC_SUCCESS) {
    return result;
  }
  result = UINT16_Unmarshal(&target->keySize, buffer, size);
  if (result != TPM_RC_SUCCESS) {
    return result;
  }
  result = TPMT_KDF_SCHEME_Unmarshal(&target->kdf, buffer, size);
  if (result != TPM_RC_SUCCESS) {
    return result;
  }
  result = TPMT_ECC_SCHEME_Unmarshal(&target->sign, buffer, size);
  if (result != TPM_RC_SUCCESS) {
    return result;
  }
  result = TPM2B_ECC_PARAMETER_Unmarshal(&target->p, buffer, size);
  if (result != TPM_RC_SUCCESS) {
    return result;
  }
  result = TPM2B_ECC_PARAMETER_Unmarshal(&target->a, buffer, size);
  if (result != TPM_RC_SUCCESS) {
    return result;
  }
  result = TPM2B_ECC_PARAMETER_Unmarshal(&target->b, buffer, size);
  if (result != TPM_RC_SUCCESS) {
    return result;
  }
  result = TPM2B_ECC_PARAMETER_Unmarshal(&target->gX, buffer, size);
  if (result != TPM_RC_SUCCESS) {
    return result;
  }
  result = TPM2B_ECC_PARAMETER_Unmarshal(&target->gY, buffer, size);
  if (result != TPM_RC_SUCCESS) {
    return result;
  }
  result = TPM2B_ECC_PARAMETER_Unmarshal(&target->n, buffer, size);
  if (result != TPM_RC_SUCCESS) {
    return result;
  }
  result = TPM2B_ECC_PARAMETER_Unmarshal(&target->h, buffer, size);
  if (result != TPM_RC_SUCCESS) {
    return result;
  }
  return TPM_RC_SUCCESS;
}

UINT16 UINT64_Marshal(UINT64* source, BYTE** buffer, INT32* size) {
  return uint64_t_Marshal(source, buffer, size);
}

TPM_RC UINT64_Unmarshal(UINT64* target, BYTE** buffer, INT32* size) {
  return uint64_t_Unmarshal(target, buffer, size);
}

UINT16 TPM_GENERATED_Marshal(TPM_GENERATED* source,
                             BYTE** buffer,
                             INT32* size) {
  return uint32_t_Marshal(source, buffer, size);
}

TPM_RC TPM_GENERATED_Unmarshal(TPM_GENERATED* target,
                               BYTE** buffer,
                               INT32* size) {
  TPM_RC result;
  result = uint32_t_Unmarshal(target, buffer, size);
  if (result != TPM_RC_SUCCESS) {
    return result;
  }
  if (*target == TPM_GENERATED_VALUE) {
    return TPM_RC_SUCCESS;
  }
  return TPM_RC_VALUE;
}

UINT16 TPMS_CREATION_INFO_Marshal(TPMS_CREATION_INFO* source,
                                  BYTE** buffer,
                                  INT32* size) {
  UINT16 total_size = 0;
  total_size += TPM2B_NAME_Marshal(&source->objectName, buffer, size);
  total_size += TPM2B_DIGEST_Marshal(&source->creationHash, buffer, size);
  return total_size;
}

TPM_RC TPMS_CREATION_INFO_Unmarshal(TPMS_CREATION_INFO* target,
                                    BYTE** buffer,
                                    INT32* size) {
  TPM_RC result;
  result = TPM2B_NAME_Unmarshal(&target->objectName, buffer, size);
  if (result != TPM_RC_SUCCESS) {
    return result;
  }
  result = TPM2B_DIGEST_Unmarshal(&target->creationHash, buffer, size);
  if (result != TPM_RC_SUCCESS) {
    return result;
  }
  return TPM_RC_SUCCESS;
}

UINT16 TPMS_COMMAND_AUDIT_INFO_Marshal(TPMS_COMMAND_AUDIT_INFO* source,
                                       BYTE** buffer,
                                       INT32* size) {
  UINT16 total_size = 0;
  total_size += UINT64_Marshal(&source->auditCounter, buffer, size);
  total_size += TPM_ALG_ID_Marshal(&source->digestAlg, buffer, size);
  total_size += TPM2B_DIGEST_Marshal(&source->auditDigest, buffer, size);
  total_size += TPM2B_DIGEST_Marshal(&source->commandDigest, buffer, size);
  return total_size;
}

TPM_RC TPMS_COMMAND_AUDIT_INFO_Unmarshal(TPMS_COMMAND_AUDIT_INFO* target,
                                         BYTE** buffer,
                                         INT32* size) {
  TPM_RC result;
  result = UINT64_Unmarshal(&target->auditCounter, buffer, size);
  if (result != TPM_RC_SUCCESS) {
    return result;
  }
  result = TPM_ALG_ID_Unmarshal(&target->digestAlg, buffer, size);
  if (result != TPM_RC_SUCCESS) {
    return result;
  }
  result = TPM2B_DIGEST_Unmarshal(&target->auditDigest, buffer, size);
  if (result != TPM_RC_SUCCESS) {
    return result;
  }
  result = TPM2B_DIGEST_Unmarshal(&target->commandDigest, buffer, size);
  if (result != TPM_RC_SUCCESS) {
    return result;
  }
  return TPM_RC_SUCCESS;
}

UINT16 TPMS_QUOTE_INFO_Marshal(TPMS_QUOTE_INFO* source,
                               BYTE** buffer,
                               INT32* size) {
  UINT16 total_size = 0;
  total_size += TPML_PCR_SELECTION_Marshal(&source->pcrSelect, buffer, size);
  total_size += TPM2B_DIGEST_Marshal(&source->pcrDigest, buffer, size);
  return total_size;
}

TPM_RC TPMS_QUOTE_INFO_Unmarshal(TPMS_QUOTE_INFO* target,
                                 BYTE** buffer,
                                 INT32* size) {
  TPM_RC result;
  result = TPML_PCR_SELECTION_Unmarshal(&target->pcrSelect, buffer, size);
  if (result != TPM_RC_SUCCESS) {
    return result;
  }
  result = TPM2B_DIGEST_Unmarshal(&target->pcrDigest, buffer, size);
  if (result != TPM_RC_SUCCESS) {
    return result;
  }
  return TPM_RC_SUCCESS;
}

UINT16 TPMS_CERTIFY_INFO_Marshal(TPMS_CERTIFY_INFO* source,
                                 BYTE** buffer,
                                 INT32* size) {
  UINT16 total_size = 0;
  total_size += TPM2B_NAME_Marshal(&source->name, buffer, size);
  total_size += TPM2B_NAME_Marshal(&source->qualifiedName, buffer, size);
  return total_size;
}

TPM_RC TPMS_CERTIFY_INFO_Unmarshal(TPMS_CERTIFY_INFO* target,
                                   BYTE** buffer,
                                   INT32* size) {
  TPM_RC result;
  result = TPM2B_NAME_Unmarshal(&target->name, buffer, size);
  if (result != TPM_RC_SUCCESS) {
    return result;
  }
  result = TPM2B_NAME_Unmarshal(&target->qualifiedName, buffer, size);
  if (result != TPM_RC_SUCCESS) {
    return result;
  }
  return TPM_RC_SUCCESS;
}

UINT16 TPMS_SESSION_AUDIT_INFO_Marshal(TPMS_SESSION_AUDIT_INFO* source,
                                       BYTE** buffer,
                                       INT32* size) {
  UINT16 total_size = 0;
  total_size += TPMI_YES_NO_Marshal(&source->exclusiveSession, buffer, size);
  total_size += TPM2B_DIGEST_Marshal(&source->sessionDigest, buffer, size);
  return total_size;
}

TPM_RC TPMS_SESSION_AUDIT_INFO_Unmarshal(TPMS_SESSION_AUDIT_INFO* target,
                                         BYTE** buffer,
                                         INT32* size) {
  TPM_RC result;
  result = TPMI_YES_NO_Unmarshal(&target->exclusiveSession, buffer, size);
  if (result != TPM_RC_SUCCESS) {
    return result;
  }
  result = TPM2B_DIGEST_Unmarshal(&target->sessionDigest, buffer, size);
  if (result != TPM_RC_SUCCESS) {
    return result;
  }
  return TPM_RC_SUCCESS;
}

UINT16 TPMS_CLOCK_INFO_Marshal(TPMS_CLOCK_INFO* source,
                               BYTE** buffer,
                               INT32* size) {
  UINT16 total_size = 0;
  total_size += UINT64_Marshal(&source->clock, buffer, size);
  total_size += UINT32_Marshal(&source->resetCount, buffer, size);
  total_size += UINT32_Marshal(&source->restartCount, buffer, size);
  total_size += TPMI_YES_NO_Marshal(&source->safe, buffer, size);
  return total_size;
}

TPM_RC TPMS_CLOCK_INFO_Unmarshal(TPMS_CLOCK_INFO* target,
                                 BYTE** buffer,
                                 INT32* size) {
  TPM_RC result;
  result = UINT64_Unmarshal(&target->clock, buffer, size);
  if (result != TPM_RC_SUCCESS) {
    return result;
  }
  result = UINT32_Unmarshal(&target->resetCount, buffer, size);
  if (result != TPM_RC_SUCCESS) {
    return result;
  }
  result = UINT32_Unmarshal(&target->restartCount, buffer, size);
  if (result != TPM_RC_SUCCESS) {
    return result;
  }
  result = TPMI_YES_NO_Unmarshal(&target->safe, buffer, size);
  if (result != TPM_RC_SUCCESS) {
    return result;
  }
  return TPM_RC_SUCCESS;
}

UINT16 TPMS_TIME_INFO_Marshal(TPMS_TIME_INFO* source,
                              BYTE** buffer,
                              INT32* size) {
  UINT16 total_size = 0;
  total_size += UINT64_Marshal(&source->time, buffer, size);
  total_size += TPMS_CLOCK_INFO_Marshal(&source->clockInfo, buffer, size);
  return total_size;
}

TPM_RC TPMS_TIME_INFO_Unmarshal(TPMS_TIME_INFO* target,
                                BYTE** buffer,
                                INT32* size) {
  TPM_RC result;
  result = UINT64_Unmarshal(&target->time, buffer, size);
  if (result != TPM_RC_SUCCESS) {
    return result;
  }
  result = TPMS_CLOCK_INFO_Unmarshal(&target->clockInfo, buffer, size);
  if (result != TPM_RC_SUCCESS) {
    return result;
  }
  return TPM_RC_SUCCESS;
}

UINT16 TPMS_TIME_ATTEST_INFO_Marshal(TPMS_TIME_ATTEST_INFO* source,
                                     BYTE** buffer,
                                     INT32* size) {
  UINT16 total_size = 0;
  total_size += TPMS_TIME_INFO_Marshal(&source->time, buffer, size);
  total_size += UINT64_Marshal(&source->firmwareVersion, buffer, size);
  return total_size;
}

TPM_RC TPMS_TIME_ATTEST_INFO_Unmarshal(TPMS_TIME_ATTEST_INFO* target,
                                       BYTE** buffer,
                                       INT32* size) {
  TPM_RC result;
  result = TPMS_TIME_INFO_Unmarshal(&target->time, buffer, size);
  if (result != TPM_RC_SUCCESS) {
    return result;
  }
  result = UINT64_Unmarshal(&target->firmwareVersion, buffer, size);
  if (result != TPM_RC_SUCCESS) {
    return result;
  }
  return TPM_RC_SUCCESS;
}

UINT16 TPMS_NV_CERTIFY_INFO_Marshal(TPMS_NV_CERTIFY_INFO* source,
                                    BYTE** buffer,
                                    INT32* size) {
  UINT16 total_size = 0;
  total_size += TPM2B_NAME_Marshal(&source->indexName, buffer, size);
  total_size += UINT16_Marshal(&source->offset, buffer, size);
  total_size += TPM2B_MAX_NV_BUFFER_Marshal(&source->nvContents, buffer, size);
  return total_size;
}

TPM_RC TPMS_NV_CERTIFY_INFO_Unmarshal(TPMS_NV_CERTIFY_INFO* target,
                                      BYTE** buffer,
                                      INT32* size) {
  TPM_RC result;
  result = TPM2B_NAME_Unmarshal(&target->indexName, buffer, size);
  if (result != TPM_RC_SUCCESS) {
    return result;
  }
  result = UINT16_Unmarshal(&target->offset, buffer, size);
  if (result != TPM_RC_SUCCESS) {
    return result;
  }
  result = TPM2B_MAX_NV_BUFFER_Unmarshal(&target->nvContents, buffer, size);
  if (result != TPM_RC_SUCCESS) {
    return result;
  }
  return TPM_RC_SUCCESS;
}

UINT16 TPMU_ATTEST_Marshal(TPMU_ATTEST* source,
                           BYTE** buffer,
                           INT32* size,
                           UINT32 selector) {
  switch (selector) {
    case TPM_ST_ATTEST_CERTIFY:
      return TPMS_CERTIFY_INFO_Marshal((TPMS_CERTIFY_INFO*)&source->certify,
                                       buffer, size);
    case TPM_ST_ATTEST_CREATION:
      return TPMS_CREATION_INFO_Marshal((TPMS_CREATION_INFO*)&source->creation,
                                        buffer, size);
    case TPM_ST_ATTEST_QUOTE:
      return TPMS_QUOTE_INFO_Marshal((TPMS_QUOTE_INFO*)&source->quote, buffer,
                                     size);
    case TPM_ST_ATTEST_COMMAND_AUDIT:
      return TPMS_COMMAND_AUDIT_INFO_Marshal(
          (TPMS_COMMAND_AUDIT_INFO*)&source->commandAudit, buffer, size);
    case TPM_ST_ATTEST_SESSION_AUDIT:
      return TPMS_SESSION_AUDIT_INFO_Marshal(
          (TPMS_SESSION_AUDIT_INFO*)&source->sessionAudit, buffer, size);
    case TPM_ST_ATTEST_TIME:
      return TPMS_TIME_ATTEST_INFO_Marshal(
          (TPMS_TIME_ATTEST_INFO*)&source->time, buffer, size);
    case TPM_ST_ATTEST_NV:
      return TPMS_NV_CERTIFY_INFO_Marshal((TPMS_NV_CERTIFY_INFO*)&source->nv,
                                          buffer, size);
  }
  return 0;
}

TPM_RC TPMU_ATTEST_Unmarshal(TPMU_ATTEST* target,
                             BYTE** buffer,
                             INT32* size,
                             UINT32 selector) {
  switch (selector) {
    case TPM_ST_ATTEST_CERTIFY:
      return TPMS_CERTIFY_INFO_Unmarshal((TPMS_CERTIFY_INFO*)&target->certify,
                                         buffer, size);
    case TPM_ST_ATTEST_CREATION:
      return TPMS_CREATION_INFO_Unmarshal(
          (TPMS_CREATION_INFO*)&target->creation, buffer, size);
    case TPM_ST_ATTEST_QUOTE:
      return TPMS_QUOTE_INFO_Unmarshal((TPMS_QUOTE_INFO*)&target->quote, buffer,
                                       size);
    case TPM_ST_ATTEST_COMMAND_AUDIT:
      return TPMS_COMMAND_AUDIT_INFO_Unmarshal(
          (TPMS_COMMAND_AUDIT_INFO*)&target->commandAudit, buffer, size);
    case TPM_ST_ATTEST_SESSION_AUDIT:
      return TPMS_SESSION_AUDIT_INFO_Unmarshal(
          (TPMS_SESSION_AUDIT_INFO*)&target->sessionAudit, buffer, size);
    case TPM_ST_ATTEST_TIME:
      return TPMS_TIME_ATTEST_INFO_Unmarshal(
          (TPMS_TIME_ATTEST_INFO*)&target->time, buffer, size);
    case TPM_ST_ATTEST_NV:
      return TPMS_NV_CERTIFY_INFO_Unmarshal((TPMS_NV_CERTIFY_INFO*)&target->nv,
                                            buffer, size);
  }
  return TPM_RC_SELECTOR;
}

UINT16 TPMS_ATTEST_Marshal(TPMS_ATTEST* source, BYTE** buffer, INT32* size) {
  UINT16 total_size = 0;
  total_size += TPM_GENERATED_Marshal(&source->magic, buffer, size);
  total_size += TPMI_ST_ATTEST_Marshal(&source->type, buffer, size);
  total_size += TPM2B_NAME_Marshal(&source->qualifiedSigner, buffer, size);
  total_size += TPM2B_DATA_Marshal(&source->extraData, buffer, size);
  total_size += TPMS_CLOCK_INFO_Marshal(&source->clockInfo, buffer, size);
  total_size += UINT64_Marshal(&source->firmwareVersion, buffer, size);
  total_size +=
      TPMU_ATTEST_Marshal(&source->attested, buffer, size, source->type);
  return total_size;
}

TPM_RC TPMS_ATTEST_Unmarshal(TPMS_ATTEST* target, BYTE** buffer, INT32* size) {
  TPM_RC result;
  result = TPM_GENERATED_Unmarshal(&target->magic, buffer, size);
  if (result != TPM_RC_SUCCESS) {
    return result;
  }
  result = TPMI_ST_ATTEST_Unmarshal(&target->type, buffer, size);
  if (result != TPM_RC_SUCCESS) {
    return result;
  }
  result = TPM2B_NAME_Unmarshal(&target->qualifiedSigner, buffer, size);
  if (result != TPM_RC_SUCCESS) {
    return result;
  }
  result = TPM2B_DATA_Unmarshal(&target->extraData, buffer, size);
  if (result != TPM_RC_SUCCESS) {
    return result;
  }
  result = TPMS_CLOCK_INFO_Unmarshal(&target->clockInfo, buffer, size);
  if (result != TPM_RC_SUCCESS) {
    return result;
  }
  result = UINT64_Unmarshal(&target->firmwareVersion, buffer, size);
  if (result != TPM_RC_SUCCESS) {
    return result;
  }
  result = TPMU_ATTEST_Unmarshal(&target->attested, buffer, size, target->type);
  if (result != TPM_RC_SUCCESS) {
    return result;
  }
  return TPM_RC_SUCCESS;
}

UINT16 TPMS_AUTH_COMMAND_Marshal(TPMS_AUTH_COMMAND* source,
                                 BYTE** buffer,
                                 INT32* size) {
  UINT16 total_size = 0;
  total_size +=
      TPMI_SH_AUTH_SESSION_Marshal(&source->sessionHandle, buffer, size);
  total_size += TPM2B_NONCE_Marshal(&source->nonce, buffer, size);
  total_size += TPMA_SESSION_Marshal(&source->sessionAttributes, buffer, size);
  total_size += TPM2B_AUTH_Marshal(&source->hmac, buffer, size);
  return total_size;
}

TPM_RC TPMS_AUTH_COMMAND_Unmarshal(TPMS_AUTH_COMMAND* target,
                                   BYTE** buffer,
                                   INT32* size) {
  TPM_RC result;
  result = TPMI_SH_AUTH_SESSION_Unmarshal(&target->sessionHandle, buffer, size,
                                          TRUE);
  if (result != TPM_RC_SUCCESS) {
    return result;
  }
  result = TPM2B_NONCE_Unmarshal(&target->nonce, buffer, size);
  if (result != TPM_RC_SUCCESS) {
    return result;
  }
  result = TPMA_SESSION_Unmarshal(&target->sessionAttributes, buffer, size);
  if (result != TPM_RC_SUCCESS) {
    return result;
  }
  result = TPM2B_AUTH_Unmarshal(&target->hmac, buffer, size);
  if (result != TPM_RC_SUCCESS) {
    return result;
  }
  return TPM_RC_SUCCESS;
}

UINT16 TPMS_AUTH_RESPONSE_Marshal(TPMS_AUTH_RESPONSE* source,
                                  BYTE** buffer,
                                  INT32* size) {
  UINT16 total_size = 0;
  total_size += TPM2B_NONCE_Marshal(&source->nonce, buffer, size);
  total_size += TPMA_SESSION_Marshal(&source->sessionAttributes, buffer, size);
  total_size += TPM2B_AUTH_Marshal(&source->hmac, buffer, size);
  return total_size;
}

TPM_RC TPMS_AUTH_RESPONSE_Unmarshal(TPMS_AUTH_RESPONSE* target,
                                    BYTE** buffer,
                                    INT32* size) {
  TPM_RC result;
  result = TPM2B_NONCE_Unmarshal(&target->nonce, buffer, size);
  if (result != TPM_RC_SUCCESS) {
    return result;
  }
  result = TPMA_SESSION_Unmarshal(&target->sessionAttributes, buffer, size);
  if (result != TPM_RC_SUCCESS) {
    return result;
  }
  result = TPM2B_AUTH_Unmarshal(&target->hmac, buffer, size);
  if (result != TPM_RC_SUCCESS) {
    return result;
  }
  return TPM_RC_SUCCESS;
}

UINT16 TPM_CAP_Marshal(TPM_CAP* source, BYTE** buffer, INT32* size) {
  return uint32_t_Marshal(source, buffer, size);
}

TPM_RC TPM_CAP_Unmarshal(TPM_CAP* target, BYTE** buffer, INT32* size) {
  TPM_RC result;
  result = uint32_t_Unmarshal(target, buffer, size);
  if (result != TPM_RC_SUCCESS) {
    return result;
  }
  if (*target == TPM_CAP_FIRST) {
    return TPM_RC_SUCCESS;
  }
  if (*target == TPM_CAP_ALGS) {
    return TPM_RC_SUCCESS;
  }
  if (*target == TPM_CAP_HANDLES) {
    return TPM_RC_SUCCESS;
  }
  if (*target == TPM_CAP_COMMANDS) {
    return TPM_RC_SUCCESS;
  }
  if (*target == TPM_CAP_PP_COMMANDS) {
    return TPM_RC_SUCCESS;
  }
  if (*target == TPM_CAP_AUDIT_COMMANDS) {
    return TPM_RC_SUCCESS;
  }
  if (*target == TPM_CAP_PCRS) {
    return TPM_RC_SUCCESS;
  }
  if (*target == TPM_CAP_TPM_PROPERTIES) {
    return TPM_RC_SUCCESS;
  }
  if (*target == TPM_CAP_PCR_PROPERTIES) {
    return TPM_RC_SUCCESS;
  }
  if (*target == TPM_CAP_ECC_CURVES) {
    return TPM_RC_SUCCESS;
  }
  if (*target == TPM_CAP_LAST) {
    return TPM_RC_SUCCESS;
  }
  if (*target == TPM_CAP_VENDOR_PROPERTY) {
    return TPM_RC_SUCCESS;
  }
  return TPM_RC_VALUE;
}

UINT16 TPMU_CAPABILITIES_Marshal(TPMU_CAPABILITIES* source,
                                 BYTE** buffer,
                                 INT32* size,
                                 UINT32 selector) {
  switch (selector) {
    case TPM_CAP_ALGS:
      return TPML_ALG_PROPERTY_Marshal((TPML_ALG_PROPERTY*)&source->algorithms,
                                       buffer, size);
    case TPM_CAP_HANDLES:
      return TPML_HANDLE_Marshal((TPML_HANDLE*)&source->handles, buffer, size);
    case TPM_CAP_COMMANDS:
      return TPML_CCA_Marshal((TPML_CCA*)&source->command, buffer, size);
    case TPM_CAP_PP_COMMANDS:
      return TPML_CC_Marshal((TPML_CC*)&source->ppCommands, buffer, size);
    case TPM_CAP_AUDIT_COMMANDS:
      return TPML_CC_Marshal((TPML_CC*)&source->auditCommands, buffer, size);
    case TPM_CAP_PCRS:
      return TPML_PCR_SELECTION_Marshal(
          (TPML_PCR_SELECTION*)&source->assignedPCR, buffer, size);
    case TPM_CAP_TPM_PROPERTIES:
      return TPML_TAGGED_TPM_PROPERTY_Marshal(
          (TPML_TAGGED_TPM_PROPERTY*)&source->tpmProperties, buffer, size);
    case TPM_CAP_PCR_PROPERTIES:
      return TPML_TAGGED_PCR_PROPERTY_Marshal(
          (TPML_TAGGED_PCR_PROPERTY*)&source->pcrProperties, buffer, size);
    case TPM_CAP_ECC_CURVES:
      return TPML_ECC_CURVE_Marshal((TPML_ECC_CURVE*)&source->eccCurves, buffer,
                                    size);
  }
  return 0;
}

TPM_RC TPMU_CAPABILITIES_Unmarshal(TPMU_CAPABILITIES* target,
                                   BYTE** buffer,
                                   INT32* size,
                                   UINT32 selector) {
  switch (selector) {
    case TPM_CAP_ALGS:
      return TPML_ALG_PROPERTY_Unmarshal(
          (TPML_ALG_PROPERTY*)&target->algorithms, buffer, size);
    case TPM_CAP_HANDLES:
      return TPML_HANDLE_Unmarshal((TPML_HANDLE*)&target->handles, buffer,
                                   size);
    case TPM_CAP_COMMANDS:
      return TPML_CCA_Unmarshal((TPML_CCA*)&target->command, buffer, size);
    case TPM_CAP_PP_COMMANDS:
      return TPML_CC_Unmarshal((TPML_CC*)&target->ppCommands, buffer, size);
    case TPM_CAP_AUDIT_COMMANDS:
      return TPML_CC_Unmarshal((TPML_CC*)&target->auditCommands, buffer, size);
    case TPM_CAP_PCRS:
      return TPML_PCR_SELECTION_Unmarshal(
          (TPML_PCR_SELECTION*)&target->assignedPCR, buffer, size);
    case TPM_CAP_TPM_PROPERTIES:
      return TPML_TAGGED_TPM_PROPERTY_Unmarshal(
          (TPML_TAGGED_TPM_PROPERTY*)&target->tpmProperties, buffer, size);
    case TPM_CAP_PCR_PROPERTIES:
      return TPML_TAGGED_PCR_PROPERTY_Unmarshal(
          (TPML_TAGGED_PCR_PROPERTY*)&target->pcrProperties, buffer, size);
    case TPM_CAP_ECC_CURVES:
      return TPML_ECC_CURVE_Unmarshal((TPML_ECC_CURVE*)&target->eccCurves,
                                      buffer, size);
  }
  return TPM_RC_SELECTOR;
}

UINT16 TPMS_CAPABILITY_DATA_Marshal(TPMS_CAPABILITY_DATA* source,
                                    BYTE** buffer,
                                    INT32* size) {
  UINT16 total_size = 0;
  total_size += TPM_CAP_Marshal(&source->capability, buffer, size);
  total_size += TPMU_CAPABILITIES_Marshal(&source->data, buffer, size,
                                          source->capability);
  return total_size;
}

TPM_RC TPMS_CAPABILITY_DATA_Unmarshal(TPMS_CAPABILITY_DATA* target,
                                      BYTE** buffer,
                                      INT32* size) {
  TPM_RC result;
  result = TPM_CAP_Unmarshal(&target->capability, buffer, size);
  if (result != TPM_RC_SUCCESS) {
    return result;
  }
  result = TPMU_CAPABILITIES_Unmarshal(&target->data, buffer, size,
                                       target->capability);
  if (result != TPM_RC_SUCCESS) {
    return result;
  }
  return TPM_RC_SUCCESS;
}

UINT16 TPMS_CONTEXT_Marshal(TPMS_CONTEXT* source, BYTE** buffer, INT32* size) {
  UINT16 total_size = 0;
  total_size += UINT64_Marshal(&source->sequence, buffer, size);
  total_size += TPMI_DH_CONTEXT_Marshal(&source->savedHandle, buffer, size);
  total_size += TPMI_RH_HIERARCHY_Marshal(&source->hierarchy, buffer, size);
  total_size += TPM2B_CONTEXT_DATA_Marshal(&source->contextBlob, buffer, size);
  return total_size;
}

TPM_RC TPMS_CONTEXT_Unmarshal(TPMS_CONTEXT* target,
                              BYTE** buffer,
                              INT32* size) {
  TPM_RC result;
  result = UINT64_Unmarshal(&target->sequence, buffer, size);
  if (result != TPM_RC_SUCCESS) {
    return result;
  }
  result = TPMI_DH_CONTEXT_Unmarshal(&target->savedHandle, buffer, size);
  if (result != TPM_RC_SUCCESS) {
    return result;
  }
  result = TPMI_RH_HIERARCHY_Unmarshal(&target->hierarchy, buffer, size, TRUE);
  if (result != TPM_RC_SUCCESS) {
    return result;
  }
  result = TPM2B_CONTEXT_DATA_Unmarshal(&target->contextBlob, buffer, size);
  if (result != TPM_RC_SUCCESS) {
    return result;
  }
  return TPM_RC_SUCCESS;
}

UINT16 TPMS_CONTEXT_DATA_Marshal(TPMS_CONTEXT_DATA* source,
                                 BYTE** buffer,
                                 INT32* size) {
  UINT16 total_size = 0;
  total_size += TPM2B_DIGEST_Marshal(&source->integrity, buffer, size);
  total_size +=
      TPM2B_CONTEXT_SENSITIVE_Marshal(&source->encrypted, buffer, size);
  return total_size;
}

TPM_RC TPMS_CONTEXT_DATA_Unmarshal(TPMS_CONTEXT_DATA* target,
                                   BYTE** buffer,
                                   INT32* size) {
  TPM_RC result;
  result = TPM2B_DIGEST_Unmarshal(&target->integrity, buffer, size);
  if (result != TPM_RC_SUCCESS) {
    return result;
  }
  result = TPM2B_CONTEXT_SENSITIVE_Unmarshal(&target->encrypted, buffer, size);
  if (result != TPM_RC_SUCCESS) {
    return result;
  }
  return TPM_RC_SUCCESS;
}

UINT16 TPMS_PCR_SELECT_Marshal(TPMS_PCR_SELECT* source,
                               BYTE** buffer,
                               INT32* size) {
  UINT16 total_size = 0;
  INT32 i;
  total_size += UINT8_Marshal(&source->sizeofSelect, buffer, size);
  for (i = 0; i < source->sizeofSelect; ++i) {
    total_size += BYTE_Marshal(&source->pcrSelect[i], buffer, size);
  }
  return total_size;
}

TPM_RC TPMS_PCR_SELECT_Unmarshal(TPMS_PCR_SELECT* target,
                                 BYTE** buffer,
                                 INT32* size) {
  TPM_RC result;
  INT32 i;
  result = UINT8_Unmarshal(&target->sizeofSelect, buffer, size);
  if (result != TPM_RC_SUCCESS) {
    return result;
  }
  if (target->sizeofSelect > PCR_SELECT_MAX) {
    return TPM_RC_VALUE;
  }
  if (target->sizeofSelect < PCR_SELECT_MIN) {
    return TPM_RC_VALUE;
  }
  for (i = 0; i < target->sizeofSelect; ++i) {
    result = BYTE_Unmarshal(&target->pcrSelect[i], buffer, size);
    if (result != TPM_RC_SUCCESS) {
      return result;
    }
  }
  return TPM_RC_SUCCESS;
}

UINT16 TPMS_SIGNATURE_ECC_Marshal(TPMS_SIGNATURE_ECC* source,
                                  BYTE** buffer,
                                  INT32* size) {
  UINT16 total_size = 0;
  total_size += TPMI_ALG_HASH_Marshal(&source->hash, buffer, size);
  total_size += TPM2B_ECC_PARAMETER_Marshal(&source->signatureR, buffer, size);
  total_size += TPM2B_ECC_PARAMETER_Marshal(&source->signatureS, buffer, size);
  return total_size;
}

TPM_RC TPMS_SIGNATURE_ECC_Unmarshal(TPMS_SIGNATURE_ECC* target,
                                    BYTE** buffer,
                                    INT32* size) {
  TPM_RC result;
  result = TPMI_ALG_HASH_Unmarshal(&target->hash, buffer, size, FALSE);
  if (result != TPM_RC_SUCCESS) {
    return result;
  }
  result = TPM2B_ECC_PARAMETER_Unmarshal(&target->signatureR, buffer, size);
  if (result != TPM_RC_SUCCESS) {
    return result;
  }
  result = TPM2B_ECC_PARAMETER_Unmarshal(&target->signatureS, buffer, size);
  if (result != TPM_RC_SUCCESS) {
    return result;
  }
  return TPM_RC_SUCCESS;
}

UINT16 TPMS_SIGNATURE_ECDAA_Marshal(TPMS_SIGNATURE_ECDAA* source,
                                    BYTE** buffer,
                                    INT32* size) {
  return TPMS_SIGNATURE_ECC_Marshal(source, buffer, size);
}

TPM_RC TPMS_SIGNATURE_ECDAA_Unmarshal(TPMS_SIGNATURE_ECDAA* target,
                                      BYTE** buffer,
                                      INT32* size) {
  return TPMS_SIGNATURE_ECC_Unmarshal(target, buffer, size);
}

UINT16 TPMS_SIGNATURE_ECDSA_Marshal(TPMS_SIGNATURE_ECDSA* source,
                                    BYTE** buffer,
                                    INT32* size) {
  return TPMS_SIGNATURE_ECC_Marshal(source, buffer, size);
}

TPM_RC TPMS_SIGNATURE_ECDSA_Unmarshal(TPMS_SIGNATURE_ECDSA* target,
                                      BYTE** buffer,
                                      INT32* size) {
  return TPMS_SIGNATURE_ECC_Unmarshal(target, buffer, size);
}

UINT16 TPMS_SIGNATURE_ECSCHNORR_Marshal(TPMS_SIGNATURE_ECSCHNORR* source,
                                        BYTE** buffer,
                                        INT32* size) {
  return TPMS_SIGNATURE_ECC_Marshal(source, buffer, size);
}

TPM_RC TPMS_SIGNATURE_ECSCHNORR_Unmarshal(TPMS_SIGNATURE_ECSCHNORR* target,
                                          BYTE** buffer,
                                          INT32* size) {
  return TPMS_SIGNATURE_ECC_Unmarshal(target, buffer, size);
}

UINT16 TPMS_SIGNATURE_RSA_Marshal(TPMS_SIGNATURE_RSA* source,
                                  BYTE** buffer,
                                  INT32* size) {
  UINT16 total_size = 0;
  total_size += TPMI_ALG_HASH_Marshal(&source->hash, buffer, size);
  total_size += TPM2B_PUBLIC_KEY_RSA_Marshal(&source->sig, buffer, size);
  return total_size;
}

TPM_RC TPMS_SIGNATURE_RSA_Unmarshal(TPMS_SIGNATURE_RSA* target,
                                    BYTE** buffer,
                                    INT32* size) {
  TPM_RC result;
  result = TPMI_ALG_HASH_Unmarshal(&target->hash, buffer, size, FALSE);
  if (result != TPM_RC_SUCCESS) {
    return result;
  }
  result = TPM2B_PUBLIC_KEY_RSA_Unmarshal(&target->sig, buffer, size);
  if (result != TPM_RC_SUCCESS) {
    return result;
  }
  return TPM_RC_SUCCESS;
}

UINT16 TPMS_SIGNATURE_RSAPSS_Marshal(TPMS_SIGNATURE_RSAPSS* source,
                                     BYTE** buffer,
                                     INT32* size) {
  return TPMS_SIGNATURE_RSA_Marshal(source, buffer, size);
}

TPM_RC TPMS_SIGNATURE_RSAPSS_Unmarshal(TPMS_SIGNATURE_RSAPSS* target,
                                       BYTE** buffer,
                                       INT32* size) {
  return TPMS_SIGNATURE_RSA_Unmarshal(target, buffer, size);
}

UINT16 TPMS_SIGNATURE_RSASSA_Marshal(TPMS_SIGNATURE_RSASSA* source,
                                     BYTE** buffer,
                                     INT32* size) {
  return TPMS_SIGNATURE_RSA_Marshal(source, buffer, size);
}

TPM_RC TPMS_SIGNATURE_RSASSA_Unmarshal(TPMS_SIGNATURE_RSASSA* target,
                                       BYTE** buffer,
                                       INT32* size) {
  return TPMS_SIGNATURE_RSA_Unmarshal(target, buffer, size);
}

UINT16 TPMS_SIGNATURE_SM2_Marshal(TPMS_SIGNATURE_SM2* source,
                                  BYTE** buffer,
                                  INT32* size) {
  return TPMS_SIGNATURE_ECC_Marshal(source, buffer, size);
}

TPM_RC TPMS_SIGNATURE_SM2_Unmarshal(TPMS_SIGNATURE_SM2* target,
                                    BYTE** buffer,
                                    INT32* size) {
  return TPMS_SIGNATURE_ECC_Unmarshal(target, buffer, size);
}

UINT16 TPMT_PUBLIC_PARMS_Marshal(TPMT_PUBLIC_PARMS* source,
                                 BYTE** buffer,
                                 INT32* size) {
  UINT16 total_size = 0;
  total_size += TPMI_ALG_PUBLIC_Marshal(&source->type, buffer, size);
  total_size += TPMU_PUBLIC_PARMS_Marshal(&source->parameters, buffer, size,
                                          source->type);
  return total_size;
}

TPM_RC TPMT_PUBLIC_PARMS_Unmarshal(TPMT_PUBLIC_PARMS* target,
                                   BYTE** buffer,
                                   INT32* size) {
  TPM_RC result;
  result = TPMI_ALG_PUBLIC_Unmarshal(&target->type, buffer, size);
  if (result != TPM_RC_SUCCESS) {
    return result;
  }
  result = TPMU_PUBLIC_PARMS_Unmarshal(&target->parameters, buffer, size,
                                       target->type);
  if (result != TPM_RC_SUCCESS) {
    return result;
  }
  return TPM_RC_SUCCESS;
}

UINT16 TPMT_RSA_DECRYPT_Marshal(TPMT_RSA_DECRYPT* source,
                                BYTE** buffer,
                                INT32* size) {
  UINT16 total_size = 0;
  total_size += TPMI_ALG_RSA_DECRYPT_Marshal(&source->scheme, buffer, size);
  total_size +=
      TPMU_ASYM_SCHEME_Marshal(&source->details, buffer, size, source->scheme);
  return total_size;
}

TPM_RC TPMT_RSA_DECRYPT_Unmarshal(TPMT_RSA_DECRYPT* target,
                                  BYTE** buffer,
                                  INT32* size) {
  TPM_RC result;
  result = TPMI_ALG_RSA_DECRYPT_Unmarshal(&target->scheme, buffer, size, TRUE);
  if (result != TPM_RC_SUCCESS) {
    return result;
  }
  result = TPMU_ASYM_SCHEME_Unmarshal(&target->details, buffer, size,
                                      target->scheme);
  if (result != TPM_RC_SUCCESS) {
    return result;
  }
  return TPM_RC_SUCCESS;
}

UINT16 TPMU_SIGNATURE_Marshal(TPMU_SIGNATURE* source,
                              BYTE** buffer,
                              INT32* size,
                              UINT32 selector) {
  switch (selector) {
#ifdef TPM_ALG_RSASSA
    case TPM_ALG_RSASSA:
      return TPMS_SIGNATURE_RSASSA_Marshal(
          (TPMS_SIGNATURE_RSASSA*)&source->rsassa, buffer, size);
#endif
#ifdef TPM_ALG_RSAPSS
    case TPM_ALG_RSAPSS:
      return TPMS_SIGNATURE_RSAPSS_Marshal(
          (TPMS_SIGNATURE_RSAPSS*)&source->rsapss, buffer, size);
#endif
#ifdef TPM_ALG_ECDSA
    case TPM_ALG_ECDSA:
      return TPMS_SIGNATURE_ECDSA_Marshal((TPMS_SIGNATURE_ECDSA*)&source->ecdsa,
                                          buffer, size);
#endif
#ifdef TPM_ALG_ECDAA
    case TPM_ALG_ECDAA:
      return TPMS_SIGNATURE_ECDAA_Marshal((TPMS_SIGNATURE_ECDAA*)&source->ecdaa,
                                          buffer, size);
#endif
#ifdef TPM_ALG_SM2
    case TPM_ALG_SM2:
      return TPMS_SIGNATURE_SM2_Marshal((TPMS_SIGNATURE_SM2*)&source->sm2,
                                        buffer, size);
#endif
#ifdef TPM_ALG_ECSCHNORR
    case TPM_ALG_ECSCHNORR:
      return TPMS_SIGNATURE_ECSCHNORR_Marshal(
          (TPMS_SIGNATURE_ECSCHNORR*)&source->ecschnorr, buffer, size);
#endif
#ifdef TPM_ALG_HMAC
    case TPM_ALG_HMAC:
      return TPMT_HA_Marshal((TPMT_HA*)&source->hmac, buffer, size);
#endif
#ifdef TPM_ALG_NULL
    case TPM_ALG_NULL:
      return 0;
#endif
  }
  return 0;
}

TPM_RC TPMU_SIGNATURE_Unmarshal(TPMU_SIGNATURE* target,
                                BYTE** buffer,
                                INT32* size,
                                UINT32 selector) {
  switch (selector) {
#ifdef TPM_ALG_RSASSA
    case TPM_ALG_RSASSA:
      return TPMS_SIGNATURE_RSASSA_Unmarshal(
          (TPMS_SIGNATURE_RSASSA*)&target->rsassa, buffer, size);
#endif
#ifdef TPM_ALG_RSAPSS
    case TPM_ALG_RSAPSS:
      return TPMS_SIGNATURE_RSAPSS_Unmarshal(
          (TPMS_SIGNATURE_RSAPSS*)&target->rsapss, buffer, size);
#endif
#ifdef TPM_ALG_ECDSA
    case TPM_ALG_ECDSA:
      return TPMS_SIGNATURE_ECDSA_Unmarshal(
          (TPMS_SIGNATURE_ECDSA*)&target->ecdsa, buffer, size);
#endif
#ifdef TPM_ALG_ECDAA
    case TPM_ALG_ECDAA:
      return TPMS_SIGNATURE_ECDAA_Unmarshal(
          (TPMS_SIGNATURE_ECDAA*)&target->ecdaa, buffer, size);
#endif
#ifdef TPM_ALG_SM2
    case TPM_ALG_SM2:
      return TPMS_SIGNATURE_SM2_Unmarshal((TPMS_SIGNATURE_SM2*)&target->sm2,
                                          buffer, size);
#endif
#ifdef TPM_ALG_ECSCHNORR
    case TPM_ALG_ECSCHNORR:
      return TPMS_SIGNATURE_ECSCHNORR_Unmarshal(
          (TPMS_SIGNATURE_ECSCHNORR*)&target->ecschnorr, buffer, size);
#endif
#ifdef TPM_ALG_HMAC
    case TPM_ALG_HMAC:
      return TPMT_HA_Unmarshal((TPMT_HA*)&target->hmac, buffer, size);
#endif
#ifdef TPM_ALG_NULL
    case TPM_ALG_NULL:
      return TPM_RC_SUCCESS;
#endif
  }
  return TPM_RC_SELECTOR;
}

UINT16 TPMT_SIGNATURE_Marshal(TPMT_SIGNATURE* source,
                              BYTE** buffer,
                              INT32* size) {
  UINT16 total_size = 0;
  total_size += TPMI_ALG_SIG_SCHEME_Marshal(&source->sigAlg, buffer, size);
  total_size +=
      TPMU_SIGNATURE_Marshal(&source->signature, buffer, size, source->sigAlg);
  return total_size;
}

TPM_RC TPMT_SIGNATURE_Unmarshal(TPMT_SIGNATURE* target,
                                BYTE** buffer,
                                INT32* size) {
  TPM_RC result;
  result = TPMI_ALG_SIG_SCHEME_Unmarshal(&target->sigAlg, buffer, size, TRUE);
  if (result != TPM_RC_SUCCESS) {
    return result;
  }
  result = TPMU_SIGNATURE_Unmarshal(&target->signature, buffer, size,
                                    target->sigAlg);
  if (result != TPM_RC_SUCCESS) {
    return result;
  }
  return TPM_RC_SUCCESS;
}

UINT16 TPMU_SIG_SCHEME_Marshal(TPMU_SIG_SCHEME* source,
                               BYTE** buffer,
                               INT32* size,
                               UINT32 selector) {
  switch (selector) {
#ifdef TPM_ALG_RSASSA
    case TPM_ALG_RSASSA:
      return TPMS_SIG_SCHEME_RSASSA_Marshal(
          (TPMS_SIG_SCHEME_RSASSA*)&source->rsassa, buffer, size);
#endif
#ifdef TPM_ALG_RSAPSS
    case TPM_ALG_RSAPSS:
      return TPMS_SIG_SCHEME_RSAPSS_Marshal(
          (TPMS_SIG_SCHEME_RSAPSS*)&source->rsapss, buffer, size);
#endif
#ifdef TPM_ALG_ECDSA
    case TPM_ALG_ECDSA:
      return TPMS_SIG_SCHEME_ECDSA_Marshal(
          (TPMS_SIG_SCHEME_ECDSA*)&source->ecdsa, buffer, size);
#endif
#ifdef TPM_ALG_ECDAA
    case TPM_ALG_ECDAA:
      return TPMS_SIG_SCHEME_ECDAA_Marshal(
          (TPMS_SIG_SCHEME_ECDAA*)&source->ecdaa, buffer, size);
#endif
#ifdef TPM_ALG_SM2
    case TPM_ALG_SM2:
      return TPMS_SIG_SCHEME_SM2_Marshal((TPMS_SIG_SCHEME_SM2*)&source->sm2,
                                         buffer, size);
#endif
#ifdef TPM_ALG_ECSCHNORR
    case TPM_ALG_ECSCHNORR:
      return TPMS_SIG_SCHEME_ECSCHNORR_Marshal(
          (TPMS_SIG_SCHEME_ECSCHNORR*)&source->ecschnorr, buffer, size);
#endif
#ifdef TPM_ALG_HMAC
    case TPM_ALG_HMAC:
      return TPMS_SCHEME_HMAC_Marshal((TPMS_SCHEME_HMAC*)&source->hmac, buffer,
                                      size);
#endif
#ifdef TPM_ALG_NULL
    case TPM_ALG_NULL:
      return 0;
#endif
  }
  return 0;
}

TPM_RC TPMU_SIG_SCHEME_Unmarshal(TPMU_SIG_SCHEME* target,
                                 BYTE** buffer,
                                 INT32* size,
                                 UINT32 selector) {
  switch (selector) {
#ifdef TPM_ALG_RSASSA
    case TPM_ALG_RSASSA:
      return TPMS_SIG_SCHEME_RSASSA_Unmarshal(
          (TPMS_SIG_SCHEME_RSASSA*)&target->rsassa, buffer, size);
#endif
#ifdef TPM_ALG_RSAPSS
    case TPM_ALG_RSAPSS:
      return TPMS_SIG_SCHEME_RSAPSS_Unmarshal(
          (TPMS_SIG_SCHEME_RSAPSS*)&target->rsapss, buffer, size);
#endif
#ifdef TPM_ALG_ECDSA
    case TPM_ALG_ECDSA:
      return TPMS_SIG_SCHEME_ECDSA_Unmarshal(
          (TPMS_SIG_SCHEME_ECDSA*)&target->ecdsa, buffer, size);
#endif
#ifdef TPM_ALG_ECDAA
    case TPM_ALG_ECDAA:
      return TPMS_SIG_SCHEME_ECDAA_Unmarshal(
          (TPMS_SIG_SCHEME_ECDAA*)&target->ecdaa, buffer, size);
#endif
#ifdef TPM_ALG_SM2
    case TPM_ALG_SM2:
      return TPMS_SIG_SCHEME_SM2_Unmarshal((TPMS_SIG_SCHEME_SM2*)&target->sm2,
                                           buffer, size);
#endif
#ifdef TPM_ALG_ECSCHNORR
    case TPM_ALG_ECSCHNORR:
      return TPMS_SIG_SCHEME_ECSCHNORR_Unmarshal(
          (TPMS_SIG_SCHEME_ECSCHNORR*)&target->ecschnorr, buffer, size);
#endif
#ifdef TPM_ALG_HMAC
    case TPM_ALG_HMAC:
      return TPMS_SCHEME_HMAC_Unmarshal((TPMS_SCHEME_HMAC*)&target->hmac,
                                        buffer, size);
#endif
#ifdef TPM_ALG_NULL
    case TPM_ALG_NULL:
      return TPM_RC_SUCCESS;
#endif
  }
  return TPM_RC_SELECTOR;
}

UINT16 TPMT_SIG_SCHEME_Marshal(TPMT_SIG_SCHEME* source,
                               BYTE** buffer,
                               INT32* size) {
  UINT16 total_size = 0;
  total_size += TPMI_ALG_SIG_SCHEME_Marshal(&source->scheme, buffer, size);
  total_size +=
      TPMU_SIG_SCHEME_Marshal(&source->details, buffer, size, source->scheme);
  return total_size;
}

TPM_RC TPMT_SIG_SCHEME_Unmarshal(TPMT_SIG_SCHEME* target,
                                 BYTE** buffer,
                                 INT32* size) {
  TPM_RC result;
  result = TPMI_ALG_SIG_SCHEME_Unmarshal(&target->scheme, buffer, size, TRUE);
  if (result != TPM_RC_SUCCESS) {
    return result;
  }
  result =
      TPMU_SIG_SCHEME_Unmarshal(&target->details, buffer, size, target->scheme);
  if (result != TPM_RC_SUCCESS) {
    return result;
  }
  return TPM_RC_SUCCESS;
}

UINT16 TPMT_SYM_DEF_Marshal(TPMT_SYM_DEF* source, BYTE** buffer, INT32* size) {
  UINT16 total_size = 0;
  total_size += TPMI_ALG_SYM_Marshal(&source->algorithm, buffer, size);
  total_size += TPMU_SYM_KEY_BITS_Marshal(&source->keyBits, buffer, size,
                                          source->algorithm);
  total_size +=
      TPMU_SYM_MODE_Marshal(&source->mode, buffer, size, source->algorithm);
  return total_size;
}

TPM_RC TPMT_SYM_DEF_Unmarshal(TPMT_SYM_DEF* target,
                              BYTE** buffer,
                              INT32* size) {
  TPM_RC result;
  result = TPMI_ALG_SYM_Unmarshal(&target->algorithm, buffer, size, TRUE);
  if (result != TPM_RC_SUCCESS) {
    return result;
  }
  result = TPMU_SYM_KEY_BITS_Unmarshal(&target->keyBits, buffer, size,
                                       target->algorithm);
  if (result != TPM_RC_SUCCESS) {
    return result;
  }
  result =
      TPMU_SYM_MODE_Unmarshal(&target->mode, buffer, size, target->algorithm);
  if (result != TPM_RC_SUCCESS) {
    return result;
  }
  return TPM_RC_SUCCESS;
}

UINT16 TPM_ST_Marshal(TPM_ST* source, BYTE** buffer, INT32* size) {
  return uint16_t_Marshal(source, buffer, size);
}

TPM_RC TPM_ST_Unmarshal(TPM_ST* target, BYTE** buffer, INT32* size) {
  TPM_RC result;
  result = uint16_t_Unmarshal(target, buffer, size);
  if (result != TPM_RC_SUCCESS) {
    return result;
  }
  if (*target == TPM_ST_RSP_COMMAND) {
    return TPM_RC_SUCCESS;
  }
  if (*target == TPM_ST_NULL) {
    return TPM_RC_SUCCESS;
  }
  if (*target == TPM_ST_NO_SESSIONS) {
    return TPM_RC_SUCCESS;
  }
  if (*target == TPM_ST_SESSIONS) {
    return TPM_RC_SUCCESS;
  }
  if (*target == TPM_ST_ATTEST_NV) {
    return TPM_RC_SUCCESS;
  }
  if (*target == TPM_ST_ATTEST_COMMAND_AUDIT) {
    return TPM_RC_SUCCESS;
  }
  if (*target == TPM_ST_ATTEST_SESSION_AUDIT) {
    return TPM_RC_SUCCESS;
  }
  if (*target == TPM_ST_ATTEST_CERTIFY) {
    return TPM_RC_SUCCESS;
  }
  if (*target == TPM_ST_ATTEST_QUOTE) {
    return TPM_RC_SUCCESS;
  }
  if (*target == TPM_ST_ATTEST_TIME) {
    return TPM_RC_SUCCESS;
  }
  if (*target == TPM_ST_ATTEST_CREATION) {
    return TPM_RC_SUCCESS;
  }
  if (*target == TPM_ST_CREATION) {
    return TPM_RC_SUCCESS;
  }
  if (*target == TPM_ST_VERIFIED) {
    return TPM_RC_SUCCESS;
  }
  if (*target == TPM_ST_AUTH_SECRET) {
    return TPM_RC_SUCCESS;
  }
  if (*target == TPM_ST_HASHCHECK) {
    return TPM_RC_SUCCESS;
  }
  if (*target == TPM_ST_AUTH_SIGNED) {
    return TPM_RC_SUCCESS;
  }
  if (*target == TPM_ST_FU_MANIFEST) {
    return TPM_RC_SUCCESS;
  }
  return TPM_RC_VALUE;
}

UINT16 TPMT_TK_AUTH_Marshal(TPMT_TK_AUTH* source, BYTE** buffer, INT32* size) {
  UINT16 total_size = 0;
  total_size += TPM_ST_Marshal(&source->tag, buffer, size);
  total_size += TPMI_RH_HIERARCHY_Marshal(&source->hierarchy, buffer, size);
  total_size += TPM2B_DIGEST_Marshal(&source->digest, buffer, size);
  return total_size;
}

TPM_RC TPMT_TK_AUTH_Unmarshal(TPMT_TK_AUTH* target,
                              BYTE** buffer,
                              INT32* size) {
  TPM_RC result;
  result = TPM_ST_Unmarshal(&target->tag, buffer, size);
  if (result != TPM_RC_SUCCESS) {
    return result;
  }
  result = TPMI_RH_HIERARCHY_Unmarshal(&target->hierarchy, buffer, size, TRUE);
  if (result != TPM_RC_SUCCESS) {
    return result;
  }
  result = TPM2B_DIGEST_Unmarshal(&target->digest, buffer, size);
  if (result != TPM_RC_SUCCESS) {
    return result;
  }
  return TPM_RC_SUCCESS;
}

UINT16 TPMT_TK_CREATION_Marshal(TPMT_TK_CREATION* source,
                                BYTE** buffer,
                                INT32* size) {
  UINT16 total_size = 0;
  total_size += TPM_ST_Marshal(&source->tag, buffer, size);
  total_size += TPMI_RH_HIERARCHY_Marshal(&source->hierarchy, buffer, size);
  total_size += TPM2B_DIGEST_Marshal(&source->digest, buffer, size);
  return total_size;
}

TPM_RC TPMT_TK_CREATION_Unmarshal(TPMT_TK_CREATION* target,
                                  BYTE** buffer,
                                  INT32* size) {
  TPM_RC result;
  result = TPM_ST_Unmarshal(&target->tag, buffer, size);
  if (result != TPM_RC_SUCCESS) {
    return result;
  }
  result = TPMI_RH_HIERARCHY_Unmarshal(&target->hierarchy, buffer, size, TRUE);
  if (result != TPM_RC_SUCCESS) {
    return result;
  }
  result = TPM2B_DIGEST_Unmarshal(&target->digest, buffer, size);
  if (result != TPM_RC_SUCCESS) {
    return result;
  }
  return TPM_RC_SUCCESS;
}

UINT16 TPMT_TK_HASHCHECK_Marshal(TPMT_TK_HASHCHECK* source,
                                 BYTE** buffer,
                                 INT32* size) {
  UINT16 total_size = 0;
  total_size += TPM_ST_Marshal(&source->tag, buffer, size);
  total_size += TPMI_RH_HIERARCHY_Marshal(&source->hierarchy, buffer, size);
  total_size += TPM2B_DIGEST_Marshal(&source->digest, buffer, size);
  return total_size;
}

TPM_RC TPMT_TK_HASHCHECK_Unmarshal(TPMT_TK_HASHCHECK* target,
                                   BYTE** buffer,
                                   INT32* size) {
  TPM_RC result;
  result = TPM_ST_Unmarshal(&target->tag, buffer, size);
  if (result != TPM_RC_SUCCESS) {
    return result;
  }
  result = TPMI_RH_HIERARCHY_Unmarshal(&target->hierarchy, buffer, size, TRUE);
  if (result != TPM_RC_SUCCESS) {
    return result;
  }
  result = TPM2B_DIGEST_Unmarshal(&target->digest, buffer, size);
  if (result != TPM_RC_SUCCESS) {
    return result;
  }
  return TPM_RC_SUCCESS;
}

UINT16 TPMT_TK_VERIFIED_Marshal(TPMT_TK_VERIFIED* source,
                                BYTE** buffer,
                                INT32* size) {
  UINT16 total_size = 0;
  total_size += TPM_ST_Marshal(&source->tag, buffer, size);
  total_size += TPMI_RH_HIERARCHY_Marshal(&source->hierarchy, buffer, size);
  total_size += TPM2B_DIGEST_Marshal(&source->digest, buffer, size);
  return total_size;
}

TPM_RC TPMT_TK_VERIFIED_Unmarshal(TPMT_TK_VERIFIED* target,
                                  BYTE** buffer,
                                  INT32* size) {
  TPM_RC result;
  result = TPM_ST_Unmarshal(&target->tag, buffer, size);
  if (result != TPM_RC_SUCCESS) {
    return result;
  }
  result = TPMI_RH_HIERARCHY_Unmarshal(&target->hierarchy, buffer, size, TRUE);
  if (result != TPM_RC_SUCCESS) {
    return result;
  }
  result = TPM2B_DIGEST_Unmarshal(&target->digest, buffer, size);
  if (result != TPM_RC_SUCCESS) {
    return result;
  }
  return TPM_RC_SUCCESS;
}

UINT16 TPMU_SYM_DETAILS_Marshal(TPMU_SYM_DETAILS* source,
                                BYTE** buffer,
                                INT32* size,
                                UINT32 selector) {
  switch (selector) {
#ifdef TPM_ALG_AES
    case TPM_ALG_AES:
      return 0;
#endif
#ifdef TPM_ALG_SM4
    case TPM_ALG_SM4:
      return 0;
#endif
#ifdef TPM_ALG_CAMELLIA
    case TPM_ALG_CAMELLIA:
      return 0;
#endif
#ifdef TPM_ALG_XOR
    case TPM_ALG_XOR:
      return 0;
#endif
#ifdef TPM_ALG_NULL
    case TPM_ALG_NULL:
      return 0;
#endif
  }
  return 0;
}

TPM_RC TPMU_SYM_DETAILS_Unmarshal(TPMU_SYM_DETAILS* target,
                                  BYTE** buffer,
                                  INT32* size,
                                  UINT32 selector) {
  switch (selector) {
#ifdef TPM_ALG_AES
    case TPM_ALG_AES:
      return TPM_RC_SUCCESS;
#endif
#ifdef TPM_ALG_SM4
    case TPM_ALG_SM4:
      return TPM_RC_SUCCESS;
#endif
#ifdef TPM_ALG_CAMELLIA
    case TPM_ALG_CAMELLIA:
      return TPM_RC_SUCCESS;
#endif
#ifdef TPM_ALG_XOR
    case TPM_ALG_XOR:
      return TPM_RC_SUCCESS;
#endif
#ifdef TPM_ALG_NULL
    case TPM_ALG_NULL:
      return TPM_RC_SUCCESS;
#endif
  }
  return TPM_RC_SELECTOR;
}

UINT16 TPM_ALGORITHM_ID_Marshal(TPM_ALGORITHM_ID* source,
                                BYTE** buffer,
                                INT32* size) {
  return uint32_t_Marshal(source, buffer, size);
}

TPM_RC TPM_ALGORITHM_ID_Unmarshal(TPM_ALGORITHM_ID* target,
                                  BYTE** buffer,
                                  INT32* size) {
  return uint32_t_Unmarshal(target, buffer, size);
}

UINT16 TPM_AUTHORIZATION_SIZE_Marshal(TPM_AUTHORIZATION_SIZE* source,
                                      BYTE** buffer,
                                      INT32* size) {
  return uint32_t_Marshal(source, buffer, size);
}

TPM_RC TPM_AUTHORIZATION_SIZE_Unmarshal(TPM_AUTHORIZATION_SIZE* target,
                                        BYTE** buffer,
                                        INT32* size) {
  return uint32_t_Unmarshal(target, buffer, size);
}

UINT16 TPM_CLOCK_ADJUST_Marshal(TPM_CLOCK_ADJUST* source,
                                BYTE** buffer,
                                INT32* size) {
  return int8_t_Marshal(source, buffer, size);
}

TPM_RC TPM_CLOCK_ADJUST_Unmarshal(TPM_CLOCK_ADJUST* target,
                                  BYTE** buffer,
                                  INT32* size) {
  TPM_RC result;
  result = int8_t_Unmarshal(target, buffer, size);
  if (result != TPM_RC_SUCCESS) {
    return result;
  }
  if (*target == TPM_CLOCK_COARSE_SLOWER) {
    return TPM_RC_SUCCESS;
  }
  if (*target == TPM_CLOCK_MEDIUM_SLOWER) {
    return TPM_RC_SUCCESS;
  }
  if (*target == TPM_CLOCK_FINE_SLOWER) {
    return TPM_RC_SUCCESS;
  }
  if (*target == TPM_CLOCK_NO_CHANGE) {
    return TPM_RC_SUCCESS;
  }
  if (*target == TPM_CLOCK_FINE_FASTER) {
    return TPM_RC_SUCCESS;
  }
  if (*target == TPM_CLOCK_MEDIUM_FASTER) {
    return TPM_RC_SUCCESS;
  }
  if (*target == TPM_CLOCK_COARSE_FASTER) {
    return TPM_RC_SUCCESS;
  }
  return TPM_RC_VALUE;
}

UINT16 TPM_EO_Marshal(TPM_EO* source, BYTE** buffer, INT32* size) {
  return uint16_t_Marshal(source, buffer, size);
}

TPM_RC TPM_EO_Unmarshal(TPM_EO* target, BYTE** buffer, INT32* size) {
  TPM_RC result;
  result = uint16_t_Unmarshal(target, buffer, size);
  if (result != TPM_RC_SUCCESS) {
    return result;
  }
  if (*target == TPM_EO_EQ) {
    return TPM_RC_SUCCESS;
  }
  if (*target == TPM_EO_NEQ) {
    return TPM_RC_SUCCESS;
  }
  if (*target == TPM_EO_SIGNED_GT) {
    return TPM_RC_SUCCESS;
  }
  if (*target == TPM_EO_UNSIGNED_GT) {
    return TPM_RC_SUCCESS;
  }
  if (*target == TPM_EO_SIGNED_LT) {
    return TPM_RC_SUCCESS;
  }
  if (*target == TPM_EO_UNSIGNED_LT) {
    return TPM_RC_SUCCESS;
  }
  if (*target == TPM_EO_SIGNED_GE) {
    return TPM_RC_SUCCESS;
  }
  if (*target == TPM_EO_UNSIGNED_GE) {
    return TPM_RC_SUCCESS;
  }
  if (*target == TPM_EO_SIGNED_LE) {
    return TPM_RC_SUCCESS;
  }
  if (*target == TPM_EO_UNSIGNED_LE) {
    return TPM_RC_SUCCESS;
  }
  if (*target == TPM_EO_BITSET) {
    return TPM_RC_SUCCESS;
  }
  if (*target == TPM_EO_BITCLEAR) {
    return TPM_RC_SUCCESS;
  }
  return TPM_RC_VALUE;
}

UINT16 TPM_HC_Marshal(TPM_HC* source, BYTE** buffer, INT32* size) {
  return uint32_t_Marshal(source, buffer, size);
}

TPM_RC TPM_HC_Unmarshal(TPM_HC* target, BYTE** buffer, INT32* size) {
  TPM_RC result;
  result = uint32_t_Unmarshal(target, buffer, size);
  if (result != TPM_RC_SUCCESS) {
    return result;
  }
  if (*target == HR_HANDLE_MASK) {
    return TPM_RC_SUCCESS;
  }
  if (*target == HR_RANGE_MASK) {
    return TPM_RC_SUCCESS;
  }
  if (*target == HR_SHIFT) {
    return TPM_RC_SUCCESS;
  }
  if (*target == HR_PCR) {
    return TPM_RC_SUCCESS;
  }
  if (*target == HR_HMAC_SESSION) {
    return TPM_RC_SUCCESS;
  }
  if (*target == HR_POLICY_SESSION) {
    return TPM_RC_SUCCESS;
  }
  if (*target == HR_TRANSIENT) {
    return TPM_RC_SUCCESS;
  }
  if (*target == HR_PERSISTENT) {
    return TPM_RC_SUCCESS;
  }
  if (*target == HR_NV_INDEX) {
    return TPM_RC_SUCCESS;
  }
  if (*target == HR_PERMANENT) {
    return TPM_RC_SUCCESS;
  }
  if (*target == PCR_FIRST) {
    return TPM_RC_SUCCESS;
  }
  if (*target == PCR_LAST) {
    return TPM_RC_SUCCESS;
  }
  if (*target == HMAC_SESSION_FIRST) {
    return TPM_RC_SUCCESS;
  }
  if (*target == HMAC_SESSION_LAST) {
    return TPM_RC_SUCCESS;
  }
  if (*target == LOADED_SESSION_FIRST) {
    return TPM_RC_SUCCESS;
  }
  if (*target == LOADED_SESSION_LAST) {
    return TPM_RC_SUCCESS;
  }
  if (*target == POLICY_SESSION_FIRST) {
    return TPM_RC_SUCCESS;
  }
  if (*target == POLICY_SESSION_LAST) {
    return TPM_RC_SUCCESS;
  }
  if (*target == TRANSIENT_FIRST) {
    return TPM_RC_SUCCESS;
  }
  if (*target == ACTIVE_SESSION_FIRST) {
    return TPM_RC_SUCCESS;
  }
  if (*target == ACTIVE_SESSION_LAST) {
    return TPM_RC_SUCCESS;
  }
  if (*target == TRANSIENT_LAST) {
    return TPM_RC_SUCCESS;
  }
  if (*target == PERSISTENT_FIRST) {
    return TPM_RC_SUCCESS;
  }
  if (*target == PERSISTENT_LAST) {
    return TPM_RC_SUCCESS;
  }
  if (*target == PLATFORM_PERSISTENT) {
    return TPM_RC_SUCCESS;
  }
  if (*target == NV_INDEX_FIRST) {
    return TPM_RC_SUCCESS;
  }
  if (*target == NV_INDEX_LAST) {
    return TPM_RC_SUCCESS;
  }
  if (*target == PERMANENT_FIRST) {
    return TPM_RC_SUCCESS;
  }
  if (*target == PERMANENT_LAST) {
    return TPM_RC_SUCCESS;
  }
  return TPM_RC_VALUE;
}

UINT16 TPM_HT_Marshal(TPM_HT* source, BYTE** buffer, INT32* size) {
  return uint8_t_Marshal(source, buffer, size);
}

TPM_RC TPM_HT_Unmarshal(TPM_HT* target, BYTE** buffer, INT32* size) {
  TPM_RC result;
  result = uint8_t_Unmarshal(target, buffer, size);
  if (result != TPM_RC_SUCCESS) {
    return result;
  }
  if (*target == TPM_HT_PCR) {
    return TPM_RC_SUCCESS;
  }
  if (*target == TPM_HT_NV_INDEX) {
    return TPM_RC_SUCCESS;
  }
  if (*target == TPM_HT_HMAC_SESSION) {
    return TPM_RC_SUCCESS;
  }
  if (*target == TPM_HT_LOADED_SESSION) {
    return TPM_RC_SUCCESS;
  }
  if (*target == TPM_HT_POLICY_SESSION) {
    return TPM_RC_SUCCESS;
  }
  if (*target == TPM_HT_ACTIVE_SESSION) {
    return TPM_RC_SUCCESS;
  }
  if (*target == TPM_HT_PERMANENT) {
    return TPM_RC_SUCCESS;
  }
  if (*target == TPM_HT_TRANSIENT) {
    return TPM_RC_SUCCESS;
  }
  if (*target == TPM_HT_PERSISTENT) {
    return TPM_RC_SUCCESS;
  }
  return TPM_RC_VALUE;
}

UINT16 TPM_KEY_SIZE_Marshal(TPM_KEY_SIZE* source, BYTE** buffer, INT32* size) {
  return uint16_t_Marshal(source, buffer, size);
}

TPM_RC TPM_KEY_SIZE_Unmarshal(TPM_KEY_SIZE* target,
                              BYTE** buffer,
                              INT32* size) {
  return uint16_t_Unmarshal(target, buffer, size);
}

UINT16 TPM_MODIFIER_INDICATOR_Marshal(TPM_MODIFIER_INDICATOR* source,
                                      BYTE** buffer,
                                      INT32* size) {
  return uint32_t_Marshal(source, buffer, size);
}

TPM_RC TPM_MODIFIER_INDICATOR_Unmarshal(TPM_MODIFIER_INDICATOR* target,
                                        BYTE** buffer,
                                        INT32* size) {
  return uint32_t_Unmarshal(target, buffer, size);
}

UINT16 TPM_NV_INDEX_Marshal(TPM_NV_INDEX* source, BYTE** buffer, INT32* size) {
  return uint32_t_Marshal((uint32_t*)source, buffer, size);
}

TPM_RC TPM_NV_INDEX_Unmarshal(TPM_NV_INDEX* target,
                              BYTE** buffer,
                              INT32* size) {
  TPM_RC result;
  result = uint32_t_Unmarshal((uint32_t*)target, buffer, size);
  if (result != TPM_RC_SUCCESS) {
    return result;
  }
  return TPM_RC_SUCCESS;
}

UINT16 TPM_PARAMETER_SIZE_Marshal(TPM_PARAMETER_SIZE* source,
                                  BYTE** buffer,
                                  INT32* size) {
  return uint32_t_Marshal(source, buffer, size);
}

TPM_RC TPM_PARAMETER_SIZE_Unmarshal(TPM_PARAMETER_SIZE* target,
                                    BYTE** buffer,
                                    INT32* size) {
  return uint32_t_Unmarshal(target, buffer, size);
}

UINT16 TPM_PS_Marshal(TPM_PS* source, BYTE** buffer, INT32* size) {
  return uint32_t_Marshal(source, buffer, size);
}

TPM_RC TPM_PS_Unmarshal(TPM_PS* target, BYTE** buffer, INT32* size) {
  TPM_RC result;
  result = uint32_t_Unmarshal(target, buffer, size);
  if (result != TPM_RC_SUCCESS) {
    return result;
  }
  if (*target == TPM_PS_MAIN) {
    return TPM_RC_SUCCESS;
  }
  if (*target == TPM_PS_PC) {
    return TPM_RC_SUCCESS;
  }
  if (*target == TPM_PS_PDA) {
    return TPM_RC_SUCCESS;
  }
  if (*target == TPM_PS_CELL_PHONE) {
    return TPM_RC_SUCCESS;
  }
  if (*target == TPM_PS_SERVER) {
    return TPM_RC_SUCCESS;
  }
  if (*target == TPM_PS_PERIPHERAL) {
    return TPM_RC_SUCCESS;
  }
  if (*target == TPM_PS_TSS) {
    return TPM_RC_SUCCESS;
  }
  if (*target == TPM_PS_STORAGE) {
    return TPM_RC_SUCCESS;
  }
  if (*target == TPM_PS_AUTHENTICATION) {
    return TPM_RC_SUCCESS;
  }
  if (*target == TPM_PS_EMBEDDED) {
    return TPM_RC_SUCCESS;
  }
  if (*target == TPM_PS_HARDCOPY) {
    return TPM_RC_SUCCESS;
  }
  if (*target == TPM_PS_INFRASTRUCTURE) {
    return TPM_RC_SUCCESS;
  }
  if (*target == TPM_PS_VIRTUALIZATION) {
    return TPM_RC_SUCCESS;
  }
  if (*target == TPM_PS_TNC) {
    return TPM_RC_SUCCESS;
  }
  if (*target == TPM_PS_MULTI_TENANT) {
    return TPM_RC_SUCCESS;
  }
  if (*target == TPM_PS_TC) {
    return TPM_RC_SUCCESS;
  }
  return TPM_RC_VALUE;
}

UINT16 TPM_PT_PCR_Marshal(TPM_PT_PCR* source, BYTE** buffer, INT32* size) {
  return uint32_t_Marshal(source, buffer, size);
}

TPM_RC TPM_PT_PCR_Unmarshal(TPM_PT_PCR* target, BYTE** buffer, INT32* size) {
  TPM_RC result;
  result = uint32_t_Unmarshal(target, buffer, size);
  if (result != TPM_RC_SUCCESS) {
    return result;
  }
  if (*target == TPM_PT_PCR_FIRST) {
    return TPM_RC_SUCCESS;
  }
  if (*target == TPM_PT_PCR_SAVE) {
    return TPM_RC_SUCCESS;
  }
  if (*target == TPM_PT_PCR_EXTEND_L0) {
    return TPM_RC_SUCCESS;
  }
  if (*target == TPM_PT_PCR_RESET_L0) {
    return TPM_RC_SUCCESS;
  }
  if (*target == TPM_PT_PCR_EXTEND_L1) {
    return TPM_RC_SUCCESS;
  }
  if (*target == TPM_PT_PCR_RESET_L1) {
    return TPM_RC_SUCCESS;
  }
  if (*target == TPM_PT_PCR_EXTEND_L2) {
    return TPM_RC_SUCCESS;
  }
  if (*target == TPM_PT_PCR_RESET_L2) {
    return TPM_RC_SUCCESS;
  }
  if (*target == TPM_PT_PCR_EXTEND_L3) {
    return TPM_RC_SUCCESS;
  }
  if (*target == TPM_PT_PCR_RESET_L3) {
    return TPM_RC_SUCCESS;
  }
  if (*target == TPM_PT_PCR_EXTEND_L4) {
    return TPM_RC_SUCCESS;
  }
  if (*target == TPM_PT_PCR_RESET_L4) {
    return TPM_RC_SUCCESS;
  }
  if (*target == TPM_PT_PCR_NO_INCREMENT) {
    return TPM_RC_SUCCESS;
  }
  if (*target == TPM_PT_PCR_DRTM_RESET) {
    return TPM_RC_SUCCESS;
  }
  if (*target == TPM_PT_PCR_POLICY) {
    return TPM_RC_SUCCESS;
  }
  if (*target == TPM_PT_PCR_AUTH) {
    return TPM_RC_SUCCESS;
  }
  if (*target == TPM_PT_PCR_LAST) {
    return TPM_RC_SUCCESS;
  }
  return TPM_RC_VALUE;
}

UINT16 TPM_RC_Marshal(TPM_RC* source, BYTE** buffer, INT32* size) {
  return uint32_t_Marshal(source, buffer, size);
}

TPM_RC TPM_RC_Unmarshal(TPM_RC* target, BYTE** buffer, INT32* size) {
  TPM_RC result;
  result = uint32_t_Unmarshal(target, buffer, size);
  if (result != TPM_RC_SUCCESS) {
    return result;
  }
  if (*target == TPM_RC_SUCCESS) {
    return TPM_RC_SUCCESS;
  }
  if (*target == TPM_RC_BAD_TAG) {
    return TPM_RC_SUCCESS;
  }
  if (*target == RC_VER1) {
    return TPM_RC_SUCCESS;
  }
  if (*target == TPM_RC_INITIALIZE) {
    return TPM_RC_SUCCESS;
  }
  if (*target == TPM_RC_FAILURE) {
    return TPM_RC_SUCCESS;
  }
  if (*target == TPM_RC_SEQUENCE) {
    return TPM_RC_SUCCESS;
  }
  if (*target == TPM_RC_PRIVATE) {
    return TPM_RC_SUCCESS;
  }
  if (*target == TPM_RC_HMAC) {
    return TPM_RC_SUCCESS;
  }
  if (*target == TPM_RC_DISABLED) {
    return TPM_RC_SUCCESS;
  }
  if (*target == TPM_RC_EXCLUSIVE) {
    return TPM_RC_SUCCESS;
  }
  if (*target == TPM_RC_AUTH_TYPE) {
    return TPM_RC_SUCCESS;
  }
  if (*target == TPM_RC_AUTH_MISSING) {
    return TPM_RC_SUCCESS;
  }
  if (*target == TPM_RC_POLICY) {
    return TPM_RC_SUCCESS;
  }
  if (*target == TPM_RC_PCR) {
    return TPM_RC_SUCCESS;
  }
  if (*target == TPM_RC_PCR_CHANGED) {
    return TPM_RC_SUCCESS;
  }
  if (*target == TPM_RC_UPGRADE) {
    return TPM_RC_SUCCESS;
  }
  if (*target == TPM_RC_TOO_MANY_CONTEXTS) {
    return TPM_RC_SUCCESS;
  }
  if (*target == TPM_RC_AUTH_UNAVAILABLE) {
    return TPM_RC_SUCCESS;
  }
  if (*target == TPM_RC_REBOOT) {
    return TPM_RC_SUCCESS;
  }
  if (*target == TPM_RC_UNBALANCED) {
    return TPM_RC_SUCCESS;
  }
  if (*target == TPM_RC_COMMAND_SIZE) {
    return TPM_RC_SUCCESS;
  }
  if (*target == TPM_RC_COMMAND_CODE) {
    return TPM_RC_SUCCESS;
  }
  if (*target == TPM_RC_AUTHSIZE) {
    return TPM_RC_SUCCESS;
  }
  if (*target == TPM_RC_AUTH_CONTEXT) {
    return TPM_RC_SUCCESS;
  }
  if (*target == TPM_RC_NV_RANGE) {
    return TPM_RC_SUCCESS;
  }
  if (*target == TPM_RC_NV_SIZE) {
    return TPM_RC_SUCCESS;
  }
  if (*target == TPM_RC_NV_LOCKED) {
    return TPM_RC_SUCCESS;
  }
  if (*target == TPM_RC_NV_AUTHORIZATION) {
    return TPM_RC_SUCCESS;
  }
  if (*target == TPM_RC_NV_UNINITIALIZED) {
    return TPM_RC_SUCCESS;
  }
  if (*target == TPM_RC_NV_SPACE) {
    return TPM_RC_SUCCESS;
  }
  if (*target == TPM_RC_NV_DEFINED) {
    return TPM_RC_SUCCESS;
  }
  if (*target == TPM_RC_BAD_CONTEXT) {
    return TPM_RC_SUCCESS;
  }
  if (*target == TPM_RC_CPHASH) {
    return TPM_RC_SUCCESS;
  }
  if (*target == TPM_RC_PARENT) {
    return TPM_RC_SUCCESS;
  }
  if (*target == TPM_RC_NEEDS_TEST) {
    return TPM_RC_SUCCESS;
  }
  if (*target == TPM_RC_NO_RESULT) {
    return TPM_RC_SUCCESS;
  }
  if (*target == TPM_RC_SENSITIVE) {
    return TPM_RC_SUCCESS;
  }
  if (*target == RC_MAX_FM0) {
    return TPM_RC_SUCCESS;
  }
  if (*target == RC_FMT1) {
    return TPM_RC_SUCCESS;
  }
  if (*target == TPM_RC_ASYMMETRIC) {
    return TPM_RC_SUCCESS;
  }
  if (*target == TPM_RC_ATTRIBUTES) {
    return TPM_RC_SUCCESS;
  }
  if (*target == TPM_RC_HASH) {
    return TPM_RC_SUCCESS;
  }
  if (*target == TPM_RC_VALUE) {
    return TPM_RC_SUCCESS;
  }
  if (*target == TPM_RC_HIERARCHY) {
    return TPM_RC_SUCCESS;
  }
  if (*target == TPM_RC_KEY_SIZE) {
    return TPM_RC_SUCCESS;
  }
  if (*target == TPM_RC_MGF) {
    return TPM_RC_SUCCESS;
  }
  if (*target == TPM_RC_MODE) {
    return TPM_RC_SUCCESS;
  }
  if (*target == TPM_RC_TYPE) {
    return TPM_RC_SUCCESS;
  }
  if (*target == TPM_RC_HANDLE) {
    return TPM_RC_SUCCESS;
  }
  if (*target == TPM_RC_KDF) {
    return TPM_RC_SUCCESS;
  }
  if (*target == TPM_RC_RANGE) {
    return TPM_RC_SUCCESS;
  }
  if (*target == TPM_RC_AUTH_FAIL) {
    return TPM_RC_SUCCESS;
  }
  if (*target == TPM_RC_NONCE) {
    return TPM_RC_SUCCESS;
  }
  if (*target == TPM_RC_PP) {
    return TPM_RC_SUCCESS;
  }
  if (*target == TPM_RC_SCHEME) {
    return TPM_RC_SUCCESS;
  }
  if (*target == TPM_RC_SIZE) {
    return TPM_RC_SUCCESS;
  }
  if (*target == TPM_RC_SYMMETRIC) {
    return TPM_RC_SUCCESS;
  }
  if (*target == TPM_RC_TAG) {
    return TPM_RC_SUCCESS;
  }
  if (*target == TPM_RC_SELECTOR) {
    return TPM_RC_SUCCESS;
  }
  if (*target == TPM_RC_INSUFFICIENT) {
    return TPM_RC_SUCCESS;
  }
  if (*target == TPM_RC_SIGNATURE) {
    return TPM_RC_SUCCESS;
  }
  if (*target == TPM_RC_KEY) {
    return TPM_RC_SUCCESS;
  }
  if (*target == TPM_RC_POLICY_FAIL) {
    return TPM_RC_SUCCESS;
  }
  if (*target == TPM_RC_INTEGRITY) {
    return TPM_RC_SUCCESS;
  }
  if (*target == TPM_RC_TICKET) {
    return TPM_RC_SUCCESS;
  }
  if (*target == TPM_RC_RESERVED_BITS) {
    return TPM_RC_SUCCESS;
  }
  if (*target == TPM_RC_BAD_AUTH) {
    return TPM_RC_SUCCESS;
  }
  if (*target == TPM_RC_EXPIRED) {
    return TPM_RC_SUCCESS;
  }
  if (*target == TPM_RC_POLICY_CC) {
    return TPM_RC_SUCCESS;
  }
  if (*target == TPM_RC_BINDING) {
    return TPM_RC_SUCCESS;
  }
  if (*target == TPM_RC_CURVE) {
    return TPM_RC_SUCCESS;
  }
  if (*target == TPM_RC_ECC_POINT) {
    return TPM_RC_SUCCESS;
  }
  if (*target == RC_WARN) {
    return TPM_RC_SUCCESS;
  }
  if (*target == TPM_RC_CONTEXT_GAP) {
    return TPM_RC_SUCCESS;
  }
  if (*target == TPM_RC_OBJECT_MEMORY) {
    return TPM_RC_SUCCESS;
  }
  if (*target == TPM_RC_SESSION_MEMORY) {
    return TPM_RC_SUCCESS;
  }
  if (*target == TPM_RC_MEMORY) {
    return TPM_RC_SUCCESS;
  }
  if (*target == TPM_RC_SESSION_HANDLES) {
    return TPM_RC_SUCCESS;
  }
  if (*target == TPM_RC_OBJECT_HANDLES) {
    return TPM_RC_SUCCESS;
  }
  if (*target == TPM_RC_LOCALITY) {
    return TPM_RC_SUCCESS;
  }
  if (*target == TPM_RC_YIELDED) {
    return TPM_RC_SUCCESS;
  }
  if (*target == TPM_RC_CANCELED) {
    return TPM_RC_SUCCESS;
  }
  if (*target == TPM_RC_TESTING) {
    return TPM_RC_SUCCESS;
  }
  if (*target == TPM_RC_REFERENCE_H0) {
    return TPM_RC_SUCCESS;
  }
  if (*target == TPM_RC_REFERENCE_H1) {
    return TPM_RC_SUCCESS;
  }
  if (*target == TPM_RC_REFERENCE_H2) {
    return TPM_RC_SUCCESS;
  }
  if (*target == TPM_RC_REFERENCE_H3) {
    return TPM_RC_SUCCESS;
  }
  if (*target == TPM_RC_REFERENCE_H4) {
    return TPM_RC_SUCCESS;
  }
  if (*target == TPM_RC_REFERENCE_H5) {
    return TPM_RC_SUCCESS;
  }
  if (*target == TPM_RC_REFERENCE_H6) {
    return TPM_RC_SUCCESS;
  }
  if (*target == TPM_RC_REFERENCE_S0) {
    return TPM_RC_SUCCESS;
  }
  if (*target == TPM_RC_REFERENCE_S1) {
    return TPM_RC_SUCCESS;
  }
  if (*target == TPM_RC_REFERENCE_S2) {
    return TPM_RC_SUCCESS;
  }
  if (*target == TPM_RC_REFERENCE_S3) {
    return TPM_RC_SUCCESS;
  }
  if (*target == TPM_RC_REFERENCE_S4) {
    return TPM_RC_SUCCESS;
  }
  if (*target == TPM_RC_REFERENCE_S5) {
    return TPM_RC_SUCCESS;
  }
  if (*target == TPM_RC_REFERENCE_S6) {
    return TPM_RC_SUCCESS;
  }
  if (*target == TPM_RC_NV_RATE) {
    return TPM_RC_SUCCESS;
  }
  if (*target == TPM_RC_LOCKOUT) {
    return TPM_RC_SUCCESS;
  }
  if (*target == TPM_RC_RETRY) {
    return TPM_RC_SUCCESS;
  }
  if (*target == TPM_RC_NV_UNAVAILABLE) {
    return TPM_RC_SUCCESS;
  }
  if (*target == TPM_RC_NOT_USED) {
    return TPM_RC_SUCCESS;
  }
  if (*target == TPM_RC_H) {
    return TPM_RC_SUCCESS;
  }
  if (*target == TPM_RC_P) {
    return TPM_RC_SUCCESS;
  }
  if (*target == TPM_RC_S) {
    return TPM_RC_SUCCESS;
  }
  if (*target == TPM_RC_1) {
    return TPM_RC_SUCCESS;
  }
  if (*target == TPM_RC_2) {
    return TPM_RC_SUCCESS;
  }
  if (*target == TPM_RC_3) {
    return TPM_RC_SUCCESS;
  }
  if (*target == TPM_RC_4) {
    return TPM_RC_SUCCESS;
  }
  if (*target == TPM_RC_5) {
    return TPM_RC_SUCCESS;
  }
  if (*target == TPM_RC_6) {
    return TPM_RC_SUCCESS;
  }
  if (*target == TPM_RC_7) {
    return TPM_RC_SUCCESS;
  }
  if (*target == TPM_RC_8) {
    return TPM_RC_SUCCESS;
  }
  if (*target == TPM_RC_9) {
    return TPM_RC_SUCCESS;
  }
  if (*target == TPM_RC_A) {
    return TPM_RC_SUCCESS;
  }
  if (*target == TPM_RC_B) {
    return TPM_RC_SUCCESS;
  }
  if (*target == TPM_RC_C) {
    return TPM_RC_SUCCESS;
  }
  if (*target == TPM_RC_D) {
    return TPM_RC_SUCCESS;
  }
  if (*target == TPM_RC_E) {
    return TPM_RC_SUCCESS;
  }
  if (*target == TPM_RC_F) {
    return TPM_RC_SUCCESS;
  }
  if (*target == TPM_RC_N_MASK) {
    return TPM_RC_SUCCESS;
  }
  return TPM_RC_VALUE;
}

UINT16 TPM_RH_Marshal(TPM_RH* source, BYTE** buffer, INT32* size) {
  return uint32_t_Marshal(source, buffer, size);
}

TPM_RC TPM_RH_Unmarshal(TPM_RH* target, BYTE** buffer, INT32* size) {
  TPM_RC result;
  result = uint32_t_Unmarshal(target, buffer, size);
  if (result != TPM_RC_SUCCESS) {
    return result;
  }
  if (*target == TPM_RH_FIRST) {
    return TPM_RC_SUCCESS;
  }
  if (*target == TPM_RH_SRK) {
    return TPM_RC_SUCCESS;
  }
  if (*target == TPM_RH_OWNER) {
    return TPM_RC_SUCCESS;
  }
  if (*target == TPM_RH_REVOKE) {
    return TPM_RC_SUCCESS;
  }
  if (*target == TPM_RH_TRANSPORT) {
    return TPM_RC_SUCCESS;
  }
  if (*target == TPM_RH_OPERATOR) {
    return TPM_RC_SUCCESS;
  }
  if (*target == TPM_RH_ADMIN) {
    return TPM_RC_SUCCESS;
  }
  if (*target == TPM_RH_EK) {
    return TPM_RC_SUCCESS;
  }
  if (*target == TPM_RH_NULL) {
    return TPM_RC_SUCCESS;
  }
  if (*target == TPM_RH_UNASSIGNED) {
    return TPM_RC_SUCCESS;
  }
  if (*target == TPM_RS_PW) {
    return TPM_RC_SUCCESS;
  }
  if (*target == TPM_RH_LOCKOUT) {
    return TPM_RC_SUCCESS;
  }
  if (*target == TPM_RH_ENDORSEMENT) {
    return TPM_RC_SUCCESS;
  }
  if (*target == TPM_RH_PLATFORM) {
    return TPM_RC_SUCCESS;
  }
  if (*target == TPM_RH_PLATFORM_NV) {
    return TPM_RC_SUCCESS;
  }
  if (*target == TPM_RH_AUTH_00) {
    return TPM_RC_SUCCESS;
  }
  if (*target == TPM_RH_AUTH_FF) {
    return TPM_RC_SUCCESS;
  }
  if (*target == TPM_RH_LAST) {
    return TPM_RC_SUCCESS;
  }
  return TPM_RC_VALUE;
}

UINT16 TPM_SE_Marshal(TPM_SE* source, BYTE** buffer, INT32* size) {
  return uint8_t_Marshal(source, buffer, size);
}

TPM_RC TPM_SE_Unmarshal(TPM_SE* target, BYTE** buffer, INT32* size) {
  TPM_RC result;
  result = uint8_t_Unmarshal(target, buffer, size);
  if (result != TPM_RC_SUCCESS) {
    return result;
  }
  if (*target == TPM_SE_HMAC) {
    return TPM_RC_SUCCESS;
  }
  if (*target == TPM_SE_POLICY) {
    return TPM_RC_SUCCESS;
  }
  if (*target == TPM_SE_TRIAL) {
    return TPM_RC_SUCCESS;
  }
  return TPM_RC_VALUE;
}

UINT16 TPM_SPEC_Marshal(TPM_SPEC* source, BYTE** buffer, INT32* size) {
  return uint32_t_Marshal(source, buffer, size);
}

TPM_RC TPM_SPEC_Unmarshal(TPM_SPEC* target, BYTE** buffer, INT32* size) {
  TPM_RC result;
  result = uint32_t_Unmarshal(target, buffer, size);
  if (result != TPM_RC_SUCCESS) {
    return result;
  }
  if (*target == TPM_SPEC_FAMILY) {
    return TPM_RC_SUCCESS;
  }
  if (*target == TPM_SPEC_LEVEL) {
    return TPM_RC_SUCCESS;
  }
  if (*target == TPM_SPEC_VERSION) {
    return TPM_RC_SUCCESS;
  }
  if (*target == TPM_SPEC_YEAR) {
    return TPM_RC_SUCCESS;
  }
  if (*target == TPM_SPEC_DAY_OF_YEAR) {
    return TPM_RC_SUCCESS;
  }
  return TPM_RC_VALUE;
}

UINT16 TPM_SU_Marshal(TPM_SU* source, BYTE** buffer, INT32* size) {
  return uint16_t_Marshal(source, buffer, size);
}

TPM_RC TPM_SU_Unmarshal(TPM_SU* target, BYTE** buffer, INT32* size) {
  TPM_RC result;
  result = uint16_t_Unmarshal(target, buffer, size);
  if (result != TPM_RC_SUCCESS) {
    return result;
  }
  if (*target == TPM_SU_CLEAR) {
    return TPM_RC_SUCCESS;
  }
  if (*target == TPM_SU_STATE) {
    return TPM_RC_SUCCESS;
  }
  return TPM_RC_VALUE;
}

UINT16 _ID_OBJECT_Marshal(_ID_OBJECT* source, BYTE** buffer, INT32* size) {
  UINT16 total_size = 0;
  total_size += TPM2B_DIGEST_Marshal(&source->integrityHMAC, buffer, size);
  total_size += TPM2B_DIGEST_Marshal(&source->encIdentity, buffer, size);
  return total_size;
}

TPM_RC _ID_OBJECT_Unmarshal(_ID_OBJECT* target, BYTE** buffer, INT32* size) {
  TPM_RC result;
  result = TPM2B_DIGEST_Unmarshal(&target->integrityHMAC, buffer, size);
  if (result != TPM_RC_SUCCESS) {
    return result;
  }
  result = TPM2B_DIGEST_Unmarshal(&target->encIdentity, buffer, size);
  if (result != TPM_RC_SUCCESS) {
    return result;
  }
  return TPM_RC_SUCCESS;
}

UINT16 _PRIVATE_Marshal(_PRIVATE* source, BYTE** buffer, INT32* size) {
  UINT16 total_size = 0;
  total_size += TPM2B_DIGEST_Marshal(&source->integrityOuter, buffer, size);
  total_size += TPM2B_DIGEST_Marshal(&source->integrityInner, buffer, size);
  total_size += TPMT_SENSITIVE_Marshal(&source->sensitive, buffer, size);
  return total_size;
}

TPM_RC _PRIVATE_Unmarshal(_PRIVATE* target, BYTE** buffer, INT32* size) {
  TPM_RC result;
  result = TPM2B_DIGEST_Unmarshal(&target->integrityOuter, buffer, size);
  if (result != TPM_RC_SUCCESS) {
    return result;
  }
  result = TPM2B_DIGEST_Unmarshal(&target->integrityInner, buffer, size);
  if (result != TPM_RC_SUCCESS) {
    return result;
  }
  result = TPMT_SENSITIVE_Unmarshal(&target->sensitive, buffer, size);
  if (result != TPM_RC_SUCCESS) {
    return result;
  }
  return TPM_RC_SUCCESS;
}
