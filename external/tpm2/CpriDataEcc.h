// This file was extracted from the TCG Published
// Trusted Platform Module Library
// Part 4: Supporting Routines
// Family "2.0"
// Level 00 Revision 01.16
// October 30, 2014

#ifndef        _CRYPTDATAECC_H_
#define        _CRYPTDATAECC_H_
//
//     Structure for the curve parameters. This is an analog to the TPMS_ALGORITHM_DETAIL_ECC
//
typedef struct {
   const TPM2B     *p;         // a prime number
   const TPM2B     *a;         // linear coefficient
   const TPM2B     *b;         // constant term
   const TPM2B     *x;         // generator x coordinate
   const TPM2B     *y;         // generator y coordinate
   const TPM2B     *n;         // the order of the curve
   const TPM2B     *h;         // cofactor
} ECC_CURVE_DATA;
typedef struct
{
   TPM_ECC_CURVE            curveId;
   UINT16                   keySizeBits;
   TPMT_KDF_SCHEME          kdf;
   TPMT_ECC_SCHEME          sign;
   const ECC_CURVE_DATA    *curveData; // the address of the curve data
} ECC_CURVE;
extern const ECC_CURVE_DATA SM2_P256;
extern const ECC_CURVE_DATA NIST_P256;
extern const ECC_CURVE_DATA BN_P256;
extern const ECC_CURVE eccCurves[];
extern const UINT16 ECC_CURVE_COUNT;
#endif
