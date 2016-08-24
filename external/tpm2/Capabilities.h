// This file was extracted from the TCG Published
// Trusted Platform Module Library
// Part 4: Supporting Routines
// Family "2.0"
// Level 00 Revision 01.16
// October 30, 2014

#ifndef        _CAPABILITIES_H
#define        _CAPABILITIES_H
#define       MAX_CAP_DATA                (MAX_CAP_BUFFER-sizeof(TPM_CAP)-sizeof(UINT32))
#define       MAX_CAP_ALGS                (ALG_LAST_VALUE - ALG_FIRST_VALUE + 1)
#define       MAX_CAP_HANDLES             (MAX_CAP_DATA/sizeof(TPM_HANDLE))
#define       MAX_CAP_CC                  ((TPM_CC_LAST - TPM_CC_FIRST) + 1)
#define       MAX_TPM_PROPERTIES          (MAX_CAP_DATA/sizeof(TPMS_TAGGED_PROPERTY))
#define       MAX_PCR_PROPERTIES          (MAX_CAP_DATA/sizeof(TPMS_TAGGED_PCR_SELECT))
#define       MAX_ECC_CURVES              (MAX_CAP_DATA/sizeof(TPM_ECC_CURVE))
#endif
