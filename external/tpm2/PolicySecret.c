// This file was extracted from the TCG Published
// Trusted Platform Module Library
// Part 3: Commands
// Family "2.0"
// Level 00 Revision 01.16
// October 30, 2014

#include "InternalRoutines.h"
#include "PolicySecret_fp.h"
#include "Policy_spt_fp.h"
//
//
//     Error Returns                 Meaning
//
//     TPM_RC_CPHASH                 cpHash for policy was previously set to a value that is not the same
//                                   as cpHashA
//     TPM_RC_EXPIRED                expiration indicates a time in the past
//     TPM_RC_NONCE                  nonceTPM does not match the nonce associated with policySession
//     TPM_RC_SIZE                   cpHashA is not the size of a digest for the hash associated with
//                                   policySession
//     TPM_RC_VALUE                  input policyID or expiration does not match the internal data in policy
//                                   session
//
TPM_RC
TPM2_PolicySecret(
   PolicySecret_In    *in,                 // IN: input parameter list
   PolicySecret_Out   *out                 // OUT: output parameter list
   )
{
   TPM_RC                  result;
   SESSION                *session;
   TPM2B_NAME              entityName;
   UINT32                  expiration = (in->expiration < 0)
                                        ? -(in->expiration) : in->expiration;
   UINT64                  authTimeout = 0;

// Input Validation

   // Get pointer to the session structure
   session = SessionGet(in->policySession);

   //Only do input validation if this is not a trial policy session
   if(session->attributes.isTrialPolicy == CLEAR)
   {

       if(expiration != 0)
           authTimeout = expiration * 1000 + session->startTime;

       result = PolicyParameterChecks(session, authTimeout,
                                       &in->cpHashA, &in->nonceTPM,
                                       RC_PolicySecret_nonceTPM,
                                       RC_PolicySecret_cpHashA,
                                       RC_PolicySecret_expiration);
       if(result != TPM_RC_SUCCESS)
           return result;
   }

// Internal Data Update
   // Need the name of the authorizing entity
   entityName.t.size = EntityGetName(in->authHandle, &entityName.t.name);

   // Update policy context with input policyRef and name of auth key
   // This value is computed even for trial sessions. Possibly update the cpHash
   PolicyContextUpdate(TPM_CC_PolicySecret, &entityName, &in->policyRef,
                       &in->cpHashA, authTimeout, session);

// Command Output

   // Create ticket and timeout buffer if in->expiration < 0 and this is not
   // a trial session.
   // NOTE: PolicyParameterChecks() makes sure that nonceTPM is present
   // when expiration is non-zero.
   if(   in->expiration < 0
      && session->attributes.isTrialPolicy == CLEAR
     )
   {
       // Generate timeout buffer. The format of output timeout buffer is
       // TPM-specific.
       // Note: can't do a direct copy because the output buffer is a byte
       // array and it may not be aligned to accept a 64-bit value. The method
       // used has the side-effect of making the returned value a big-endian,
       // 64-bit value that is byte aligned.
       out->timeout.t.size = sizeof(UINT64);
       UINT64_TO_BYTE_ARRAY(authTimeout, out->timeout.t.buffer);

       // Compute policy ticket
       TicketComputeAuth(TPM_ST_AUTH_SECRET, EntityGetHierarchy(in->authHandle),
                         authTimeout, &in->cpHashA, &in->policyRef,
                         &entityName, &out->policyTicket);
   }
   else
   {
       // timeout buffer is null
       out->timeout.t.size = 0;

       // auth ticket is null
       out->policyTicket.tag = TPM_ST_AUTH_SECRET;
       out->policyTicket.hierarchy = TPM_RH_NULL;
       out->policyTicket.digest.t.size = 0;
   }

   return TPM_RC_SUCCESS;
}
