// This file was extracted from the TCG Published
// Trusted Platform Module Library
// Part 4: Supporting Routines
// Family "2.0"
// Level 00 Revision 01.16
// October 30, 2014

#include "InternalRoutines.h"
#include "PP_fp.h"

//
//
//             Functions
//
//             PhysicalPresencePreInstall_Init()
//
//       This function is used to initialize the array of commands that require confirmation with physical presence.
//       The array is an array of bits that has a correspondence with the command code.
//       This command should only ever be executable in a manufacturing setting or in a simulation.
//
void
PhysicalPresencePreInstall_Init(
     void
     )
{
     // Clear all the PP commands
     MemorySet(&gp.ppList, 0,
//
                ((TPM_CC_PP_LAST - TPM_CC_PP_FIRST + 1) + 7) / 8);
   // TPM_CC_PP_Commands always requires PP
   if(CommandIsImplemented(TPM_CC_PP_Commands))
       PhysicalPresenceCommandSet(TPM_CC_PP_Commands);
   // Write PP list to NV
   NvWriteReserved(NV_PP_LIST, &gp.ppList);
   return;
}
//
//
//          PhysicalPresenceCommandSet()
//
//     This function is used to indicate a command that requires PP confirmation.
//
void
PhysicalPresenceCommandSet(
   TPM_CC               commandCode       // IN: command code
   )
{
   UINT32         bitPos;
   // Assume command is implemented. It should be checked before this
   // function is called
   pAssert(CommandIsImplemented(commandCode));
   // If the command is not a PP command, ignore it
   if(commandCode < TPM_CC_PP_FIRST || commandCode > TPM_CC_PP_LAST)
       return;
   bitPos = commandCode - TPM_CC_PP_FIRST;
   // Set bit
   gp.ppList[bitPos/8] |= 1 << (bitPos % 8);
   return;
}
//
//
//          PhysicalPresenceCommandClear()
//
//     This function is used to indicate a command that no longer requires PP confirmation.
//
void
PhysicalPresenceCommandClear(
   TPM_CC               commandCode       // IN: command code
   )
{
   UINT32         bitPos;
   // Assume command is implemented. It should be checked before this
   // function is called
   pAssert(CommandIsImplemented(commandCode));
   // If the command is not a PP command, ignore it
   if(commandCode < TPM_CC_PP_FIRST || commandCode > TPM_CC_PP_LAST)
       return;
   // if the input code is TPM_CC_PP_Commands, it can not be cleared
   if(commandCode == TPM_CC_PP_Commands)
       return;
   bitPos = commandCode - TPM_CC_PP_FIRST;
     // Set bit
     gp.ppList[bitPos/8] |= (1 << (bitPos % 8));
     // Flip it to off
     gp.ppList[bitPos/8] ^= (1 << (bitPos % 8));
     return;
}
//
//
//           PhysicalPresenceIsRequired()
//
//      This function indicates if PP confirmation is required for a command.
//
//      Return Value                      Meaning
//
//      TRUE                              if physical presence is required
//      FALSE                             if physical presence is not required
//
BOOL
PhysicalPresenceIsRequired(
     TPM_CC             commandCode           // IN: command code
     )
{
     UINT32        bitPos;
     // if the input commandCode is not a PP command, return FALSE
     if(commandCode < TPM_CC_PP_FIRST || commandCode > TPM_CC_PP_LAST)
         return FALSE;
     bitPos = commandCode - TPM_CC_PP_FIRST;
     // Check the bit map. If the bit is SET, PP authorization is required
     return ((gp.ppList[bitPos/8] & (1 << (bitPos % 8))) != 0);
}
//
//
//           PhysicalPresenceCapGetCCList()
//
//      This function returns a list of commands that require PP confirmation. The list starts from the first
//      implemented command that has a command code that the same or greater than commandCode.
//
//      Return Value                      Meaning
//
//      YES                               if there are more command codes available
//      NO                                all the available command codes have been returned
//
TPMI_YES_NO
PhysicalPresenceCapGetCCList(
     TPM_CC             commandCode,          // IN: start command code
     UINT32             count,                // IN: count of returned TPM_CC
     TPML_CC           *commandList           // OUT: list of TPM_CC
     )
{
     TPMI_YES_NO       more = NO;
     UINT32            i;
     // Initialize output handle list
     commandList->count = 0;
     // The maximum count of command we may return is MAX_CAP_CC
     if(count > MAX_CAP_CC) count = MAX_CAP_CC;
     // Collect PP commands
     for(i = commandCode; i <= TPM_CC_PP_LAST; i++)
     {
         if(PhysicalPresenceIsRequired(i))
         {
             if(commandList->count < count)
             {
                 // If we have not filled up the return list, add this command
                 // code to it
                 commandList->commandCodes[commandList->count] = i;
                 commandList->count++;
             }
             else
             {
                 // If the return list is full but we still have PP command
                 // available, report this and stop iterating
                 more = YES;
                 break;
             }
         }
     }
     return more;
}
