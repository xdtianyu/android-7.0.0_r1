// This file was extracted from the TCG Published
// Trusted Platform Module Library
// Part 3: Commands
// Family "2.0"
// Level 00 Revision 01.16
// October 30, 2014

#include "InternalRoutines.h"
#include "PCR_Read_fp.h"
TPM_RC
TPM2_PCR_Read(
   PCR_Read_In      *in,            // IN: input parameter list
   PCR_Read_Out     *out            // OUT: output parameter list
   )
{
// Command Output

   // Call PCR read function. input pcrSelectionIn parameter could be changed
   // to reflect the actual PCR being returned
   PCRRead(&in->pcrSelectionIn, &out->pcrValues, &out->pcrUpdateCounter);

   out->pcrSelectionOut = in->pcrSelectionIn;

   return TPM_RC_SUCCESS;
}
