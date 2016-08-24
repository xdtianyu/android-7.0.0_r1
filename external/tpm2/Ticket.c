// This file was extracted from the TCG Published
// Trusted Platform Module Library
// Part 4: Supporting Routines
// Family "2.0"
// Level 00 Revision 01.16
// October 30, 2014

#include "InternalRoutines.h"
//
//
//       10.3.3       Functions
//
//       10.3.3.1       TicketIsSafe()
//
//       This function indicates if producing a ticket is safe. It checks if the leading bytes of an input buffer is
//       TPM_GENERATED_VALUE or its substring of canonical form. If so, it is not safe to produce ticket for an
//       input buffer claiming to be TPM generated buffer
//
//       Return Value                      Meaning
//
//       TRUE                              It is safe to produce ticket
//       FALSE                             It is not safe to produce ticket
//
BOOL
TicketIsSafe(
      TPM2B                *buffer
      )
{
      TPM_GENERATED        valueToCompare = TPM_GENERATED_VALUE;
      BYTE                 bufferToCompare[sizeof(valueToCompare)];
      BYTE                 *marshalBuffer;
      INT32                bufferSize;
      // If the buffer size is less than the size of TPM_GENERATED_VALUE, assume
      // it is not safe to generate a ticket
      if(buffer->size < sizeof(valueToCompare))
          return FALSE;
      marshalBuffer = bufferToCompare;
      bufferSize = sizeof(TPM_GENERATED);
   TPM_GENERATED_Marshal(&valueToCompare, &marshalBuffer, &bufferSize);
   if(MemoryEqual(buffer->buffer, bufferToCompare, sizeof(valueToCompare)))
       return FALSE;
   else
       return TRUE;
}
//
//
//     10.3.3.2   TicketComputeVerified()
//
//     This function creates a TPMT_TK_VERIFIED ticket.
//
void
TicketComputeVerified(
   TPMI_RH_HIERARCHY          hierarchy,       //   IN: hierarchy constant for ticket
   TPM2B_DIGEST              *digest,          //   IN: digest
   TPM2B_NAME                *keyName,         //   IN: name of key that signed the value
   TPMT_TK_VERIFIED          *ticket           //   OUT: verified ticket
   )
{
   TPM2B_AUTH                *proof;
   HMAC_STATE                 hmacState;
   // Fill in ticket fields
   ticket->tag = TPM_ST_VERIFIED;
   ticket->hierarchy = hierarchy;
   // Use the proof value of the hierarchy
   proof = HierarchyGetProof(hierarchy);
   // Start HMAC
   ticket->digest.t.size = CryptStartHMAC2B(CONTEXT_INTEGRITY_HASH_ALG,
                                            &proof->b, &hmacState);
   // add TPM_ST_VERIFIED
   CryptUpdateDigestInt(&hmacState, sizeof(TPM_ST), &ticket->tag);
   // add digest
   CryptUpdateDigest2B(&hmacState, &digest->b);
   // add key name
   CryptUpdateDigest2B(&hmacState, &keyName->b);
   // complete HMAC
   CryptCompleteHMAC2B(&hmacState, &ticket->digest.b);
   return;
}
//
//
//     10.3.3.3   TicketComputeAuth()
//
//     This function creates a TPMT_TK_AUTH ticket.
//
void
TicketComputeAuth(
   TPM_ST                     type,            //   IN: the type of ticket.
   TPMI_RH_HIERARCHY          hierarchy,       //   IN: hierarchy constant for ticket
   UINT64                     timeout,         //   IN: timeout
   TPM2B_DIGEST              *cpHashA,         //   IN: input cpHashA
   TPM2B_NONCE               *policyRef,       //   IN: input policyRef
   TPM2B_NAME                *entityName,      //   IN: name of entity
   TPMT_TK_AUTH              *ticket           //   OUT: Created ticket
   )
{
   TPM2B_AUTH              *proof;
   HMAC_STATE               hmacState;
   // Get proper proof
   proof = HierarchyGetProof(hierarchy);
   // Fill in ticket fields
   ticket->tag = type;
   ticket->hierarchy = hierarchy;
   // Start HMAC
   ticket->digest.t.size = CryptStartHMAC2B(CONTEXT_INTEGRITY_HASH_ALG,
                                            &proof->b, &hmacState);
   // Adding TPM_ST_AUTH
   CryptUpdateDigestInt(&hmacState, sizeof(UINT16), &ticket->tag);
   // Adding timeout
   CryptUpdateDigestInt(&hmacState, sizeof(UINT64), &timeout);
   // Adding cpHash
   CryptUpdateDigest2B(&hmacState, &cpHashA->b);
   // Adding policyRef
   CryptUpdateDigest2B(&hmacState, &policyRef->b);
   // Adding keyName
   CryptUpdateDigest2B(&hmacState, &entityName->b);
   // Compute HMAC
   CryptCompleteHMAC2B(&hmacState, &ticket->digest.b);
   return;
}
//
//
//      10.3.3.4   TicketComputeHashCheck()
//
//      This function creates a TPMT_TK_HASHCHECK ticket.
//
void
TicketComputeHashCheck(
   TPMI_RH_HIERARCHY        hierarchy,      //   IN: hierarchy constant for ticket
   TPM_ALG_ID               hashAlg,        //   IN: the hash algorithm used to create
                                            //       'digest'
   TPM2B_DIGEST            *digest,         //   IN: input digest
   TPMT_TK_HASHCHECK       *ticket          //   OUT: Created ticket
   )
{
   TPM2B_AUTH              *proof;
   HMAC_STATE               hmacState;
   // Get proper proof
   proof = HierarchyGetProof(hierarchy);
   // Fill in ticket fields
   ticket->tag = TPM_ST_HASHCHECK;
   ticket->hierarchy = hierarchy;
   ticket->digest.t.size = CryptStartHMAC2B(CONTEXT_INTEGRITY_HASH_ALG,
                                            &proof->b, &hmacState);
   // Add TPM_ST_HASHCHECK
   CryptUpdateDigestInt(&hmacState, sizeof(TPM_ST), &ticket->tag);
//
      // Add hash algorithm
      CryptUpdateDigestInt(&hmacState, sizeof(hashAlg), &hashAlg);
      // Add digest
      CryptUpdateDigest2B(&hmacState, &digest->b);
      // Compute HMAC
      CryptCompleteHMAC2B(&hmacState, &ticket->digest.b);
      return;
}
//
//
//      10.3.3.5     TicketComputeCreation()
//
//      This function creates a TPMT_TK_CREATION ticket.
//
void
TicketComputeCreation(
      TPMI_RH_HIERARCHY       hierarchy,        //   IN: hierarchy for ticket
      TPM2B_NAME             *name,             //   IN: object name
      TPM2B_DIGEST           *creation,         //   IN: creation hash
      TPMT_TK_CREATION       *ticket            //   OUT: created ticket
      )
{
      TPM2B_AUTH             *proof;
      HMAC_STATE              hmacState;
      // Get proper proof
      proof = HierarchyGetProof(hierarchy);
      // Fill in ticket fields
      ticket->tag = TPM_ST_CREATION;
      ticket->hierarchy = hierarchy;
      ticket->digest.t.size = CryptStartHMAC2B(CONTEXT_INTEGRITY_HASH_ALG,
                                               &proof->b, &hmacState);
      // Add TPM_ST_CREATION
      CryptUpdateDigestInt(&hmacState, sizeof(TPM_ST), &ticket->tag);
      // Add name
      CryptUpdateDigest2B(&hmacState, &name->b);
      // Add creation hash
      CryptUpdateDigest2B(&hmacState, &creation->b);
      // Compute HMAC
      CryptCompleteHMAC2B(&hmacState, &ticket->digest.b);
      return;
}
