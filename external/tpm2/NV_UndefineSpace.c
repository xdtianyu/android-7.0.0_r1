// This file was extracted from the TCG Published
// Trusted Platform Module Library
// Part 3: Commands
// Family "2.0"
// Level 00 Revision 01.16
// October 30, 2014

#include "InternalRoutines.h"
#include "NV_UndefineSpace_fp.h"
//
//
//     Error Returns                     Meaning
//
//     TPM_RC_ATTRIBUTES                 TPMA_NV_POLICY_DELETE is SET in the Index referenced by
//                                       nvIndex so this command may not be used to delete this Index (see
//                                       TPM2_NV_UndefineSpaceSpecial())
//     TPM_RC_NV_AUTHORIZATION           attempt to use ownerAuth to delete an index created by the platform
//
TPM_RC
TPM2_NV_UndefineSpace(
   NV_UndefineSpace_In       *in                   // IN: input parameter list
   )
{
   TPM_RC            result;
   NV_INDEX          nvIndex;

   // The command needs NV update. Check if NV is available.
   // A TPM_RC_NV_UNAVAILABLE or TPM_RC_NV_RATE error may be returned at
   // this point
   result = NvIsAvailable();
   if(result != TPM_RC_SUCCESS) return result;

// Input Validation

   // Get NV index info
   NvGetIndexInfo(in->nvIndex, &nvIndex);

   // This command can't be used to delete an index with TPMA_NV_POLICY_DELETE SET
   if(SET == nvIndex.publicArea.attributes.TPMA_NV_POLICY_DELETE)
       return TPM_RC_ATTRIBUTES + RC_NV_UndefineSpace_nvIndex;

   // The owner may only delete an index that was defined with ownerAuth. The
   // platform may delete an index that was created with either auth.
   if(   in->authHandle == TPM_RH_OWNER
      && nvIndex.publicArea.attributes.TPMA_NV_PLATFORMCREATE == SET)
       return TPM_RC_NV_AUTHORIZATION;

// Internal Data Update

   // Call implementation dependent internal routine to delete NV index
   NvDeleteEntity(in->nvIndex);

   return TPM_RC_SUCCESS;
}
