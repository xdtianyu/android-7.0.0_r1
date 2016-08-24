// This file was extracted from the TCG Published
// Trusted Platform Module Library
// Part 3: Commands
// Family "2.0"
// Level 00 Revision 01.16
// October 30, 2014

#include "InternalRoutines.h"
#include "NV_UndefineSpaceSpecial_fp.h"
//
//
//     Error Returns                    Meaning
//
//     TPM_RC_ATTRIBUTES                TPMA_NV_POLICY_DELETE is not SET in the Index referenced by
//                                      nvIndex
//
TPM_RC
TPM2_NV_UndefineSpaceSpecial(
   NV_UndefineSpaceSpecial_In      *in              // IN: input parameter list
   )
{
   TPM_RC            result;
   NV_INDEX          nvIndex;

   // The command needs NV update. Check if NV is available.
   // A TPM_RC_NV_UNAVAILABLE or TPM_RC_NV_RATE error may be returned at
   // this point
   result = NvIsAvailable();
   if(result != TPM_RC_SUCCESS)
       return result;

// Input Validation

   // Get NV index info
   NvGetIndexInfo(in->nvIndex, &nvIndex);

   // This operation only applies when the TPMA_NV_POLICY_DELETE attribute is SET
   if(CLEAR == nvIndex.publicArea.attributes.TPMA_NV_POLICY_DELETE)
       return TPM_RC_ATTRIBUTES + RC_NV_UndefineSpaceSpecial_nvIndex;

// Internal Data Update

   // Call implementation dependent internal routine to delete NV index
   NvDeleteEntity(in->nvIndex);

   return TPM_RC_SUCCESS;
}
