// This file was extracted from the TCG Published
// Trusted Platform Module Library
// Part 3: Commands
// Family "2.0"
// Level 00 Revision 01.16
// October 30, 2014

#include "InternalRoutines.h"
#include "SetCommandCodeAuditStatus_fp.h"
TPM_RC
TPM2_SetCommandCodeAuditStatus(
   SetCommandCodeAuditStatus_In      *in             // IN: input parameter list
   )
{
   TPM_RC          result;
   UINT32          i;
   BOOL            changed = FALSE;

   // The command needs NV update. Check if NV is available.
   // A TPM_RC_NV_UNAVAILABLE or TPM_RC_NV_RATE error may be returned at
   // this point
   result = NvIsAvailable();
   if(result != TPM_RC_SUCCESS)
       return result;

// Internal Data Update

   // Update hash algorithm
   if(   in->auditAlg != TPM_ALG_NULL
      && in->auditAlg != gp.auditHashAlg)
   {
       // Can't change the algorithm and command list at the same time
       if(in->setList.count != 0 || in->clearList.count != 0)
           return TPM_RC_VALUE + RC_SetCommandCodeAuditStatus_auditAlg;

       // Change the hash algorithm for audit
       gp.auditHashAlg = in->auditAlg;

       // Set the digest size to a unique value that indicates that the digest
       // algorithm has been changed. The size will be cleared to zero in the
       // command audit processing on exit.
       gr.commandAuditDigest.t.size = 1;

       // Save the change of command audit data (this sets g_updateNV so that NV
       // will be updated on exit.)
       NvWriteReserved(NV_AUDIT_HASH_ALG, &gp.auditHashAlg);

   } else {

       // Process set list
       for(i = 0; i < in->setList.count; i++)

            // If change is made in CommandAuditSet, set changed flag
            if(CommandAuditSet(in->setList.commandCodes[i]))
                changed = TRUE;

       // Process clear list
       for(i = 0; i < in->clearList.count; i++)
           // If change is made in CommandAuditClear, set changed flag
           if(CommandAuditClear(in->clearList.commandCodes[i]))
               changed = TRUE;

       // if change was made to command list, update NV
       if(changed)
           // this sets g_updateNV so that NV will be updated on exit.
           NvWriteReserved(NV_AUDIT_COMMANDS, &gp.auditComands);
   }

   return TPM_RC_SUCCESS;
}
