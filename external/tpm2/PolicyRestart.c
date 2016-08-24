// This file was extracted from the TCG Published
// Trusted Platform Module Library
// Part 3: Commands
// Family "2.0"
// Level 00 Revision 01.16
// October 30, 2014

#include "InternalRoutines.h"
#include "PolicyRestart_fp.h"
TPM_RC
TPM2_PolicyRestart(
   PolicyRestart_In      *in              // IN: input parameter list
   )
{
   SESSION                     *session;
   BOOL                         wasTrialSession;

// Internal Data Update

   session = SessionGet(in->sessionHandle);
   wasTrialSession = session->attributes.isTrialPolicy == SET;

   // Initialize policy session
   SessionResetPolicyData(session);

   session->attributes.isTrialPolicy = wasTrialSession;

   return TPM_RC_SUCCESS;
}
