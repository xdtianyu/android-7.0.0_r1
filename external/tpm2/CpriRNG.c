// This file was extracted from the TCG Published
// Trusted Platform Module Library
// Part 4: Supporting Routines
// Family "2.0"
// Level 00 Revision 01.16
// October 30, 2014

//#define __TPM_RNG_FOR_DEBUG__
//
//
//          Introduction
//
//     This file contains the interface to the OpenSSL() random number functions.
//
//          Includes
//
#include "OsslCryptoEngine.h"
int         s_entropyFailure;
//
//
//          Functions
//
//          _cpri__RngStartup()
//
//     This function is called to initialize the random number generator. It collects entropy from the platform to
//     seed the OpenSSL() random number generator.
//
LIB_EXPORT BOOL
_cpri__RngStartup(void)
{
     UINT32           entropySize;
     BYTE             entropy[MAX_RNG_ENTROPY_SIZE];
     INT32            returnedSize = 0;
     // Initialize the entropy source
     s_entropyFailure = FALSE;
     _plat__GetEntropy(NULL, 0);
     // Collect entropy until we have enough
     for(entropySize = 0;
         entropySize < MAX_RNG_ENTROPY_SIZE && returnedSize >= 0;
         entropySize += returnedSize)
     {
         returnedSize = _plat__GetEntropy(&entropy[entropySize],
                                             MAX_RNG_ENTROPY_SIZE - entropySize);
     }
     // Got some entropy on the last call and did not get an error
     if(returnedSize > 0)
     {
         // Seed OpenSSL with entropy
         RAND_seed(entropy, entropySize);
     }
     else
     {
         s_entropyFailure = TRUE;
     }
     return s_entropyFailure == FALSE;
}
//
//
//          _cpri__DrbgGetPutState()
//
//     This function is used to set the state of the RNG (direction == PUT_STATE) or to recover the state of the
//     RNG (direction == GET_STATE).
//
//
//
//     NOTE:           This not currently supported on OpenSSL() version.
//
LIB_EXPORT CRYPT_RESULT
_cpri__DrbgGetPutState(
    GET_PUT              direction,
    int                  bufferSize,
    BYTE                *buffer
    )
{
    UNREFERENCED_PARAMETER(direction);
    UNREFERENCED_PARAMETER(bufferSize);
    UNREFERENCED_PARAMETER(buffer);
    return CRYPT_SUCCESS;                 // Function is not implemented
}
//
//
//          _cpri__StirRandom()
//
//     This function is called to add external entropy to the OpenSSL() random number generator.
//
LIB_EXPORT CRYPT_RESULT
_cpri__StirRandom(
    INT32                entropySize,
    BYTE                *entropy
    )
{
    if (entropySize >= 0)
    {
        RAND_add((const void *)entropy, (int) entropySize, 0.0);
    }
    return CRYPT_SUCCESS;
}
//
//
//          _cpri__GenerateRandom()
//
//     This function is called to get a string of random bytes from the OpenSSL() random number generator. The
//     return value is the number of bytes placed in the buffer. If the number of bytes returned is not equal to the
//     number of bytes requested (randomSize) it is indicative of a failure of the OpenSSL() random number
//     generator and is probably fatal.
//
LIB_EXPORT UINT16
_cpri__GenerateRandom(
    INT32                randomSize,
    BYTE                *buffer
    )
{
    //
    // We don't do negative sizes or ones that are too large
    if (randomSize < 0 || randomSize > UINT16_MAX)
        return 0;
    // RAND_bytes uses 1 for success and we use 0
    if(RAND_bytes(buffer, randomSize) == 1)
        return (UINT16)randomSize;
    else
        return 0;
}
//
//
//
//          _cpri__GenerateSeededRandom()
//
//     This funciton is used to generate a pseudo-random number from some seed values This funciton returns
//     the same result each time it is called with the same parameters
//
LIB_EXPORT UINT16
_cpri__GenerateSeededRandom(
   INT32               randomSize,      //   IN: the size of the request
   BYTE               *random,          //   OUT: receives the data
   TPM_ALG_ID          hashAlg,         //   IN: used by KDF version but not here
   TPM2B              *seed,            //   IN: the seed value
   const char         *label,           //   IN: a label string (optional)
   TPM2B              *partyU,          //   IN: other data (oprtional)
   TPM2B              *partyV           //   IN: still more (optional)
   )
{
   return (_cpri__KDFa(hashAlg, seed, label, partyU, partyV,
                       randomSize * 8, random, NULL, FALSE));
}
