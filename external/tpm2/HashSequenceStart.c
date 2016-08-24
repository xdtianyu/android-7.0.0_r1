// This file was extracted from the TCG Published
// Trusted Platform Module Library
// Part 3: Commands
// Family "2.0"
// Level 00 Revision 01.16
// October 30, 2014

#include "InternalRoutines.h"
#include "HashSequenceStart_fp.h"
//
//
//     Error Returns                     Meaning
//
//     TPM_RC_OBJECT_MEMORY              no space to create an internal object
//
TPM_RC
TPM2_HashSequenceStart(
   HashSequenceStart_In      *in,                   // IN: input parameter list
   HashSequenceStart_Out     *out                   // OUT: output parameter list
   )
{
// Internal Data Update

   if(in->hashAlg == TPM_ALG_NULL)
       // Start a event sequence. A TPM_RC_OBJECT_MEMORY error may be
       // returned at this point
       return ObjectCreateEventSequence(&in->auth, &out->sequenceHandle);

   // Start a hash sequence. A TPM_RC_OBJECT_MEMORY error may be
   // returned at this point
   return ObjectCreateHashSequence(in->hashAlg, &in->auth, &out->sequenceHandle);
}
