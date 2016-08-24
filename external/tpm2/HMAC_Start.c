// This file was extracted from the TCG Published
// Trusted Platform Module Library
// Part 3: Commands
// Family "2.0"
// Level 00 Revision 01.16
// October 30, 2014

#include "InternalRoutines.h"
#include "HMAC_Start_fp.h"
//
//
//     Error Returns                     Meaning
//
//     TPM_RC_ATTRIBUTES                 key referenced by handle is not a signing key or is restricted
//     TPM_RC_OBJECT_MEMORY              no space to create an internal object
//     TPM_RC_KEY                        key referenced by handle is not an HMAC key
//     TPM_RC_VALUE                      hashAlg is not compatible with the hash algorithm of the scheme of
//                                       the object referenced by handle
//
TPM_RC
TPM2_HMAC_Start(
   HMAC_Start_In     *in,                 // IN: input parameter list
   HMAC_Start_Out    *out                 // OUT: output parameter list
   )
{
   OBJECT                    *hmacObject;
   TPMT_PUBLIC               *publicArea;
   TPM_ALG_ID                 hashAlg;

// Input Validation

   // Get HMAC key object and public area pointers
   hmacObject = ObjectGet(in->handle);
   publicArea = &hmacObject->publicArea;

   // Make sure that the key is an HMAC key
   if(publicArea->type != TPM_ALG_KEYEDHASH)
       return TPM_RC_TYPE + RC_HMAC_Start_handle;

   // and that it is unrestricted
   if(publicArea->objectAttributes.restricted == SET)
       return TPM_RC_ATTRIBUTES + RC_HMAC_Start_handle;

   // and that it is a signing key
   if(publicArea->objectAttributes.sign != SET)
       return TPM_RC_KEY + RC_HMAC_Start_handle;

   // See if the key has a default
   if(publicArea->parameters.keyedHashDetail.scheme.scheme == TPM_ALG_NULL)
       // it doesn't so use the input value
       hashAlg = in->hashAlg;
   else
   {
       // key has a default so use it
       hashAlg
           = publicArea->parameters.keyedHashDetail.scheme.details.hmac.hashAlg;
       // and verify that the input was either the TPM_ALG_NULL or the default
       if(in->hashAlg != TPM_ALG_NULL && in->hashAlg != hashAlg)
           hashAlg = TPM_ALG_NULL;
   }
   // if we ended up without a hash algorith then return an error
   if(hashAlg == TPM_ALG_NULL)
       return TPM_RC_VALUE + RC_HMAC_Start_hashAlg;

// Internal Data Update

  // Create a HMAC sequence object. A TPM_RC_OBJECT_MEMORY error may be
  // returned at this point
  return ObjectCreateHMACSequence(hashAlg,
                                  in->handle,
                                  &in->auth,
                                  &out->sequenceHandle);
}
