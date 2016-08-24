// This file was extracted from the TCG Published
// Trusted Platform Module Library
// Part 4: Supporting Routines
// Family "2.0"
// Level 00 Revision 01.16
// October 30, 2014

#include     "InternalRoutines.h"
#include     "ExecCommand_fp.h"
#include     "HandleProcess_fp.h"
#include     "SessionProcess_fp.h"
#include     "CommandDispatcher_fp.h"
//
//     Uncomment this next #include if doing static command/response buffer sizing
//
// #include "CommandResponseSizes_fp.h"
//
//
//           ExecuteCommand()
//
//     The function performs the following steps.
//     a) Parses the command header from input buffer.
//     b) Calls ParseHandleBuffer() to parse the handle area of the command.
//     c) Validates that each of the handles references a loaded entity.
//
//     d) Calls ParseSessionBuffer() () to:
//          1) unmarshal and parse the session area;
//          2) check the authorizations; and
//          3) when necessary, decrypt a parameter.
//     e) Calls CommandDispatcher() to:
//          1) unmarshal the command parameters from the command buffer;
//          2) call the routine that performs the command actions; and
//          3) marshal the responses into the response buffer.
//     f)   If any error occurs in any of the steps above create the error response and return.
//     g) Calls BuildResponseSession() to:
//          1) when necessary, encrypt a parameter
//          2) build the response authorization sessions
//          3) update the audit sessions and nonces
//     h) Assembles handle, parameter and session buffers for response and return.
//
LIB_EXPORT void
ExecuteCommand(
    unsigned    int      requestSize,       //   IN: command buffer size
    unsigned    char    *request,           //   IN: command buffer
    unsigned    int     *responseSize,      //   OUT: response buffer size
    unsigned    char    **response          //   OUT: response buffer
    )
{
    // Command local variables
    TPM_ST               tag;                         // these first three variables are the
    UINT32               commandSize;
    TPM_CC               commandCode = 0;
    BYTE                     *parmBufferStart;        // pointer to the first byte of an
                                                      // optional parameter buffer
    UINT32                    parmBufferSize = 0;// number of bytes in parameter area
    UINT32                    handleNum = 0;          // number of handles unmarshaled into
                                                      // the handles array
    TPM_HANDLE                handles[MAX_HANDLE_NUM];// array to hold handles in the
                                                 // command. Only handles in the handle
                                                 // area are stored here, not handles
                                                 // passed as parameters.
    // Response local variables
    TPM_RC               result;                      // return code for the command
    TPM_ST                    resTag;                 // tag for the response
    UINT32                    resHandleSize = 0; //       size of the handle area in the
                                                 //       response. This is needed so that the
                                                 //       handle area can be skipped when
                                                 //       generating the rpHash.
    UINT32                    resParmSize = 0;        // the size of the response parameters
                                                      // These values go in the rpHash.
    UINT32                    resAuthSize = 0;        // size of authorization area in the
//
                                                   // response
   INT32                      size;                // remaining data to be unmarshaled
                                                   // or remaining space in the marshaling
                                                   // buffer
   BYTE                      *buffer;              // pointer into the buffer being used
                                                   // for marshaling or unmarshaling
   INT32                      bufferSize;          // size of buffer being used for
                                                   // marshaling or unmarshaling
   UINT32                     i;                    // local temp
// This next function call is used in development to size the command and response
// buffers. The values printed are the sizes of the internal structures and
// not the sizes of the canonical forms of the command response structures. Also,
// the sizes do not include the tag, commandCode, requestSize, or the authorization
// fields.
//CommandResponseSizes();
   // Set flags for NV access state. This should happen before any other
   // operation that may require a NV write. Note, that this needs to be done
   // even when in failure mode. Otherwise, g_updateNV would stay SET while in
   // Failure mode and the NB would be written on each call.
   g_updateNV = FALSE;
   g_clearOrderly = FALSE;
   // As of Sept 25, 2013, the failure mode handling has been incorporated in the
   // reference code. This implementation requires that the system support
   // setjmp/longjmp. This code is put here because of the complexity being
   // added to the platform and simulator code to deal with all the variations
   // of errors.
   if(g_inFailureMode)
   {
       // Do failure mode processing
       TpmFailureMode (requestSize, request, responseSize, response);
       return;
   }
#ifndef EMBEDDED_MODE
   if(setjmp(g_jumpBuffer) != 0)
   {
       // Get here if we got a longjump putting us into failure mode
       g_inFailureMode = TRUE;
       result = TPM_RC_FAILURE;
       goto Fail;
   }
#endif  // EMBEDDED_MODE   ^^^ not defined
   // Assume that everything is going to work.
   result = TPM_RC_SUCCESS;
   // Query platform to get the NV state. The result state is saved internally
   // and will be reported by NvIsAvailable(). The reference code requires that
   // accessibility of NV does not change during the execution of a command.
   // Specifically, if NV is available when the command execution starts and then
   // is not available later when it is necessary to write to NV, then the TPM
   // will go into failure mode.
   NvCheckState();
   // Due to the limitations of the simulation, TPM clock must be explicitly
   // synchronized with the system clock whenever a command is received.
   // This function call is not necessary in a hardware TPM. However, taking
   // a snapshot of the hardware timer at the beginning of the command allows
   // the time value to be consistent for the duration of the command execution.
   TimeUpdateToCurrent();
   // Any command through this function will unceremoniously end the
   // _TPM_Hash_Data/_TPM_Hash_End sequence.
   if(g_DRTMHandle != TPM_RH_UNASSIGNED)
       ObjectTerminateEvent();
     // Get command buffer size and command buffer.
     size = requestSize;
     buffer = request;
     // Parse command header: tag, commandSize and commandCode.
     // First parse the tag. The unmarshaling routine will validate
     // that it is either TPM_ST_SESSIONS or TPM_ST_NO_SESSIONS.
     result = TPMI_ST_COMMAND_TAG_Unmarshal(&tag, &buffer, &size);
     if(result != TPM_RC_SUCCESS)
         goto Cleanup;
     // Unmarshal the commandSize indicator.
     result = UINT32_Unmarshal(&commandSize, &buffer, &size);
     if(result != TPM_RC_SUCCESS)
         goto Cleanup;
     // On a TPM that receives bytes on a port, the number of bytes that were
     // received on that port is requestSize it must be identical to commandSize.
     // In addition, commandSize must not be larger than MAX_COMMAND_SIZE allowed
     // by the implementation. The check against MAX_COMMAND_SIZE may be redundant
     // as the input processing (the function that receives the command bytes and
     // places them in the input buffer) would likely have the input truncated when
     // it reaches MAX_COMMAND_SIZE, and requestSize would not equal commandSize.
     if(commandSize != requestSize || commandSize > MAX_COMMAND_SIZE)
     {
         result = TPM_RC_COMMAND_SIZE;
         goto Cleanup;
     }
     // Unmarshal the command code.
     result = TPM_CC_Unmarshal(&commandCode, &buffer, &size);
     if(result != TPM_RC_SUCCESS)
         goto Cleanup;
     // Check to see if the command is implemented.
     if(!CommandIsImplemented(commandCode))
     {
         result = TPM_RC_COMMAND_CODE;
         goto Cleanup;
     }
#if   FIELD_UPGRADE_IMPLEMENTED == YES
   // If the TPM is in FUM, then the only allowed command is
   // TPM_CC_FieldUpgradeData.
   if(IsFieldUgradeMode() && (commandCode != TPM_CC_FieldUpgradeData))
   {
        result = TPM_RC_UPGRADE;
        goto Cleanup;
   }
   else
#endif
        // Excepting FUM, the TPM only accepts TPM2_Startup() after
        // _TPM_Init. After getting a TPM2_Startup(), TPM2_Startup()
        // is no longer allowed.
        if((    !TPMIsStarted() && commandCode != TPM_CC_Startup)
             || (TPMIsStarted() && commandCode == TPM_CC_Startup))
        {
             result = TPM_RC_INITIALIZE;
             goto Cleanup;
        }
     // Start regular command process.
     // Parse Handle buffer.
     result = ParseHandleBuffer(commandCode, &buffer, &size, handles, &handleNum);
     if(result != TPM_RC_SUCCESS)
        goto Cleanup;
   // Number of handles retrieved from handle area should be less than
   // MAX_HANDLE_NUM.
   pAssert(handleNum <= MAX_HANDLE_NUM);
   // All handles in the handle area are required to reference TPM-resident
   // entities.
   for(i = 0; i < handleNum; i++)
   {
       result = EntityGetLoadStatus(&handles[i], commandCode);
       if(result != TPM_RC_SUCCESS)
       {
           if(result == TPM_RC_REFERENCE_H0)
               result = result + i;
           else
               result = RcSafeAddToResult(result, TPM_RC_H + g_rcIndex[i]);
           goto Cleanup;
       }
   }
   // Authorization session handling for the command.
   if(tag == TPM_ST_SESSIONS)
   {
       BYTE        *sessionBufferStart;// address of the session area first byte
                                       // in the input buffer
        UINT32        authorizationSize;   // number of bytes in the session area
        // Find out session buffer size.
        result = UINT32_Unmarshal(&authorizationSize, &buffer, &size);
        if(result != TPM_RC_SUCCESS)
            goto Cleanup;
        // Perform sanity check on the unmarshaled    value. If it is smaller than
        // the smallest possible session or larger    than the remaining size of
        // the command, then it is an error. NOTE:    This check could pass but the
        // session size could still be wrong. That    will be determined after the
        // sessions are unmarshaled.
        if(    authorizationSize < 9
            || authorizationSize > (UINT32) size)
        {
             result = TPM_RC_SIZE;
             goto Cleanup;
        }
        // The sessions, if any, follows authorizationSize.
        sessionBufferStart = buffer;
        // The parameters follow the session area.
        parmBufferStart = sessionBufferStart + authorizationSize;
        // Any data left over after removing the authorization sessions is
        // parameter data. If the command does not have parameters, then an
        // error will be returned if the remaining size is not zero. This is
        // checked later.
        parmBufferSize = size - authorizationSize;
        // The actions of ParseSessionBuffer() are described in the introduction.
        result = ParseSessionBuffer(commandCode,
                                    handleNum,
                                    handles,
                                    sessionBufferStart,
                                    authorizationSize,
                                    parmBufferStart,
                                    parmBufferSize);
         if(result != TPM_RC_SUCCESS)
             goto Cleanup;
   }
   else
   {
       // Whatever remains in the input buffer is used for the parameters of the
       // command.
       parmBufferStart = buffer;
       parmBufferSize = size;
         // The command has no authorization sessions.
         // If the command requires authorizations, then CheckAuthNoSession() will
         // return an error.
         result = CheckAuthNoSession(commandCode, handleNum, handles,
                                      parmBufferStart, parmBufferSize);
         if(result != TPM_RC_SUCCESS)
             goto Cleanup;
   }
   // CommandDispatcher returns a response handle buffer and a response parameter
   // buffer if it succeeds. It will also set the parameterSize field in the
   // buffer if the tag is TPM_RC_SESSIONS.
   result = CommandDispatcher(tag,
                              commandCode,
                              (INT32 *) &parmBufferSize,
                              parmBufferStart,
                              handles,
                              &resHandleSize,
                              &resParmSize);
   if(result != TPM_RC_SUCCESS)
       goto Cleanup;
   // Build the session area at the end of the parameter area.
   BuildResponseSession(tag,
                        commandCode,
                        resHandleSize,
                        resParmSize,
                        &resAuthSize);
Cleanup:
   // This implementation loads an "evict" object to a transient object slot in
   // RAM whenever an "evict" object handle is used in a command so that the
   // access to any object is the same. These temporary objects need to be
   // cleared from RAM whether the command succeeds or fails.
   ObjectCleanupEvict();
#ifndef EMBEDDED_MODE
Fail:
#endif  // EMBEDDED_MODE  ^^^ not defined
   // The response will contain at least a response header.
   *responseSize = sizeof(TPM_ST) + sizeof(UINT32) + sizeof(TPM_RC);
   // If the command completed successfully, then build the rest of the response.
   if(result == TPM_RC_SUCCESS)
   {
       // Outgoing tag will be the same as the incoming tag.
       resTag = tag;
       // The overall response will include the handles, parameters,
       // and authorizations.
       *responseSize += resHandleSize + resParmSize + resAuthSize;
         // Adding parameter size field.
         if(tag == TPM_ST_SESSIONS)
             *responseSize += sizeof(UINT32);
         if(      g_clearOrderly == TRUE
               && gp.orderlyState != SHUTDOWN_NONE)
         {
                gp.orderlyState = SHUTDOWN_NONE;
                NvWriteReserved(NV_ORDERLY, &gp.orderlyState);
                g_updateNV = TRUE;
         }
     }
     else
     {
         // The command failed.
         // If this was a failure due to a bad command tag, then need to return
         // a TPM 1.2 compatible response
         if(result == TPM_RC_BAD_TAG)
              resTag = TPM_ST_RSP_COMMAND;
         else
              // return 2.0 compatible response
              resTag = TPM_ST_NO_SESSIONS;
     }
     // Try to commit all the writes to NV if any NV write happened during this
     // command execution. This check should be made for both succeeded and failed
     // commands, because a failed one may trigger a NV write in DA logic as well.
     // This is the only place in the command execution path that may call the NV
     // commit. If the NV commit fails, the TPM should be put in failure mode.
     if(g_updateNV && !g_inFailureMode)
     {
         g_updateNV = FALSE;
         if(!NvCommit())
              FAIL(FATAL_ERROR_INTERNAL);
     }
     // Marshal the response header.
     buffer = MemoryGetResponseBuffer(commandCode);
     bufferSize = 10;
     TPM_ST_Marshal(&resTag, &buffer, &bufferSize);
     UINT32_Marshal((UINT32 *)responseSize, &buffer, &bufferSize);
     pAssert(*responseSize <= MAX_RESPONSE_SIZE);
     TPM_RC_Marshal(&result, &buffer, &bufferSize);
     *response = MemoryGetResponseBuffer(commandCode);
     // Clear unused bit in response buffer.
     MemorySet(*response + *responseSize, 0, MAX_RESPONSE_SIZE - *responseSize);
     return;
}
