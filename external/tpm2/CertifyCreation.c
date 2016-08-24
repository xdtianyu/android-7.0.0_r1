// This file was extracted from the TCG Published
// Trusted Platform Module Library
// Part 3: Commands
// Family "2.0"
// Level 00 Revision 01.16
// October 30, 2014

#include "InternalRoutines.h"
#include "Attest_spt_fp.h"
#include "CertifyCreation_fp.h"
//
//
//     Error Returns               Meaning
//
//     TPM_RC_KEY                  key referenced by signHandle is not a signing key
//     TPM_RC_SCHEME               inScheme is not compatible with signHandle
//     TPM_RC_TICKET               creationTicket does not match objectHandle
//     TPM_RC_VALUE                digest generated for inScheme is greater or has larger size than the
//                                 modulus of signHandle, or the buffer for the result in signature is too
//                                 small (for an RSA key); invalid commit status (for an ECC key with a
//                                 split scheme).
//
TPM_RC
TPM2_CertifyCreation(
   CertifyCreation_In     *in,                // IN: input parameter list
   CertifyCreation_Out    *out                // OUT: output parameter list
   )
{
   TPM_RC                 result;
   TPM2B_NAME             name;
   TPMT_TK_CREATION       ticket;
   TPMS_ATTEST            certifyInfo;

// Input Validation

   // CertifyCreation specific input validation
   // Get certified object name
   name.t.size = ObjectGetName(in->objectHandle, &name.t.name);
   // Re-compute ticket
   TicketComputeCreation(in->creationTicket.hierarchy, &name,
                         &in->creationHash, &ticket);
   // Compare ticket
   if(!Memory2BEqual(&ticket.digest.b, &in->creationTicket.digest.b))
       return TPM_RC_TICKET + RC_CertifyCreation_creationTicket;

// Command Output
   // Common fields
   result = FillInAttestInfo(in->signHandle, &in->inScheme, &in->qualifyingData,
                             &certifyInfo);
   if(result != TPM_RC_SUCCESS)
   {
       if(result == TPM_RC_KEY)
           return TPM_RC_KEY + RC_CertifyCreation_signHandle;
       else
           return RcSafeAddToResult(result, RC_CertifyCreation_inScheme);
   }

   // CertifyCreation specific fields
   // Attestation type
   certifyInfo.type = TPM_ST_ATTEST_CREATION;
   certifyInfo.attested.creation.objectName = name;

   // Copy the creationHash
   certifyInfo.attested.creation.creationHash = in->creationHash;

   // Sign attestation structure.   A NULL signature will be returned if
   // signHandle is TPM_RH_NULL. A TPM_RC_NV_UNAVAILABLE, TPM_RC_NV_RATE,
   // TPM_RC_VALUE, TPM_RC_SCHEME or TPM_RC_ATTRIBUTES error may be returned at
   // this point
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
