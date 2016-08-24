// Copyright 2015 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// THIS CODE IS GENERATED - DO NOT MODIFY!

#ifndef TPM2_POLICYTICKET_FP_H_
#define TPM2_POLICYTICKET_FP_H_

#include "tpm_generated.h"

typedef struct {
  TPMI_SH_POLICY policySession;
  TPM2B_TIMEOUT timeout;
  TPM2B_DIGEST cpHashA;
  TPM2B_NONCE policyRef;
  TPM2B_NAME authName;
  TPMT_TK_AUTH ticket;
} PolicyTicket_In;

// Executes PolicyTicket with request handles and parameters from |in|.
TPM_RC TPM2_PolicyTicket(PolicyTicket_In* in);

// Initializes handle fields in |target| from |request_handles|. Unmarshals
// parameter fields in |target| from |buffer|.
TPM_RC PolicyTicket_In_Unmarshal(PolicyTicket_In* target,
                                 TPM_HANDLE request_handles[],
                                 BYTE** buffer,
                                 INT32* size);

// Unmarshals any request parameters starting at |request_parameter_buffer|.
// Executes command. Marshals any response handles and parameters to the
// global response buffer and computes |*response_handle_buffer_size| and
// |*response_parameter_buffer_size|. If |tag| == TPM_ST_SESSIONS, marshals
// parameter_size indicating the size of the parameter area. parameter_size
// field is located between the handle area and parameter area.
TPM_RC Exec_PolicyTicket(TPMI_ST_COMMAND_TAG tag,
                         BYTE** request_parameter_buffer,
                         INT32* request_parameter_buffer_size,
                         TPM_HANDLE request_handles[],
                         UINT32* response_handle_buffer_size,
                         UINT32* response_parameter_buffer_size);

#endif  // TPM2_POLICYTICKET_FP_H
