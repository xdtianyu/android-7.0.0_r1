// Copyright 2015 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef _TPM2_EXECCOMMAND_FP_H_
#define _TPM2_EXECCOMMAND_FP_H_

void ExecuteCommand(
    unsigned    int      requestSize,       //   IN: command buffer size
    unsigned    char    *request,           //   IN: command buffer
    unsigned    int     *responseSize,      //   OUT: response buffer size
    unsigned    char    **response          //   OUT: response buffer
    );

#endif  // _TPM2_EXECCOMMAND_FP_H_
