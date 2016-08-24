// This file was extracted from the TCG Published
// Trusted Platform Module Library
// Part 4: Supporting Routines
// Family "2.0"
// Level 00 Revision 01.16
// October 30, 2014

#ifndef TPM2_COMMANDDISPATCHER_FP_H_
#define TPM2_COMMANDDISPATCHER_FP_H_

TPM_RC CommandDispatcher(
    TPMI_ST_COMMAND_TAG tag,           // IN: Input command tag
    TPM_CC command_code,               // IN: Command code
    INT32 *req_parameter_buffer_size,  // IN: size of parameter buffer
    BYTE *req_parameter_buffer_start,  // IN: pointer to start of the request
                                       //     parameter buffer
    TPM_HANDLE req_handles[],          // IN: request handle array
    UINT32 *res_handle_buffer_size,    // OUT: size of handle buffer in response
    UINT32 *res_parameter_buffer_size  // OUT: size of parameter buffer in response
    );

#endif  // _COMMANDDISPATCHER_FP_H_
