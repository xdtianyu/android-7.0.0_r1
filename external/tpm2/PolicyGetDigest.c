// This file was extracted from the TCG Published
// Trusted Platform Module Library
// Part 3: Commands
// Family "2.0"
// Level 00 Revision 01.16
// October 30, 2014

#include "InternalRoutines.h"
#include "PolicyGetDigest_fp.h"
TPM_RC
TPM2_PolicyGetDigest(
   PolicyGetDigest_In        *in,             // IN: input parameter list
   PolicyGetDigest_Out       *out             // OUT: output parameter list
   )
{
   SESSION      *session;

// Command Output

   // Get pointer to the session structure
   session = SessionGet(in->policySession);

   out->policyDigest = session->u2.policyDigest;

   return TPM_RC_SUCCESS;
}
