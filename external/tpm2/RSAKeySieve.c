// This file was extracted from the TCG Published
// Trusted Platform Module Library
// Part 4: Supporting Routines
// Family "2.0"
// Level 00 Revision 01.16
// October 30, 2014

#include       "OsslCryptoEngine.h"
#ifdef       TPM_ALG_RSA
//
//     This file produces no code unless the compile switch is set to cause it to generate code.
//
#ifdef          RSA_KEY_SIEVE                          //%
#include        "RsaKeySieve.h"
//
//     This next line will show up in the header file for this code. It will make the local functions public when
//     debugging.
//
//%#ifdef       RSA_DEBUG
//
//
//      Bit Manipulation Functions
//
//          Introduction
//
//     These functions operate on a bit array. A bit array is an array of bytes with the 0th byte being the byte
//     with the lowest memory address. Within the byte, bit 0 is the least significant bit.
//
//          ClearBit()
//
//     This function will CLEAR a bit in a bit array.
//
void
ClearBit(
    unsigned char         *a,                     // IN: A pointer to an array of byte
    int                    i                      // IN: the number of the bit to CLEAR
    )
{
    a[i >> 3] &= 0xff ^ (1 << (i & 7));
}
//
//
//          SetBit()
//
//     Function to SET a bit in a bit array.
//
void
SetBit(
    unsigned char         *a,                     // IN: A pointer to an array of byte
    int                    i                      // IN: the number of the bit to SET
    )
{
    a[i >> 3] |= (1 << (i & 7));
}
//
//
//          IsBitSet()
//
//     Function to test if a bit in a bit array is SET.
//
//
//
//
//     Return Value                      Meaning
//
//     0                                 bit is CLEAR
//     1                                 bit is SET
//
UINT32
IsBitSet(
    unsigned char       *a,                   // IN: A pointer to an array of byte
    int                  i                    // IN: the number of the bit to test
    )
{
    return ((a[i >> 3] & (1 << (i & 7))) != 0);
}
//
//
//        BitsInArry()
//
//     This function counts the number of bits set in an array of bytes.
//
int
BitsInArray(
    unsigned char       *a,                   // IN: A pointer to an array of byte
    int                  i                    // IN: the number of bytes to sum
    )
{
    int     j = 0;
    for(; i ; i--)
        j += bitsInByte[*a++];
    return j;
}
//
//
//        FindNthSetBit()
//
//     This function finds the nth SET bit in a bit array. The caller should check that the offset of the returned
//     value is not out of range. If called when the array does not have n bits set, it will return a fatal error
//
UINT32
FindNthSetBit(
    const UINT16         aSize,               // IN: the size of the array to check
    const BYTE          *a,                   // IN: the array to check
    const UINT32         n                    // IN, the number of the SET bit
    )
{
    UINT32          i;
    const BYTE     *pA = a;
    UINT32          retValue;
    BYTE            sel;
    (aSize);
    //find the bit
    for(i = 0; i < n; i += bitsInByte[*pA++]);
    // The chosen bit is in the byte that was just accessed
    // Compute the offset to the start of that byte
    pA--;
    retValue = (UINT32)(pA - a) * 8;
    // Subtract the bits in the last byte added.
    i -= bitsInByte[*pA];
    // Now process the byte, one bit at a time.
    for(sel = *pA; sel != 0 ; sel = sel >> 1)
    {
        if(sel & 1)
        {
            i += 1;
            if(i == n)
                return retValue;
        }
        retValue += 1;
    }
    FAIL(FATAL_ERROR_INTERNAL);
}
//
//
//       Miscellaneous Functions
//
//          RandomForRsa()
//
//      This function uses a special form of KDFa() to produces a pseudo random sequence. It's input is a
//      structure that contains pointers to a pre-computed set of hash contexts that are set up for the HMAC
//      computations using the seed.
//      This function will test that ktx.outer will not wrap to zero if incremented. If so, the function returns FALSE.
//      Otherwise, the ktx.outer is incremented before each number is generated.
//
void
RandomForRsa(
    KDFa_CONTEXT        *ktx,                // IN: a context for the KDF
    const char          *label,              // IN: a use qualifying label
    TPM2B               *p                   // OUT: the pseudo random result
    )
{
    INT16                           i;
    UINT32                          inner;
    BYTE                            swapped[4];
    UINT16                          fill;
    BYTE                            *pb;
    UINT16                          lLen = 0;
    UINT16                          digestSize = _cpri__GetDigestSize(ktx->hashAlg);
    CPRI_HASH_STATE                 h;      // the working hash context
    if(label != NULL)
        for(lLen = 0; label[lLen++];);
    fill = digestSize;
    pb = p->buffer;
    inner = 0;
    *(ktx->outer) += 1;
    for(i = p->size; i > 0; i -= digestSize)
    {
        inner++;
         // Initialize the HMAC with saved state
         _cpri__CopyHashState(&h, &(ktx->iPadCtx));
         // Hash the inner counter (the one that changes on each HMAC iteration)
         UINT32_TO_BYTE_ARRAY(inner, swapped);
         _cpri__UpdateHash(&h, 4, swapped);
         if(lLen != 0)
             _cpri__UpdateHash(&h, lLen, (BYTE *)label);
         // Is there any party 1 data
         if(ktx->extra != NULL)
             _cpri__UpdateHash(&h, ktx->extra->size, ktx->extra->buffer);
        // Include the outer counter (the one that changes on each prime
        // prime candidate generation
        UINT32_TO_BYTE_ARRAY(*(ktx->outer), swapped);
        _cpri__UpdateHash(&h, 4, swapped);
        _cpri__UpdateHash(&h, 2, (BYTE *)&ktx->keySizeInBits);
        if(i < fill)
            fill = i;
        _cpri__CompleteHash(&h, fill, pb);
        // Restart the oPad hash
        _cpri__CopyHashState(&h, &(ktx->oPadCtx));
        // Add the last hashed data
        _cpri__UpdateHash(&h, fill, pb);
        // gives a completed HMAC
        _cpri__CompleteHash(&h, fill, pb);
        pb += fill;
   }
   return;
}
//
//
//         MillerRabinRounds()
//
//      Function returns the number of Miller-Rabin rounds necessary to give an error probability equal to the
//      security strength of the prime. These values are from FIPS 186-3.
//
UINT32
MillerRabinRounds(
   UINT32               bits                 // IN: Number of bits in the RSA prime
   )
{
   if(bits < 511) return 8;            // don't really expect this
   if(bits < 1536) return 5;           // for 512 and 1K primes
   return 4;                           // for 3K public modulus and greater
}
//
//
//         MillerRabin()
//
//      This function performs a Miller-Rabin test from FIPS 186-3. It does iterations trials on the number. I all
//      likelihood, if the number is not prime, the first test fails.
//      If a KDFa(), PRNG context is provide (ktx), then it is used to provide the random values. Otherwise, the
//      random numbers are retrieved from the random number generator.
//
//      Return Value                      Meaning
//
//      TRUE                              probably prime
//      FALSE                             composite
//
BOOL
MillerRabin(
   BIGNUM              *bnW,
   int                  iterations,
   KDFa_CONTEXT        *ktx,
   BN_CTX              *context
   )
{
   BIGNUM         *bnWm1;
   BIGNUM         *bnM;
   BIGNUM         *bnB;
   BIGNUM         *bnZ;
   BOOL         ret = FALSE;   // Assumed composite for easy exit
   TPM2B_TYPE(MAX_PRIME, MAX_RSA_KEY_BYTES/2);
   TPM2B_MAX_PRIME    b;
   int          a;
   int          j;
   int          wLen;
   int          i;
   pAssert(BN_is_bit_set(bnW, 0));
   INSTRUMENT_INC(MillerRabinTrials);    // Instrumentation
   BN_CTX_start(context);
   bnWm1 = BN_CTX_get(context);
   bnB = BN_CTX_get(context);
   bnZ = BN_CTX_get(context);
   bnM = BN_CTX_get(context);
   if(bnM == NULL)
       FAIL(FATAL_ERROR_ALLOCATION);
// Let a be the largest integer such that 2^a divides w1.
   BN_copy(bnWm1, bnW);
   BN_sub_word(bnWm1, 1);
   // Since w is odd (w-1) is even so start at bit number 1 rather than 0
   for(a = 1; !BN_is_bit_set(bnWm1, a); a++);
// 2. m = (w1) / 2^a
   BN_rshift(bnM, bnWm1, a);
// 3. wlen = len (w).
   wLen = BN_num_bits(bnW);
   pAssert((wLen & 7) == 0);
   // Set the size for the random number
   b.b.size = (UINT16)(wLen + 7)/8;
// 4. For i = 1 to iterations do
   for(i = 0; i < iterations ; i++)
   {
//  Obtain a string b of wlen bits from an RBG.
step4point1:
       // In the reference implementation, wLen is always a multiple of 8
       if(ktx != NULL)
            RandomForRsa(ktx, "Miller-Rabin witness", &b.b);
       else
            _cpri__GenerateRandom(b.t.size, b.t.buffer);
        if(BN_bin2bn(b.t.buffer, b.t.size, bnB) == NULL)
            FAIL(FATAL_ERROR_ALLOCATION);
//  If ((b 1) or (b w1)), then go to step 4.1.
       if(BN_is_zero(bnB))
           goto step4point1;
       if(BN_is_one(bnB))
           goto step4point1;
       if(BN_ucmp(bnB, bnWm1) >= 0)
           goto step4point1;
//  z = b^m mod w.
       if(BN_mod_exp(bnZ, bnB, bnM, bnW, context) != 1)
           FAIL(FATAL_ERROR_ALLOCATION);
//  If ((z = 1) or (z = w 1)), then go to step 4.7.
       if(BN_is_one(bnZ) || BN_ucmp(bnZ, bnWm1) == 0)
           goto step4point7;
//  For j = 1 to a 1 do.
       for(j = 1; j < a; j++)
       {
//  z = z^2 mod w.
           if(BN_mod_mul(bnZ, bnZ, bnZ, bnW, context) != 1)
               FAIL(FATAL_ERROR_ALLOCATION);
//  If (z = w1), then go to step 4.7.
           if(BN_ucmp(bnZ, bnWm1) == 0)
               goto step4point7;
//  If (z = 1), then go to step 4.6.
            if(BN_is_one(bnZ))
                goto step4point6;
       }
//  Return COMPOSITE.
step4point6:
       if(i > 9)
            INSTRUMENT_INC(failedAtIteration[9]);
       else
            INSTRUMENT_INC(failedAtIteration[i]);
       goto end;
//  Continue. Comment: Increment i for the do-loop in step 4.
step4point7:
       continue;
   }
// 5. Return PROBABLY PRIME
   ret = TRUE;
end:
   BN_CTX_end(context);
   return ret;
}
//
//
//         NextPrime()
//
//      This function is used to access the next prime number in the sequence of primes. It requires a pre-
//      initialized iterator.
//
UINT32
NextPrime(
   PRIME_ITERATOR      *iter
   )
{
   if(iter->index >= iter->final)
       return (iter->lastPrime = 0);
   return (iter->lastPrime += primeDiffTable[iter->index++]);
}
//
//
//         AdjustNumberOfPrimes()
//
//      Modifies the input parameter to be a valid value for the number of primes. The adjusted value is either the
//      input value rounded up to the next 512 bytes boundary or the maximum value of the implementation. If
//      the input is 0, the return is set to the maximum.
//
UINT32
AdjustNumberOfPrimes(
   UINT32               p
   )
{
   p = ((p + 511) / 512) * 512;
//
      if(p == 0 || p > PRIME_DIFF_TABLE_BYTES)
          p = PRIME_DIFF_TABLE_BYTES;
      return p;
}
//
//
//          PrimeInit()
//
//      This function is used to initialize the prime sequence generator iterator. The iterator is initialized and
//      returns the first prime that is equal to the requested starting value. If the starting value is no a prime, then
//      the iterator is initialized to the next higher prime number.
//
UINT32
PrimeInit(
      UINT32             first,              // IN: the initial prime
      PRIME_ITERATOR    *iter,               // IN/OUT: the iterator structure
      UINT32             primes              // IN: the table length
      )
{
      iter->lastPrime = 1;
      iter->index = 0;
      iter->final = AdjustNumberOfPrimes(primes);
      while(iter->lastPrime < first)
          NextPrime(iter);
      return iter->lastPrime;
}
//
//
//          SetDefaultNumberOfPrimes()
//
//      This macro sets the default number of primes to the indicated value.
//
//%#define SetDefaultNumberOfPrimes(p) (primeTableBytes = AdjustNumberOfPrimes(p))
//
//
//          IsPrimeWord()
//
//      Checks to see if a UINT32 is prime
//
//      Return Value                      Meaning
//
//      TRUE                              number is prime
//      FAIL                              number is not prime
//
BOOL
IsPrimeWord(
      UINT32              p                  // IN: number to test
      )
{
#if defined RSA_KEY_SIEVE && (PRIME_DIFF_TABLE_BYTES >= 6542)
      UINT32       test;
      UINT32       index;
      UINT32       stop;
      if((p & 1) == 0)
          return FALSE;
      if(p == 1 || p == 3)
          return TRUE;
      // Get a high value for the stopping point
      for(index = p, stop = 0; index; index >>= 2)
        stop = (stop << 1) + 1;
    stop++;
    // If the full prime difference value table is present, can check here
    test = 3;
    for(index = 1; index < PRIME_DIFF_TABLE_BYTES; index += 1)
    {
        if((p % test) == 0)
            return (p == test);
        if(test > stop)
            return TRUE;
        test += primeDiffTable[index];
    }
    return TRUE;
#else
   BYTE        b[4];
   if(p == RSA_DEFAULT_PUBLIC_EXPONENT || p == 1 || p == 3 )
       return TRUE;
   if((p & 1) == 0)
       return FALSE;
   UINT32_TO_BYTE_ARRAY(p,b);
   return _math__IsPrime(p);
#endif
}
typedef struct {
   UINT16      prime;
   UINT16      count;
} SIEVE_MARKS;
const SIEVE_MARKS sieveMarks[5] = {
   {31, 7}, {73, 5}, {241, 4}, {1621, 3}, {UINT16_MAX, 2}};
//
//
//          PrimeSieve()
//
//      This function does a prime sieve over the input field which has as its starting address the value in bnN.
//      Since this initializes the Sieve using a pre-computed field with the bits associated with 3, 5 and 7 already
//      turned off, the value of pnN may need to be adjusted by a few counts to allow the pre-computed field to
//      be used without modification. The fieldSize parameter must be 2^N + 1 and is probably not useful if it is
//      less than 129 bytes (1024 bits).
//
UINT32
PrimeSieve(
    BIGNUM        *bnN,            //   IN/OUT: number to sieve
    UINT32         fieldSize,      //   IN: size of the field area in bytes
    BYTE          *field,          //   IN: field
    UINT32         primes          //   IN: the number of primes to use
    )
{
    UINT32              i;
    UINT32              j;
    UINT32              fieldBits = fieldSize * 8;
    UINT32              r;
    const BYTE         *p1;
    BYTE               *p2;
    PRIME_ITERATOR      iter;
    UINT32              adjust;
    UINT32              mark = 0;
    UINT32              count = sieveMarks[0].count;
    UINT32              stop = sieveMarks[0].prime;
    UINT32              composite;
//      UINT64              test;           //DEBUG
   pAssert(field != NULL && bnN != NULL);
   // Need to have a field that has a size of 2^n + 1 bytes
   pAssert(BitsInArray((BYTE *)&fieldSize, 2) == 2);
   primes = AdjustNumberOfPrimes(primes);
   // If the remainder is odd, then subtracting the value
   // will give an even number, but we want an odd number,
   // so subtract the 105+rem. Otherwise, just subtract
   // the even remainder.
   adjust = BN_mod_word(bnN,105);
   if(adjust & 1)
       adjust += 105;
   // seed the field
   // This starts the pointer at the nearest byte to the input value
   p1 = &seedValues[adjust/16];
   // Reduce the number of bytes to transfer by the amount skipped
   j = sizeof(seedValues) - adjust/16;
   adjust = adjust % 16;
   BN_sub_word(bnN, adjust);
   adjust >>= 1;
   // This offsets the field
   p2 = field;
   for(i = fieldSize; i > 0; i--)
   {
       *p2++ = *p1++;
       if(--j == 0)
       {
           j = sizeof(seedValues);
           p1 = seedValues;
       }
   }
   // Mask the first bits in the field and the last byte in order to eliminate
   // bytes not in the field from consideration.
   field[0] &= 0xff << adjust;
   field[fieldSize-1] &= 0xff >> (8 - adjust);
   // Cycle through the primes, clearing bits
   // Have already done 3, 5, and 7
   PrimeInit(7, &iter, primes);
   // Get the next N primes where N is determined by the mark in the sieveMarks
   while((composite = NextPrime(&iter)) != 0)
   {
       UINT32 pList[8];
       UINT32   next = 0;
       i = count;
       pList[i--] = composite;
       for(; i > 0; i--)
       {
           next = NextPrime(&iter);
           pList[i] = next;
           if(next != 0)
               composite *= next;
       }
       composite = BN_mod_word(bnN, composite);
       for(i = count; i > 0; i--)
       {
           next = pList[i];
           if(next == 0)
               goto done;
           r = composite % next;
              if(r & 1)           j = (next - r)/2;
              else if(r == 0)     j = 0;
              else                j = next - r/2;
              for(; j < fieldBits; j += next)
                  ClearBit(field, j);
         }
         if(next >= stop)
         {
             mark++;
             count = sieveMarks[mark].count;
             stop = sieveMarks[mark].prime;
         }
   }
done:
   INSTRUMENT_INC(totalFieldsSieved);
   i = BitsInArray(field, fieldSize);
   if(i == 0) INSTRUMENT_INC(emptyFieldsSieved);
   return i;
}
//
//
//       PrimeSelectWithSieve()
//
//      This function will sieve the field around the input prime candidate. If the sieve field is not empty, one of
//      the one bits in the field is chosen for testing with Miller-Rabin. If the value is prime, pnP is updated with
//      this value and the function returns success. If this value is not prime, another pseudo-random candidate
//      is chosen and tested. This process repeats until all values in the field have been checked. If all bits in the
//      field have been checked and none is prime, the function returns FALSE and a new random value needs
//      to be chosen.
//
BOOL
PrimeSelectWithSieve(
   BIGNUM               *bnP,                    // IN/OUT: The candidate to filter
   KDFa_CONTEXT         *ktx,                    // IN: KDFa iterator structure
   UINT32                e,                      // IN: the exponent
   BN_CTX               *context                 // IN: the big number context to play in
#ifdef RSA_DEBUG                                  //%
  ,UINT16                fieldSize,              // IN: number of bytes in the field, as
                                                 //     determined by the caller
   UINT16            primes                      // IN: number of primes to use.
#endif                                            //%
)
{
   BYTE              field[MAX_FIELD_SIZE];
   UINT32            first;
   UINT32            ones;
   INT32             chosen;
   UINT32            rounds = MillerRabinRounds(BN_num_bits(bnP));
#ifndef RSA_DEBUG
   UINT32            primes;
   UINT32            fieldSize;
   // Adjust the field size and prime table list to fit the size of the prime
   // being tested.
   primes = BN_num_bits(bnP);
   if(primes <= 512)
   {
       primes = AdjustNumberOfPrimes(2048);
       fieldSize = 65;
   }
   else if(primes <= 1024)
   {
       primes = AdjustNumberOfPrimes(4096);
       fieldSize = 129;
   }
//
   else
   {
       primes = AdjustNumberOfPrimes(0);             // Set to the maximum
       fieldSize = MAX_FIELD_SIZE;
   }
   if(fieldSize > MAX_FIELD_SIZE)
       fieldSize = MAX_FIELD_SIZE;
#endif
    // Save the low-order word to use as a search generator and make sure that
    // it has some interesting range to it
    first = bnP->d[0] | 0x80000000;
   // Align to field boundary
   bnP->d[0] &= ~((UINT32)(fieldSize-3));
   pAssert(BN_is_bit_set(bnP, 0));
   bnP->d[0] &= (UINT32_MAX << (FIELD_POWER + 1)) + 1;
   ones = PrimeSieve(bnP, fieldSize, field, primes);
#ifdef RSA_FILTER_DEBUG
   pAssert(ones == BitsInArray(field, defaultFieldSize));
#endif
   for(; ones > 0; ones--)
   {
#ifdef RSA_FILTER_DEBUG
       if(ones != BitsInArray(field, defaultFieldSize))
           FAIL(FATAL_ERROR_INTERNAL);
#endif
       // Decide which bit to look at and find its offset
       if(ones == 1)
           ones = ones;
       chosen = FindNthSetBit(defaultFieldSize, field,((first % ones) + 1));
       if(chosen >= ((defaultFieldSize) * 8))
           FAIL(FATAL_ERROR_INTERNAL);
         // Set this as the trial prime
         BN_add_word(bnP, chosen * 2);
         // Use MR to see if this is prime
         if(MillerRabin(bnP, rounds, ktx, context))
         {
             // Final check is to make sure that 0 != (p-1) mod e
             // This is the same as -1 != p mod e ; or
             // (e - 1) != p mod e
             if((e <= 3) || (BN_mod_word(bnP, e) != (e-1)))
                 return TRUE;
         }
         // Back out the bit number
         BN_sub_word(bnP, chosen * 2);
         // Clear the bit just tested
         ClearBit(field, chosen);
}
    // Ran out of bits and couldn't find a prime in this field
    INSTRUMENT_INC(noPrimeFields);
    return FALSE;
}
//
//
//       AdjustPrimeCandiate()
//
//      This function adjusts the candidate prime so that it is odd and > root(2)/2. This allows the product of these
//      two numbers to be .5, which, in fixed point notation means that the most significant bit is 1. For this
//      routine, the root(2)/2 is approximated with 0xB505 which is, in fixed point is 0.7071075439453125 or an
//      error of 0.0001%. Just setting the upper two bits would give a value > 0.75 which is an error of > 6%.
//
//
//      Given the amount of time all the other computations take, reducing the error is not much of a cost, but it
//      isn't totally required either.
//      The function also puts the number on a field boundary.
//
void
AdjustPrimeCandidate(
   BYTE                *a,
   UINT16               len
   )
{
   UINT16    highBytes;
   highBytes = BYTE_ARRAY_TO_UINT16(a);
   // This is fixed point arithmetic on 16-bit values
   highBytes = ((UINT32)highBytes * (UINT32)0x4AFB) >> 16;
   highBytes += 0xB505;
   UINT16_TO_BYTE_ARRAY(highBytes, a);
   a[len-1] |= 1;
}
//
//
//       GeneratateRamdomPrime()
//
void
GenerateRandomPrime(
   TPM2B  *p,
   BN_CTX *ctx
#ifdef RSA_DEBUG               //%
  ,UINT16  field,
   UINT16  primes
#endif                         //%
   )
{
   BIGNUM *bnP;
   BN_CTX *context;
   if(ctx == NULL) context = BN_CTX_new();
   else context = ctx;
   if(context == NULL)
       FAIL(FATAL_ERROR_ALLOCATION);
   BN_CTX_start(context);
   bnP = BN_CTX_get(context);
   while(TRUE)
   {
       _cpri__GenerateRandom(p->size, p->buffer);
       p->buffer[p->size-1] |= 1;
       p->buffer[0] |= 0x80;
       BN_bin2bn(p->buffer, p->size, bnP);
#ifdef RSA_DEBUG
       if(PrimeSelectWithSieve(bnP, NULL, 0, context, field, primes))
#else
       if(PrimeSelectWithSieve(bnP, NULL, 0, context))
#endif
           break;
   }
   BnTo2B(p, bnP, (UINT16)BN_num_bytes(bnP));
   BN_CTX_end(context);
   if(ctx == NULL)
       BN_CTX_free(context);
   return;
}
KDFa_CONTEXT *
KDFaContextStart(
    KDFa_CONTEXT        *ktx,                //   IN/OUT:   the context structure to initialize
    TPM2B               *seed,               //   IN: the   seed for the digest proce
    TPM_ALG_ID           hashAlg,            //   IN: the   hash algorithm
    TPM2B               *extra,              //   IN: the   extra data
    UINT32              *outer,              //   IN: the   outer iteration counter
    UINT16               keySizeInBit
    )
{
    UINT16                     digestSize = _cpri__GetDigestSize(hashAlg);
    TPM2B_HASH_BLOCK           oPadKey;
    if(seed == NULL)
        return NULL;
    pAssert(ktx != NULL && outer != NULL && digestSize != 0);
   // Start the hash using the seed and get the intermediate hash value
   _cpri__StartHMAC(hashAlg, FALSE, &(ktx->iPadCtx), seed->size, seed->buffer,
                    &oPadKey.b);
   _cpri__StartHash(hashAlg, FALSE, &(ktx->oPadCtx));
   _cpri__UpdateHash(&(ktx->oPadCtx), oPadKey.b.size, oPadKey.b.buffer);
   ktx->extra = extra;
   ktx->hashAlg = hashAlg;
   ktx->outer = outer;
   ktx->keySizeInBits = keySizeInBits;
   return ktx;
}
void
KDFaContextEnd(
    KDFa_CONTEXT        *ktx                 // IN/OUT: the context structure to close
    )
{
    if(ktx != NULL)
    {
        // Close out the hash sessions
        _cpri__CompleteHash(&(ktx->iPadCtx), 0, NULL);
        _cpri__CompleteHash(&(ktx->oPadCtx), 0, NULL);
    }
}
//%#endif
//
//
//       Public Function
//
//         Introduction
//
//      This is the external entry for this replacement function. All this file provides is the substitute function to
//      generate an RSA key. If the compiler settings are set appropriately, this this function will be used instead
//      of the similarly named function in CpriRSA.c.
//
//         _cpri__GenerateKeyRSA()
//
//      Generate an RSA key from a provided seed
//
//      Return Value                     Meaning
//
//      CRYPT_FAIL                       exponent is not prime or is less than 3; or could not find a prime using
//                                       the provided parameters
//      CRYPT_CANCEL                     operation was canceled
//
LIB_EXPORT CRYPT_RESULT
_cpri__GenerateKeyRSA(
   TPM2B              *n,               // OUT: The public modulus
   TPM2B              *p,               // OUT: One of the prime factors of n
   UINT16              keySizeInBits,   // IN: Size of the public modulus in bits
   UINT32              e,               // IN: The public exponent
   TPM_ALG_ID          hashAlg,         // IN: hash algorithm to use in the key
                                        //     generation process
   TPM2B              *seed,            // IN: the seed to use
   const char         *label,           // IN: A label for the generation process.
   TPM2B              *extra,           // IN: Party 1 data for the KDF
   UINT32             *counter          // IN/OUT: Counter value to allow KDF
                                        //         iteration to be propagated across
                                        //         multiple routines
#ifdef RSA_DEBUG                         //%
  ,UINT16              primes,          // IN: number of primes to test
   UINT16              fieldSize        // IN: the field size to use
#endif                                   //%
   )
{
   CRYPT_RESULT             retVal;
   UINT32                   myCounter = 0;
   UINT32                  *pCtr = (counter == NULL) ? &myCounter : counter;
   KDFa_CONTEXT             ktx;
   KDFa_CONTEXT            *ktxPtr;
   UINT32                   i;
   BIGNUM                  *bnP;
   BIGNUM                  *bnQ;
   BIGNUM                  *bnT;
   BIGNUM                  *bnE;
   BIGNUM                  *bnN;
   BN_CTX                  *context;
   // Make sure that the required pointers are provided
   pAssert(n != NULL && p != NULL);
   // If the seed is provided, then use KDFa for generation of the 'random'
   // values
   ktxPtr = KDFaContextStart(&ktx, seed, hashAlg, extra, pCtr, keySizeInBits);
   n->size = keySizeInBits/8;
   p->size = n->size / 2;
   // Validate exponent
   if(e == 0 || e == RSA_DEFAULT_PUBLIC_EXPONENT)
       e = RSA_DEFAULT_PUBLIC_EXPONENT;
   else
       if(!IsPrimeWord(e))
           return CRYPT_FAIL;
   // Get structures for the big number representations
   context = BN_CTX_new();
   BN_CTX_start(context);
   bnP = BN_CTX_get(context);
   bnQ = BN_CTX_get(context);
   bnT = BN_CTX_get(context);
   bnE = BN_CTX_get(context);
   bnN = BN_CTX_get(context);
   if(bnN == NULL)
       FAIL(FATAL_ERROR_INTERNAL);
   //   Set Q to zero. This is used as a flag. The prime is computed in P. When a
   //   new prime is found, Q is checked to see if it is zero. If so, P is copied
   //   to Q and a new P is found. When both P and Q are non-zero, the modulus and
   //   private exponent are computed and a trial encryption/decryption is
   //   performed. If the encrypt/decrypt fails, assume that at least one of the
   //   primes is composite. Since we don't know which one, set Q to zero and start
   // over and find a new pair of primes.
   BN_zero(bnQ);
   BN_set_word(bnE, e);
   // Each call to generate a random value will increment ktx.outer
   // it doesn't matter if ktx.outer wraps. This lets the caller
   // use the initial value of the counter for additional entropy.
   for(i = 0; i < UINT32_MAX; i++)
   {
       if(_plat__IsCanceled())
       {
            retVal = CRYPT_CANCEL;
            goto end;
       }
       // Get a random prime candidate.
       if(seed == NULL)
            _cpri__GenerateRandom(p->size, p->buffer);
       else
            RandomForRsa(&ktx, label, p);
       AdjustPrimeCandidate(p->buffer, p->size);
         // Convert the candidate to a BN
         if(BN_bin2bn(p->buffer, p->size, bnP) == NULL)
             FAIL(FATAL_ERROR_INTERNAL);
         // If this is the second prime, make sure that it differs from the
         // first prime by at least 2^100. Since BIGNUMS use words, the check
         // below will make sure they are different by at least 128 bits
         if(!BN_is_zero(bnQ))
         { // bnQ is non-zero, we have a first value
             UINT32       *pP = (UINT32 *)(&bnP->d[4]);
             UINT32       *pQ = (UINT32 *)(&bnQ->d[4]);
             INT32        k = ((INT32)bnP->top) - 4;
             for(;k > 0; k--)
                 if(*pP++ != *pQ++)
                     break;
             // Didn't find any difference so go get a new value
             if(k == 0)
                 continue;
         }
         // If PrimeSelectWithSieve   returns success, bnP is a prime,
#ifdef    RSA_DEBUG
         if(!PrimeSelectWithSieve(bnP, ktxPtr, e, context, fieldSize, primes))
#else
         if(!PrimeSelectWithSieve(bnP, ktxPtr, e, context))
#endif
              continue;      // If not, get another
         // Found a prime, is this the first or second.
         if(BN_is_zero(bnQ))
         {    // copy p to q and compute another prime in p
              BN_copy(bnQ, bnP);
              continue;
         }
         //Form the public modulus
        if(    BN_mul(bnN, bnP, bnQ, context) != 1
            || BN_num_bits(bnN) != keySizeInBits)
              FAIL(FATAL_ERROR_INTERNAL);
        // Save the public modulus
        BnTo2B(n, bnN, n->size);
        // And one prime
        BnTo2B(p, bnP, p->size);
#ifdef EXTENDED_CHECKS
       // Finish by making sure that we can form the modular inverse of PHI
       // with respect to the public exponent
       // Compute PHI = (p - 1)(q - 1) = n - p - q + 1
        // Make sure that we can form the modular inverse
        if(    BN_sub(bnT, bnN, bnP) != 1
            || BN_sub(bnT, bnT, bnQ) != 1
            || BN_add_word(bnT, 1) != 1)
             FAIL(FATAL_ERROR_INTERNAL);
        // find d such that (Phi * d) mod e ==1
        // If there isn't then we are broken because we took the step
        // of making sure that the prime != 1 mod e so the modular inverse
        // must exist
        if(    BN_mod_inverse(bnT, bnE, bnT, context) == NULL
            || BN_is_zero(bnT))
             FAIL(FATAL_ERROR_INTERNAL);
        // And, finally, do a trial encryption decryption
        {
            TPM2B_TYPE(RSA_KEY, MAX_RSA_KEY_BYTES);
            TPM2B_RSA_KEY        r;
            r.t.size = sizeof(r.t.buffer);
            // If we are using a seed, then results must be reproducible on each
            // call. Otherwise, just get a random number
            if(seed == NULL)
                _cpri__GenerateRandom(keySizeInBits/8, r.t.buffer);
            else
                RandomForRsa(&ktx, label, &r.b);
             // Make sure that the number is smaller than the public modulus
             r.t.buffer[0] &= 0x7F;
                    // Convert
             if(    BN_bin2bn(r.t.buffer, r.t.size, bnP) == NULL
                    // Encrypt with the public exponent
                 || BN_mod_exp(bnQ, bnP, bnE, bnN, context) != 1
                    // Decrypt with the private exponent
                 || BN_mod_exp(bnQ, bnQ, bnT, bnN, context) != 1)
                  FAIL(FATAL_ERROR_INTERNAL);
             // If the starting and ending values are not the same, start over )-;
             if(BN_ucmp(bnP, bnQ) != 0)
             {
                  BN_zero(bnQ);
                  continue;
             }
       }
#endif // EXTENDED_CHECKS
       retVal = CRYPT_SUCCESS;
       goto end;
   }
   retVal = CRYPT_FAIL;
end:
   KDFaContextEnd(&ktx);
   // Free up allocated BN values
   BN_CTX_end(context);
   BN_CTX_free(context);
   return retVal;
}
#endif              //%
#endif // TPM_ALG_RSA
