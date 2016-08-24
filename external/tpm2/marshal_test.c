#include <stdio.h>
#include <string.h>
#include "TPM_Types.h"

void BasicTypesSuccessTest();
void BasicTypesFailureTest();
void TypedefSuccessTest();
void TypedefFailureTest();
void ConstantTypeSuccessTest();
void ConstantTypeFailureTest();
void AttributeStructSuccessTest();
void AttributeStructFailureTest();
void InterfaceSuccessTest();
void InterfaceRangeFailureTest();
void InterfaceNullFailureTest();
void InterfaceValueFailureTest();
void InterfaceKeyBitsTest();
void StructureSuccessNormalTest();
void StructureSuccessValueTest();
void StructureFailureNullTest();
void StructureSuccessArrayTest();
void StructureSuccessNullTest();
void StructureFailureTagTest();
void StructureSuccessSizeCheckTest();

/* gtest like macro */
#define CHECK_EQ(a, b) if (a != b) printf("[ERROR:%d] CHECK_EQ(%s == %s) failed\n", __LINE__, #a, #b);

#define SETUP_TYPE(type, val)                 \
  const uint16_t num_bytes = sizeof(type);    \
  INT32 size = num_bytes;                     \
  BYTE buffer[size];                          \
  BYTE *buffer_ptr = buffer;                  \
  type value = val;

#define SETUP_STRUCT(type, val)               \
  const uint16_t num_bytes = sizeof(type);    \
  INT32 size = num_bytes;                     \
  BYTE buffer[size];                          \
  BYTE *buffer_ptr = buffer;                  \
  type value;                                 \
  memset(&value, val, sizeof(type));

#define RESET_TYPE(val)            \
  value = val;                     \
  buffer_ptr = buffer;             \
  size = num_bytes;

#define RESET_STRUCT(type, val)          \
  memset(&value, val, sizeof(type));     \
  buffer_ptr = buffer;                   \
  size = num_bytes;

int main() {
  printf("\nRunning marshal unit tests.\n\n");
  BasicTypesSuccessTest();
  BasicTypesFailureTest();
  TypedefSuccessTest();
  TypedefFailureTest();
  ConstantTypeSuccessTest();
  ConstantTypeFailureTest();
  AttributeStructSuccessTest();
  AttributeStructFailureTest();
  InterfaceSuccessTest();
  InterfaceRangeFailureTest();
  InterfaceNullFailureTest();
  InterfaceValueFailureTest();
  InterfaceKeyBitsTest();
  StructureSuccessNormalTest();
  StructureSuccessValueTest();
  StructureFailureNullTest();
  StructureSuccessArrayTest();
  StructureSuccessNullTest();
  StructureFailureTagTest();
  StructureSuccessSizeCheckTest();
  printf("\nFinished all tests.\n\n");
}


void BasicTypesSuccessTest() {
  printf("Running BasicTypesSuccessTest.\n");
  SETUP_TYPE(uint32_t, 12345)
  UINT16 bytes_marshalled = uint32_t_Marshal(&value, &buffer_ptr, &size);
  CHECK_EQ(bytes_marshalled, num_bytes)
  CHECK_EQ(size, 0)
  CHECK_EQ(buffer_ptr, buffer+num_bytes)

  RESET_TYPE(0)
  TPM_RC rc = uint32_t_Unmarshal(&value, &buffer_ptr, &size);
  CHECK_EQ(rc, TPM_RC_SUCCESS);
  CHECK_EQ(size, 0);
  CHECK_EQ(buffer_ptr, buffer+num_bytes);
  /* Checking that value was marshalled then unmarshalled successfully */
  CHECK_EQ(value, 12345);
}

void BasicTypesFailureTest() {
  printf("Running BasicTypesFailureTest.\n");
  SETUP_TYPE(uint32_t, 12345)
  --size;
  UINT16 bytes_marshalled = uint32_t_Marshal(&value, &buffer_ptr, &size);
  CHECK_EQ(size, num_bytes-1);
  CHECK_EQ(bytes_marshalled, num_bytes);
  CHECK_EQ(buffer, buffer_ptr);

  bytes_marshalled = uint32_t_Marshal(&value, &buffer_ptr, NULL);
  CHECK_EQ(bytes_marshalled, num_bytes);
  CHECK_EQ(buffer, buffer_ptr);

  TPM_RC rc = uint32_t_Unmarshal(&value, &buffer_ptr, &size);
  CHECK_EQ(rc, TPM_RC_INSUFFICIENT);

  rc = uint32_t_Unmarshal(&value, &buffer_ptr, NULL);
  CHECK_EQ(rc, TPM_RC_INSUFFICIENT);
}

void TypedefSuccessTest() {
  printf("Running TypedefSuccessTest.\n");
  SETUP_TYPE(TPM_KEY_BITS, 12345)
  UINT16 bytes_marshalled = TPM_KEY_BITS_Marshal(&value, &buffer_ptr, &size);
  CHECK_EQ(bytes_marshalled, num_bytes);
  CHECK_EQ(size, 0);
  CHECK_EQ(buffer_ptr, buffer+num_bytes);

  RESET_TYPE(0)
  TPM_RC rc = TPM_KEY_BITS_Unmarshal(&value, &buffer_ptr, &size);
  CHECK_EQ(rc, TPM_RC_SUCCESS);
  CHECK_EQ(size, 0);
  CHECK_EQ(buffer_ptr, buffer+num_bytes);
  /* Checking that value was marshalled then unmarshalled successfully */
  CHECK_EQ(value, 12345);
}

void TypedefFailureTest() {
  printf("Running TypedefFailureTest.\n");
  SETUP_TYPE(TPM_KEY_BITS, 12345)
  --size;
  UINT16 bytes_marshalled = TPM_KEY_BITS_Marshal(&value, &buffer_ptr, &size);
  CHECK_EQ(size, num_bytes-1);
  CHECK_EQ(bytes_marshalled, num_bytes);
  CHECK_EQ(buffer, buffer_ptr);

  bytes_marshalled = TPM_KEY_BITS_Marshal(&value, &buffer_ptr, NULL);
  CHECK_EQ(bytes_marshalled, num_bytes);
  CHECK_EQ(buffer, buffer_ptr);

  TPM_RC rc = TPM_KEY_BITS_Unmarshal(&value, &buffer_ptr, &size);
  CHECK_EQ(rc, TPM_RC_INSUFFICIENT);

  rc = TPM_KEY_BITS_Unmarshal(&value, &buffer_ptr, NULL);
  CHECK_EQ(rc, TPM_RC_INSUFFICIENT);
}

void ConstantTypeSuccessTest() {
  printf("Runnint ConstantTypeSuccessTest.\n");
  SETUP_TYPE(TPM_ST, TPM_ST_ATTEST_NV)
  UINT16 bytes_marshalled = TPM_ST_Marshal(&value, &buffer_ptr, &size);
  CHECK_EQ(bytes_marshalled, num_bytes);
  CHECK_EQ(size, 0);
  CHECK_EQ(buffer_ptr, buffer+num_bytes);

  RESET_TYPE(0)
  TPM_RC rc = TPM_ST_Unmarshal(&value, &buffer_ptr, &size);
  CHECK_EQ(rc, TPM_RC_SUCCESS);
  CHECK_EQ(size, 0);
  CHECK_EQ(buffer_ptr, buffer+num_bytes);
  CHECK_EQ(value, TPM_ST_ATTEST_NV);
}

void ConstantTypeFailureTest() {
  printf("Running ConstantTypeFailureTest.\n");
  SETUP_TYPE(TPM_ECC_CURVE, 12345)

  TPM_RC rc = TPM_ECC_CURVE_Unmarshal(&value, &buffer_ptr, &size);
  CHECK_EQ(rc, TPM_RC_CURVE);
  CHECK_EQ(size, 0);
}

void AttributeStructSuccessTest() {
  printf("Running AttributeStructSuccessTest.\n");
  SETUP_STRUCT(TPMA_OBJECT, 0)
  /* Set some bits to ensure validity */
  value.fixedTPM = 1;
  value.fixedParent = 1;
  UINT16 bytes_marshalled = TPMA_OBJECT_Marshal(&value, &buffer_ptr, &size);
  CHECK_EQ(bytes_marshalled, num_bytes);
  CHECK_EQ(size, 0);
  CHECK_EQ(buffer_ptr, buffer+num_bytes);

  RESET_STRUCT(TPMA_OBJECT, 0)
  TPM_RC rc = TPMA_OBJECT_Unmarshal(&value, &buffer_ptr, &size);
  CHECK_EQ(rc, TPM_RC_SUCCESS);
  CHECK_EQ(size, 0);
  CHECK_EQ(buffer_ptr, buffer+num_bytes);
  CHECK_EQ(value.fixedTPM, 1);
  CHECK_EQ(value.fixedParent, 1);
}

void AttributeStructFailureTest() {
  printf("Running AttributeStructFailureTest.\n");
  SETUP_STRUCT(TPMA_OBJECT, 0)
  /* Failure occurs when reserved bit is set */
  value.reserved8_9 = 1;
  TPMA_OBJECT_Marshal(&value, &buffer_ptr, &size);
  RESET_STRUCT(TPMA_OBJECT, 0)
  TPM_RC rc = TPMA_OBJECT_Unmarshal(&value, &buffer_ptr, &size);
  CHECK_EQ(rc, TPM_RC_RESERVED_BITS);
  CHECK_EQ(size, 0);
}

void InterfaceSuccessTest() {
  printf("Running InterfaceSuccessTest.\n");
  SETUP_TYPE(TPMI_DH_ENTITY, TRANSIENT_FIRST+1)
  /* Value has valid value from table */
  UINT16 bytes_marshalled = TPMI_DH_ENTITY_Marshal(&value, &buffer_ptr, &size);
  CHECK_EQ(bytes_marshalled, num_bytes);
  CHECK_EQ(size, 0);
  CHECK_EQ(buffer_ptr, buffer+num_bytes);

  RESET_TYPE(0)
  TPM_RC rc = TPMI_DH_ENTITY_Unmarshal(&value, &buffer_ptr, &size, FALSE);
  CHECK_EQ(rc, TPM_RC_SUCCESS);
  CHECK_EQ(size, 0);
  CHECK_EQ(buffer_ptr, buffer+num_bytes);
  CHECK_EQ(value, TRANSIENT_FIRST+1);

  /* Value is optional value and TRUE is passed in as flag parameter*/
  RESET_TYPE(TPM_RH_NULL)
  bytes_marshalled = TPMI_DH_ENTITY_Marshal(&value, &buffer_ptr, &size);
  CHECK_EQ(bytes_marshalled, num_bytes);
  CHECK_EQ(size, 0);
  CHECK_EQ(buffer_ptr, buffer+num_bytes);

  RESET_TYPE(0)
  rc = TPMI_DH_ENTITY_Unmarshal(&value, &buffer_ptr, &size, TRUE);
  CHECK_EQ(rc, TPM_RC_SUCCESS);
  CHECK_EQ(size, 0);
  CHECK_EQ(buffer_ptr, buffer+num_bytes);
  CHECK_EQ(value, TPM_RH_NULL);

  /* Value has valid value from table */
  RESET_TYPE(TPM_RH_OWNER)
  bytes_marshalled = TPMI_DH_ENTITY_Marshal(&value, &buffer_ptr, &size);
  CHECK_EQ(bytes_marshalled, num_bytes);
  CHECK_EQ(size, 0);
  CHECK_EQ(buffer_ptr, buffer+num_bytes);

  RESET_TYPE(0)
  rc = TPMI_DH_ENTITY_Unmarshal(&value, &buffer_ptr, &size, FALSE);
  CHECK_EQ(rc, TPM_RC_SUCCESS);
  CHECK_EQ(size, 0);
  CHECK_EQ(buffer_ptr, buffer+num_bytes);
  CHECK_EQ(value, TPM_RH_OWNER);
}

void InterfaceRangeFailureTest() {
  printf("Running InterfaceRangeFailureTest.\n");
  /* Value is out of range */
  SETUP_TYPE(TPMI_DH_OBJECT, TRANSIENT_FIRST-1)
  TPMI_DH_OBJECT_Marshal(&value, &buffer_ptr, &size);

  RESET_TYPE(0)
  TPM_RC rc = TPMI_DH_OBJECT_Unmarshal(&value, &buffer_ptr, &size, FALSE);
  CHECK_EQ(rc, TPM_RC_VALUE);

  RESET_TYPE(PERSISTENT_LAST+1)
  TPMI_DH_OBJECT_Marshal(&value, &buffer_ptr, &size);
  RESET_TYPE(0)
  rc = TPMI_DH_OBJECT_Unmarshal(&value, &buffer_ptr, &size, FALSE);
  CHECK_EQ(rc, TPM_RC_VALUE);
}

void InterfaceNullFailureTest() {
  printf("Running InterfaceNullFailureTest.\n");
  SETUP_TYPE(TPMI_DH_OBJECT, TPM_RH_NULL)
  TPMI_DH_OBJECT_Marshal(&value, &buffer_ptr, &size);
  RESET_TYPE(0)
  TPM_RC rc = TPMI_DH_OBJECT_Unmarshal(&value, &buffer_ptr, &size, FALSE);
  CHECK_EQ(rc, TPM_RC_VALUE);
}

void InterfaceValueFailureTest() {
  printf("Running InterfaceValueFailureTest.\n");
  SETUP_TYPE(TPMI_DH_ENTITY, TPM_RH_REVOKE)
  TPMI_DH_ENTITY_Marshal(&value, &buffer_ptr, &size);
  RESET_TYPE(0)
  TPM_RC rc = TPMI_DH_ENTITY_Unmarshal(&value, &buffer_ptr, &size, TRUE);
  CHECK_EQ(rc, TPM_RC_VALUE);
}

void InterfaceKeyBitsTest() {
  printf("Running InterfaceKeyBitsTest\n");
  uint16_t vals[] = AES_KEY_SIZES_BITS;
  SETUP_TYPE(TPMI_AES_KEY_BITS, vals[0])
  TPMI_AES_KEY_BITS_Marshal(&value, &buffer_ptr, &size);
  UINT16 bytes_marshalled = TPMI_AES_KEY_BITS_Marshal(&value, &buffer_ptr, &size);
  CHECK_EQ(bytes_marshalled, num_bytes);
  CHECK_EQ(size, 0);
  CHECK_EQ(buffer_ptr, buffer+num_bytes);
  RESET_TYPE(0)
  TPM_RC rc = TPMI_AES_KEY_BITS_Unmarshal(&value, &buffer_ptr, &size, TRUE);
  CHECK_EQ(rc, TPM_RC_SUCCESS);
  CHECK_EQ(value, vals[0]);
}

void StructureSuccessNormalTest() {
  /* Basic success case of structure marshalling */
  printf("Running StructureSuccessNormalTest.\n");
  SETUP_STRUCT(TPMS_CLOCK_INFO, 0)
  value.clock = 12345;
  value.resetCount = 123;
  value.restartCount = 45;
  value.safe = YES;
  TPMS_CLOCK_INFO_Marshal(&value, &buffer_ptr, &size);
  RESET_STRUCT(TPMS_CLOCK_INFO, 0)
  TPM_RC rc = TPMS_CLOCK_INFO_Unmarshal(&value, &buffer_ptr, &size);
  CHECK_EQ(rc, TPM_RC_SUCCESS);
  CHECK_EQ(value.safe, YES);
  CHECK_EQ(value.clock, 12345);
  CHECK_EQ(value.resetCount, 123);
  CHECK_EQ(value.restartCount, 45);
}

void StructureSuccessValueTest() {
  /* Success case of structure marshalling involving field value checking */
  printf("Running StructureSuccessValueTest\n");
  SETUP_STRUCT(TPML_DIGEST, 0)
  value.count = 4;
  UINT16 bytes_marshalled = TPML_DIGEST_Marshal(&value, &buffer_ptr, &size);
  CHECK_EQ(bytes_marshalled, sizeof(UINT32)+4*sizeof(UINT16));
  RESET_STRUCT(TPML_DIGEST, 0)
  TPM_RC rc = TPML_DIGEST_Unmarshal(&value, &buffer_ptr, &size);
  CHECK_EQ(rc, TPM_RC_SUCCESS);
  CHECK_EQ(value.count, 4);
}

void StructureFailureNullTest() {
  /* Failure case of structure marshalling where TPMI field is NULL */
  printf("Running StructureFailureNullTest\n");
  SETUP_STRUCT(TPMS_PCR_SELECTION, 0)
  value.hash = TPM_ALG_NULL;
  value.sizeofSelect = PCR_SELECT_MIN;
  TPMS_PCR_SELECTION_Marshal(&value, &buffer_ptr, &size);
  RESET_STRUCT(TPMS_PCR_SELECTION, 0)
  TPM_RC rc = TPMS_PCR_SELECTION_Unmarshal(&value, &buffer_ptr, &size);
  CHECK_EQ(rc, TPM_RC_HASH);
}

void StructureSuccessArrayTest() {
  /* Success case of structure marshalling involving array */
  printf("Running StructureSuccessArrayTest\n");
  SETUP_STRUCT(TPM2B_DIGEST, 0)
  value.t.size = sizeof(TPMU_HA)-1;
  UINT16 bytes_marshalled = TPM2B_DIGEST_Marshal(&value, &buffer_ptr, &size);
  UINT16 expected_bytes = sizeof(UINT16)+(sizeof(TPMU_HA)-1)*sizeof(BYTE);
  CHECK_EQ(bytes_marshalled, expected_bytes);
  RESET_STRUCT(TPM2B_DIGEST, 0)
  TPM_RC rc = TPM2B_DIGEST_Unmarshal(&value, &buffer_ptr, &size);
  CHECK_EQ(size, sizeof(TPM2B_DIGEST)-expected_bytes);
  CHECK_EQ(rc, TPM_RC_SUCCESS);
}

void StructureSuccessNullTest() {
  /* Success case of structure marshalling involving valid null value and
   * valid tag value
   */
  printf("Running StructureSuccessNullTest\n");
  SETUP_STRUCT(TPMT_TK_HASHCHECK, 0)
  value.tag = TPM_ST_HASHCHECK;
  value.hierarchy = TPM_RH_NULL;
  UINT16 bytes_marshalled = TPMT_TK_HASHCHECK_Marshal(&value, &buffer_ptr, &size);
  UINT16 expected_bytes = sizeof(TPM_ST)+sizeof(TPMI_RH_HIERARCHY)+sizeof(UINT16);
  CHECK_EQ(bytes_marshalled, expected_bytes);
  RESET_STRUCT(TPMT_TK_HASHCHECK, 0)
  TPM_RC rc = TPMT_TK_HASHCHECK_Unmarshal(&value, &buffer_ptr, &size);
  CHECK_EQ(size, sizeof(TPMT_TK_HASHCHECK)-expected_bytes);
  CHECK_EQ(rc, TPM_RC_SUCCESS);
}

void StructureFailureTagTest() {
  /* Failure case of structure marshalling with invalid tag value */
  printf("Running StructureFailureTagTest\n");
  SETUP_STRUCT(TPMT_TK_HASHCHECK, 0)
  value.tag = TPM_ST_RSP_COMMAND;
  UINT16 bytes_marshalled = TPMT_TK_HASHCHECK_Marshal(&value, &buffer_ptr, &size);
  UINT16 expected_bytes = sizeof(TPM_ST)+sizeof(TPMI_RH_HIERARCHY)+sizeof(UINT16);
  CHECK_EQ(bytes_marshalled, expected_bytes);
  RESET_STRUCT(TPMT_TK_HASHCHECK, 0)
  TPM_RC rc = TPMT_TK_HASHCHECK_Unmarshal(&value, &buffer_ptr, &size);
  CHECK_EQ(rc, TPM_RC_TAG);
}

void StructureSuccessSizeCheckTest() {
  /* Success case of structure marshalling with size= field */
  printf("Running StructureSuccessSizeCheckTest\n");
  SETUP_STRUCT(TPM2B_NV_PUBLIC, 0)
  value.t.size = sizeof(TPMI_RH_NV_INDEX)+sizeof(TPMI_ALG_HASH)+sizeof(TPMA_NV)+sizeof(UINT16)+sizeof(UINT16);
  value.t.nvPublic.nvIndex = NV_INDEX_FIRST;
  value.t.nvPublic.nameAlg = TPM_ALG_SHA1;
  UINT16 bytes_marshalled = TPM2B_NV_PUBLIC_Marshal(&value, &buffer_ptr, &size);
  RESET_STRUCT(TPM2B_NV_PUBLIC, 0)
  TPM_RC rc = TPM2B_NV_PUBLIC_Unmarshal(&value, &buffer_ptr, &size);
  CHECK_EQ(rc, TPM_RC_SUCCESS)
}
