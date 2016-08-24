// This file was extracted from the TCG Published
// Trusted Platform Module Library
// Part 4: Supporting Routines
// Family "2.0"
// Level 00 Revision 01.16
// October 30, 2014

#include    "Tpm.h"
#include    "InternalRoutines.h"
typedef UINT16          ATTRIBUTE_TYPE;
//
//     The following file is produced from the command tables in part 3 of the specification. It defines the
//     attributes for each of the commands.
//
//     NOTE:           This file is currently produced by an automated process. Files produced from Part 2 or Part 3 tables through
//                     automated processes are not included in the specification so that their is no ambiguity about the table
//                     containing the information being the normative definition.
//
#include       "CommandAttributeData.c"
//
//
//          Command Attribute Functions
//
//          CommandAuthRole()
//
//     This function returns the authorization role required of a handle.
//
//     Return Value                       Meaning
//
//     AUTH_NONE                          no authorization is required
//     AUTH_USER                          user role authorization is required
//     AUTH_ADMIN                         admin role authorization is required
//     AUTH_DUP                           duplication role authorization is required
//
AUTH_ROLE
CommandAuthRole(
     TPM_CC        commandCode,                 // IN: command code
     UINT32        handleIndex                  // IN: handle index (zero based)
     )
{
   if(handleIndex > 1)
       return AUTH_NONE;
   if(handleIndex == 0) {
       ATTRIBUTE_TYPE properties = s_commandAttributes[commandCode - TPM_CC_FIRST];
       if(properties & HANDLE_1_USER) return AUTH_USER;
       if(properties & HANDLE_1_ADMIN) return AUTH_ADMIN;
       if(properties & HANDLE_1_DUP) return AUTH_DUP;
       return AUTH_NONE;
   }
   if(s_commandAttributes[commandCode - TPM_CC_FIRST] & HANDLE_2_USER)
           return AUTH_USER;
   return AUTH_NONE;
}
//
//
//          CommandIsImplemented()
//
//     This function indicates if a command is implemented.
//
//     Return Value                      Meaning
//
//     TRUE                              if the command is implemented
//     FALSE                             if the command is not implemented
//
BOOL
CommandIsImplemented(
    TPM_CC                commandCode          // IN: command code
    )
{
    if(commandCode < TPM_CC_FIRST || commandCode > TPM_CC_LAST)
        return FALSE;
    if((s_commandAttributes[commandCode - TPM_CC_FIRST] & IS_IMPLEMENTED))
        return TRUE;
    else
        return FALSE;
}
//
//
//          CommandGetAttribute()
//
//     return a TPMA_CC structure for the given command code
//
TPMA_CC
CommandGetAttribute(
    TPM_CC                commandCode          // IN: command code
    )
{
    UINT32      size = sizeof(s_ccAttr) / sizeof(s_ccAttr[0]);
    UINT32      i;
    for(i = 0; i < size; i++) {
        if(s_ccAttr[i].commandIndex == (UINT16) commandCode)
            return s_ccAttr[i];
    }
    // This function should be called in the way that the command code
    // attribute is available.
    FAIL(FATAL_ERROR_INTERNAL);

    return s_ccAttr[0]; // Just to appease the compiler, never reached.
}
//
//
//          EncryptSize()
//
//     This function returns the size of the decrypt size field. This function returns 0 if encryption is not allowed
//
//     Return Value                      Meaning
//
//     0                                 encryption not allowed
//     2                                 size field is two bytes
//     4                                 size field is four bytes
//
int
EncryptSize(
    TPM_CC                commandCode          // IN: commandCode
    )
{
    COMMAND_ATTRIBUTES        ca = s_commandAttributes[commandCode - TPM_CC_FIRST];
    if(ca & ENCRYPT_2)
        return 2;
    if(ca & ENCRYPT_4)
        return 4;
    return 0;
}
//
//
//          DecryptSize()
//
//     This function returns the size of the decrypt size field. This function returns 0 if decryption is not allowed
//
//     Return Value                      Meaning
//
//     0                                 encryption not allowed
//     2                                 size field is two bytes
//     4                                 size field is four bytes
//
int
DecryptSize(
    TPM_CC                commandCode          // IN: commandCode
    )
{
    COMMAND_ATTRIBUTES        ca = s_commandAttributes[commandCode - TPM_CC_FIRST];
    if(ca & DECRYPT_2)
        return 2;
    if(ca & DECRYPT_4)
        return 4;
    return 0;
}
//
//
//          IsSessionAllowed()
//
//     This function indicates if the command is allowed to have sessions.
//     This function must not be called if the command is not known to be implemented.
//
//     Return Value                      Meaning
//
//     TRUE                              session is allowed with this command
//     FALSE                             session is not allowed with this command
//
BOOL
IsSessionAllowed(
    TPM_CC                commandCode          // IN: the command to be checked
    )
{
    if(s_commandAttributes[commandCode - TPM_CC_FIRST] & NO_SESSIONS)
        return FALSE;
    else
        return TRUE;
}
//
//
//          IsHandleInResponse()
//
BOOL
IsHandleInResponse(
    TPM_CC                commandCode
    )
{
    if(s_commandAttributes[commandCode - TPM_CC_FIRST] & R_HANDLE)
        return TRUE;
    else
        return FALSE;
//
}
//
//
//           IsWriteOperation()
//
//      Checks to see if an operation will write to NV memory
//
BOOL
IsWriteOperation(
   TPM_CC               command           // IN: Command to check
   )
{
   switch (command)
   {
       case TPM_CC_NV_Write:
       case TPM_CC_NV_Increment:
       case TPM_CC_NV_SetBits:
       case TPM_CC_NV_Extend:
       // Nv write lock counts as a write operation for authorization purposes.
       // We check to see if the NV is write locked before we do the authorization
       // If it is locked, we fail the command early.
       case TPM_CC_NV_WriteLock:
           return TRUE;
       default:
           break;
   }
   return FALSE;
}
//
//
//           IsReadOperation()
//
//      Checks to see if an operation will write to NV memory
//
BOOL
IsReadOperation(
   TPM_CC               command           // IN: Command to check
   )
{
   switch (command)
   {
       case TPM_CC_NV_Read:
       case TPM_CC_PolicyNV:
       case TPM_CC_NV_Certify:
       // Nv read lock counts as a read operation for authorization purposes.
       // We check to see if the NV is read locked before we do the authorization
       // If it is locked, we fail the command early.
       case TPM_CC_NV_ReadLock:
           return TRUE;
       default:
           break;
   }
   return FALSE;
}
//
//
//          CommandCapGetCCList()
//
//      This function returns a list of implemented commands and command attributes starting from the
//      command in commandCode.
//
//
//
//
//      Return Value                      Meaning
//
//      YES                               more command attributes are available
//      NO                                no more command attributes are available
//
TPMI_YES_NO
CommandCapGetCCList(
     TPM_CC            commandCode,         // IN: start command code
     UINT32            count,               // IN: maximum count for number of entries in
                                            //     'commandList'
     TPML_CCA         *commandList          // OUT: list of TPMA_CC
     )
{
     TPMI_YES_NO       more = NO;
     UINT32            i;
     // initialize output handle list count
     commandList->count = 0;
     // The maximum count of commands that may be return is MAX_CAP_CC.
     if(count > MAX_CAP_CC) count = MAX_CAP_CC;
     // If the command code is smaller than TPM_CC_FIRST, start from TPM_CC_FIRST
     if(commandCode < TPM_CC_FIRST) commandCode = TPM_CC_FIRST;
     // Collect command attributes
     for(i = commandCode; i <= TPM_CC_LAST; i++)
     {
         if(CommandIsImplemented(i))
         {
             if(commandList->count < count)
             {
                 // If the list is not full, add the attributes for this command.
                 commandList->commandAttributes[commandList->count]
                     = CommandGetAttribute(i);
                 commandList->count++;
             }
             else
             {
                 // If the list is full but there are more commands to report,
                 // indicate this and return.
                 more = YES;
                 break;
             }
         }
     }
     return more;
}
