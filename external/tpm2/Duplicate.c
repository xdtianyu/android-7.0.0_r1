// This file was extracted from the TCG Published
// Trusted Platform Module Library
// Part 3: Commands
// Family "2.0"
// Level 00 Revision 01.16
// October 30, 2014

#include "InternalRoutines.h"
#include "Duplicate_fp.h"
#include "Object_spt_fp.h"
//
//
//     Error Returns                Meaning
//
//     TPM_RC_ATTRIBUTES            key to duplicate has fixedParent SET
//     TPM_RC_HIERARCHY             encryptedDuplication is SET and newParentHandle specifies Null
//                                  Hierarchy
//     TPM_RC_KEY                   newParentHandle references invalid ECC key (public point not on the
//                                  curve)
//     TPM_RC_SIZE                  input encryption key size does not match the size specified in
//                                  symmetric algorithm
//     TPM_RC_SYMMETRIC             encryptedDuplication is SET but no symmetric algorithm is provided
//     TPM_RC_TYPE                  newParentHandle is neither a storage key nor TPM_RH_NULL; or
//                                  the object has a NULL nameAlg
//
TPM_RC
TPM2_Duplicate(
   Duplicate_In      *in,            // IN: input parameter list
   Duplicate_Out     *out            // OUT: output parameter list
   )
{
   TPM_RC                   result = TPM_RC_SUCCESS;
   TPMT_SENSITIVE           sensitive;

   UINT16                   innerKeySize = 0; // encrypt key size for inner wrap

   OBJECT                   *object;
   TPM2B_DATA               data;

// Input Validation

   // Get duplicate object pointer
   object = ObjectGet(in->objectHandle);

   // duplicate key must have fixParent bit CLEAR.
   if(object->publicArea.objectAttributes.fixedParent == SET)
       return TPM_RC_ATTRIBUTES + RC_Duplicate_objectHandle;

   // Do not duplicate object with NULL nameAlg
   if(object->publicArea.nameAlg == TPM_ALG_NULL)
       return TPM_RC_TYPE + RC_Duplicate_objectHandle;

   // new parent key must be a storage object or TPM_RH_NULL
   if(in->newParentHandle != TPM_RH_NULL
           && !ObjectIsStorage(in->newParentHandle))
       return TPM_RC_TYPE + RC_Duplicate_newParentHandle;

   // If the duplicates object has encryptedDuplication SET, then there must be
   // an inner wrapper and the new parent may not be TPM_RH_NULL
   if(object->publicArea.objectAttributes.encryptedDuplication == SET)
   {
       if(in->symmetricAlg.algorithm == TPM_ALG_NULL)
           return TPM_RC_SYMMETRIC + RC_Duplicate_symmetricAlg;
       if(in->newParentHandle == TPM_RH_NULL)
            return TPM_RC_HIERARCHY + RC_Duplicate_newParentHandle;
   }

   if(in->symmetricAlg.algorithm == TPM_ALG_NULL)
   {
       // if algorithm is TPM_ALG_NULL, input key size must be 0
       if(in->encryptionKeyIn.t.size != 0)
           return TPM_RC_SIZE + RC_Duplicate_encryptionKeyIn;
   }
   else
   {
       // Get inner wrap key size
       innerKeySize = in->symmetricAlg.keyBits.sym;

       // If provided the input symmetric key must match the size of the algorithm
       if(in->encryptionKeyIn.t.size != 0
               && in->encryptionKeyIn.t.size != (innerKeySize + 7) / 8)
           return TPM_RC_SIZE + RC_Duplicate_encryptionKeyIn;
   }

// Command Output

   if(in->newParentHandle != TPM_RH_NULL)
   {

       // Make encrypt key and its associated secret structure. A TPM_RC_KEY
       // error may be returned at this point
       out->outSymSeed.t.size = sizeof(out->outSymSeed.t.secret);
       result = CryptSecretEncrypt(in->newParentHandle,
                                   "DUPLICATE", &data, &out->outSymSeed);
       pAssert(result != TPM_RC_VALUE);
       if(result != TPM_RC_SUCCESS)
           return result;
   }
   else
   {
       // Do not apply outer wrapper
       data.t.size = 0;
       out->outSymSeed.t.size = 0;
   }

   // Copy sensitive area
   sensitive = object->sensitive;

   // Prepare output private data from sensitive
   SensitiveToDuplicate(&sensitive, &object->name, in->newParentHandle,
                        object->publicArea.nameAlg, (TPM2B_SEED *) &data,
                        &in->symmetricAlg, &in->encryptionKeyIn,
                        &out->duplicate);

   out->encryptionKeyOut = in->encryptionKeyIn;

   return TPM_RC_SUCCESS;
}
