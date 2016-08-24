// This file was extracted from the TCG Published
// Trusted Platform Module Library
// Part 3: Commands
// Family "2.0"
// Level 00 Revision 01.16
// October 30, 2014

#include "InternalRoutines.h"
#include "PCR_Extend_fp.h"
//
//
//     Error Returns                     Meaning
//
//     TPM_RC_LOCALITY                   current command locality is not allowed to extend the PCR
//                                       referenced by pcrHandle
//
TPM_RC
TPM2_PCR_Extend(
   PCR_Extend_In     *in                 // IN: input parameter list
   )
{
   TPM_RC                  result;
   UINT32                  i;

// Input Validation

   //   NOTE: This function assumes that the unmarshaling function for 'digests' will
   //   have validated that all of the indicated hash algorithms are valid. If the
   //   hash algorithms are correct, the unmarshaling code will unmarshal a digest
   //   of the size indicated by the hash algorithm. If the overall size is not
   //   consistent, the unmarshaling code will run out of input data or have input
   //   data left over. In either case, it will cause an unmarshaling error and this
   //   function will not be called.

   // For NULL handle, do nothing and return success
   if(in->pcrHandle == TPM_RH_NULL)
       return TPM_RC_SUCCESS;

   // Check if the extend operation is allowed by the current command locality
   if(!PCRIsExtendAllowed(in->pcrHandle))
       return TPM_RC_LOCALITY;

   // If PCR is state saved and we need to update orderlyState, check NV
   // availability
   if(PCRIsStateSaved(in->pcrHandle) && gp.orderlyState != SHUTDOWN_NONE)
   {
       result = NvIsAvailable();
       if(result != TPM_RC_SUCCESS) return result;
       g_clearOrderly = TRUE;
   }

// Internal Data Update

   // Iterate input digest list to extend
   for(i = 0; i < in->digests.count; i++)
   {
       PCRExtend(in->pcrHandle, in->digests.digests[i].hashAlg,
                 CryptGetHashDigestSize(in->digests.digests[i].hashAlg),
                 (BYTE *) &in->digests.digests[i].digest);
   }

   return TPM_RC_SUCCESS;
}
