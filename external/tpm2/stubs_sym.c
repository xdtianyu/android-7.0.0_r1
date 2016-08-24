/* This file includes functions that were extracted from the TPM2
 * source, but were present in files not included in compilation.
 */
#include "Global.h"
#include "CryptoEngine.h"

INT16 _cpri__GetSymmetricBlockSize(
  TPM_ALG_ID symmetricAlg,      // IN: the symmetric algorithm
  UINT16 keySizeInBits          // IN: the key size
  )
{
   switch (symmetricAlg)
   {
#ifdef TPM_ALG_AES
   case TPM_ALG_AES:
#endif
#ifdef TPM_ALG_SM4 // Both AES and SM4 use the same block size
   case TPM_ALG_SM4:
#endif
       if(keySizeInBits != 0) // This is mostly to have a reference to
              // keySizeInBits for the compiler
              return 16;
         else
             return 0;
         break;
    default:
        return 0;
    }
}
