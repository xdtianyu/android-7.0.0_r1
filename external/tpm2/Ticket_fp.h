/*
 * Copyright 2015 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef __TPM2_TICKET_FP_H
#define __TPM2_TICKET_FP_H

void TicketComputeAuth(
    TPM_ST type,                  //   IN: the type of ticket.
    TPMI_RH_HIERARCHY hierarchy,  //   IN: hierarchy constant for ticket
    UINT64 timeout,               //   IN: timeout
    TPM2B_DIGEST *cpHashA,        //   IN: input cpHashA
    TPM2B_NONCE *policyRef,       //   IN: input policyRef
    TPM2B_NAME *entityName,       //   IN: name of entity
    TPMT_TK_AUTH *ticket          //   OUT: Created ticket
    );
void TicketComputeCreation(
    TPMI_RH_HIERARCHY hierarchy,  //   IN: hierarchy for ticket
    TPM2B_NAME *name,             //   IN: object name
    TPM2B_DIGEST *creation,       //   IN: creation hash
    TPMT_TK_CREATION *ticket      //   OUT: created ticket
    );
void TicketComputeHashCheck(
    TPMI_RH_HIERARCHY hierarchy,  //   IN: hierarchy constant for ticket
    TPM_ALG_ID hashAlg,    //   IN: the hash algorithm used to create 'digest'
    TPM2B_DIGEST *digest,  //   IN: input digest
    TPMT_TK_HASHCHECK *ticket  //   OUT: Created ticket
    );
void TicketComputeVerified(
    TPMI_RH_HIERARCHY hierarchy,  //   IN: hierarchy constant for ticket
    TPM2B_DIGEST *digest,         //   IN: digest
    TPM2B_NAME *keyName,          //   IN: name of key that signed the value
    TPMT_TK_VERIFIED *ticket      //   OUT: verified ticket
    );
BOOL TicketIsSafe(TPM2B *buffer);

#endif  // __TPM2_TICKET_FP_H
