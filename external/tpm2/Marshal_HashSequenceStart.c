// Copyright 2015 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// THIS CODE IS GENERATED - DO NOT MODIFY!

#include "MemoryLib_fp.h"
#include "HashSequenceStart_fp.h"

UINT16 HashSequenceStart_Out_Marshal(HashSequenceStart_Out* source,
                                     TPMI_ST_COMMAND_TAG tag,
                                     BYTE** buffer,
                                     INT32* size) {
  UINT16 total_size = 0;
  UINT32 parameter_size = 0;
  BYTE* parameter_size_location;
  INT32 parameter_size_size = sizeof(UINT32);
  UINT32 num_response_handles = 1;
  // Marshal response handles.
  total_size += TPMI_DH_OBJECT_Marshal(&source->sequenceHandle, buffer, size);
  // Add parameter_size=0 to indicate size of the parameter area. Will be
  // replaced later by computed parameter_size.
  if (tag == TPM_ST_SESSIONS) {
    parameter_size_location = *buffer;
    // Don't add to total_size, but increment *buffer and decrement *size.
    UINT32_Marshal(&parameter_size, buffer, size);
  }
  // Compute actual parameter_size. Don't add result to total_size.
  if (tag == TPM_ST_SESSIONS) {
    parameter_size = total_size - num_response_handles * sizeof(TPM_HANDLE);
    UINT32_Marshal(&parameter_size, &parameter_size_location,
                   &parameter_size_size);
  }
  return total_size;
}

TPM_RC HashSequenceStart_In_Unmarshal(HashSequenceStart_In* target,
                                      TPM_HANDLE request_handles[],
                                      BYTE** buffer,
                                      INT32* size) {
  TPM_RC result = TPM_RC_SUCCESS;
  // Unmarshal request parameters.
  result = TPM2B_AUTH_Unmarshal(&target->auth, buffer, size);
  if (result != TPM_RC_SUCCESS) {
    return result;
  }
  result = TPMI_ALG_HASH_Unmarshal(&target->hashAlg, buffer, size, TRUE);
  if (result != TPM_RC_SUCCESS) {
    return result;
  }
  if ((result == TPM_RC_SUCCESS) && *size) {
    result = TPM_RC_SIZE;
  }
  return result;
}

TPM_RC Exec_HashSequenceStart(TPMI_ST_COMMAND_TAG tag,
                              BYTE** request_parameter_buffer,
                              INT32* request_parameter_buffer_size,
                              TPM_HANDLE request_handles[],
                              UINT32* response_handle_buffer_size,
                              UINT32* response_parameter_buffer_size) {
  TPM_RC result = TPM_RC_SUCCESS;
  HashSequenceStart_In in;
  HashSequenceStart_Out out;
#ifdef TPM_CC_HashSequenceStart
  BYTE* response_buffer;
  INT32 response_buffer_size;
  UINT16 bytes_marshalled;
  UINT16 num_response_handles = 1;
#endif
  *response_handle_buffer_size = 0;
  *response_parameter_buffer_size = 0;
  // Unmarshal request parameters to input structure.
  result = HashSequenceStart_In_Unmarshal(&in, request_handles,
                                          request_parameter_buffer,
                                          request_parameter_buffer_size);
  if (result != TPM_RC_SUCCESS) {
    return result;
  }
  // Execute command.
  result = TPM2_HashSequenceStart(&in, &out);
  if (result != TPM_RC_SUCCESS) {
    return result;
  }
// Marshal output structure to global response buffer.
#ifdef TPM_CC_HashSequenceStart
  response_buffer = MemoryGetResponseBuffer(TPM_CC_HashSequenceStart) + 10;
  response_buffer_size = MAX_RESPONSE_SIZE - 10;
  bytes_marshalled = HashSequenceStart_Out_Marshal(&out, tag, &response_buffer,
                                                   &response_buffer_size);
  *response_handle_buffer_size = num_response_handles * sizeof(TPM_HANDLE);
  *response_parameter_buffer_size =
      bytes_marshalled - *response_handle_buffer_size;
  return TPM_RC_SUCCESS;
#endif
  return TPM_RC_COMMAND_CODE;
}
