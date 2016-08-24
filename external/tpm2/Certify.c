// This file was extracted from the TCG Published
// Trusted Platform Module Library
// Part 3: Commands
// Family "2.0"
// Level 00 Revision 01.16
// October 30, 2014

#include "InternalRoutines.h"
#include "Attest_spt_fp.h"
#include "Certify_fp.h"
//
//
//     Error Returns               Meaning
//
//     TPM_RC_KEY                  key referenced by signHandle is not a signing key
//     TPM_RC_SCHEME               inScheme is not compatible with signHandle
//     TPM_RC_VALUE                digest generated for inScheme is greater or has larger size than the
//                                 modulus of signHandle, or the buffer for the result in signature is too
//                                 small (for an RSA key); invalid commit status (for an ECC key with a
//                                 split scheme).
//
TPM_RC
TPM2_Certify(
   Certify_In      *in,             // IN: input parameter list
   Certify_Out     *out             // OUT: output parameter list
   )
{
   TPM_RC                 result;
   TPMS_ATTEST            certifyInfo;

// Command Output

   // Filling in attest information
   // Common fields
   result = FillInAttestInfo(in->signHandle,
                             &in->inScheme,
                             &in->qualifyingData,
                             &certifyInfo);
   if(result != TPM_RC_SUCCESS)
   {
       if(result == TPM_RC_KEY)
           return TPM_RC_KEY + RC_Certify_signHandle;
       else
           return RcSafeAddToResult(result, RC_Certify_inScheme);
   }
   // Certify specific fields
   // Attestation type
   certifyInfo.type = TPM_ST_ATTEST_CERTIFY;
   // Certified object name
   certifyInfo.attested.certify.name.t.size =
       ObjectGetName(in->objectHandle,
                     &certifyInfo.attested.certify.name.t.name);
   // Certified object qualified name
   ObjectGetQualifiedName(in->objectHandle,
                          &certifyInfo.attested.certify.qualifiedName);

   // Sign attestation structure. A NULL signature will be returned if
   // signHandle is TPM_RH_NULL. A TPM_RC_NV_UNAVAILABLE, TPM_RC_NV_RATE,
   // TPM_RC_VALUE, TPM_RC_SCHEME or TPM_RC_ATTRIBUTES error may be returned
   // by SignAttestInfo()
   result = SignAttestInfo(in->signHandle,
                           &in->inScheme,
                           &certifyInfo,
                           &in->qualifyingData,
                           &out->certifyInfo,
                           &out->signature);

   // TPM_RC_ATTRIBUTES cannot be returned here as FillInAttestInfo would already
   // have returned TPM_RC_KEY
   pAssert(result != TPM_RC_ATTRIBUTES);

   if(result != TPM_RC_SUCCESS)
       return result;

   // orderly state should be cleared because of the reporting of clock info
   // if signing happens
   if(in->signHandle != TPM_RH_NULL)
       g_clearOrderly = TRUE;

   return TPM_RC_SUCCESS;
}
