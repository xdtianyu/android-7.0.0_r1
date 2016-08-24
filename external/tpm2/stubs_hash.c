/* This file includes functions that were extracted from the TPM2
 * source, but were present in files not included in compilation.
 */
#include "Global.h"
#include "CryptoEngine.h"

#include <string.h>

UINT16 _cpri__StartHMAC(
  TPM_ALG_ID hashAlg,           //   IN: the algorithm to use
  BOOL sequence,                //   IN: indicates if the state should be saved
  CPRI_HASH_STATE * state,      //   IN/OUT: the state buffer
  UINT16 keySize,               //   IN: the size of the HMAC key
  BYTE * key,                   //   IN: the HMAC key
  TPM2B * oPadKey               //   OUT: the key prepared for the oPad round
  )
{
      CPRI_HASH_STATE localState;
      UINT16           blockSize = _cpri__GetHashBlockSize(hashAlg);
      UINT16           digestSize;
      BYTE            *pb;         // temp pointer
      UINT32           i;
      // If the key size is larger than the block size, then the hash of the key
      // is used as the key
      if(keySize > blockSize)
      {
          // large key so digest
          if((digestSize = _cpri__StartHash(hashAlg, FALSE, &localState)) == 0)
              return 0;
          _cpri__UpdateHash(&localState, keySize, key);
          _cpri__CompleteHash(&localState, digestSize, oPadKey->buffer);
          oPadKey->size = digestSize;
      }
      else
      {
          // key size is ok
          memcpy(oPadKey->buffer, key, keySize);
          oPadKey->size = keySize;
      }
      // XOR the key with iPad (0x36)
      pb = oPadKey->buffer;
      for(i = oPadKey->size; i > 0; i--)
          *pb++ ^= 0x36;
      // if the keySize is smaller than a block, fill the rest with 0x36
      for(i = blockSize - oPadKey->size; i > 0; i--)
          *pb++ = 0x36;
      // Increase the oPadSize to a full block
      oPadKey->size = blockSize;
      // Start a new hash with the HMAC key
      // This will go in the caller's state structure and may be a sequence or not
      if((digestSize = _cpri__StartHash(hashAlg, sequence, state)) > 0)
      {
          _cpri__UpdateHash(state, oPadKey->size, oPadKey->buffer);
          // XOR the key block with 0x5c ^ 0x36
          for(pb = oPadKey->buffer, i = blockSize; i > 0; i--)
              *pb++ ^= (0x5c ^ 0x36);
      }
      return digestSize;
}

UINT16 _cpri__CompleteHMAC(
  CPRI_HASH_STATE * hashState,  //   IN: the state of hash stack
  TPM2B * oPadKey,              //   IN: the HMAC key in oPad format
  UINT32 dOutSize,              //   IN: size of digest buffer
  BYTE * dOut                   //   OUT: hash digest
  )
{
      BYTE             digest[MAX_DIGEST_SIZE];
      CPRI_HASH_STATE *state = (CPRI_HASH_STATE *)hashState;
      CPRI_HASH_STATE localState;
      UINT16           digestSize = _cpri__GetDigestSize(state->hashAlg);
      _cpri__CompleteHash(hashState, digestSize, digest);
      // Using the local hash state, do a hash with the oPad
      if(_cpri__StartHash(state->hashAlg, FALSE, &localState) != digestSize)
          return 0;
      _cpri__UpdateHash(&localState, oPadKey->size, oPadKey->buffer);
      _cpri__UpdateHash(&localState, digestSize, digest);
      return _cpri__CompleteHash(&localState, dOutSize, dOut);
}

UINT16 _cpri__KDFa(
  TPM_ALG_ID hashAlg,           //   IN: hash algorithm used in HMAC
  TPM2B * key,                  //   IN: HMAC key
  const char *label,            //   IN: a 0-byte terminated label used in KDF
  TPM2B * contextU,             //   IN: context U
  TPM2B * contextV,             //   IN: context V
  UINT32 sizeInBits,            //   IN: size of generated key in bit
  BYTE * keyStream,             //   OUT: key buffer
  UINT32 * counterInOut,        //   IN/OUT: caller may provide the iteration
  //   counter for incremental operations to
  //   avoid large intermediate buffers.
  BOOL once                     //   IN: TRUE if only one iteration is
  // performed FALSE if iteration count determined by "sizeInBits"
  )
{
    UINT32                         counter = 0;    // counter value
    INT32                          lLen = 0;       // length of the label
    INT16                          hLen;           // length of the hash
    INT16                          bytes;          // number of bytes to produce
    BYTE                          *stream = keyStream;
    BYTE                           marshaledUint32[4];
    CPRI_HASH_STATE                hashState;
    TPM2B_MAX_HASH_BLOCK           hmacKey;
    pAssert(key != NULL && keyStream != NULL);
    pAssert(once == FALSE || (sizeInBits & 7) == 0);
    if(counterInOut != NULL)
        counter = *counterInOut;
    // Prepare label buffer. Calculate its size and keep the last 0 byte
    if(label != NULL)
        for(lLen = 0; label[lLen++] != 0; );
    // Get the hash size. If it is less than or 0, either the
    // algorithm is not supported or the hash is TPM_ALG_NULL
//
   // In either case the digest size is zero. This is the only return
   // other than the one at the end. All other exits from this function
   // are fatal errors. After we check that the algorithm is supported
   // anything else that goes wrong is an implementation flaw.
   if((hLen = (INT16) _cpri__GetDigestSize(hashAlg)) == 0)
       return 0;
   // If the size of the request is larger than the numbers will handle,
   // it is a fatal error.
   pAssert(((sizeInBits + 7)/ 8) <= INT16_MAX);
   bytes = once ? hLen : (INT16)((sizeInBits + 7) / 8);
   // Generate required bytes
   for (; bytes > 0; stream = &stream[hLen], bytes = bytes - hLen)
   {
       if(bytes < hLen)
           hLen = bytes;
        counter++;
        // Start HMAC
        if(_cpri__StartHMAC(hashAlg,
                            FALSE,
                            &hashState,
                            key->size,
                            &key->buffer[0],
                            &hmacKey.b)          <= 0)
            FAIL(FATAL_ERROR_INTERNAL);
        // Adding counter
        UINT32_TO_BYTE_ARRAY(counter, marshaledUint32);
        _cpri__UpdateHash(&hashState, sizeof(UINT32), marshaledUint32);
        // Adding label
        if(label != NULL)
            _cpri__UpdateHash(&hashState,   lLen, (BYTE *)label);
        // Adding contextU
        if(contextU != NULL)
            _cpri__UpdateHash(&hashState, contextU->size, contextU->buffer);
        // Adding contextV
        if(contextV != NULL)
            _cpri__UpdateHash(&hashState, contextV->size, contextV->buffer);
        // Adding size in bits
        UINT32_TO_BYTE_ARRAY(sizeInBits, marshaledUint32);
        _cpri__UpdateHash(&hashState, sizeof(UINT32), marshaledUint32);
        // Compute HMAC. At the start of each iteration, hLen is set
        // to the smaller of hLen and bytes. This causes bytes to decrement
        // exactly to zero to complete the loop
        _cpri__CompleteHMAC(&hashState, &hmacKey.b, hLen, stream);
   }
   // Mask off bits if the required bits is not a multiple of byte size
   if((sizeInBits % 8) != 0)
       keyStream[0] &= ((1 << (sizeInBits % 8)) - 1);
   if(counterInOut != NULL)
       *counterInOut = counter;
   return (CRYPT_RESULT)((sizeInBits + 7)/8);
}

UINT16 _cpri__KDFe(
  TPM_ALG_ID hashAlg,           //   IN: hash algorithm used in HMAC
  TPM2B * Z,                    //   IN: Z
  const char *label,            //   IN: a 0 terminated label using in KDF
  TPM2B * partyUInfo,           //   IN: PartyUInfo
  TPM2B * partyVInfo,           //   IN: PartyVInfo
  UINT32 sizeInBits,            //   IN: size of generated key in bit
  BYTE * keyStream              //   OUT: key buffer
  )
{
    UINT32       counter = 0;        // counter value
    UINT32       lSize = 0;
    BYTE        *stream = keyStream;
    CPRI_HASH_STATE         hashState;
    INT16        hLen = (INT16) _cpri__GetDigestSize(hashAlg);
    INT16        bytes;              // number of bytes to generate
    BYTE         marshaledUint32[4];
    pAssert(     keyStream != NULL
                 && Z != NULL
                 && ((sizeInBits + 7) / 8) < INT16_MAX);
    if(hLen == 0)
        return 0;
    bytes = (INT16)((sizeInBits + 7) / 8);
    // Prepare label buffer. Calculate its size and keep the last 0 byte
    if(label != NULL)
        for(lSize = 0; label[lSize++] != 0;);
    // Generate required bytes
    //The inner loop of that KDF uses:
    // Hashi := H(counter | Z | OtherInfo) (5)
    // Where:
    // Hashi    the hash generated on the i-th iteration of the loop.
    // H()      an approved hash function
    // counter a 32-bit counter that is initialized to 1 and incremented
    //          on each iteration
    // Z        the X coordinate of the product of a public ECC key and a
    //          different private ECC key.
    // OtherInfo    a collection of qualifying data for the KDF defined below.
    // In this specification, OtherInfo will be constructed by:
    //      OtherInfo := Use | PartyUInfo | PartyVInfo
    for (; bytes > 0; stream = &stream[hLen], bytes = bytes - hLen)
    {
        if(bytes < hLen)
            hLen = bytes;
//
        counter++;
        // Start hash
        if(_cpri__StartHash(hashAlg, FALSE,   &hashState) == 0)
            return 0;
        // Add counter
        UINT32_TO_BYTE_ARRAY(counter, marshaledUint32);
        _cpri__UpdateHash(&hashState, sizeof(UINT32), marshaledUint32);
        // Add Z
        if(Z != NULL)
            _cpri__UpdateHash(&hashState, Z->size, Z->buffer);
        // Add label
        if(label != NULL)
             _cpri__UpdateHash(&hashState, lSize, (BYTE *)label);
        else
              // The SP800-108 specification requires a zero between the label
              // and the context.
              _cpri__UpdateHash(&hashState, 1, (BYTE *)"");
        // Add PartyUInfo
        if(partyUInfo != NULL)
            _cpri__UpdateHash(&hashState, partyUInfo->size, partyUInfo->buffer);
        // Add PartyVInfo
        if(partyVInfo != NULL)
            _cpri__UpdateHash(&hashState, partyVInfo->size, partyVInfo->buffer);
        // Compute Hash. hLen was changed to be the smaller of bytes or hLen
        // at the start of each iteration.
        _cpri__CompleteHash(&hashState, hLen, stream);
   }
   // Mask off bits if the required bits is not a multiple of byte size
   if((sizeInBits % 8) != 0)
       keyStream[0] &= ((1 << (sizeInBits % 8)) - 1);
   return (CRYPT_RESULT)((sizeInBits + 7) / 8);
}

UINT16 _cpri__GenerateSeededRandom(
  INT32 randomSize,             //   IN: the size of the request
  BYTE * random,                //   OUT: receives the data
  TPM_ALG_ID hashAlg,           //   IN: used by KDF version but not here
  TPM2B * seed,                 //   IN: the seed value
  const char *label,            //   IN: a label string (optional)
  TPM2B * partyU,               //   IN: other data (oprtional)
  TPM2B * partyV                //   IN: still more (optional)
  )
{
   return (_cpri__KDFa(hashAlg, seed, label, partyU, partyV,
                       randomSize * 8, random, NULL, FALSE));
}
