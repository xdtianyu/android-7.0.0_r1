// This file was extracted from the TCG Published
// Trusted Platform Module Library
// Part 3: Commands
// Family "2.0"
// Level 00 Revision 01.16
// October 30, 2014

#include "InternalRoutines.h"
#include "PolicyTicket_fp.h"
#include "Policy_spt_fp.h"
//
//
//     Error Returns                Meaning
//
//     TPM_RC_CPHASH                policy's cpHash was previously set to a different value
//     TPM_RC_EXPIRED               timeout value in the ticket is in the past and the ticket has expired
//     TPM_RC_SIZE                  timeout or cpHash has invalid size for the
//     TPM_RC_TICKET                ticket is not valid
//
TPM_RC
TPM2_PolicyTicket(
   PolicyTicket_In    *in                   // IN: input parameter list
   )
{
   TPM_RC                    result;
   SESSION                  *session;
   UINT64                    timeout;
   TPMT_TK_AUTH              ticketToCompare;
   TPM_CC                    commandCode = TPM_CC_PolicySecret;

// Input Validation

   // Get pointer to the session structure
   session = SessionGet(in->policySession);

   // NOTE: A trial policy session is not allowed to use this command.
   // A ticket is used in place of a previously given authorization. Since
   // a trial policy doesn't actually authenticate, the validated
   // ticket is not necessary and, in place of using a ticket, one
   // should use the intended authorization for which the ticket
   // would be a substitute.
   if(session->attributes.isTrialPolicy)
       return TPM_RC_ATTRIBUTES + RC_PolicyTicket_policySession;

   // Restore timeout data. The format of timeout buffer is TPM-specific.
   // In this implementation, we simply copy the value of timeout to the
   // buffer.
   if(in->timeout.t.size != sizeof(UINT64))
       return TPM_RC_SIZE + RC_PolicyTicket_timeout;
   timeout = BYTE_ARRAY_TO_UINT64(in->timeout.t.buffer);

   // Do the normal checks on the cpHashA and timeout values
   result = PolicyParameterChecks(session, timeout,
                                  &in->cpHashA, NULL,
                                  0,                       // no bad nonce return
                                  RC_PolicyTicket_cpHashA,
                                  RC_PolicyTicket_timeout);
   if(result != TPM_RC_SUCCESS)
       return result;

   // Validate Ticket
   // Re-generate policy ticket by input parameters
   TicketComputeAuth(in->ticket.tag, in->ticket.hierarchy, timeout, &in->cpHashA,
                     &in->policyRef, &in->authName, &ticketToCompare);

   // Compare generated digest with input ticket digest
   if(!Memory2BEqual(&in->ticket.digest.b, &ticketToCompare.digest.b))
       return TPM_RC_TICKET + RC_PolicyTicket_ticket;

// Internal Data Update

   // Is this ticket to take the place of a TPM2_PolicySigned() or
   // a TPM2_PolicySecret()?
   if(in->ticket.tag == TPM_ST_AUTH_SIGNED)
       commandCode = TPM_CC_PolicySigned;
   else if(in->ticket.tag == TPM_ST_AUTH_SECRET)
       commandCode = TPM_CC_PolicySecret;
   else
       // There could only be two possible tag values. Any other value should
       // be caught by the ticket validation process.
       pAssert(FALSE);

   // Update policy context
   PolicyContextUpdate(commandCode, &in->authName, &in->policyRef,
                       &in->cpHashA, timeout, session);

   return TPM_RC_SUCCESS;
}
