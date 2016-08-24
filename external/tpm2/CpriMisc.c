// This file was extracted from the TCG Published
// Trusted Platform Module Library
// Part 4: Supporting Routines
// Family "2.0"
// Level 00 Revision 01.16
// October 30, 2014

#include "OsslCryptoEngine.h"
//
//
//      Functions
//
//      BnTo2B()
//
//     This function is used to convert a BigNum() to a byte array of the specified size. If the number is too large
//     to fit, then 0 is returned. Otherwise, the number is converted into the low-order bytes of the provided array
//     and the upper bytes are set to zero.
//
//     Return Value                     Meaning
//
//     0                                failure (probably fatal)
//     1                                conversion successful
//
BOOL
BnTo2B(
    TPM2B               *outVal,             // OUT: place for the result
    BIGNUM              *inVal,              // IN: number to convert
    UINT16               size                // IN: size of the output.
    )
{
    BYTE      *pb = outVal->buffer;
    outVal->size = size;
    size = size - (((UINT16) BN_num_bits(inVal) + 7) / 8);
    if(size < 0)
        return FALSE;
    for(;size > 0; size--)
        *pb++ = 0;
    BN_bn2bin(inVal, pb);
    return TRUE;
}
//
//
//      Copy2B()
//
//     This function copies a TPM2B structure. The compiler can't generate a copy of a TPM2B generic
//     structure because the actual size is not known. This function performs the copy on any TPM2B pair. The
//     size of the destination should have been checked before this call to make sure that it will hold the TPM2B
//     being copied.
//     This replicates the functionality in the MemoryLib.c.
//
void
Copy2B(
    TPM2B               *out,                // OUT: The TPM2B to receive the copy
    TPM2B               *in                  // IN: the TPM2B to copy
    )
{
    BYTE        *pIn = in->buffer;
    BYTE        *pOut = out->buffer;
    int          count;
    out->size = in->size;
    for(count = in->size; count > 0; count--)
       *pOut++ = *pIn++;
   return;
}
//
//
//      BnFrom2B()
//
//     This function creates a BIGNUM from a TPM2B and fails if the conversion fails.
//
BIGNUM *
BnFrom2B(
   BIGNUM              *out,              // OUT: The BIGNUM
   const TPM2B         *in                // IN: the TPM2B to copy
   )
{
   if(BN_bin2bn(in->buffer, in->size, out) == NULL)
       FAIL(FATAL_ERROR_INTERNAL);
   return out;
}
