// Copyright 2015 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// THIS CODE IS GENERATED - DO NOT MODIFY!

#include "MemoryLib_fp.h"
#include "NV_Read_fp.h"

UINT16 NV_Read_Out_Marshal(NV_Read_Out* source,
                           TPMI_ST_COMMAND_TAG tag,
                           BYTE** buffer,
                           INT32* size) {
  UINT16 total_size = 0;
  UINT32 parameter_size = 0;
  BYTE* parameter_size_location;
  INT32 parameter_size_size = sizeof(UINT32);
  UINT32 num_response_handles = 0;
  // Add parameter_size=0 to indicate size of the parameter area. Will be
  // replaced later by computed parameter_size.
  if (tag == TPM_ST_SESSIONS) {
    parameter_size_location = *buffer;
    // Don't add to total_size, but increment *buffer and decrement *size.
    UINT32_Marshal(&parameter_size, buffer, size);
  }
  // Marshal response parameters.
  total_size += TPM2B_MAX_NV_BUFFER_Marshal(&source->data, buffer, size);
  // Compute actual parameter_size. Don't add result to total_size.
  if (tag == TPM_ST_SESSIONS) {
    parameter_size = total_size - num_response_handles * sizeof(TPM_HANDLE);
    UINT32_Marshal(&parameter_size, &parameter_size_location,
                   &parameter_size_size);
  }
  return total_size;
}

TPM_RC NV_Read_In_Unmarshal(NV_Read_In* target,
                            TPM_HANDLE request_handles[],
                            BYTE** buffer,
                            INT32* size) {
  TPM_RC result = TPM_RC_SUCCESS;
  // Get request handles from request_handles array.
  target->authHandle = request_handles[0];
  target->nvIndex = request_handles[1];
  // Unmarshal request parameters.
  result = UINT16_Unmarshal(&target->size, buffer, size);
  if (result != TPM_RC_SUCCESS) {
    return result;
  }
  result = UINT16_Unmarshal(&target->offset, buffer, size);
  if (result != TPM_RC_SUCCESS) {
    return result;
  }
  if ((result == TPM_RC_SUCCESS) && *size) {
    result = TPM_RC_SIZE;
  }
  return result;
}

TPM_RC Exec_NV_Read(TPMI_ST_COMMAND_TAG tag,
                    BYTE** request_parameter_buffer,
                    INT32* request_parameter_buffer_size,
                    TPM_HANDLE request_handles[],
                    UINT32* response_handle_buffer_size,
                    UINT32* response_parameter_buffer_size) {
  TPM_RC result = TPM_RC_SUCCESS;
  NV_Read_In in;
  NV_Read_Out out;
#ifdef TPM_CC_NV_Read
  BYTE* response_buffer;
  INT32 response_buffer_size;
  UINT16 bytes_marshalled;
  UINT16 num_response_handles = 0;
#endif
  *response_handle_buffer_size = 0;
  *response_parameter_buffer_size = 0;
  // Unmarshal request parameters to input structure.
  result = NV_Read_In_Unmarshal(&in, request_handles, request_parameter_buffer,
                                request_parameter_buffer_size);
  if (result != TPM_RC_SUCCESS) {
    return result;
  }
  // Execute command.
  result = TPM2_NV_Read(&in, &out);
  if (result != TPM_RC_SUCCESS) {
    return result;
  }
// Marshal output structure to global response buffer.
#ifdef TPM_CC_NV_Read
  response_buffer = MemoryGetResponseBuffer(TPM_CC_NV_Read) + 10;
  response_buffer_size = MAX_RESPONSE_SIZE - 10;
  bytes_marshalled =
      NV_Read_Out_Marshal(&out, tag, &response_buffer, &response_buffer_size);
  *response_handle_buffer_size = num_response_handles * sizeof(TPM_HANDLE);
  *response_parameter_buffer_size =
      bytes_marshalled - *response_handle_buffer_size;
  return TPM_RC_SUCCESS;
#endif
  return TPM_RC_COMMAND_CODE;
}
