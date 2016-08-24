// This file was extracted from the TCG Published
// Trusted Platform Module Library
// Part 3: Commands
// Family "2.0"
// Level 00 Revision 01.16
// October 30, 2014

#include "InternalRoutines.h"
#include "FieldUpgradeStart_fp.h"
#ifdef TPM_CC_FieldUpgradeStart // Conditional expansion of this file
TPM_RC
TPM2_FieldUpgradeStart(
   FieldUpgradeStart_In     *in             // IN: input parameter list
   )
{
   // Not implemented
   UNUSED_PARAMETER(in);
   return TPM_RC_SUCCESS;
}
#endif
