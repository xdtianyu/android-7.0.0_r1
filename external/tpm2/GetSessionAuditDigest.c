// This file was extracted from the TCG Published
// Trusted Platform Module Library
// Part 3: Commands
// Family "2.0"
// Level 00 Revision 01.16
// October 30, 2014

#include "InternalRoutines.h"
#include "Attest_spt_fp.h"
#include "GetSessionAuditDigest_fp.h"
//
//
//     Error Returns                 Meaning
//
//     TPM_RC_KEY                    key referenced by signHandle is not a signing key
//     TPM_RC_SCHEME                 inScheme is incompatible with signHandle type; or both scheme and
//                                   key's default scheme are empty; or scheme is empty while key's
//                                   default scheme requires explicit input scheme (split signing); or non-
//                                   empty default key scheme differs from scheme
//     TPM_RC_TYPE                   sessionHandle does not reference an audit session
//     TPM_RC_VALUE                  digest generated for the given scheme is greater than the modulus of
//                                   signHandle (for an RSA key); invalid commit status or failed to
//                                   generate r value (for an ECC key)
//
TPM_RC
TPM2_GetSessionAuditDigest(
   GetSessionAuditDigest_In      *in,                // IN: input parameter list
   GetSessionAuditDigest_Out     *out                // OUT: output parameter list
   )
{
   TPM_RC                  result;
   SESSION                *session;
   TPMS_ATTEST             auditInfo;

// Input Validation

   // SessionAuditDigest specific input validation
   // Get session pointer
   session = SessionGet(in->sessionHandle);

   // session must be an audit session
   if(session->attributes.isAudit == CLEAR)
       return TPM_RC_TYPE + RC_GetSessionAuditDigest_sessionHandle;

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
           return TPM_RC_KEY + RC_GetSessionAuditDigest_signHandle;
       else
           return RcSafeAddToResult(result, RC_GetSessionAuditDigest_inScheme);
   }

   // SessionAuditDigest specific fields
   // Attestation type
   auditInfo.type = TPM_ST_ATTEST_SESSION_AUDIT;

   // Copy digest
   auditInfo.attested.sessionAudit.sessionDigest = session->u2.auditDigest;

   // Exclusive audit session
   if(g_exclusiveAuditSession == in->sessionHandle)
       auditInfo.attested.sessionAudit.exclusiveSession = TRUE;
   else
       auditInfo.attested.sessionAudit.exclusiveSession = FALSE;

   // Sign attestation structure. A NULL signature will be returned if
   // signHandle is TPM_RH_NULL. A TPM_RC_NV_UNAVAILABLE, TPM_RC_NV_RATE,
   // TPM_RC_VALUE, TPM_RC_SCHEME or TPM_RC_ATTRIBUTES error may be returned at
   // this point
   result = SignAttestInfo(in->signHandle,
                           &in->inScheme,
                           &auditInfo,
                           &in->qualifyingData,
                           &out->auditInfo,
                           &out->signature);
   if(result != TPM_RC_SUCCESS)
       return result;

   // orderly state should be cleared because of the reporting of clock info
   // if signing happens
   if(in->signHandle != TPM_RH_NULL)
       g_clearOrderly = TRUE;

   return TPM_RC_SUCCESS;
}
