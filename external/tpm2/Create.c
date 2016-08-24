// This file was extracted from the TCG Published
// Trusted Platform Module Library
// Part 3: Commands
// Family "2.0"
// Level 00 Revision 01.16
// October 30, 2014

#include "InternalRoutines.h"
#include "Object_spt_fp.h"
#include "Create_fp.h"
//
//
//     Error Returns               Meaning
//
//     TPM_RC_ASYMMETRIC           non-duplicable storage key and its parent have different public
//                                 parameters
//     TPM_RC_ATTRIBUTES           sensitiveDataOrigin is CLEAR when 'sensitive.data' is an Empty
//                                 Buffer, or is SET when 'sensitive.data' is not empty; fixedTPM,
//                                 fixedParent, or encryptedDuplication attributes are inconsistent
//                                 between themselves or with those of the parent object; inconsistent
//                                 restricted, decrypt and sign attributes; attempt to inject sensitive data
//                                 for an asymmetric key; attempt to create a symmetric cipher key that
//                                 is not a decryption key
//     TPM_RC_HASH                 non-duplicable storage key and its parent have different name
//                                 algorithm
//     TPM_RC_KDF                  incorrect KDF specified for decrypting keyed hash object
//     TPM_RC_KEY                  invalid key size values in an asymmetric key public area
//     TPM_RC_KEY_SIZE             key size in public area for symmetric key differs from the size in the
//                                 sensitive creation area; may also be returned if the TPM does not
//                                 allow the key size to be used for a Storage Key
//     TPM_RC_RANGE                the exponent value of an RSA key is not supported.
//     TPM_RC_SCHEME               inconsistent attributes decrypt, sign, restricted and key's scheme ID;
//                                 or hash algorithm is inconsistent with the scheme ID for keyed hash
//                                 object
//     TPM_RC_SIZE                 size of public auth policy or sensitive auth value does not match
//                                 digest size of the name algorithm sensitive data size for the keyed
//                                 hash object is larger than is allowed for the scheme
//     TPM_RC_SYMMETRIC            a storage key with no symmetric algorithm specified; or non-storage
//                                 key with symmetric algorithm different from TPM_ALG_NULL
//     TPM_RC_TYPE                 unknown object type; non-duplicable storage key and its parent have
//                                 different types; parentHandle does not reference a restricted
//                                 decryption key in the storage hierarchy with both public and sensitive
//                                 portion loaded
//     TPM_RC_VALUE                exponent is not prime or could not find a prime using the provided
//                                 parameters for an RSA key; unsupported name algorithm for an ECC
//                                 key
//     TPM_RC_OBJECT_MEMORY        there is no free slot for the object. This implementation does not
//                                 return this error.
//
TPM_RC
TPM2_Create(
   Create_In        *in,            // IN: input parameter list
   Create_Out       *out            // OUT: output parameter list
   )
{
   TPM_RC                  result = TPM_RC_SUCCESS;
   TPMT_SENSITIVE          sensitive;
   TPM2B_NAME              name;

// Input Validation

   OBJECT       *parentObject;

   parentObject = ObjectGet(in->parentHandle);

   // Does parent have the proper attributes?
   if(!AreAttributesForParent(parentObject))
       return TPM_RC_TYPE + RC_Create_parentHandle;

   // The sensitiveDataOrigin attribute must be consistent with the setting of
   // the size of the data object in inSensitive.
   if(   (in->inPublic.t.publicArea.objectAttributes.sensitiveDataOrigin == SET)
      != (in->inSensitive.t.sensitive.data.t.size == 0))
       // Mismatch between the object attributes and the parameter.
       return TPM_RC_ATTRIBUTES + RC_Create_inSensitive;

   // Check attributes in input public area. TPM_RC_ASYMMETRIC, TPM_RC_ATTRIBUTES,
   // TPM_RC_HASH, TPM_RC_KDF, TPM_RC_SCHEME, TPM_RC_SIZE, TPM_RC_SYMMETRIC,
   // or TPM_RC_TYPE error may be returned at this point.
   result = PublicAttributesValidation(FALSE, in->parentHandle,
                                       &in->inPublic.t.publicArea);
   if(result != TPM_RC_SUCCESS)
       return RcSafeAddToResult(result, RC_Create_inPublic);

   // Validate the sensitive area values
   if( MemoryRemoveTrailingZeros(&in->inSensitive.t.sensitive.userAuth)
           > CryptGetHashDigestSize(in->inPublic.t.publicArea.nameAlg))
       return TPM_RC_SIZE + RC_Create_inSensitive;

// Command Output

   // Create object crypto data
   result = CryptCreateObject(in->parentHandle, &in->inPublic.t.publicArea,
                              &in->inSensitive.t.sensitive, &sensitive);
   if(result != TPM_RC_SUCCESS)
       return result;

   // Fill in creation data
   FillInCreationData(in->parentHandle, in->inPublic.t.publicArea.nameAlg,
                      &in->creationPCR, &in->outsideInfo,
                      &out->creationData, &out->creationHash);

   // Copy public area from input to output
   out->outPublic.t.publicArea = in->inPublic.t.publicArea;

   // Compute name from public area
   ObjectComputeName(&(out->outPublic.t.publicArea), &name);

   // Compute creation ticket
   TicketComputeCreation(EntityGetHierarchy(in->parentHandle), &name,
                         &out->creationHash, &out->creationTicket);

   // Prepare output private data from sensitive
   SensitiveToPrivate(&sensitive, &name, in->parentHandle,
                      out->outPublic.t.publicArea.nameAlg,
                      &out->outPrivate);

   return TPM_RC_SUCCESS;
}
