// This file was extracted from the TCG Published
// Trusted Platform Module Library
// Part 4: Supporting Routines
// Family "2.0"
// Level 00 Revision 01.16
// October 30, 2014

#include "InternalRoutines.h"
#include "NV_spt_fp.h"
//
//
//           Fuctions
//
//          NvReadAccessChecks()
//
//      Common routine for validating a read Used by TPM2_NV_Read(), TPM2_NV_ReadLock() and
//      TPM2_PolicyNV()
//
//     Error Returns                     Meaning
//
//     TPM_RC_NV_AUTHORIZATION           autHandle is not allowed to authorize read of the index
//     TPM_RC_NV_LOCKED                  Read locked
//     TPM_RC_NV_UNINITIALIZED           Try to read an uninitialized index
//
TPM_RC
NvReadAccessChecks(
   TPM_HANDLE          authHandle,             // IN: the handle that provided the
                                               //     authorization
   TPM_HANDLE          nvHandle                // IN: the handle of the NV index to be written
   )
{
   NV_INDEX            nvIndex;
   // Get NV index info
   NvGetIndexInfo(nvHandle, &nvIndex);
// This check may be done before doing authorization checks as is done in this
// version of the reference code. If not done there, then uncomment the next
// three lines.
//    // If data is read locked, returns an error
//    if(nvIndex.publicArea.attributes.TPMA_NV_READLOCKED == SET)
//        return TPM_RC_NV_LOCKED;
   // If the authorization was provided by the owner or platform, then check
   // that the attributes allow the read. If the authorization handle
   // is the same as the index, then the checks were made when the authorization
   // was checked..
   if(authHandle == TPM_RH_OWNER)
   {
       // If Owner provided auth then ONWERWRITE must be SET
       if(! nvIndex.publicArea.attributes.TPMA_NV_OWNERREAD)
           return TPM_RC_NV_AUTHORIZATION;
   }
   else if(authHandle == TPM_RH_PLATFORM)
   {
       // If Platform provided auth then PPWRITE must be SET
       if(!nvIndex.publicArea.attributes.TPMA_NV_PPREAD)
           return TPM_RC_NV_AUTHORIZATION;
   }
   // If neither Owner nor Platform provided auth, make sure that it was
   // provided by this index.
   else if(authHandle != nvHandle)
           return TPM_RC_NV_AUTHORIZATION;
   // If the index has not been written, then the value cannot be read
   // NOTE: This has to come after other access checks to make sure that
   // the proper authorization is given to TPM2_NV_ReadLock()
   if(nvIndex.publicArea.attributes.TPMA_NV_WRITTEN == CLEAR)
       return TPM_RC_NV_UNINITIALIZED;
   return TPM_RC_SUCCESS;
}
//
//
//         NvWriteAccessChecks()
//
//     Common routine for validating a write               Used    by    TPM2_NV_Write(),          TPM2_NV_Increment(),
//     TPM2_SetBits(), and TPM2_NV_WriteLock()
//
//
//
//
//     Error Returns                  Meaning
//
//     TPM_RC_NV_AUTHORIZATION        Authorization fails
//     TPM_RC_NV_LOCKED               Write locked
//
TPM_RC
NvWriteAccessChecks(
     TPM_HANDLE        authHandle,           // IN: the handle that provided the
                                             //     authorization
     TPM_HANDLE        nvHandle              // IN: the handle of the NV index to be written
     )
{
     NV_INDEX          nvIndex;
     // Get NV index info
     NvGetIndexInfo(nvHandle, &nvIndex);
// This check may be done before doing authorization checks as is done in this
// version of the reference code. If not done there, then uncomment the next
// three lines.
//    // If data is write locked, returns an error
//    if(nvIndex.publicArea.attributes.TPMA_NV_WRITELOCKED == SET)
//        return TPM_RC_NV_LOCKED;
     // If the authorization was provided by the owner or platform, then check
     // that the attributes allow the write. If the authorization handle
     // is the same as the index, then the checks were made when the authorization
     // was checked..
     if(authHandle == TPM_RH_OWNER)
     {
         // If Owner provided auth then ONWERWRITE must be SET
         if(! nvIndex.publicArea.attributes.TPMA_NV_OWNERWRITE)
             return TPM_RC_NV_AUTHORIZATION;
     }
     else if(authHandle == TPM_RH_PLATFORM)
     {
         // If Platform provided auth then PPWRITE must be SET
         if(!nvIndex.publicArea.attributes.TPMA_NV_PPWRITE)
             return TPM_RC_NV_AUTHORIZATION;
     }
     // If neither Owner nor Platform provided auth, make sure that it was
     // provided by this index.
     else if(authHandle != nvHandle)
             return TPM_RC_NV_AUTHORIZATION;
     return TPM_RC_SUCCESS;
}
