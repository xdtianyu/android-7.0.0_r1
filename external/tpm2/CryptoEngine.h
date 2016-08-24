// This file was extracted from the TCG Published
// Trusted Platform Module Library
// Part 4: Supporting Routines
// Family "2.0"
// Level 00 Revision 01.16
// October 30, 2014

#ifndef _CRYPT_PRI_H
#define _CRYPT_PRI_H
#include     <stddef.h>
#include     "TpmBuildSwitches.h"
#include     "BaseTypes.h"
#include     "TpmError.h"
#include     "swap.h"
#include     "Implementation.h"
#include     "TPM_Types.h"
//#include     "TPMB.h"
#include     "bool.h"
#include     "Platform.h"
#ifndef NULL
#define NULL     0
#endif
typedef UINT16 NUMBYTES;          // When a size is a number of bytes
typedef UINT32 NUMDIGITS;         // When a size is a number of "digits"
//        General Purpose Macros
//
#ifndef MAX
#   define MAX(a, b) ((a) > (b) ? (a) : b)
#endif
//
//     This is the definition of a bit array with one bit per algorithm
//
typedef BYTE         ALGORITHM_VECTOR[(ALG_LAST_VALUE + 7) / 8];
//
//
//        Self-test
//
//     This structure is used to contain self-test tracking information for the crypto engine. Each of the major
//     modules is given a 32-bit value in which it may maintain its own self test information. The convention for
//     this state is that when all of the bits in this structure are 0, all functions need to be tested.
//
typedef struct {
   UINT32       rng;
   UINT32       hash;
   UINT32       sym;
#ifdef TPM_ALG_RSA
   UINT32       rsa;
#endif
#ifdef TPM_ALG_ECC
   UINT32       ecc;
#endif
} CRYPTO_SELF_TEST_STATE;
//
//
//        Hash-related Structures
//
typedef struct {
   const TPM_ALG_ID              alg;
   const NUMBYTES                digestSize;
   const NUMBYTES                blockSize;
   const NUMBYTES                derSize;
   const BYTE                    der[20];
} HASH_INFO;
//
//     This value will change with each implementation. The value of 16 is used to account for any slop in the
//     context values. The overall size needs to be as large as any of the hash contexts. The structure needs to
//     start on an alignment boundary and be an even multiple of the alignment
//
#define ALIGNED_SIZE(x, b) ((((x) + (b) - 1) / (b)) * (b))
#define MAX_HASH_STATE_SIZE ((2 * MAX_HASH_BLOCK_SIZE) + 16)
#if defined USER_MIN_HASH_STATE_SIZE && \
  (MAX_HASH_STATE_SIZE < (USER_MIN_HASH_STATE_SIZE))
#define REQUIRED_HASH_STATE_SIZE USER_MIN_HASH_STATE_SIZE
#else
#define REQUIRED_HASH_STATE_SIZE MAX_HASH_STATE_SIZE
#endif
#define MAX_HASH_STATE_SIZE_ALIGNED                                                              \
                   ALIGNED_SIZE(REQUIRED_HASH_STATE_SIZE, CRYPTO_ALIGNMENT)
//
//     This is an byte array that will hold any of the hash contexts.
//
typedef CRYPTO_ALIGNED BYTE ALIGNED_HASH_STATE[MAX_HASH_STATE_SIZE_ALIGNED];
//
//     Macro to align an address to the next higher size
//
#define AlignPointer(address, align)                                                             \
  ((((intptr_t)&(address)) + (align - 1)) & ~(align - 1))
//
//     Macro to test alignment
//
#define IsAddressAligned(address, align)                                                         \
                   (((intptr_t)(address) & (align - 1)) == 0)
//
//     This is the structure that is used for passing a context into the hashing functions. It should be the same
//     size as the function context used within the hashing functions. This is checked when the hash function is
//     initialized. This version uses a new layout for the contexts and a different definition. The state buffer is an
//     array of HASH_UNIT values so that a decent compiler will put the structure on a HASH_UNIT boundary.
//     If the structure is not properly aligned, the code that manipulates the structure will copy to a properly
//     aligned structure before it is used and copy the result back. This just makes things slower.
//
typedef struct _HASH_STATE
{
   ALIGNED_HASH_STATE       state;
   TPM_ALG_ID               hashAlg;
} CPRI_HASH_STATE, *PCPRI_HASH_STATE;
extern const HASH_INFO   g_hashData[HASH_COUNT + 1];
//
//     This is for the external hash state. This implementation assumes that the size of the exported hash state
//     is no larger than the internal hash state. There is a compile-time check to make sure that this is true.
//
typedef struct {
   ALIGNED_HASH_STATE             buffer;
   TPM_ALG_ID                     hashAlg;
} EXPORT_HASH_STATE;
typedef enum {
   IMPORT_STATE,             // Converts externally formatted state to internal
   EXPORT_STATE              // Converts internal formatted state to external
} IMPORT_EXPORT;
//
//     Values and structures for the random number generator. These values are defined in this header file so
//     that the size of the RNG state can be known to TPM.lib. This allows the allocation of some space in NV
//     memory for the state to be stored on an orderly shutdown. The GET_PUT enum is used by
//     _cpri__DrbgGetPutState() to indicate the direction of data flow.
//
typedef enum {
   GET_STATE,           // Get the state to save to NV
   PUT_STATE            // Restore the state from NV
} GET_PUT;
//
//     The DRBG based on a symmetric block cipher is defined by three values,
//     a) the key size
//     b) the block size (the IV size)
//     c) the symmetric algorithm
//
#define DRBG_KEY_SIZE_BITS       MAX_AES_KEY_BITS
#define DRBG_IV_SIZE_BITS        (MAX_AES_BLOCK_SIZE_BYTES * 8)
#define DRBG_ALGORITHM           TPM_ALG_AES
#if ((DRBG_KEY_SIZE_BITS % 8) != 0) || ((DRBG_IV_SIZE_BITS % 8) != 0)
#error "Key size and IV for DRBG must be even multiples of 8"
#endif
#if (DRBG_KEY_SIZE_BITS % DRBG_IV_SIZE_BITS) != 0
#error "Key size for DRBG must be even multiple of the cypher block size"
#endif
typedef UINT32     DRBG_SEED[(DRBG_KEY_SIZE_BITS + DRBG_IV_SIZE_BITS) / 32];
typedef struct {
   UINT64       reseedCounter;
   UINT32       magic;
   DRBG_SEED    seed; // contains the key and IV for the counter mode DRBG
   UINT32       lastValue[4];   // used when the TPM does continuous self-test
                                // for FIPS compliance of DRBG
} DRBG_STATE, *pDRBG_STATE;
//
//
//           Asymmetric Structures and Values
//
#ifdef TPM_ALG_ECC
//
//
//          ECC-related Structures
//
//      This structure replicates the structure definition in TPM_Types.h. It is duplicated to avoid inclusion of all of
//      TPM_Types.h This structure is similar to the RSA_KEY structure below. The purpose of these structures
//      is to reduce the overhead of a function call and to make the code less dependent on key types as much
//      as possible.
//
typedef struct {
   UINT32                        curveID;            // The curve identifier
   TPMS_ECC_POINT               *publicPoint;        // Pointer to the public point
   TPM2B_ECC_PARAMETER          *privateKey;         // Pointer to the private key
} ECC_KEY;
#endif // TPM_ALG_ECC
#ifdef TPM_ALG_RSA
//
//
//          RSA-related Structures
//
//      This structure is a succinct representation of the cryptographic components of an RSA key.
//
typedef struct {
   UINT32        exponent;                 // The public exponent pointer
   TPM2B        *publicKey;                // Pointer to the public modulus
   TPM2B        *privateKey;               // The private exponent (not a prime)
} RSA_KEY;
#endif // TPM_ALG_RSA
//
//
//           Miscelaneous
//
#ifdef TPM_ALG_RSA
#   ifdef TPM_ALG_ECC
#       if    MAX_RSA_KEY_BYTES > MAX_ECC_KEY_BYTES
#            define MAX_NUMBER_SIZE          MAX_RSA_KEY_BYTES
#       else
#            define MAX_NUMBER_SIZE          MAX_ECC_KEY_BYTES
#       endif
#   else // RSA but no ECC
#       define MAX_NUMBER_SIZE               MAX_RSA_KEY_BYTES
#   endif
#elif defined TPM_ALG_ECC
#   define MAX_NUMBER_SIZE                  MAX_ECC_KEY_BYTES
#else
#   error No assymmetric algorithm implemented.
#endif
typedef INT16      CRYPT_RESULT;
#define CRYPT_RESULT_MIN     INT16_MIN
#define CRYPT_RESULT_MAX     INT16_MAX
//
//
//      <0                                recoverable error
//
//      0                                 success
//      >0                                command specific return value (generally a digest size)
//
#define CRYPT_FAIL                  ((CRYPT_RESULT) 1)
#define CRYPT_SUCCESS               ((CRYPT_RESULT) 0)
#define CRYPT_NO_RESULT             ((CRYPT_RESULT) -1)
//
#define CRYPT_SCHEME        ((CRYPT_RESULT) -2)
#define CRYPT_PARAMETER     ((CRYPT_RESULT) -3)
#define CRYPT_UNDERFLOW     ((CRYPT_RESULT) -4)
#define CRYPT_POINT         ((CRYPT_RESULT) -5)
#define CRYPT_CANCEL        ((CRYPT_RESULT) -6)
#include    "CpriCryptPri_fp.h"
#ifdef TPM_ALG_ECC
#   include "CpriDataEcc.h"
#   include "CpriECC_fp.h"
#endif
#include    "MathFunctions_fp.h"
#include    "CpriRNG_fp.h"
#include    "CpriHash_fp.h"
#include    "CpriSym_fp.h"
#ifdef TPM_ALG_RSA
#   include    "CpriRSA_fp.h"
#endif
#endif // !_CRYPT_PRI_H
