// This file was extracted from the TCG Published
// Trusted Platform Module Library
// Part 4: Supporting Routines
// Family "2.0"
// Level 00 Revision 01.16
// October 30, 2014

#include <string.h>

#include     "OsslCryptoEngine.h"
#include     "CpriHashData.c"
#define OSSL_HASH_STATE_DATA_SIZE     (MAX_HASH_STATE_SIZE - 8)
typedef struct {
   union    {
       EVP_MD_CTX context;
       BYTE         data[OSSL_HASH_STATE_DATA_SIZE];
   } u;
   INT16            copySize;
} OSSL_HASH_STATE;
//
//     Temporary aliasing of SM3 to SHA256 until SM3 is available
//
#define EVP_sm3_256 EVP_sha256
//
//
//          Static Functions
//
//          GetHashServer()
//
//     This function returns the address of the hash server function
//
static EVP_MD *
GetHashServer(
     TPM_ALG_ID      hashAlg
)
{
   switch (hashAlg)
   {
#ifdef TPM_ALG_SHA1
   case TPM_ALG_SHA1:
       return (EVP_MD *)EVP_sha1();
       break;
#endif
#ifdef TPM_ALG_SHA256
   case TPM_ALG_SHA256:
       return (EVP_MD *)EVP_sha256();
       break;
#endif
#ifdef TPM_ALG_SHA384
   case TPM_ALG_SHA384:
       return (EVP_MD *)EVP_sha384();
       break;
#endif
#ifdef TPM_ALG_SHA512
   case TPM_ALG_SHA512:
       return (EVP_MD *)EVP_sha512();
       break;
#endif
#ifdef TPM_ALG_SM3_256
   case TPM_ALG_SM3_256:
       return (EVP_MD *)EVP_sm3_256();
       break;
#endif
   case TPM_ALG_NULL:
       return NULL;
   default:
       FAIL(FATAL_ERROR_INTERNAL);
   }
   return NULL; // Never reached.
}
//
//
//        MarshalHashState()
//
//     This function copies an OpenSSL() hash context into a caller provided buffer.
//
//     Return Value                     Meaning
//
//     >0                               the number of bytes of buf used.
//
static UINT16
MarshalHashState(
    EVP_MD_CTX         *ctxt,               // IN: Context to marshal
    BYTE               *buf                 // OUT: The buffer that will receive the
                                            //     context. This buffer is at least
                                            //     MAX_HASH_STATE_SIZE byte
    )
{
    // make sure everything will fit
    pAssert(ctxt->digest->ctx_size <= OSSL_HASH_STATE_DATA_SIZE);
    // Copy the context data
    memcpy(buf, (void*) ctxt->md_data, ctxt->digest->ctx_size);
    return (UINT16)ctxt->digest->ctx_size;
}
//
//
//        GetHashState()
//
//     This function will unmarshal a caller provided buffer into an OpenSSL() hash context. The function returns
//     the number of bytes copied (which may be zero).
//
static UINT16
GetHashState(
    EVP_MD_CTX         *ctxt,               // OUT: The context structure to receive the
                                            //     result of unmarshaling.
    TPM_ALG_ID          algType,            // IN: The hash algorithm selector
    BYTE               *buf                 // IN: Buffer containing marshaled hash data
    )
{
    EVP_MD             *evpmdAlgorithm = NULL;
    pAssert(ctxt != NULL);
    EVP_MD_CTX_init(ctxt);
    evpmdAlgorithm = GetHashServer(algType);
    if(evpmdAlgorithm == NULL)
        return 0;
    // This also allocates the ctxt->md_data
    if((EVP_DigestInit_ex(ctxt, evpmdAlgorithm, NULL)) != 1)
        FAIL(FATAL_ERROR_INTERNAL);
    pAssert(ctxt->digest->ctx_size < sizeof(ALIGNED_HASH_STATE));
    memcpy(ctxt->md_data, buf, ctxt->digest->ctx_size);
//
    return (UINT16)ctxt->digest->ctx_size;
}
//
//
//          GetHashInfoPointer()
//
//      This function returns a pointer to the hash info for the algorithm. If the algorithm is not supported, function
//      returns a pointer to the data block associated with TPM_ALG_NULL.
//
static const HASH_INFO *
GetHashInfoPointer(
    TPM_ALG_ID           hashAlg
    )
{
    UINT32 i, tableSize;
    // Get the table size of g_hashData
    tableSize = sizeof(g_hashData) / sizeof(g_hashData[0]);
    for(i = 0; i < tableSize - 1; i++)
    {
        if(g_hashData[i].alg == hashAlg)
            return &g_hashData[i];
    }
    return &g_hashData[tableSize-1];
}
//
//
//           Hash Functions
//
//          _cpri__HashStartup()
//
//      Function that is called to initialize the hash service. In this implementation, this function does nothing but
//      it is called by the CryptUtilStartup() function and must be present.
//
LIB_EXPORT BOOL
_cpri__HashStartup(
    void
    )
{
    // On startup, make sure that the structure sizes are compatible. It would
    // be nice if this could be done at compile time but I couldn't figure it out.
    CPRI_HASH_STATE *cpriState = NULL;
//     NUMBYTES        evpCtxSize = sizeof(EVP_MD_CTX);
    NUMBYTES        cpriStateSize = sizeof(cpriState->state);
//     OSSL_HASH_STATE *osslState;
    NUMBYTES        osslStateSize = sizeof(OSSL_HASH_STATE);
//     int             dataSize = sizeof(osslState->u.data);
    pAssert(cpriStateSize >= osslStateSize);
    return TRUE;
}
//
//
//          _cpri__GetHashAlgByIndex()
//
//      This function is used to iterate through the hashes. TPM_ALG_NULL is returned for all indexes that are
//      not valid hashes. If the TPM implements 3 hashes, then an index value of 0 will return the first
//      implemented hash and and index of 2 will return the last. All other index values will return
//      TPM_ALG_NULL.
//
//
//
//
//      Return Value                      Meaning
//
//      TPM_ALG_xxx()                     a hash algorithm
//      TPM_ALG_NULL                      this can be used as a stop value
//
LIB_EXPORT TPM_ALG_ID
_cpri__GetHashAlgByIndex(
    UINT32               index               // IN: the index
    )
{
    if(index >= HASH_COUNT)
        return TPM_ALG_NULL;
    return g_hashData[index].alg;
}
//
//
//         _cpri__GetHashBlockSize()
//
//      Returns the size of the block used for the hash
//
//      Return Value                      Meaning
//
//      <0                                the algorithm is not a supported hash
//      >=                                the digest size (0 for TPM_ALG_NULL)
//
LIB_EXPORT UINT16
_cpri__GetHashBlockSize(
    TPM_ALG_ID           hashAlg             // IN: hash algorithm to look up
    )
{
    return GetHashInfoPointer(hashAlg)->blockSize;
}
//
//
//         _cpri__GetHashDER
//
//      This function returns a pointer to the DER string for the algorithm and indicates its size.
//
LIB_EXPORT UINT16
_cpri__GetHashDER(
    TPM_ALG_ID           hashAlg,            // IN: the algorithm to look up
    const BYTE          **p
    )
{
    const HASH_INFO       *q;
    q = GetHashInfoPointer(hashAlg);
    *p = &q->der[0];
    return q->derSize;
}
//
//
//         _cpri__GetDigestSize()
//
//      Gets the digest size of the algorithm. The algorithm is required to be supported.
//
//      Return Value                      Meaning
//
//      =0                                the digest size for TPM_ALG_NULL
//      >0                                the digest size of a hash algorithm
//
LIB_EXPORT UINT16
_cpri__GetDigestSize(
    TPM_ALG_ID           hashAlg               // IN: hash algorithm to look up
    )
{
    return GetHashInfoPointer(hashAlg)->digestSize;
}
//
//
//         _cpri__GetContextAlg()
//
//      This function returns the algorithm associated with a hash context
//
LIB_EXPORT TPM_ALG_ID
_cpri__GetContextAlg(
    CPRI_HASH_STATE         *hashState             // IN: the hash context
    )
{
    return hashState->hashAlg;
}
//
//
//         _cpri__CopyHashState
//
//      This function is used to clone a CPRI_HASH_STATE. The return value is the size of the state.
//
LIB_EXPORT UINT16
_cpri__CopyHashState (
    CPRI_HASH_STATE         *out,                  // OUT: destination of the state
    CPRI_HASH_STATE         *in                    // IN: source of the state
    )
{
    OSSL_HASH_STATE    *i = (OSSL_HASH_STATE *)&in->state;
    OSSL_HASH_STATE    *o = (OSSL_HASH_STATE *)&out->state;
    pAssert(sizeof(i) <= sizeof(in->state));
    EVP_MD_CTX_init(&o->u.context);
    EVP_MD_CTX_copy_ex(&o->u.context, &i->u.context);
    o->copySize = i->copySize;
    out->hashAlg = in->hashAlg;
    return sizeof(CPRI_HASH_STATE);
}
//
//
//         _cpri__StartHash()
//
//      Functions starts a hash stack Start a hash stack and returns the digest size. As a side effect, the value of
//      stateSize in hashState is updated to indicate the number of bytes of state that were saved. This function
//      calls GetHashServer() and that function will put the TPM into failure mode if the hash algorithm is not
//      supported.
//
//      Return Value                      Meaning
//
//      0                                 hash is TPM_ALG_NULL
//      >0                                digest size
//
LIB_EXPORT UINT16
_cpri__StartHash(
    TPM_ALG_ID               hashAlg,              // IN: hash algorithm
    BOOL                     sequence,             // IN: TRUE if the state should be saved
    CPRI_HASH_STATE         *hashState             // OUT: the state of hash stack.
    )
{
    EVP_MD_CTX           localState;
   OSSL_HASH_STATE    *state = (OSSL_HASH_STATE *)&hashState->state;
   BYTE               *stateData = state->u.data;
   EVP_MD_CTX         *context;
   EVP_MD             *evpmdAlgorithm = NULL;
   UINT16              retVal = 0;
   if(sequence)
       context = &localState;
   else
       context = &state->u.context;
   hashState->hashAlg = hashAlg;
   EVP_MD_CTX_init(context);
   evpmdAlgorithm = GetHashServer(hashAlg);
   if(evpmdAlgorithm == NULL)
       goto Cleanup;
   if(EVP_DigestInit_ex(context, evpmdAlgorithm, NULL) != 1)
       FAIL(FATAL_ERROR_INTERNAL);
   retVal = (CRYPT_RESULT)EVP_MD_CTX_size(context);
Cleanup:
   if(retVal > 0)
   {
       if (sequence)
       {
            if((state->copySize = MarshalHashState(context, stateData)) == 0)
            {
                // If MarshalHashState returns a negative number, it is an error
                // code and not a hash size so copy the error code to be the return
                // from this function and set the actual stateSize to zero.
                retVal = state->copySize;
                state->copySize = 0;
            }
            // Do the cleanup
            EVP_MD_CTX_cleanup(context);
       }
       else
            state->copySize = -1;
   }
   else
       state->copySize = 0;
   return retVal;
}
//
//
//         _cpri__UpdateHash()
//
//      Add data to a hash or HMAC stack.
//
LIB_EXPORT void
_cpri__UpdateHash(
   CPRI_HASH_STATE           *hashState,      // IN: the hash context information
   UINT32                     dataSize,       // IN: the size of data to be added to the
                                              //     digest
   BYTE                      *data            // IN: data to be hashed
   )
{
   EVP_MD_CTX       localContext;
   OSSL_HASH_STATE *state = (OSSL_HASH_STATE *)&hashState->state;
   BYTE            *stateData = state->u.data;
   EVP_MD_CTX      *context;
   CRYPT_RESULT     retVal = CRYPT_SUCCESS;
//
    // If there is no context, return
    if(state->copySize == 0)
        return;
    if(state->copySize > 0)
    {
        context = &localContext;
        if((retVal = GetHashState(context, hashState->hashAlg, stateData)) <= 0)
            return;
    }
    else
        context = &state->u.context;
    if(EVP_DigestUpdate(context, data, dataSize) != 1)
        FAIL(FATAL_ERROR_INTERNAL);
    else if(    state->copySize > 0
                && (retVal= MarshalHashState(context, stateData)) >= 0)
    {
        // retVal is the size of the marshaled data. Make sure that it is consistent
        // by ensuring that we didn't get more than allowed
        if(retVal < state->copySize)
             FAIL(FATAL_ERROR_INTERNAL);
        else
             EVP_MD_CTX_cleanup(context);
    }
    return;
}
//
//
//       _cpri__CompleteHash()
//
//      Complete a hash or HMAC computation. This function will place the smaller of digestSize or the size of
//      the digest in dOut. The number of bytes in the placed in the buffer is returned. If there is a failure, the
//      returned value is <= 0.
//
//      Return Value                      Meaning
//
//      0                                 no data returned
//      >0                                the number of bytes in the digest
//
LIB_EXPORT UINT16
_cpri__CompleteHash(
    CPRI_HASH_STATE         *hashState,             // IN: the state of hash stack
    UINT32                   dOutSize,              // IN: size of digest buffer
    BYTE                    *dOut                   // OUT: hash digest
    )
{
    EVP_MD_CTX          localState;
    OSSL_HASH_STATE    *state = (OSSL_HASH_STATE *)&hashState->state;
    BYTE               *stateData = state->u.data;
    EVP_MD_CTX         *context;
    UINT16              retVal;
    int                 hLen;
    BYTE                temp[MAX_DIGEST_SIZE];
    BYTE               *rBuffer = dOut;
    if(state->copySize == 0)
        return 0;
    if(state->copySize > 0)
    {
        context = &localState;
        if((retVal = GetHashState(context, hashState->hashAlg, stateData)) <= 0)
            goto Cleanup;
    }
    else
        context = &state->u.context;
   hLen = EVP_MD_CTX_size(context);
   if((unsigned)hLen > dOutSize)
       rBuffer = temp;
   if(EVP_DigestFinal_ex(context, rBuffer, NULL) == 1)
   {
       if(rBuffer != dOut)
       {
            if(dOut != NULL)
            {
                memcpy(dOut, temp, dOutSize);
            }
            retVal = (UINT16)dOutSize;
       }
       else
       {
            retVal = (UINT16)hLen;
       }
       state->copySize = 0;
   }
   else
   {
       retVal = 0; // Indicate that no data is returned
   }
Cleanup:
   EVP_MD_CTX_cleanup(context);
   return retVal;
}
//
//
//       _cpri__ImportExportHashState()
//
//      This function is used to import or export the hash state. This function would be called to export state when
//      a sequence object was being prepared for export
//
LIB_EXPORT void
_cpri__ImportExportHashState(
   CPRI_HASH_STATE           *osslFmt,          // IN/OUT: the hash state formated for use
                                                //     by openSSL
   EXPORT_HASH_STATE         *externalFmt,      // IN/OUT: the exported hash state
   IMPORT_EXPORT              direction         //
   )
{
   UNREFERENCED_PARAMETER(direction);
   UNREFERENCED_PARAMETER(externalFmt);
   UNREFERENCED_PARAMETER(osslFmt);
   return;
#if 0
   if(direction == IMPORT_STATE)
   {
       // don't have the import export functions yet so just copy
       _cpri__CopyHashState(osslFmt, (CPRI_HASH_STATE *)externalFmt);
   }
   else
   {
       _cpri__CopyHashState((CPRI_HASH_STATE *)externalFmt, osslFmt);
   }
#endif
}
//
//
//
//       _cpri__HashBlock()
//
//      Start a hash, hash a single block, update digest and return the size of the results.
//      The digestSize parameter can be smaller than the digest. If so, only the more significant bytes are
//      returned.
//
//      Return Value                      Meaning
//
//      >= 0                              number of bytes in digest (may be zero)
//
LIB_EXPORT UINT16
_cpri__HashBlock(
      TPM_ALG_ID         hashAlg,            //   IN: The hash algorithm
      UINT32             dataSize,           //   IN: size of buffer to hash
      BYTE              *data,               //   IN: the buffer to hash
      UINT32             digestSize,         //   IN: size of the digest buffer
      BYTE              *digest              //   OUT: hash digest
      )
{
      EVP_MD_CTX        hashContext;
      EVP_MD           *hashServer = NULL;
      UINT16            retVal = 0;
      BYTE              b[MAX_DIGEST_SIZE]; // temp buffer in case digestSize not
      // a full digest
      unsigned int      dSize = _cpri__GetDigestSize(hashAlg);
      // If there is no digest to compute return
      if(dSize == 0)
          return 0;
      // After the call to EVP_MD_CTX_init(), will need to call EVP_MD_CTX_cleanup()
      EVP_MD_CTX_init(&hashContext);     // Initialize the local hash context
      hashServer = GetHashServer(hashAlg); // Find the hash server
      // It is an error if the digest size is non-zero but there is no                server
      if(   (hashServer == NULL)
         || (EVP_DigestInit_ex(&hashContext, hashServer, NULL) != 1)
         || (EVP_DigestUpdate(&hashContext, data, dataSize) != 1))
          FAIL(FATAL_ERROR_INTERNAL);
      else
      {
          // If the size of the digest produced (dSize) is larger than                the available
          // buffer (digestSize), then put the digest in a temp buffer                and only copy
          // the most significant part into the available buffer.
          if(dSize > digestSize)
          {
               if(EVP_DigestFinal_ex(&hashContext, b, &dSize) != 1)
                   FAIL(FATAL_ERROR_INTERNAL);
               memcpy(digest, b, digestSize);
               retVal = (UINT16)digestSize;
          }
          else
          {
               if((EVP_DigestFinal_ex(&hashContext, digest, &dSize)) !=               1)
                   FAIL(FATAL_ERROR_INTERNAL);
               retVal = (UINT16) dSize;
          }
      }
      EVP_MD_CTX_cleanup(&hashContext);
      return retVal;
}
//
//
//
//           HMAC Functions
//
//          _cpri__StartHMAC
//
//      This function is used to start an HMAC using a temp hash context. The function does the initialization of
//      the hash with the HMAC key XOR iPad and updates the HMAC key XOR oPad.
//      The function returns the number of bytes in a digest produced by hashAlg.
//
//      Return Value                    Meaning
//
//      >= 0                            number of bytes in digest produced by hashAlg (may be zero)
//
LIB_EXPORT UINT16
_cpri__StartHMAC(
      TPM_ALG_ID              hashAlg,          //   IN: the algorithm to use
      BOOL                    sequence,         //   IN: indicates if the state should be
                                                //       saved
      CPRI_HASH_STATE        *state,            //   IN/OUT: the state buffer
      UINT16                  keySize,          //   IN: the size of the HMAC key
      BYTE                   *key,              //   IN: the HMAC key
      TPM2B                  *oPadKey           //   OUT: the key prepared for the oPad round
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
//
//
//          _cpri_CompleteHMAC()
//
//      This function is called to complete an HMAC. It will finish the current digest, and start a new digest. It will
//      then add the oPadKey and the completed digest and return the results in dOut. It will not return more than
//      dOutSize bytes.
//
//      Return Value                      Meaning
//
//      >= 0                              number of bytes in dOut (may be zero)
//
LIB_EXPORT UINT16
_cpri__CompleteHMAC(
      CPRI_HASH_STATE        *hashState,          //   IN: the state of hash stack
      TPM2B                  *oPadKey,            //   IN: the HMAC key in oPad format
      UINT32                  dOutSize,           //   IN: size of digest buffer
      BYTE                   *dOut                //   OUT: hash digest
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
//
//
//           Mask and Key Generation Functions
//
//          _crypi_MGF1()
//
//      This function performs MGF1 using the selected hash. MGF1 is T(n) = T(n-1) || H(seed || counter). This
//      function returns the length of the mask produced which could be zero if the digest algorithm is not
//      supported
//
//      Return Value                      Meaning
//
//      0                                 hash algorithm not supported
//      >0                                should be the same as mSize
//
LIB_EXPORT CRYPT_RESULT
_cpri__MGF1(
   UINT32              mSize,          //   IN: length of the mask to be produced
   BYTE               *mask,           //   OUT: buffer to receive the mask
   TPM_ALG_ID          hashAlg,        //   IN: hash to use
   UINT32              sSize,          //   IN: size of the seed
   BYTE               *seed            //   IN: seed size
   )
{
   EVP_MD_CTX           hashContext;
   EVP_MD              *hashServer = NULL;
   CRYPT_RESULT         retVal = 0;
   BYTE                 b[MAX_DIGEST_SIZE]; // temp buffer in case mask is not an
   // even multiple of a full digest
   CRYPT_RESULT         dSize = _cpri__GetDigestSize(hashAlg);
   unsigned int         digestSize = (UINT32)dSize;
   UINT32               remaining;
   UINT32               counter;
   BYTE                 swappedCounter[4];
   // Parameter check
   if(mSize > (1024*16)) // Semi-arbitrary maximum
       FAIL(FATAL_ERROR_INTERNAL);
   // If there is no digest to compute return
   if(dSize <= 0)
       return 0;
   EVP_MD_CTX_init(&hashContext);     // Initialize the local hash context
   hashServer = GetHashServer(hashAlg); // Find the hash server
   if(hashServer == NULL)
       // If there is no server, then there is no digest
       return 0;
   for(counter = 0, remaining = mSize; remaining > 0; counter++)
   {
       // Because the system may be either Endian...
       UINT32_TO_BYTE_ARRAY(counter, swappedCounter);
        // Start the hash and include the seed and counter
        if(    (EVP_DigestInit_ex(&hashContext, hashServer, NULL) != 1)
            || (EVP_DigestUpdate(&hashContext, seed, sSize) != 1)
            || (EVP_DigestUpdate(&hashContext, swappedCounter, 4) != 1)
          )
             FAIL(FATAL_ERROR_INTERNAL);
        // Handling the completion depends on how much space remains in the mask
        // buffer. If it can hold the entire digest, put it there. If not
        // put the digest in a temp buffer and only copy the amount that
        // will fit into the mask buffer.
        if(remaining < (unsigned)dSize)
        {
             if(EVP_DigestFinal_ex(&hashContext, b, &digestSize) != 1)
                 FAIL(FATAL_ERROR_INTERNAL);
             memcpy(mask, b, remaining);
             break;
        }
        else
        {
             if(EVP_DigestFinal_ex(&hashContext, mask, &digestSize) != 1)
                 FAIL(FATAL_ERROR_INTERNAL);
             remaining -= dSize;
             mask = &mask[dSize];
        }
        retVal = (CRYPT_RESULT)mSize;
   }
   EVP_MD_CTX_cleanup(&hashContext);
    return retVal;
}
//
//
//          _cpri_KDFa()
//
//      This function performs the key generation according to Part 1 of the TPM specification.
//      This function returns the number of bytes generated which may be zero.
//      The key and keyStream pointers are not allowed to be NULL. The other pointer values may be NULL.
//      The value of sizeInBits must be no larger than (2^18)-1 = 256K bits (32385 bytes).
//      The once parameter is set to allow incremental generation of a large value. If this flag is TRUE,
//      sizeInBits will be used in the HMAC computation but only one iteration of the KDF is performed. This
//      would be used for XOR obfuscation so that the mask value can be generated in digest-sized chunks
//      rather than having to be generated all at once in an arbitrarily large buffer and then XORed() into the
//      result. If once is TRUE, then sizeInBits must be a multiple of 8.
//      Any error in the processing of this command is considered fatal.
//
//      Return Value                      Meaning
//
//      0                                 hash algorithm is not supported or is TPM_ALG_NULL
//      >0                                the number of bytes in the keyStream buffer
//
LIB_EXPORT UINT16
_cpri__KDFa(
    TPM_ALG_ID          hashAlg,             //   IN: hash algorithm used in HMAC
    TPM2B              *key,                 //   IN: HMAC key
    const char         *label,               //   IN: a 0-byte terminated label used in KDF
    TPM2B              *contextU,            //   IN: context U
    TPM2B              *contextV,            //   IN: context V
    UINT32              sizeInBits,          //   IN: size of generated key in bit
    BYTE               *keyStream,           //   OUT: key buffer
    UINT32             *counterInOut,        //   IN/OUT: caller may provide the iteration
                                             //       counter for incremental operations to
                                             //       avoid large intermediate buffers.
    BOOL                once                 //   IN: TRUE if only one iteration is performed
                                             //       FALSE if iteration count determined by
                                             //       "sizeInBits"
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
//
//
//
//         _cpri__KDFe()
//
//      KDFe() as defined in TPM specification part 1.
//      This function returns the number of bytes generated which may be zero.
//      The Z and keyStream pointers are not allowed to be NULL. The other pointer values may be NULL. The
//      value of sizeInBits must be no larger than (2^18)-1 = 256K bits (32385 bytes). Any error in the processing
//      of this command is considered fatal.
//
//      Return Value                      Meaning
//
//      0                                 hash algorithm is not supported or is TPM_ALG_NULL
//      >0                                the number of bytes in the keyStream buffer
//
LIB_EXPORT UINT16
_cpri__KDFe(
    TPM_ALG_ID          hashAlg,             //   IN: hash algorithm used in HMAC
    TPM2B              *Z,                   //   IN: Z
    const char         *label,               //   IN: a 0 terminated label using in KDF
    TPM2B              *partyUInfo,          //   IN: PartyUInfo
    TPM2B              *partyVInfo,          //   IN: PartyVInfo
    UINT32              sizeInBits,          //   IN: size of generated key in bit
    BYTE               *keyStream            //   OUT: key buffer
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
