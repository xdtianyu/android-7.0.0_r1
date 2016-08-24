// This file was extracted from the TCG Published
// Trusted Platform Module Library
// Part 3: Commands
// Family "2.0"
// Level 00 Revision 01.16
// October 30, 2014

#include "InternalRoutines.h"
//
//     This function is called to process a _TPM_Hash_End() indication.
//
void
_TPM_Hash_End(
   void
   )
{

   UINT32            i;
   TPM2B_DIGEST      digest;
   HASH_OBJECT      *hashObject;
   TPMI_DH_PCR       pcrHandle;

   // If the DRTM handle is not being used, then either _TPM_Hash_Start has not
   // been called, _TPM_Hash_End was previously called, or some other command
   // was executed and the sequence was aborted.
   if(g_DRTMHandle == TPM_RH_UNASSIGNED)
       return;

   // Get DRTM sequence object
   hashObject = (HASH_OBJECT *)ObjectGet(g_DRTMHandle);

   // Is this _TPM_Hash_End after Startup or before
   if(TPMIsStarted())
   {
       // After

         // Reset the DRTM PCR
         PCRResetDynamics();

         // Extend the DRTM_PCR.
         pcrHandle = PCR_FIRST + DRTM_PCR;

         // DRTM sequence increments restartCount
         gr.restartCount++;
   }
   else
   {
       pcrHandle = PCR_FIRST + HCRTM_PCR;
   }

   // Complete hash and extend PCR, or if this is an HCRTM, complete
   // the hash, reset the H-CRTM register (PCR[0]) to 0...04, and then
   // extend the H-CRTM data
   for(i = 0; i < HASH_COUNT; i++)
   {
       TPMI_ALG_HASH       hash = CryptGetHashAlgByIndex(i);
       // make sure that the PCR is implemented for this algorithm
       if(PcrIsAllocated(pcrHandle,
                           hashObject->state.hashState[i].state.hashAlg))
       {
           // Complete hash
           digest.t.size = CryptGetHashDigestSize(hash);
           CryptCompleteHash2B(&hashObject->state.hashState[i], &digest.b);

              PcrDrtm(pcrHandle, hash, &digest);
         }
   }

   // Flush sequence object.
//
   ObjectFlush(g_DRTMHandle);

   g_DRTMHandle = TPM_RH_UNASSIGNED;

   g_DrtmPreStartup = TRUE;

   return;
}
