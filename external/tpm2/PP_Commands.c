// This file was extracted from the TCG Published
// Trusted Platform Module Library
// Part 3: Commands
// Family "2.0"
// Level 00 Revision 01.16
// October 30, 2014

#include "InternalRoutines.h"
#include "PP_Commands_fp.h"
TPM_RC
TPM2_PP_Commands(
   PP_Commands_In   *in           // IN: input parameter list
   )
{
   UINT32           i;

   TPM_RC      result;

   // The command needs NV update. Check if NV is available.
   // A TPM_RC_NV_UNAVAILABLE or TPM_RC_NV_RATE error may be returned at
   // this point
   result = NvIsAvailable();
   if(result != TPM_RC_SUCCESS) return result;

// Internal Data Update

   // Process set list
   for(i = 0; i < in->setList.count; i++)
       // If command is implemented, set it as PP required. If the input
       // command is not a PP command, it will be ignored at
       // PhysicalPresenceCommandSet().
       if(CommandIsImplemented(in->setList.commandCodes[i]))
           PhysicalPresenceCommandSet(in->setList.commandCodes[i]);

   // Process clear list
   for(i = 0; i < in->clearList.count; i++)
       // If command is implemented, clear it as PP required. If the input
       // command is not a PP command, it will be ignored at
       // PhysicalPresenceCommandClear(). If the input command is
       // TPM2_PP_Commands, it will be ignored as well
       if(CommandIsImplemented(in->clearList.commandCodes[i]))
           PhysicalPresenceCommandClear(in->clearList.commandCodes[i]);

   // Save the change of PP list
   NvWriteReserved(NV_PP_LIST, &gp.ppList);

   return TPM_RC_SUCCESS;
}
