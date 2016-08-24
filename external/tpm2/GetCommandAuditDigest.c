// This file was extracted from the TCG Published
// Trusted Platform Module Library
// Part 3: Commands
// Family "2.0"
// Level 00 Revision 01.16
// October 30, 2014

#include "InternalRoutines.h"
#include "Attest_spt_fp.h"
#include "GetCommandAuditDigest_fp.h"
//
//
//     Error Returns                 Meaning
//
//     TPM_RC_KEY                    key referenced by signHandle is not a signing key
//     TPM_RC_SCHEME                 inScheme is incompatible with signHandle type; or both scheme and
//                                   key's default scheme are empty; or scheme is empty while key's
//                                   default scheme requires explicit input scheme (split signing); or non-
//                                   empty default key scheme differs from scheme
//     TPM_RC_VALUE                  digest generated for the given scheme is greater than the modulus of
//                                   signHandle (for an RSA key); invalid commit status or failed to
//                                   generate r value (for an ECC key)
//
TPM_RC
TPM2_GetCommandAuditDigest(
   GetCommandAuditDigest_In      *in,                // IN: input parameter list
   GetCommandAuditDigest_Out     *out                // OUT: output parameter list
   )
{
   TPM_RC                   result;
   TPMS_ATTEST              auditInfo;

// Command Output

   // Filling in attest information
   // Common fields
   result = FillInAttestInfo(in->signHandle,
                             &in->inScheme,
                             &in->qualifyingData,
                             &auditInfo);
   if(result != TPM_RC_SUCCESS)
   {
       if(result == TPM_RC_KEY)
           return TPM_RC_KEY + RC_GetCommandAuditDigest_signHandle;
       else
           return RcSafeAddToResult(result, RC_GetCommandAuditDigest_inScheme);
   }

   // CommandAuditDigest specific fields
   // Attestation type
   auditInfo.type = TPM_ST_ATTEST_COMMAND_AUDIT;

   // Copy audit hash algorithm
   auditInfo.attested.commandAudit.digestAlg = gp.auditHashAlg;

   // Copy counter value
   auditInfo.attested.commandAudit.auditCounter = gp.auditCounter;

   // Copy command audit log
   auditInfo.attested.commandAudit.auditDigest = gr.commandAuditDigest;
   CommandAuditGetDigest(&auditInfo.attested.commandAudit.commandDigest);

   //   Sign attestation structure. A NULL signature will be returned if
   //   signHandle is TPM_RH_NULL. A TPM_RC_NV_UNAVAILABLE, TPM_RC_NV_RATE,
   //   TPM_RC_VALUE, TPM_RC_SCHEME or TPM_RC_ATTRIBUTES error may be returned at
   //   this point
   result = SignAttestInfo(in->signHandle,
                           &in->inScheme,
                           &auditInfo,
                           &in->qualifyingData,
                           &out->auditInfo,
                           &out->signature);

   if(result != TPM_RC_SUCCESS)
       return result;

// Internal Data Update

   if(in->signHandle != TPM_RH_NULL)
   {
       // Reset log
       gr.commandAuditDigest.t.size = 0;

       // orderly state should be cleared because of the update in
       // commandAuditDigest, as well as the reporting of clock info
       g_clearOrderly = TRUE;
   }

   return TPM_RC_SUCCESS;
}
