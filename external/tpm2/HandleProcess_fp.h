// This file was extracted from the TCG Published
// Trusted Platform Module Library
// Part 4: Supporting Routines
// Family "2.0"
// Level 00 Revision 01.16
// October 30, 2014

#ifndef TPM2_HANDLEPROCESS_FP_H_
#define TPM2_HANDLEPROCESS_FP_H_

TPM_RC ParseHandleBuffer(
    TPM_CC command_code,              //   IN: Command being processed
    BYTE **req_handle_buffer_start,   //   IN/OUT: command buffer where handles are
                                      //   located. Updated as handles are unmarshaled
    INT32 *req_buffer_remaining_size, //   IN/OUT: indicates the amount of data left
                                      //   in the command buffer. Updated as handles
                                      //   are unmarshaled
    TPM_HANDLE req_handles[],         //   OUT: Array that receives the handles
    UINT32 *req_handles_num           //   OUT: Receives the count of handles
    );

#endif  // _TPM2_HANDLEPROCESS_FP_H_
