// This file was extracted from the TCG Published
// Trusted Platform Module Library
// Part 3: Commands
// Family "2.0"
// Level 00 Revision 01.16
// October 30, 2014

#include "InternalRoutines.h"
#include "FirmwareRead_fp.h"
TPM_RC
TPM2_FirmwareRead(
   FirmwareRead_In    *in,            // IN: input parameter list
   FirmwareRead_Out   *out            // OUT: output parameter list
   )
{
   // Not implemented
   UNUSED_PARAMETER(in);
   UNUSED_PARAMETER(out);
   return TPM_RC_SUCCESS;
}
