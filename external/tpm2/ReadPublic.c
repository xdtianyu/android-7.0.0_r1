// This file was extracted from the TCG Published
// Trusted Platform Module Library
// Part 3: Commands
// Family "2.0"
// Level 00 Revision 01.16
// October 30, 2014

#include "InternalRoutines.h"
#include "ReadPublic_fp.h"
//
//
//     Error Returns                     Meaning
//
//     TPM_RC_SEQUENCE                   can not read the public area of a sequence object
//
TPM_RC
TPM2_ReadPublic(
   ReadPublic_In     *in,                // IN: input parameter list
   ReadPublic_Out    *out                // OUT: output parameter list
   )
{
   OBJECT                    *object;

// Input Validation

   // Get loaded object pointer
   object = ObjectGet(in->objectHandle);

   // Can not read public area of a sequence object
   if(ObjectIsSequence(object))
       return TPM_RC_SEQUENCE;

// Command Output

   // Compute size of public area in canonical form
   out->outPublic.t.size = TPMT_PUBLIC_Marshal(&object->publicArea, NULL, NULL);

   // Copy public area to output
   out->outPublic.t.publicArea = object->publicArea;

   // Copy name to output
   out->name.t.size = ObjectGetName(in->objectHandle, &out->name.t.name);

   // Copy qualified name to output
   ObjectGetQualifiedName(in->objectHandle, &out->qualifiedName);

   return TPM_RC_SUCCESS;
}
