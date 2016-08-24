// This file was extracted from the TCG Published
// Trusted Platform Module Library
// Part 3: Commands
// Family "2.0"
// Level 00 Revision 01.16
// October 30, 2014

#include "InternalRoutines.h"
#include "EncryptDecrypt_fp.h"
//
//
//     Error Returns                   Meaning
//
//     TPM_RC_KEY                      is not a symmetric decryption key with both public and private
//                                     portions loaded
//     TPM_RC_SIZE                     IvIn size is incompatible with the block cipher mode; or inData size is
//                                     not an even multiple of the block size for CBC or ECB mode
//     TPM_RC_VALUE                    keyHandle is restricted and the argument mode does not match the
//                                     key's mode
//
TPM_RC
TPM2_EncryptDecrypt(
   EncryptDecrypt_In    *in,                 // IN: input parameter list
   EncryptDecrypt_Out   *out                 // OUT: output parameter list
   )
{
   OBJECT               *symKey;
   UINT16               keySize;
   UINT16               blockSize;
   BYTE                 *key;
   TPM_ALG_ID           alg;

// Input Validation
   symKey = ObjectGet(in->keyHandle);

   // The input key should be a symmetric decrypt key.
   if(    symKey->publicArea.type != TPM_ALG_SYMCIPHER
      || symKey->attributes.publicOnly == SET)
       return TPM_RC_KEY + RC_EncryptDecrypt_keyHandle;

   // If the input mode is TPM_ALG_NULL, use the key's mode
   if( in->mode == TPM_ALG_NULL)
       in->mode = symKey->publicArea.parameters.symDetail.sym.mode.sym;

   // If the key is restricted, the input symmetric mode should match the key's
   // symmetric mode
   if(   symKey->publicArea.objectAttributes.restricted == SET
      && symKey->publicArea.parameters.symDetail.sym.mode.sym != in->mode)
       return TPM_RC_VALUE + RC_EncryptDecrypt_mode;

   // If the mode is null, then we have a problem.
   // Note: Construction of a TPMT_SYM_DEF does not allow the 'mode' to be
   // TPM_ALG_NULL so setting in->mode to the mode of the key should have
   // produced a valid mode. However, this is suspenders.
   if(in->mode == TPM_ALG_NULL)
       return TPM_RC_VALUE + RC_EncryptDecrypt_mode;

   // The input iv for ECB mode should be null. All the other modes should
   // have an iv size same as encryption block size

   keySize = symKey->publicArea.parameters.symDetail.sym.keyBits.sym;
   alg = symKey->publicArea.parameters.symDetail.sym.algorithm;
   blockSize = CryptGetSymmetricBlockSize(alg, keySize);
   if(   (in->mode == TPM_ALG_ECB && in->ivIn.t.size != 0)
      || (in->mode != TPM_ALG_ECB && in->ivIn.t.size != blockSize))
       return TPM_RC_SIZE + RC_EncryptDecrypt_ivIn;

   // The input data size of CBC mode or ECB mode must be an even multiple of
   // the symmetric algorithm's block size
   if(   (in->mode == TPM_ALG_CBC || in->mode == TPM_ALG_ECB)
      && (in->inData.t.size % blockSize) != 0)
       return TPM_RC_SIZE + RC_EncryptDecrypt_inData;

   // Copy IV
   // Note: This is copied here so that the calls to the encrypt/decrypt functions
   // will modify the output buffer, not the input buffer
   out->ivOut = in->ivIn;

// Command Output

   key = symKey->sensitive.sensitive.sym.t.buffer;
   // For symmetric encryption, the cipher data size is the same as plain data
   // size.
   out->outData.t.size = in->inData.t.size;
   if(in->decrypt == YES)
   {
       // Decrypt data to output
       CryptSymmetricDecrypt(out->outData.t.buffer,
                             alg,
                             keySize, in->mode, key,
                             &(out->ivOut),
                             in->inData.t.size,
                             in->inData.t.buffer);
   }
   else
   {
       // Encrypt data to output
       CryptSymmetricEncrypt(out->outData.t.buffer,
                             alg,
                             keySize,
                             in->mode, key,
                             &(out->ivOut),
                             in->inData.t.size,
                             in->inData.t.buffer);
   }

   return TPM_RC_SUCCESS;
}
