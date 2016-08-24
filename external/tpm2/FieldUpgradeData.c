// This file was extracted from the TCG Published
// Trusted Platform Module Library
// Part 3: Commands
// Family "2.0"
// Level 00 Revision 01.16
// October 30, 2014

#include "InternalRoutines.h"
#include "FieldUpgradeData_fp.h"
#ifdef TPM_CC_FieldUpgradeData // Conditional expansion of this file
TPM_RC
TPM2_FieldUpgradeData(
   FieldUpgradeData_In       *in,             // IN: input parameter list
   FieldUpgradeData_Out      *out             // OUT: output parameter list
   )
{
   // Not implemented
   UNUSED_PARAMETER(in);
   UNUSED_PARAMETER(out);
   return TPM_RC_SUCCESS;
}
#endif
