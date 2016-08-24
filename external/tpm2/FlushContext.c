// This file was extracted from the TCG Published
// Trusted Platform Module Library
// Part 3: Commands
// Family "2.0"
// Level 00 Revision 01.16
// October 30, 2014

#include "InternalRoutines.h"
#include "FlushContext_fp.h"
//
//
//     Error Returns                     Meaning
//
//     TPM_RC_HANDLE                     flushHandle does not reference a loaded object or session
//
TPM_RC
TPM2_FlushContext(
   FlushContext_In       *in                  // IN: input parameter list
   )
{
// Internal Data Update

   // Call object or session specific routine to flush
   switch(HandleGetType(in->flushHandle))
   {
   case TPM_HT_TRANSIENT:
       if(!ObjectIsPresent(in->flushHandle))
           return TPM_RC_HANDLE;
       // Flush object
       ObjectFlush(in->flushHandle);
       break;
   case TPM_HT_HMAC_SESSION:
   case TPM_HT_POLICY_SESSION:
       if(   !SessionIsLoaded(in->flushHandle)
          && !SessionIsSaved(in->flushHandle)
         )
           return TPM_RC_HANDLE;

       // If the session to be flushed is the exclusive audit session, then
       // indicate that there is no exclusive audit session any longer.
       if(in->flushHandle == g_exclusiveAuditSession)
           g_exclusiveAuditSession = TPM_RH_UNASSIGNED;

       // Flush session
       SessionFlush(in->flushHandle);
       break;
   default:
       // This command only take object or session handle.              Other handles
       // should be filtered out at handle unmarshal
       pAssert(FALSE);
       break;
   }

   return TPM_RC_SUCCESS;
}
