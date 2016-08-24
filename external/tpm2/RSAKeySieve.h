// This file was extracted from the TCG Published
// Trusted Platform Module Library
// Part 4: Supporting Routines
// Family "2.0"
// Level 00 Revision 01.16
// October 30, 2014

#ifndef        RSA_H
#define        RSA_H
//
//       This value is used to set the size of the table that is searched by the prime iterator. This is used during
//       the generation of different primes. The smaller tables are used when generating smaller primes.
//
extern const UINT16        primeTableBytes;
//
//       The following define determines how large the prime number difference table will be defined. The value of
//       13 will allocate the maximum size table which allows generation of the first 6542 primes which is all the
//       primes less than 2^16.
#define PRIME_DIFF_TABLE_512_BYTE_PAGES                  13
//
//     This set of macros used the value above to set the table size.
//
#ifndef PRIME_DIFF_TABLE_512_BYTE_PAGES
#   define PRIME_DIFF_TABLE_512_BYTE_PAGES      4
#endif
#ifdef PRIME_DIFF_TABLE_512_BYTE_PAGES
#   if PRIME_DIFF_TABLE_512_BYTE_PAGES > 12
#        define PRIME_DIFF_TABLE_BYTES 6542
#   else
#        if PRIME_DIFF_TABLE_512_BYTE_PAGES <= 0
#             define PRIME_DIFF_TABLE_BYTES 512
#        else
#             define PRIME_DIFF_TABLE_BYTES (PRIME_DIFF_TABLE_512_BYTE_PAGES * 512)
#        endif
#   endif
#endif
extern const BYTE primeDiffTable [PRIME_DIFF_TABLE_BYTES];
//
//     This determines the number of bits in the sieve field This must be a power of two.
//
#define FIELD_POWER            14  // This is the only value in this group that should be
                                  // changed
#define FIELD_BITS             (1 << FIELD_POWER)
#define MAX_FIELD_SIZE             ((FIELD_BITS / 8) + 1)
//
//     This is the pre-sieved table. It already has the bits for multiples of 3, 5, and 7 cleared.
//
#define SEED_VALUES_SIZE                    105
const extern BYTE                           seedValues[SEED_VALUES_SIZE];
//
//     This allows determination of the number of bits that are set in a byte without having to count them
//     individually.
//
const extern BYTE                           bitsInByte[256];
//
//     This is the iterator structure for accessing the compressed prime number table. The expectation is that
//     values will need to be accesses sequentially. This tries to save some data access.
//
typedef struct {
   UINT32       lastPrime;
   UINT32       index;
   UINT32       final;
} PRIME_ITERATOR;
#ifdef RSA_INSTRUMENT
#   define INSTRUMENT_SET(a, b) ((a) = (b))
#   define INSTRUMENT_ADD(a, b) (a) = (a) + (b)
#   define INSTRUMENT_INC(a)     (a) = (a) + 1
extern UINT32 failedAtIteration[10];
extern UINT32 MillerRabinTrials;
extern UINT32 totalFieldsSieved;
extern UINT32 emptyFieldsSieved;
extern UINT32 noPrimeFields;
extern UINT32 primesChecked;
extern UINT16    lastSievePrime;
#else
#   define INSTRUMENT_SET(a, b)
#   define INSTRUMENT_ADD(a, b)
#   define INSTRUMENT_INC(a)
#endif
#ifdef RSA_DEBUG
extern UINT16    defaultFieldSize;
#define NUM_PRIMES                2047
extern const __int16              primes[NUM_PRIMES];
#else
#define defaultFieldSize          MAX_FIELD_SIZE
#endif
#endif
