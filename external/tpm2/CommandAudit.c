// This file was extracted from the TCG Published
// Trusted Platform Module Library
// Part 4: Supporting Routines
// Family "2.0"
// Level 00 Revision 01.16
// October 30, 2014

#include "InternalRoutines.h"
//
//
//           Functions
//
//           CommandAuditPreInstall_Init()
//
//     This function initializes the command audit list. This function is simulates the behavior of manufacturing. A
//     function is used instead of a structure definition because this is easier than figuring out the initialization
//     value for a bit array.
//     This function would not be implemented outside of a manufacturing or simulation environment.
//
void
CommandAuditPreInstall_Init(
     void
     )
{
     // Clear all the audit commands
     MemorySet(gp.auditComands, 0x00,
               ((TPM_CC_LAST - TPM_CC_FIRST + 1) + 7) / 8);
     // TPM_CC_SetCommandCodeAuditStatus always being audited
     if(CommandIsImplemented(TPM_CC_SetCommandCodeAuditStatus))
         CommandAuditSet(TPM_CC_SetCommandCodeAuditStatus);
     // Set initial command audit hash algorithm to be context integrity hash
     // algorithm
     gp.auditHashAlg = CONTEXT_INTEGRITY_HASH_ALG;
     // Set up audit counter to be 0
     gp.auditCounter = 0;
     // Write command audit persistent data to NV
     NvWriteReserved(NV_AUDIT_COMMANDS, &gp.auditComands);
     NvWriteReserved(NV_AUDIT_HASH_ALG, &gp.auditHashAlg);
     NvWriteReserved(NV_AUDIT_COUNTER, &gp.auditCounter);
     return;
}
//
//
//           CommandAuditStartup()
//
//     This function clears the command audit digest on a TPM Reset.
//
void
CommandAuditStartup(
     STARTUP_TYPE        type               // IN: start up type
     )
{
   if(type == SU_RESET)
   {
       // Reset the digest size to initialize the digest
       gr.commandAuditDigest.t.size = 0;
   }
}
//
//
//         CommandAuditSet()
//
//     This function will SET the audit flag for a command. This function will not SET the audit flag for a
//     command that is not implemented. This ensures that the audit status is not SET when
//     TPM2_GetCapability() is used to read the list of audited commands.
//     This function is only used by TPM2_SetCommandCodeAuditStatus().
//     The actions in TPM2_SetCommandCodeAuditStatus() are expected to cause the changes to be saved to
//     NV after it is setting and clearing bits.
//
//     Return Value                      Meaning
//
//     TRUE                              the command code audit status was changed
//     FALSE                             the command code audit status was not changed
//
BOOL
CommandAuditSet(
   TPM_CC              commandCode          // IN: command code
   )
{
   UINT32         bitPos;
   // Only SET a bit if the corresponding command is implemented
   if(CommandIsImplemented(commandCode))
   {
       // Can't audit shutdown
       if(commandCode != TPM_CC_Shutdown)
       {
           bitPos = commandCode - TPM_CC_FIRST;
           if(!BitIsSet(bitPos, &gp.auditComands[0], sizeof(gp.auditComands)))
           {
               // Set bit
               BitSet(bitPos, &gp.auditComands[0], sizeof(gp.auditComands));
               return TRUE;
           }
       }
   }
   // No change
   return FALSE;
}
//
//
//         CommandAuditClear()
//
//     This function will CLEAR the audit flag for a command. It will not CLEAR the audit flag for
//     TPM_CC_SetCommandCodeAuditStatus().
//     This function is only used by TPM2_SetCommandCodeAuditStatus().
//     The actions in TPM2_SetCommandCodeAuditStatus() are expected to cause the changes to be saved to
//     NV after it is setting and clearing bits.
//
//
//
//      Return Value                     Meaning
//
//      TRUE                             the command code audit status was changed
//      FALSE                            the command code audit status was not changed
//
BOOL
CommandAuditClear(
    TPM_CC               commandCode        // IN: command code
    )
{
    UINT32         bitPos;
    // Do nothing if the command is not implemented
    if(CommandIsImplemented(commandCode))
    {
        // The bit associated with TPM_CC_SetCommandCodeAuditStatus() cannot be
        // cleared
        if(commandCode != TPM_CC_SetCommandCodeAuditStatus)
        {
            bitPos = commandCode - TPM_CC_FIRST;
            if(BitIsSet(bitPos, &gp.auditComands[0], sizeof(gp.auditComands)))
            {
                // Clear bit
                BitClear(bitPos, &gp.auditComands[0], sizeof(gp.auditComands));
                return TRUE;
            }
        }
    }
    // No change
    return FALSE;
}
//
//
//           CommandAuditIsRequired()
//
//      This function indicates if the audit flag is SET for a command.
//
//      Return Value                     Meaning
//
//      TRUE                             if command is audited
//      FALSE                            if command is not audited
//
BOOL
CommandAuditIsRequired(
    TPM_CC               commandCode        // IN: command code
    )
{
    UINT32         bitPos;
    bitPos = commandCode - TPM_CC_FIRST;
    // Check the bit map. If the bit is SET, command audit is required
    if((gp.auditComands[bitPos/8] & (1 << (bitPos % 8))) != 0)
        return TRUE;
    else
        return FALSE;
}
//
//
//           CommandAuditCapGetCCList()
//
//      This function returns a list of commands that have their audit bit SET.
//      Family "2.0"                                 TCG Published                                        Page 111
//      Level 00 Revision 01.16              Copyright © TCG 2006-2014                           October 30, 2014
//      Trusted Platform Module Library                                                  Part 4: Supporting Routines
//
//
//      The list starts at the input commandCode.
//
//      Return Value                      Meaning
//
//      YES                               if there are more command code available
//      NO                                all the available command code has been returned
//
TPMI_YES_NO
CommandAuditCapGetCCList(
     TPM_CC            commandCode,          // IN: start command code
     UINT32            count,                // IN: count of returned TPM_CC
     TPML_CC          *commandList           // OUT: list of TPM_CC
     )
{
     TPMI_YES_NO      more = NO;
     UINT32           i;
     // Initialize output handle list
     commandList->count = 0;
     // The maximum count of command we may return is MAX_CAP_CC
     if(count > MAX_CAP_CC) count = MAX_CAP_CC;
     // If the command code is smaller than TPM_CC_FIRST, start from TPM_CC_FIRST
     if(commandCode < TPM_CC_FIRST) commandCode = TPM_CC_FIRST;
     // Collect audit commands
     for(i = commandCode; i <= TPM_CC_LAST; i++)
     {
         if(CommandAuditIsRequired(i))
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
                 // If the return list is full but we still have command
                 // available, report this and stop iterating
                 more = YES;
                 break;
             }
         }
     }
     return more;
}
//
//
//          CommandAuditGetDigest
//
//      This command is used to create a digest of the commands being audited. The commands are processed
//      in ascending numeric order with a list of TPM_CC being added to a hash. This operates as if all the
//      audited command codes were concatenated and then hashed.
//
void
CommandAuditGetDigest(
     TPM2B_DIGEST     *digest                // OUT: command digest
     )
{
     TPM_CC                               i;
     HASH_STATE                           hashState;
     // Start hash
     digest->t.size = CryptStartHash(gp.auditHashAlg, &hashState);
     // Add command code
     for(i = TPM_CC_FIRST; i <= TPM_CC_LAST; i++)
     {
         if(CommandAuditIsRequired(i))
         {
             CryptUpdateDigestInt(&hashState, sizeof(i), &i);
         }
     }
     // Complete hash
     CryptCompleteHash2B(&hashState, &digest->b);
     return;
}
