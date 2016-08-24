// This file was extracted from the TCG Published
// Trusted Platform Module Library
// Part 3: Commands
// Family "2.0"
// Level 00 Revision 01.16
// October 30, 2014

#include "InternalRoutines.h"
#include "ClockRateAdjust_fp.h"
TPM_RC
TPM2_ClockRateAdjust(
   ClockRateAdjust_In    *in            // IN: input parameter list
   )
{
// Internal Data Update
   TimeSetAdjustRate(in->rateAdjust);

   return TPM_RC_SUCCESS;
}
