// This file was extracted from the TCG Published
// Trusted Platform Module Library
// Part 3: Commands
// Family "2.0"
// Level 00 Revision 01.16
// October 30, 2014

#include "InternalRoutines.h"
#include "Load_fp.h"
#include "Object_spt_fp.h"
//
//
//     Error Returns               Meaning
//
//     TPM_RC_ASYMMETRIC           storage key with different asymmetric type than parent
//     TPM_RC_ATTRIBUTES           inPulblic attributes are not allowed with selected parent
//     TPM_RC_BINDING              inPrivate and inPublic are not cryptographically bound
//     TPM_RC_HASH                 incorrect hash selection for signing key
//     TPM_RC_INTEGRITY            HMAC on inPrivate was not valid
//     TPM_RC_KDF                  KDF selection not allowed
//     TPM_RC_KEY                  the size of the object's unique field is not consistent with the indicated
//                                 size in the object's parameters
//     TPM_RC_OBJECT_MEMORY        no available object slot
//     TPM_RC_SCHEME               the signing scheme is not valid for the key
//     TPM_RC_SENSITIVE            the inPrivate did not unmarshal correctly
//     TPM_RC_SIZE                 inPrivate missing, or authPolicy size for inPublic or is not valid
//     TPM_RC_SYMMETRIC            symmetric algorithm not provided when required
//     TPM_RC_TYPE                 parentHandle is not a storage key, or the object to load is a storage
//                                 key but its parameters do not match the parameters of the parent.
//     TPM_RC_VALUE                decryption failure
//
TPM_RC
TPM2_Load(
   Load_In         *in,             // IN: input parameter list
   Load_Out        *out             // OUT: output parameter list
   )
{
   TPM_RC                  result = TPM_RC_SUCCESS;
   TPMT_SENSITIVE          sensitive;
   TPMI_RH_HIERARCHY       hierarchy;
   OBJECT                 *parentObject = NULL;
   BOOL                    skipChecks = FALSE;

// Input Validation
   if(in->inPrivate.t.size == 0)
       return TPM_RC_SIZE + RC_Load_inPrivate;

   parentObject = ObjectGet(in->parentHandle);
   // Is the object that is being used as the parent actually a parent.
   if(!AreAttributesForParent(parentObject))
       return TPM_RC_TYPE + RC_Load_parentHandle;

   // If the parent is fixedTPM, then the attributes of the object
   // are either "correct by construction" or were validated
   // when the object was imported. If they pass the integrity
   // check, then the values are valid
   if(parentObject->publicArea.objectAttributes.fixedTPM)
       skipChecks = TRUE;
   else
   {
       // If parent doesn't have fixedTPM SET, then this can't have
       // fixedTPM SET.
       if(in->inPublic.t.publicArea.objectAttributes.fixedTPM == SET)
           return TPM_RC_ATTRIBUTES + RC_Load_inPublic;

       // Perform self check on input public area. A TPM_RC_SIZE, TPM_RC_SCHEME,
       // TPM_RC_VALUE, TPM_RC_SYMMETRIC, TPM_RC_TYPE, TPM_RC_HASH,
       // TPM_RC_ASYMMETRIC, TPM_RC_ATTRIBUTES or TPM_RC_KDF error may be returned
       // at this point
       result = PublicAttributesValidation(TRUE, in->parentHandle,
                                           &in->inPublic.t.publicArea);
       if(result != TPM_RC_SUCCESS)
           return RcSafeAddToResult(result, RC_Load_inPublic);
   }

   // Compute the name of object
   ObjectComputeName(&in->inPublic.t.publicArea, &out->name);

   // Retrieve sensitive data. PrivateToSensitive() may return TPM_RC_INTEGRITY or
   // TPM_RC_SENSITIVE
   // errors may be returned at this point
   result = PrivateToSensitive(&in->inPrivate, &out->name, in->parentHandle,
                               in->inPublic.t.publicArea.nameAlg,
                               &sensitive);
   if(result != TPM_RC_SUCCESS)
       return RcSafeAddToResult(result, RC_Load_inPrivate);

// Internal Data Update

   // Get hierarchy of parent
   hierarchy = ObjectGetHierarchy(in->parentHandle);

   // Create internal object. A lot of different errors may be returned by this
   // loading operation as it will do several validations, including the public
   // binding check
   result = ObjectLoad(hierarchy, &in->inPublic.t.publicArea, &sensitive,
                       &out->name, in->parentHandle, skipChecks,
                       &out->objectHandle);

   if(result != TPM_RC_SUCCESS)
       return result;

   return TPM_RC_SUCCESS;
}
