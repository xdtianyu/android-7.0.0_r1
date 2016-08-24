// This file was extracted from the TCG Published
// Trusted Platform Module Library
// Part 3: Commands
// Family "2.0"
// Level 00 Revision 01.16
// October 30, 2014

#include "InternalRoutines.h"
#include "TestParms_fp.h"
TPM_RC
TPM2_TestParms(
   TestParms_In   *in             // IN: input parameter list
   )
{
   // Input parameter is not reference in command action
   in = NULL;

   // The parameters are tested at unmarshal process.   We do nothing in command
   // action
   return TPM_RC_SUCCESS;
}
