// This file was extracted from the TCG Published
// Trusted Platform Module Library
// Part 3: Commands
// Family "2.0"
// Level 00 Revision 01.16
// October 30, 2014

#include "InternalRoutines.h"
#include "ObjectChangeAuth_fp.h"
#include "Object_spt_fp.h"
//
//
//     Error Returns               Meaning
//
//     TPM_RC_SIZE                 newAuth is larger than the size of the digest of the Name algorithm of
//                                 objectHandle
//     TPM_RC_TYPE                 the key referenced by parentHandle is not the parent of the object
//                                 referenced by objectHandle; or objectHandle is a sequence object.
//
TPM_RC
TPM2_ObjectChangeAuth(
   ObjectChangeAuth_In    *in,                // IN: input parameter list
   ObjectChangeAuth_Out   *out                // OUT: output parameter list
   )
{
   TPMT_SENSITIVE          sensitive;

   OBJECT                 *object;
   TPM2B_NAME              objectQN, QNCompare;
   TPM2B_NAME              parentQN;

// Input Validation

   // Get object pointer
   object = ObjectGet(in->objectHandle);

   // Can not change auth on sequence object
   if(ObjectIsSequence(object))
       return TPM_RC_TYPE + RC_ObjectChangeAuth_objectHandle;

   // Make sure that the auth value is consistent with the nameAlg
   if( MemoryRemoveTrailingZeros(&in->newAuth)
           > CryptGetHashDigestSize(object->publicArea.nameAlg))
       return TPM_RC_SIZE + RC_ObjectChangeAuth_newAuth;

   // Check parent for object
   // parent handle must be the parent of object handle. In this
   // implementation we verify this by checking the QN of object. Other
   // implementation may choose different method to verify this attribute.
   ObjectGetQualifiedName(in->parentHandle, &parentQN);
   ObjectComputeQualifiedName(&parentQN, object->publicArea.nameAlg,
                              &object->name, &QNCompare);

   ObjectGetQualifiedName(in->objectHandle, &objectQN);
   if(!Memory2BEqual(&objectQN.b, &QNCompare.b))
       return TPM_RC_TYPE + RC_ObjectChangeAuth_parentHandle;

// Command Output

   // Copy internal sensitive area
   sensitive = object->sensitive;
   // Copy authValue
   sensitive.authValue = in->newAuth;

   // Prepare output private data from sensitive
   SensitiveToPrivate(&sensitive, &object->name, in->parentHandle,
                      object->publicArea.nameAlg,
                       &out->outPrivate);

   return TPM_RC_SUCCESS;
}
