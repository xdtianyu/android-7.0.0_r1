// This file was extracted from the TCG Published
// Trusted Platform Module Library
// Part 3: Commands
// Family "2.0"
// Level 00 Revision 01.16
// October 30, 2014

#include "InternalRoutines.h"
#include "Unseal_fp.h"
//
//
//     Error Returns                     Meaning
//
//     TPM_RC_ATTRIBUTES                 itemHandle has wrong attributes
//     TPM_RC_TYPE                       itemHandle is not a KEYEDHASH data object
//
TPM_RC
TPM2_Unseal(
   Unseal_In         *in,
   Unseal_Out        *out
   )
{
   OBJECT                    *object;

// Input Validation

   // Get pointer to loaded object
   object = ObjectGet(in->itemHandle);

   // Input handle must be a data object
   if(object->publicArea.type != TPM_ALG_KEYEDHASH)
       return TPM_RC_TYPE + RC_Unseal_itemHandle;
   if(   object->publicArea.objectAttributes.decrypt == SET
      || object->publicArea.objectAttributes.sign == SET
      || object->publicArea.objectAttributes.restricted == SET)
       return TPM_RC_ATTRIBUTES + RC_Unseal_itemHandle;

// Command Output

   // Copy data
   MemoryCopy2B(&out->outData.b, &object->sensitive.sensitive.bits.b,
                sizeof(out->outData.t.buffer));

   return TPM_RC_SUCCESS;
}
