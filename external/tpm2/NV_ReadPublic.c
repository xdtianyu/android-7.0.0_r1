// This file was extracted from the TCG Published
// Trusted Platform Module Library
// Part 3: Commands
// Family "2.0"
// Level 00 Revision 01.16
// October 30, 2014

#include "InternalRoutines.h"
#include "NV_ReadPublic_fp.h"
TPM_RC
TPM2_NV_ReadPublic(
   NV_ReadPublic_In      *in,          // IN: input parameter list
   NV_ReadPublic_Out     *out          // OUT: output parameter list
   )
{
   NV_INDEX         nvIndex;

// Command Output

   // Get NV index info
   NvGetIndexInfo(in->nvIndex, &nvIndex);

   // Copy data to output
   out->nvPublic.t.nvPublic = nvIndex.publicArea;

   // Compute NV name
   out->nvName.t.size = NvGetName(in->nvIndex, &out->nvName.t.name);

   return TPM_RC_SUCCESS;
}
