// This file was extracted from the TCG Published
// Trusted Platform Module Library
// Part 3: Commands
// Family "2.0"
// Level 00 Revision 01.16
// October 30, 2014

#include "InternalRoutines.h"
#include "MakeCredential_fp.h"
#include "Object_spt_fp.h"
//
//
//     Error Returns                   Meaning
//
//     TPM_RC_KEY                      handle referenced an ECC key that has a unique field that is not a
//                                     point on the curve of the key
//     TPM_RC_SIZE                     credential is larger than the digest size of Name algorithm of handle
//     TPM_RC_TYPE                     handle does not reference an asymmetric decryption key
//
TPM_RC
TPM2_MakeCredential(
   MakeCredential_In    *in,                 // IN: input parameter list
   MakeCredential_Out   *out                 // OUT: output parameter list
   )
{
   TPM_RC                    result = TPM_RC_SUCCESS;

   OBJECT                    *object;
   TPM2B_DATA                data;

// Input Validation

   // Get object pointer
   object = ObjectGet(in->handle);

   // input key must be an asymmetric, restricted decryption key
   // NOTE: Needs to be restricted to have a symmetric value.
   if(   !CryptIsAsymAlgorithm(object->publicArea.type)
      || object->publicArea.objectAttributes.decrypt == CLEAR
      || object->publicArea.objectAttributes.restricted == CLEAR
     )
       return TPM_RC_TYPE + RC_MakeCredential_handle;

   // The credential information may not be larger than the digest size used for
   // the Name of the key associated with handle.
   if(in->credential.t.size > CryptGetHashDigestSize(object->publicArea.nameAlg))
       return TPM_RC_SIZE + RC_MakeCredential_credential;

// Command Output

   // Make encrypt key and its associated secret structure.
   // Even though CrypeSecretEncrypt() may return
   out->secret.t.size = sizeof(out->secret.t.secret);
   result = CryptSecretEncrypt(in->handle, "IDENTITY", &data, &out->secret);
   if(result != TPM_RC_SUCCESS)
       return result;

   // Prepare output credential data from secret
   SecretToCredential(&in->credential, &in->objectName, (TPM2B_SEED *) &data,
                      in->handle, &out->credentialBlob);

   return TPM_RC_SUCCESS;
}
