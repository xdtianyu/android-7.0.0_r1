// This file was extracted from the TCG Published
// Trusted Platform Module Library
// Part 3: Commands
// Family "2.0"
// Level 00 Revision 01.16
// October 30, 2014

#include "InternalRoutines.h"
#include "ActivateCredential_fp.h"
#include "Object_spt_fp.h"
//
//
//     Error Returns                Meaning
//
//     TPM_RC_ATTRIBUTES            keyHandle does not reference a decryption key
//     TPM_RC_ECC_POINT             secret is invalid (when keyHandle is an ECC key)
//     TPM_RC_INSUFFICIENT          secret is invalid (when keyHandle is an ECC key)
//     TPM_RC_INTEGRITY             credentialBlob fails integrity test
//     TPM_RC_NO_RESULT             secret is invalid (when keyHandle is an ECC key)
//     TPM_RC_SIZE                  secret size is invalid or the credentialBlob does not unmarshal
//                                  correctly
//     TPM_RC_TYPE                  keyHandle does not reference an asymmetric key.
//     TPM_RC_VALUE                 secret is invalid (when keyHandle is an RSA key)
//
TPM_RC
TPM2_ActivateCredential(
   ActivateCredential_In    *in,                 // IN: input parameter list
   ActivateCredential_Out   *out                 // OUT: output parameter list
   )
{
   TPM_RC                        result = TPM_RC_SUCCESS;
   OBJECT                       *object;        // decrypt key
   OBJECT                       *activateObject;// key associated with
   // credential
   TPM2B_DATA                      data;              // credential data

// Input Validation

   // Get decrypt key pointer
   object = ObjectGet(in->keyHandle);

   // Get certificated object pointer
   activateObject = ObjectGet(in->activateHandle);

   // input decrypt key must be an asymmetric, restricted decryption key
   if(   !CryptIsAsymAlgorithm(object->publicArea.type)
      || object->publicArea.objectAttributes.decrypt == CLEAR
      || object->publicArea.objectAttributes.restricted == CLEAR)
       return TPM_RC_TYPE + RC_ActivateCredential_keyHandle;

// Command output

   // Decrypt input credential data via asymmetric decryption. A
   // TPM_RC_VALUE, TPM_RC_KEY or unmarshal errors may be returned at this
   // point
   result = CryptSecretDecrypt(in->keyHandle, NULL,
                               "IDENTITY", &in->secret, &data);
   if(result != TPM_RC_SUCCESS)
   {
       if(result == TPM_RC_KEY)
           return TPM_RC_FAILURE;
       return RcSafeAddToResult(result, RC_ActivateCredential_secret);
   }

   // Retrieve secret data. A TPM_RC_INTEGRITY error or unmarshal
   // errors may be returned at this point
   result = CredentialToSecret(&in->credentialBlob,
                               &activateObject->name,
                               (TPM2B_SEED *) &data,
                               in->keyHandle,
                               &out->certInfo);
   if(result != TPM_RC_SUCCESS)
       return RcSafeAddToResult(result,RC_ActivateCredential_credentialBlob);

   return TPM_RC_SUCCESS;
}
