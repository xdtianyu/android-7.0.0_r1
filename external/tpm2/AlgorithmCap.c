// This file was extracted from the TCG Published
// Trusted Platform Module Library
// Part 4: Supporting Routines
// Family "2.0"
// Level 00 Revision 01.16
// October 30, 2014

#include "InternalRoutines.h"
typedef struct
{
   TPM_ALG_ID          algID;
   TPMA_ALGORITHM      attributes;
} ALGORITHM;
static const ALGORITHM    s_algorithms[]      =
{
#ifdef TPM_ALG_RSA
   {TPM_ALG_RSA,           {1, 0, 0, 1,       0, 0, 0, 0, 0}},
#endif
#ifdef TPM_ALG_DES
   {TPM_ALG_DES,           {0, 1, 0, 0,       0, 0, 0, 0, 0}},
#endif
#ifdef TPM_ALG_3DES
   {TPM_ALG__3DES,         {0, 1, 0, 0,       0, 0, 0, 0, 0}},
#endif
#ifdef TPM_ALG_SHA1
   {TPM_ALG_SHA1,          {0, 0, 1, 0,       0, 0, 0, 0, 0}},
#endif
#ifdef TPM_ALG_HMAC
   {TPM_ALG_HMAC,          {0, 0, 1, 0,       0, 1, 0, 0, 0}},
#endif
#ifdef TPM_ALG_AES
   {TPM_ALG_AES,           {0, 1, 0, 0,       0, 0, 0, 0, 0}},
#endif
#ifdef TPM_ALG_MGF1
   {TPM_ALG_MGF1,          {0, 0, 1, 0,       0, 0, 0, 1, 0}},
#endif
     {TPM_ALG_KEYEDHASH,         {0, 0, 1, 1, 0, 1, 1, 0, 0}},
#ifdef TPM_ALG_XOR
   {TPM_ALG_XOR,                 {0, 1, 1, 0, 0, 0, 0, 0, 0}},
#endif
#ifdef TPM_ALG_SHA256
   {TPM_ALG_SHA256,              {0, 0, 1, 0, 0, 0, 0, 0, 0}},
#endif
#ifdef TPM_ALG_SHA384
   {TPM_ALG_SHA384,              {0, 0, 1, 0, 0, 0, 0, 0, 0}},
#endif
#ifdef TPM_ALG_SHA512
   {TPM_ALG_SHA512,              {0, 0, 1, 0, 0, 0, 0, 0, 0}},
#endif
#ifdef TPM_ALG_WHIRLPOOL512
   {TPM_ALG_WHIRLPOOL512,        {0, 0, 1, 0, 0, 0, 0, 0, 0}},
#endif
#ifdef TPM_ALG_SM3_256
   {TPM_ALG_SM3_256,             {0, 0, 1, 0, 0, 0, 0, 0, 0}},
#endif
#ifdef TPM_ALG_SM4
   {TPM_ALG_SM4,          {0, 1, 0, 0, 0, 0, 0, 0, 0}},
#endif
#ifdef TPM_ALG_RSASSA
   {TPM_ALG_RSASSA,        {1, 0, 0, 0, 0, 1, 0, 0, 0}},
#endif
#ifdef TPM_ALG_RSAES
   {TPM_ALG_RSAES,         {1, 0, 0, 0, 0, 0, 1, 0, 0}},
#endif
#ifdef TPM_ALG_RSAPSS
   {TPM_ALG_RSAPSS,        {1, 0, 0, 0, 0, 1, 0, 0, 0}},
#endif
#ifdef TPM_ALG_OAEP
   {TPM_ALG_OAEP,          {1, 0, 0, 0, 0, 0, 1, 0, 0}},
#endif
#ifdef TPM_ALG_ECDSA
   {TPM_ALG_ECDSA,         {1, 0, 0, 0, 0, 1, 0, 1, 0}},
#endif
#ifdef TPM_ALG_ECDH
   {TPM_ALG_ECDH,          {1, 0, 0, 0, 0, 0, 0, 1, 0}},
#endif
#ifdef TPM_ALG_ECDAA
   {TPM_ALG_ECDAA,         {1, 0, 0, 0, 0, 1, 0, 0, 0}},
#endif
#ifdef TPM_ALG_ECSCHNORR
   {TPM_ALG_ECSCHNORR,     {1, 0, 0, 0, 0, 1, 0, 0, 0}},
#endif
#ifdef TPM_ALG_KDF1_SP800_56A
   {TPM_ALG_KDF1_SP800_56A,{0, 0, 1, 0, 0, 0, 0, 1, 0}},
#endif
#ifdef TPM_ALG_KDF2
   {TPM_ALG_KDF2,          {0, 0, 1, 0, 0, 0, 0, 1, 0}},
#endif
#ifdef TPM_ALG_KDF1_SP800_108
   {TPM_ALG_KDF1_SP800_108,{0, 0, 1, 0, 0, 0, 0, 1, 0}},
#endif
#ifdef TPM_ALG_ECC
   {TPM_ALG_ECC,           {1, 0, 0, 1, 0, 0, 0, 0, 0}},
#endif
   {TPM_ALG_SYMCIPHER,           {0, 0, 0, 1, 0, 0, 0, 0, 0}},
#ifdef TPM_ALG_CTR
   {TPM_ALG_CTR,                 {0, 1, 0, 0, 0, 0, 1, 0, 0}},
#endif
#ifdef TPM_ALG_OFB
   {TPM_ALG_OFB,                 {0, 1, 0, 0, 0, 0, 1, 0, 0}},
#endif
#ifdef TPM_ALG_CBC
   {TPM_ALG_CBC,                 {0, 1, 0, 0, 0, 0, 1, 0, 0}},
#endif
#ifdef TPM_ALG_CFB
   {TPM_ALG_CFB,                 {0, 1, 0, 0, 0, 0, 1, 0, 0}},
#endif
#ifdef TPM_ALG_ECB
   {TPM_ALG_ECB,                 {0, 1, 0, 0, 0, 0, 1, 0, 0}},
#endif
};
//
//
//          AlgorithmCapGetImplemented()
//
//      This function is used by TPM2_GetCapability() to return a list of the implemented algorithms.
//
//
//
//
//      Return Value                      Meaning
//
//      YES                               more algorithms to report
//      NO                                no more algorithms to report
//
TPMI_YES_NO
AlgorithmCapGetImplemented(
     TPM_ALG_ID                          algID,         // IN: the starting algorithm ID
     UINT32                              count,         // IN: count of returned algorithms
     TPML_ALG_PROPERTY                  *algList        // OUT: algorithm list
)
{
     TPMI_YES_NO      more = NO;
     UINT32           i;
     UINT32           algNum;
     // initialize output algorithm list
     algList->count = 0;
     // The maximum count of algorithms we may return is MAX_CAP_ALGS.
     if(count > MAX_CAP_ALGS)
         count = MAX_CAP_ALGS;
     // Compute how many algorithms are defined in s_algorithms array.
     algNum = sizeof(s_algorithms) / sizeof(s_algorithms[0]);
     // Scan the implemented algorithm list to see if there is a match to 'algID'.
     for(i = 0; i < algNum; i++)
     {
         // If algID is less than the starting algorithm ID, skip it
         if(s_algorithms[i].algID < algID)
              continue;
         if(algList->count < count)
         {
              // If we have not filled up the return list, add more algorithms
              // to it
              algList->algProperties[algList->count].alg = s_algorithms[i].algID;
              algList->algProperties[algList->count].algProperties =
                  s_algorithms[i].attributes;
              algList->count++;
         }
         else
         {
              // If the return list is full but we still have algorithms
              // available, report this and stop scanning.
              more = YES;
              break;
         }
     }
     return more;
}
LIB_EXPORT
void
AlgorithmGetImplementedVector(
     ALGORITHM_VECTOR      *implemented            // OUT: the implemented bits are SET
     )
{
     int                            index;
     // Nothing implemented until we say it is
     MemorySet(implemented, 0, sizeof(ALGORITHM_VECTOR));
     for(index = (sizeof(s_algorithms) / sizeof(s_algorithms[0])) - 1;
         index >= 0;
         index--)
             SET_BIT(s_algorithms[index].algID, *implemented);
     return;
}
