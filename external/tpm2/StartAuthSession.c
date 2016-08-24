// This file was extracted from the TCG Published
// Trusted Platform Module Library
// Part 3: Commands
// Family "2.0"
// Level 00 Revision 01.16
// October 30, 2014

#include "InternalRoutines.h"
#include "StartAuthSession_fp.h"
//
//
//     Error Returns                     Meaning
//
//     TPM_RC_ATTRIBUTES                 tpmKey does not reference a decrypt key
//     TPM_RC_CONTEXT_GAP                the difference between the most recently created active context and
//                                       the oldest active context is at the limits of the TPM
//     TPM_RC_HANDLE                     input decrypt key handle only has public portion loaded
//     TPM_RC_MODE                       symmetric specifies a block cipher but the mode is not
//                                       TPM_ALG_CFB.
//     TPM_RC_SESSION_HANDLES            no session handle is available
//     TPM_RC_SESSION_MEMORY             no more slots for loading a session
//     TPM_RC_SIZE                       nonce less than 16 octets or greater than the size of the digest
//                                       produced by authHash
//     TPM_RC_VALUE                      secret size does not match decrypt key type; or the recovered secret
//                                       is larger than the digest size of the nameAlg of tpmKey; or, for an
//                                       RSA decrypt key, if encryptedSecret is greater than the public
//                                       exponent of tpmKey.
//
TPM_RC
TPM2_StartAuthSession(
   StartAuthSession_In       *in,                   // IN: input parameter buffer
   StartAuthSession_Out      *out                   // OUT: output parameter buffer
   )
{
   TPM_RC                     result = TPM_RC_SUCCESS;
   OBJECT                    *tpmKey;                // TPM key for decrypt salt
   SESSION                   *session;               // session internal data
   TPM2B_DATA                 salt;

// Input Validation

   // Check input nonce size. IT should be at least 16 bytes but not larger
   // than the digest size of session hash.
   if(    in->nonceCaller.t.size < 16
       || in->nonceCaller.t.size > CryptGetHashDigestSize(in->authHash))
       return TPM_RC_SIZE + RC_StartAuthSession_nonceCaller;

   // If an decrypt key is passed in, check its validation
   if(in->tpmKey != TPM_RH_NULL)
   {
       // secret size cannot be 0
       if(in->encryptedSalt.t.size == 0)
           return TPM_RC_VALUE + RC_StartAuthSession_encryptedSalt;

       // Get pointer to loaded decrypt key
       tpmKey = ObjectGet(in->tpmKey);

       // Decrypting salt requires accessing the private portion of a key.
       // Therefore, tmpKey can not be a key with only public portion loaded
       if(tpmKey->attributes.publicOnly)
           return TPM_RC_HANDLE + RC_StartAuthSession_tpmKey;

       // HMAC session input handle check.
         // tpmKey should be a decryption key
         if(tpmKey->publicArea.objectAttributes.decrypt != SET)
             return TPM_RC_ATTRIBUTES + RC_StartAuthSession_tpmKey;

         // Secret Decryption. A TPM_RC_VALUE, TPM_RC_KEY or Unmarshal errors
         // may be returned at this point
         result = CryptSecretDecrypt(in->tpmKey, &in->nonceCaller, "SECRET",
                                     &in->encryptedSalt, &salt);
         if(result != TPM_RC_SUCCESS)
             return TPM_RC_VALUE + RC_StartAuthSession_encryptedSalt;

  }
  else
  {
      // secret size must be 0
      if(in->encryptedSalt.t.size != 0)
          return TPM_RC_VALUE + RC_StartAuthSession_encryptedSalt;
      salt.t.size = 0;
  }
  // If the bind handle references a transient object, make sure that the
  // sensitive area is loaded so that the authValue can be accessed.
  if(     HandleGetType(in->bind) == TPM_HT_TRANSIENT
      && ObjectGet(in->bind)->attributes.publicOnly == SET)
      return TPM_RC_HANDLE + RC_StartAuthSession_bind;

  // If 'symmetric' is a symmetric block cipher (not TPM_ALG_NULL or TPM_ALG_XOR)
  // then the mode must be CFB.
  if(    in->symmetric.algorithm != TPM_ALG_NULL
      && in->symmetric.algorithm != TPM_ALG_XOR
      && in->symmetric.mode.sym != TPM_ALG_CFB)
      return TPM_RC_MODE + RC_StartAuthSession_symmetric;

// Internal Data Update

  // Create internal session structure. TPM_RC_CONTEXT_GAP, TPM_RC_NO_HANDLES
  // or TPM_RC_SESSION_MEMORY errors may be returned returned at this point.
  //
  // The detailed actions for creating the session context are not shown here
  // as the details are implementation dependent
  // SessionCreate sets the output handle
  result = SessionCreate(in->sessionType, in->authHash,
                         &in->nonceCaller, &in->symmetric,
                         in->bind, &salt, &out->sessionHandle);

  if(result != TPM_RC_SUCCESS)
      return result;

// Command Output

  // Get session pointer
  session = SessionGet(out->sessionHandle);

  // Copy nonceTPM
  out->nonceTPM = session->nonceTPM;

   return TPM_RC_SUCCESS;
}
