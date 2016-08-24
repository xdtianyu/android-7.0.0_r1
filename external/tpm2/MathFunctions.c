// This file was extracted from the TCG Published
// Trusted Platform Module Library
// Part 4: Supporting Routines
// Family "2.0"
// Level 00 Revision 01.16
// October 30, 2014

#include <string.h>

#include "OsslCryptoEngine.h"
//
//
//          Externally Accessible Functions
//
//           _math__Normalize2B()
//
//     This function will normalize the value in a TPM2B. If there are leading bytes of zero, the first non-zero
//     byte is shifted up.
//
//     Return Value                     Meaning
//
//     0                                no significant bytes, value is zero
//     >0                               number of significant bytes
//
LIB_EXPORT UINT16
_math__Normalize2B(
     TPM2B               *b                  // IN/OUT: number to normalize
     )
{
     UINT16        from;
     UINT16        to;
     UINT16        size = b->size;
     for(from = 0; b->buffer[from] == 0 && from < size; from++);
     b->size -= from;
     for(to = 0; from < size; to++, from++ )
         b->buffer[to] = b->buffer[from];
     return b->size;
}
//
//
//
//        _math__Denormalize2B()
//
//     This function is used to adjust a TPM2B so that the number has the desired number of bytes. This is
//     accomplished by adding bytes of zero at the start of the number.
//
//     Return Value                      Meaning
//
//     TRUE                              number de-normalized
//     FALSE                             number already larger than the desired size
//
LIB_EXPORT BOOL
_math__Denormalize2B(
    TPM2B              *in,                   // IN:OUT TPM2B number to de-normalize
    UINT32              size                  // IN: the desired size
    )
{
    UINT32       to;
    UINT32       from;
    // If the current size is greater than the requested size, see if this can be
    // normalized to a value smaller than the requested size and then de-normalize
    if(in->size > size)
    {
        _math__Normalize2B(in);
        if(in->size > size)
            return FALSE;
    }
    // If the size is already what is requested, leave
    if(in->size == size)
        return TRUE;
    // move the bytes to the 'right'
    for(from = in->size, to = size; from > 0;)
        in->buffer[--to] = in->buffer[--from];
    // 'to' will always be greater than 0 because we checked for equal above.
    for(; to > 0;)
        in->buffer[--to] = 0;
    in->size = (UINT16)size;
    return TRUE;
}
//
//
//        _math__sub()
//
//     This function to subtract one unsigned value from another c = a - b. c may be the same as a or b.
//
//     Return Value                      Meaning
//
//     1                                 if (a > b) so no borrow
//     0                                 if (a = b) so no borrow and b == a
//     -1                                if (a < b) so there was a borrow
//
LIB_EXPORT int
_math__sub(
    const UINT32        aSize,                //   IN: size   of a
    const BYTE         *a,                    //   IN: a
    const UINT32        bSize,                //   IN: size   of b
    const BYTE         *b,                    //   IN: b
    UINT16             *cSize,                //   OUT: set   to MAX(aSize, bSize)
    BYTE               *c                     //   OUT: the   difference
    )
{
    int               borrow = 0;
    int               notZero = 0;
    int               i;
    int               i2;
    // set c to the longer of a or b
    *cSize = (UINT16)((aSize > bSize) ? aSize : bSize);
    // pick the shorter of a and b
    i = (aSize > bSize) ? bSize : aSize;
    i2 = *cSize - i;
    a = &a[aSize - 1];
    b = &b[bSize - 1];
    c = &c[*cSize - 1];
    for(; i > 0; i--)
    {
        borrow = *a-- - *b-- + borrow;
        *c-- = (BYTE)borrow;
        notZero = notZero || borrow;
        borrow >>= 8;
    }
    if(aSize > bSize)
    {
        for(;i2 > 0; i2--)
        {
            borrow = *a-- + borrow;
            *c-- = (BYTE)borrow;
            notZero = notZero || borrow;
            borrow >>= 8;
        }
    }
    else if(aSize < bSize)
    {
        for(;i2 > 0; i2--)
        {
            borrow = 0 - *b-- + borrow;
            *c-- = (BYTE)borrow;
            notZero = notZero || borrow;
            borrow >>= 8;
        }
    }
    // if there is a borrow, then b > a
    if(borrow)
        return -1;
    // either a > b or they are the same
    return notZero;
}
//
//
//         _math__Inc()
//
//      This function increments a large, big-endian number value by one.
//
//      Return Value                   Meaning
//
//      0                              result is zero
//      !0                             result is not zero
//
LIB_EXPORT int
_math__Inc(
    UINT32             aSize,              // IN: size of a
    BYTE              *a                   // IN: a
    )
{
//
      for(a = &a[aSize-1];aSize > 0; aSize--)
      {
          if((*a-- += 1) != 0)
              return 1;
      }
      return 0;
}
//
//
//          _math__Dec()
//
//      This function decrements a large, ENDIAN value by one.
//
LIB_EXPORT void
_math__Dec(
      UINT32            aSize,                // IN: size of a
      BYTE             *a                     // IN: a
      )
{
      for(a = &a[aSize-1]; aSize > 0; aSize--)
      {
          if((*a-- -= 1) != 0xff)
              return;
      }
      return;
}
//
//
//          _math__Mul()
//
//      This function is used to multiply two large integers: p = a* b. If the size of p is not specified (pSize ==
//      NULL), the size of the results p is assumed to be aSize + bSize and the results are de-normalized so that
//      the resulting size is exactly aSize + bSize. If pSize is provided, then the actual size of the result is
//      returned. The initial value for pSize must be at least aSize + pSize.
//
//      Return Value                      Meaning
//
//      <0                                indicates an error
//      >= 0                              the size of the product
//
LIB_EXPORT int
_math__Mul(
      const UINT32      aSize,                //   IN: size of a
      const BYTE       *a,                    //   IN: a
      const UINT32      bSize,                //   IN: size of b
      const BYTE       *b,                    //   IN: b
      UINT32           *pSize,                //   IN/OUT: size of the product
      BYTE             *p                     //   OUT: product. length of product = aSize +
                                              //       bSize
      )
{
      BIGNUM           *bnA;
      BIGNUM           *bnB;
      BIGNUM           *bnP;
      BN_CTX           *context;
      int              retVal = 0;
      // First check that pSize is large enough if present
      if((pSize != NULL) && (*pSize < (aSize + bSize)))
          return CRYPT_PARAMETER;
      pAssert(pSize == NULL || *pSize <= MAX_2B_BYTES);
      //
//
    // Allocate space for BIGNUM context
    //
    context = BN_CTX_new();
    if(context == NULL)
        FAIL(FATAL_ERROR_ALLOCATION);
    bnA = BN_CTX_get(context);
    bnB = BN_CTX_get(context);
    bnP = BN_CTX_get(context);
    if (bnP == NULL)
        FAIL(FATAL_ERROR_ALLOCATION);
    // Convert the inputs to BIGNUMs
    //
    if (BN_bin2bn(a, aSize, bnA) == NULL || BN_bin2bn(b, bSize, bnB) == NULL)
        FAIL(FATAL_ERROR_INTERNAL);
    // Perform the multiplication
    //
    if (BN_mul(bnP, bnA, bnB, context) != 1)
        FAIL(FATAL_ERROR_INTERNAL);
    // If the size of the results is allowed to float, then set the return
    // size. Otherwise, it might be necessary to de-normalize the results
    retVal = BN_num_bytes(bnP);
    if(pSize == NULL)
    {
        BN_bn2bin(bnP, &p[aSize + bSize - retVal]);
        memset(p, 0, aSize + bSize - retVal);
        retVal = aSize + bSize;
    }
    else
    {
        BN_bn2bin(bnP, p);
        *pSize = retVal;
    }
    BN_CTX_end(context);
    BN_CTX_free(context);
    return retVal;
}
//
//
//         _math__Div()
//
//      Divide an integer (n) by an integer (d) producing a quotient (q) and a remainder (r). If q or r is not needed,
//      then the pointer to them may be set to NULL.
//
//      Return Value                     Meaning
//
//      CRYPT_SUCCESS                    operation complete
//      CRYPT_UNDERFLOW                  q or r is too small to receive the result
//
LIB_EXPORT CRYPT_RESULT
_math__Div(
    const TPM2B         *n,                  //   IN: numerator
    const TPM2B         *d,                  //   IN: denominator
    TPM2B               *q,                  //   OUT: quotient
    TPM2B               *r                   //   OUT: remainder
    )
{
    BIGNUM              *bnN;
    BIGNUM              *bnD;
    BIGNUM              *bnQ;
    BIGNUM              *bnR;
    BN_CTX            *context;
    CRYPT_RESULT       retVal = CRYPT_SUCCESS;
    // Get structures for the big number representations
    context = BN_CTX_new();
    if(context == NULL)
        FAIL(FATAL_ERROR_ALLOCATION);
    BN_CTX_start(context);
    bnN = BN_CTX_get(context);
    bnD = BN_CTX_get(context);
    bnQ = BN_CTX_get(context);
    bnR = BN_CTX_get(context);
    // Errors in BN_CTX_get() are sticky so only need to check the last allocation
    if (    bnR == NULL
         || BN_bin2bn(n->buffer, n->size, bnN) == NULL
         || BN_bin2bn(d->buffer, d->size, bnD) == NULL)
             FAIL(FATAL_ERROR_INTERNAL);
    // Check for divide by zero.
    if(BN_num_bits(bnD) == 0)
        FAIL(FATAL_ERROR_DIVIDE_ZERO);
    // Perform the division
    if (BN_div(bnQ, bnR, bnN, bnD, context) != 1)
        FAIL(FATAL_ERROR_INTERNAL);
    // Convert the BIGNUM result back to our format
    if(q != NULL)   // If the quotient is being returned
    {
        if(!BnTo2B(q, bnQ, q->size))
        {
            retVal = CRYPT_UNDERFLOW;
            goto Done;
        }
      }
    if(r != NULL)   // If the remainder is being returned
    {
        if(!BnTo2B(r, bnR, r->size))
            retVal = CRYPT_UNDERFLOW;
    }
Done:
   BN_CTX_end(context);
   BN_CTX_free(context);
    return retVal;
}
//
//
//         _math__uComp()
//
//      This function compare two unsigned values.
//
//      Return Value                      Meaning
//
//      1                                 if (a > b)
//      0                                 if (a = b)
//      -1                                if (a < b)
//
LIB_EXPORT int
_math__uComp(
    const UINT32       aSize,                 // IN: size of a
    const BYTE        *a,                     // IN: a
    const UINT32       bSize,                // IN: size of b
    const BYTE        *b                     // IN: b
    )
{
    int              borrow = 0;
    int              notZero = 0;
    int              i;
    // If a has more digits than b, then a is greater than b if
    // any of the more significant bytes is non zero
    if((i = (int)aSize - (int)bSize) > 0)
        for(; i > 0; i--)
            if(*a++) // means a > b
                 return 1;
    // If b has more digits than a, then b is greater if any of the
    // more significant bytes is non zero
    if(i < 0) // Means that b is longer than a
        for(; i < 0; i++)
            if(*b++) // means that b > a
                 return -1;
    // Either the vales are the same size or the upper bytes of a or b are
    // all zero, so compare the rest
    i = (aSize > bSize) ? bSize : aSize;
    a = &a[i-1];
    b = &b[i-1];
    for(; i > 0; i--)
    {
        borrow = *a-- - *b-- + borrow;
        notZero = notZero || borrow;
        borrow >>= 8;
    }
    // if there is a borrow, then b > a
    if(borrow)
        return -1;
    // either a > b or they are the same
    return notZero;
}
//
//
//           _math__Comp()
//
//      Compare two signed integers:
//
//      Return Value                    Meaning
//
//      1                               if a > b
//      0                               if a = b
//      -1                              if a < b
//
LIB_EXPORT int
_math__Comp(
    const   UINT32     aSize,                //   IN:   size of a
    const   BYTE      *a,                    //   IN:   a buffer
    const   UINT32     bSize,                //   IN:   size of b
    const   BYTE      *b                     //   IN:   b buffer
    )
{
    int        signA, signB;              // sign of a and b
    // For positive or 0, sign_a is 1
    // for negative, sign_a is 0
    signA = ((a[0] & 0x80) == 0) ? 1 : 0;
    // For positive or 0, sign_b is 1
    // for negative, sign_b is 0
   signB = ((b[0] & 0x80) == 0) ? 1 : 0;
   if(signA != signB)
   {
       return signA - signB;
   }
   if(signA == 1)
       // do unsigned compare function
       return _math__uComp(aSize, a, bSize, b);
   else
       // do unsigned compare the other way
       return 0 - _math__uComp(aSize, a, bSize, b);
}
//
//
//       _math__ModExp
//
//      This function is used to do modular exponentiation in support of RSA. The most typical uses are: c = m^e
//      mod n (RSA encrypt) and m = c^d mod n (RSA decrypt). When doing decryption, the e parameter of the
//      function will contain the private exponent d instead of the public exponent e.
//      If the results will not fit in the provided buffer, an error is returned (CRYPT_ERROR_UNDERFLOW). If
//      the results is smaller than the buffer, the results is de-normalized.
//      This version is intended for use with RSA and requires that m be less than n.
//
//      Return Value                      Meaning
//
//      CRYPT_SUCCESS                     exponentiation succeeded
//      CRYPT_PARAMETER                   number to exponentiate is larger than the modulus
//      CRYPT_UNDERFLOW                   result will not fit into the provided buffer
//
LIB_EXPORT CRYPT_RESULT
_math__ModExp(
   UINT32               cSize,                 //   IN: size of the result
   BYTE                *c,                     //   OUT: results buffer
   const UINT32         mSize,                 //   IN: size of number to be exponentiated
   const BYTE          *m,                     //   IN: number to be exponentiated
   const UINT32         eSize,                 //   IN: size of power
   const BYTE          *e,                     //   IN: power
   const UINT32         nSize,                 //   IN: modulus size
   const BYTE          *n                      //   IN: modulu
   )
{
   CRYPT_RESULT         retVal = CRYPT_SUCCESS;
   BN_CTX              *context;
   BIGNUM              *bnC;
   BIGNUM              *bnM;
   BIGNUM              *bnE;
   BIGNUM              *bnN;
   INT32                i;
   context = BN_CTX_new();
   if(context == NULL)
       FAIL(FATAL_ERROR_ALLOCATION);
   BN_CTX_start(context);
   bnC = BN_CTX_get(context);
   bnM = BN_CTX_get(context);
   bnE = BN_CTX_get(context);
   bnN = BN_CTX_get(context);
   // Errors for BN_CTX_get are sticky so only need to check last allocation
   if(bnN == NULL)
         FAIL(FATAL_ERROR_ALLOCATION);
    //convert arguments
    if (    BN_bin2bn(m, mSize, bnM) == NULL
         || BN_bin2bn(e, eSize, bnE) == NULL
         || BN_bin2bn(n, nSize, bnN) == NULL)
             FAIL(FATAL_ERROR_INTERNAL);
    // Don't do exponentiation if the number being exponentiated is
    // larger than the modulus.
    if(BN_ucmp(bnM, bnN) >= 0)
    {
        retVal = CRYPT_PARAMETER;
        goto Cleanup;
    }
    // Perform the exponentiation
    if(!(BN_mod_exp(bnC, bnM, bnE, bnN, context)))
        FAIL(FATAL_ERROR_INTERNAL);
    // Convert the results
    // Make sure that the results will fit in the provided buffer.
    if((unsigned)BN_num_bytes(bnC) > cSize)
    {
        retVal = CRYPT_UNDERFLOW;
        goto Cleanup;
    }
    i = cSize - BN_num_bytes(bnC);
    BN_bn2bin(bnC, &c[i]);
    memset(c, 0, i);
Cleanup:
   // Free up allocated BN values
   BN_CTX_end(context);
   BN_CTX_free(context);
   return retVal;
}
//
//
//       _math__IsPrime()
//
//      Check if an 32-bit integer is a prime.
//
//      Return Value                      Meaning
//
//      TRUE                              if the integer is probably a prime
//      FALSE                             if the integer is definitely not a prime
//
LIB_EXPORT BOOL
_math__IsPrime(
    const UINT32         prime
    )
{
    int       isPrime;
    BIGNUM    *p;
    // Assume the size variables are not overflow, which should not happen in
    // the contexts that this function will be called.
    if((p = BN_new()) == NULL)
        FAIL(FATAL_ERROR_ALLOCATION);
    if(!BN_set_word(p, prime))
        FAIL(FATAL_ERROR_INTERNAL);
    //
    // BN_is_prime returning -1 means that it ran into an error.
//
   // It should only return 0 or 1
   //
   if((isPrime = BN_is_prime_ex(p, BN_prime_checks, NULL, NULL)) < 0)
       FAIL(FATAL_ERROR_INTERNAL);
   if(p != NULL)
       BN_clear_free(p);
   return (isPrime == 1);
}
