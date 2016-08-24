// This file was extracted from the TCG Published
// Trusted Platform Module Library
// Part 3: Commands
// Family "2.0"
// Level 00 Revision 01.16
// October 30, 2014

#include "InternalRoutines.h"
#include "VerifySignature_fp.h"
//
//
//     Error Returns                     Meaning
//
//     TPM_RC_ATTRIBUTES                 keyHandle does not reference a signing key
//     TPM_RC_SIGNATURE                  signature is not genuine
//     TPM_RC_SCHEME                     CryptVerifySignature()
//     TPM_RC_HANDLE                     the input handle is references an HMAC key but the private portion is
//                                       not loaded
//
TPM_RC
TPM2_VerifySignature(
   VerifySignature_In        *in,                   // IN: input parameter list
   VerifySignature_Out       *out                   // OUT: output parameter list
   )
{
   TPM_RC                     result;
   TPM2B_NAME                 name;
   OBJECT                    *signObject;
   TPMI_RH_HIERARCHY          hierarchy;

// Input Validation

   // Get sign object pointer
   signObject = ObjectGet(in->keyHandle);

   // The object to validate the signature must be a signing key.
   if(signObject->publicArea.objectAttributes.sign != SET)
       return TPM_RC_ATTRIBUTES + RC_VerifySignature_keyHandle;

   // Validate Signature. TPM_RC_SCHEME, TPM_RC_HANDLE or TPM_RC_SIGNATURE
   // error may be returned by CryptCVerifySignatrue()
   result = CryptVerifySignature(in->keyHandle, &in->digest, &in->signature);
   if(result != TPM_RC_SUCCESS)
       return RcSafeAddToResult(result, RC_VerifySignature_signature);

// Command Output

   hierarchy = ObjectGetHierarchy(in->keyHandle);
   if(   hierarchy == TPM_RH_NULL
      || signObject->publicArea.nameAlg == TPM_ALG_NULL)
   {
       // produce empty ticket if hierarchy is TPM_RH_NULL or nameAlg is
       // TPM_ALG_NULL
       out->validation.tag = TPM_ST_VERIFIED;
       out->validation.hierarchy = TPM_RH_NULL;
       out->validation.digest.t.size = 0;
   }
   else
   {
       // Get object name that verifies the signature
       name.t.size = ObjectGetName(in->keyHandle, &name.t.name);
       // Compute ticket
       TicketComputeVerified(hierarchy, &in->digest, &name, &out->validation);
   }

   return TPM_RC_SUCCESS;
}
