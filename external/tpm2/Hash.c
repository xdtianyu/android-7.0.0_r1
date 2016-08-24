// This file was extracted from the TCG Published
// Trusted Platform Module Library
// Part 3: Commands
// Family "2.0"
// Level 00 Revision 01.16
// October 30, 2014

#include "InternalRoutines.h"
#include "Hash_fp.h"
TPM_RC
TPM2_Hash(
   Hash_In         *in,            // IN: input parameter list
   Hash_Out        *out            // OUT: output parameter list
   )
{
   HASH_STATE         hashState;

// Command Output

   // Output hash
       // Start hash stack
   out->outHash.t.size = CryptStartHash(in->hashAlg, &hashState);
       // Adding hash data
   CryptUpdateDigest2B(&hashState, &in->data.b);
       // Complete hash
   CryptCompleteHash2B(&hashState, &out->outHash.b);

   // Output ticket
   out->validation.tag = TPM_ST_HASHCHECK;
   out->validation.hierarchy = in->hierarchy;

   if(in->hierarchy == TPM_RH_NULL)
   {
       // Ticket is not required
       out->validation.hierarchy = TPM_RH_NULL;
       out->validation.digest.t.size = 0;
   }
   else if( in->data.t.size >= sizeof(TPM_GENERATED)
           && !TicketIsSafe(&in->data.b))
   {
       // Ticket is not safe
       out->validation.hierarchy = TPM_RH_NULL;
       out->validation.digest.t.size = 0;
   }
   else
   {
       // Compute ticket
       TicketComputeHashCheck(in->hierarchy, in->hashAlg,
                              &out->outHash, &out->validation);
   }

   return TPM_RC_SUCCESS;
}
