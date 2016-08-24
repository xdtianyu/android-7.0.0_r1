// This file was extracted from the TCG Published
// Trusted Platform Module Library
// Part 4: Supporting Routines
// Family "2.0"
// Level 00 Revision 01.16
// October 30, 2014

#include "InternalRoutines.h"
#include "Object_spt_fp.h"
#include "Platform.h"
//
//
//
//          Local Functions
//
//          EqualCryptSet()
//
//     Check if the crypto sets in two public areas are equal
//
//     Error Returns                     Meaning
//
//     TPM_RC_ASYMMETRIC                 mismatched parameters
//     TPM_RC_HASH                       mismatched name algorithm
//     TPM_RC_TYPE                       mismatched type
//
static TPM_RC
EqualCryptSet(
   TPMT_PUBLIC         *publicArea1,        // IN: public area 1
   TPMT_PUBLIC         *publicArea2         // IN: public area 2
   )
{
   UINT16                   size1;
   UINT16                   size2;
   BYTE                     params1[sizeof(TPMU_PUBLIC_PARMS)];
   BYTE                     params2[sizeof(TPMU_PUBLIC_PARMS)];
   BYTE                     *buffer;
   INT32                    bufferSize;
   // Compare name hash
   if(publicArea1->nameAlg != publicArea2->nameAlg)
       return TPM_RC_HASH;
   // Compare algorithm
   if(publicArea1->type != publicArea2->type)
       return TPM_RC_TYPE;
   // TPMU_PUBLIC_PARMS field should be identical
   buffer = params1;
   bufferSize = sizeof(TPMU_PUBLIC_PARMS);
   size1 = TPMU_PUBLIC_PARMS_Marshal(&publicArea1->parameters, &buffer,
                                     &bufferSize, publicArea1->type);
   buffer = params2;
   bufferSize = sizeof(TPMU_PUBLIC_PARMS);
   size2 = TPMU_PUBLIC_PARMS_Marshal(&publicArea2->parameters, &buffer,
                                     &bufferSize, publicArea2->type);
   if(size1 != size2 || !MemoryEqual(params1, params2, size1))
       return TPM_RC_ASYMMETRIC;
   return TPM_RC_SUCCESS;
}
//
//
//          GetIV2BSize()
//
//     Get the size of TPM2B_IV in canonical form that will be append to the start of the sensitive data. It
//     includes both size of size field and size of iv data
//
//     Return Value                      Meaning
//
static UINT16
GetIV2BSize(
   TPM_HANDLE            protectorHandle           // IN: the protector handle
   )
{
   OBJECT                   *protector = NULL; // Pointer to the protector object
   TPM_ALG_ID               symAlg;
//
   UINT16                    keyBits;
   // Determine the symmetric algorithm and size of key
   if(protectorHandle == TPM_RH_NULL)
   {
       // Use the context encryption algorithm and key size
       symAlg = CONTEXT_ENCRYPT_ALG;
       keyBits = CONTEXT_ENCRYPT_KEY_BITS;
   }
   else
   {
       protector = ObjectGet(protectorHandle);
       symAlg = protector->publicArea.parameters.asymDetail.symmetric.algorithm;
       keyBits= protector->publicArea.parameters.asymDetail.symmetric.keyBits.sym;
   }
   // The IV size is a UINT16 size field plus the block size of the symmetric
   // algorithm
   return sizeof(UINT16) + CryptGetSymmetricBlockSize(symAlg, keyBits);
}
//
//
//         ComputeProtectionKeyParms()
//
//     This function retrieves the symmetric protection key parameters for the sensitive data The parameters
//     retrieved from this function include encryption algorithm, key size in bit, and a TPM2B_SYM_KEY
//     containing the key material as well as the key size in bytes This function is used for any action that
//     requires encrypting or decrypting of the sensitive area of an object or a credential blob
//
static void
ComputeProtectionKeyParms(
   TPM_HANDLE          protectorHandle,       //   IN: the protector handle
   TPM_ALG_ID          hashAlg,               //   IN: hash algorithm for KDFa
   TPM2B_NAME         *name,                  //   IN: name of the object
   TPM2B_SEED         *seedIn,                //   IN: optional seed for duplication blob.
                                              //       For non duplication blob, this
                                              //       parameter should be NULL
   TPM_ALG_ID         *symAlg,                //   OUT: the symmetric algorithm
   UINT16             *keyBits,               //   OUT: the symmetric key size in bits
   TPM2B_SYM_KEY      *symKey                 //   OUT: the symmetric key
   )
{
   TPM2B_SEED                *seed = NULL;
   OBJECT                    *protector = NULL; // Pointer to the protector
   // Determine the algorithms for the KDF and the encryption/decryption
   // For TPM_RH_NULL, using context settings
   if(protectorHandle == TPM_RH_NULL)
   {
       // Use the context encryption algorithm and key size
       *symAlg = CONTEXT_ENCRYPT_ALG;
       symKey->t.size = CONTEXT_ENCRYPT_KEY_BYTES;
       *keyBits = CONTEXT_ENCRYPT_KEY_BITS;
   }
   else
   {
       TPMT_SYM_DEF_OBJECT *symDef;
       protector = ObjectGet(protectorHandle);
       symDef = &protector->publicArea.parameters.asymDetail.symmetric;
       *symAlg = symDef->algorithm;
       *keyBits= symDef->keyBits.sym;
       symKey->t.size = (*keyBits + 7) / 8;
   }
   // Get seed for KDF
   seed = GetSeedForKDF(protectorHandle, seedIn);
   // KDFa to generate symmetric key and IV value
   KDFa(hashAlg, (TPM2B *)seed, "STORAGE", (TPM2B *)name, NULL,
        symKey->t.size * 8, symKey->t.buffer, NULL);
   return;
}
//
//
//           ComputeOuterIntegrity()
//
//      The sensitive area parameter is a buffer that holds a space for the integrity value and the marshaled
//      sensitive area. The caller should skip over the area set aside for the integrity value and compute the hash
//      of the remainder of the object. The size field of sensitive is in unmarshaled form and the sensitive area
//      contents is an array of bytes.
//
static void
ComputeOuterIntegrity(
   TPM2B_NAME          *name,                   //   IN: the name of the object
   TPM_HANDLE           protectorHandle,        //   IN: The handle of the object that
                                                //       provides protection. For object, it
                                                //       is parent handle. For credential, it
                                                //       is the handle of encrypt object. For
                                                //       a Temporary Object, it is TPM_RH_NULL
   TPMI_ALG_HASH        hashAlg,                //   IN: algorithm to use for integrity
   TPM2B_SEED          *seedIn,                 //   IN: an external seed may be provided for
                                                //       duplication blob. For non duplication
                                                //       blob, this parameter should be NULL
   UINT32               sensitiveSize,          //   IN: size of the marshaled sensitive data
   BYTE                *sensitiveData,          //   IN: sensitive area
   TPM2B_DIGEST        *integrity               //   OUT: integrity
   )
{
   HMAC_STATE               hmacState;
   TPM2B_DIGEST             hmacKey;
   TPM2B_SEED               *seed = NULL;
   // Get seed for KDF
   seed = GetSeedForKDF(protectorHandle, seedIn);
   // Determine the HMAC key bits
   hmacKey.t.size = CryptGetHashDigestSize(hashAlg);
   // KDFa to generate HMAC key
   KDFa(hashAlg, (TPM2B *)seed, "INTEGRITY", NULL, NULL,
        hmacKey.t.size * 8, hmacKey.t.buffer, NULL);
   // Start HMAC and get the size of the digest which will become the integrity
   integrity->t.size = CryptStartHMAC2B(hashAlg, &hmacKey.b, &hmacState);
   // Adding the marshaled sensitive area to the integrity value
   CryptUpdateDigest(&hmacState, sensitiveSize, sensitiveData);
   // Adding name
   CryptUpdateDigest2B(&hmacState, (TPM2B *)name);
   // Compute HMAC
   CryptCompleteHMAC2B(&hmacState, &integrity->b);
   return;
}
//
//
//           ComputeInnerIntegrity()
//
//      This function computes the integrity of an inner wrap
//
static void
ComputeInnerIntegrity(
    TPM_ALG_ID           hashAlg,           //   IN: hash algorithm for inner wrap
    TPM2B_NAME          *name,              //   IN: the name of the object
    UINT16               dataSize,          //   IN: the size of sensitive data
    BYTE                *sensitiveData,     //   IN: sensitive data
    TPM2B_DIGEST        *integrity          //   OUT: inner integrity
    )
{
    HASH_STATE          hashState;
    // Start hash and get the size of the digest which will become the integrity
    integrity->t.size = CryptStartHash(hashAlg, &hashState);
    // Adding the marshaled sensitive area to the integrity value
    CryptUpdateDigest(&hashState, dataSize, sensitiveData);
    // Adding name
    CryptUpdateDigest2B(&hashState, &name->b);
    // Compute hash
    CryptCompleteHash2B(&hashState, &integrity->b);
    return;
}
//
//
//           ProduceInnerIntegrity()
//
//      This function produces an inner integrity for regular private, credential or duplication blob It requires the
//      sensitive data being marshaled to the innerBuffer, with the leading bytes reserved for integrity hash. It
//      assume the sensitive data starts at address (innerBuffer + integrity size). This function integrity at the
//      beginning of the inner buffer It returns the total size of buffer with the inner wrap
//
static UINT16
ProduceInnerIntegrity(
    TPM2B_NAME          *name,              //   IN: the name of the object
    TPM_ALG_ID           hashAlg,           //   IN: hash algorithm for inner wrap
    UINT16               dataSize,          //   IN: the size of sensitive data, excluding the
                                            //       leading integrity buffer size
    BYTE                *innerBuffer        //   IN/OUT: inner buffer with sensitive data in
                                            //       it. At input, the leading bytes of this
                                            //       buffer is reserved for integrity
    )
{
    BYTE                     *sensitiveData; // pointer to the sensitive data
    TPM2B_DIGEST             integrity;
    UINT16                   integritySize;
    BYTE                     *buffer;             // Auxiliary buffer pointer
    INT32                    bufferSize;
    // sensitiveData points to the beginning of sensitive data in innerBuffer
    integritySize = sizeof(UINT16) + CryptGetHashDigestSize(hashAlg);
    sensitiveData = innerBuffer + integritySize;
    ComputeInnerIntegrity(hashAlg, name, dataSize, sensitiveData, &integrity);
    // Add integrity at the beginning of inner buffer
    buffer = innerBuffer;
    bufferSize = sizeof(TPM2B_DIGEST);
    TPM2B_DIGEST_Marshal(&integrity, &buffer, &bufferSize);
    return dataSize + integritySize;
}
//
//
//           CheckInnerIntegrity()
//
//      This function check integrity of inner blob
//
//      Error Returns                     Meaning
//
//      TPM_RC_INTEGRITY                  if the outer blob integrity is bad
//      unmarshal errors                  unmarshal errors while unmarshaling integrity
//
static TPM_RC
CheckInnerIntegrity(
    TPM2B_NAME          *name,                //   IN: the name of the object
    TPM_ALG_ID           hashAlg,             //   IN: hash algorithm for inner wrap
    UINT16               dataSize,            //   IN: the size of sensitive data, including the
                                              //       leading integrity buffer size
    BYTE                *innerBuffer          //   IN/OUT: inner buffer with sensitive data in
                                              //       it
    )
{
    TPM_RC              result;
    TPM2B_DIGEST        integrity;
    TPM2B_DIGEST        integrityToCompare;
    BYTE                *buffer;                          // Auxiliary buffer pointer
    INT32               size;
    // Unmarshal integrity
    buffer = innerBuffer;
    size = (INT32) dataSize;
    result = TPM2B_DIGEST_Unmarshal(&integrity, &buffer, &size);
    if(result == TPM_RC_SUCCESS)
    {
        // Compute integrity to compare
        ComputeInnerIntegrity(hashAlg, name, (UINT16) size, buffer,
                              &integrityToCompare);
         // Compare outer blob integrity
         if(!Memory2BEqual(&integrity.b, &integrityToCompare.b))
             result = TPM_RC_INTEGRITY;
    }
    return result;
}
//
//
//           Public Functions
//
//           AreAttributesForParent()
//
//      This function is called by create, load, and import functions.
//
//      Return Value                      Meaning
//
//      TRUE                              properties are those of a parent
//      FALSE                             properties are not those of a parent
//
BOOL
AreAttributesForParent(
   OBJECT             *parentObject        // IN: parent handle
   )
{
   // This function is only called when a parent is needed. Any
   // time a "parent" is used, it must be authorized. When
   // the authorization is checked, both the public and sensitive
   // areas must be loaded. Just make sure...
   pAssert(parentObject->attributes.publicOnly == CLEAR);
   if(ObjectDataIsStorage(&parentObject->publicArea))
       return TRUE;
   else
       return FALSE;
}
//
//
//          SchemeChecks()
//
//      This function validates the schemes in the public area of an object. This function is called by
//      TPM2_LoadExternal() and PublicAttributesValidation().
//
//      Error Returns                   Meaning
//
//      TPM_RC_ASYMMETRIC               non-duplicable storage key and its parent have different public
//                                      parameters
//      TPM_RC_ATTRIBUTES               attempt to inject sensitive data for an asymmetric key; or attempt to
//                                      create a symmetric cipher key that is not a decryption key
//      TPM_RC_HASH                     non-duplicable storage key and its parent have different name
//                                      algorithm
//      TPM_RC_KDF                      incorrect KDF specified for decrypting keyed hash object
//      TPM_RC_KEY                      invalid key size values in an asymmetric key public area
//      TPM_RC_SCHEME                   inconsistent attributes decrypt, sign, restricted and key's scheme ID;
//                                      or hash algorithm is inconsistent with the scheme ID for keyed hash
//                                      object
//      TPM_RC_SYMMETRIC                a storage key with no symmetric algorithm specified; or non-storage
//                                      key with symmetric algorithm different from TPM_ALG_NULL
//      TPM_RC_TYPE                     unexpected object type; or non-duplicable storage key and its parent
//                                      have different types
//
TPM_RC
SchemeChecks(
   BOOL                load,               // IN: TRUE if load checks, FALSE if
                                           //     TPM2_Create()
   TPMI_DH_OBJECT      parentHandle,       // IN: input parent handle
   TPMT_PUBLIC        *publicArea          // IN: public area of the object
   )
{
   // Checks for an asymmetric key
   if(CryptIsAsymAlgorithm(publicArea->type))
   {
       TPMT_ASYM_SCHEME        *keyScheme;
       keyScheme = &publicArea->parameters.asymDetail.scheme;
         // An asymmetric key can't be injected
         // This is only checked when creating an object
         if(!load && (publicArea->objectAttributes.sensitiveDataOrigin == CLEAR))
             return TPM_RC_ATTRIBUTES;
         if(load && !CryptAreKeySizesConsistent(publicArea))
             return TPM_RC_KEY;
         // Keys that are both signing and decrypting must have TPM_ALG_NULL
         // for scheme
         if(     publicArea->objectAttributes.sign == SET
             && publicArea->objectAttributes.decrypt == SET
             && keyScheme->scheme != TPM_ALG_NULL)
              return TPM_RC_SCHEME;
         // A restrict sign key must have a non-NULL scheme
         if(     publicArea->objectAttributes.restricted == SET
             && publicArea->objectAttributes.sign == SET
             && keyScheme->scheme == TPM_ALG_NULL)
             return TPM_RC_SCHEME;
         // Keys must have a valid sign or decrypt scheme, or a TPM_ALG_NULL
         // scheme
         // NOTE: The unmarshaling for a public area will unmarshal based on the
         // object type. If the type is an RSA key, then only RSA schemes will be
         // allowed because a TPMI_ALG_RSA_SCHEME will be unmarshaled and it
         // consists only of those algorithms that are allowed with an RSA key.
         // This means that there is no need to again make sure that the algorithm
         // is compatible with the object type.
         if(    keyScheme->scheme != TPM_ALG_NULL
             && (    (    publicArea->objectAttributes.sign == SET
                       && !CryptIsSignScheme(keyScheme->scheme)
                     )
                  || (    publicArea->objectAttributes.decrypt == SET
                       && !CryptIsDecryptScheme(keyScheme->scheme)
                     )
                )
           )
              return TPM_RC_SCHEME;
       // Special checks for an ECC key
#ifdef TPM_ALG_ECC
       if(publicArea->type == TPM_ALG_ECC)
       {
           TPM_ECC_CURVE        curveID = publicArea->parameters.eccDetail.curveID;
           const TPMT_ECC_SCHEME *curveScheme = CryptGetCurveSignScheme(curveID);
           // The curveId must be valid or the unmarshaling is busted.
           pAssert(curveScheme != NULL);
             // If the curveID requires a specific scheme, then the key must select
             // the same scheme
             if(curveScheme->scheme != TPM_ALG_NULL)
             {
                 if(keyScheme->scheme != curveScheme->scheme)
                      return TPM_RC_SCHEME;
                 // The scheme can allow any hash, or not...
                 if(    curveScheme->details.anySig.hashAlg != TPM_ALG_NULL
                     && (   keyScheme->details.anySig.hashAlg
                         != curveScheme->details.anySig.hashAlg
                        )
                   )
                      return TPM_RC_SCHEME;
             }
             // For now, the KDF must be TPM_ALG_NULL
             if(publicArea->parameters.eccDetail.kdf.scheme != TPM_ALG_NULL)
                 return TPM_RC_KDF;
         }
#endif
         // Checks for a storage key (restricted + decryption)
         if(   publicArea->objectAttributes.restricted == SET
              && publicArea->objectAttributes.decrypt == SET)
        {
              // A storage key must have a valid protection key
              if(    publicArea->parameters.asymDetail.symmetric.algorithm
                  == TPM_ALG_NULL)
                   return TPM_RC_SYMMETRIC;
              // A storage key must have a null scheme
              if(publicArea->parameters.asymDetail.scheme.scheme != TPM_ALG_NULL)
                  return TPM_RC_SCHEME;
              // A storage key must match its parent algorithms unless
              // it is duplicable or a primary (including Temporary Primary Objects)
              if(    HandleGetType(parentHandle) != TPM_HT_PERMANENT
                  && publicArea->objectAttributes.fixedParent == SET
                )
              {
                   // If the object to be created is a storage key, and is fixedParent,
                   // its crypto set has to match its parent's crypto set. TPM_RC_TYPE,
                   // TPM_RC_HASH or TPM_RC_ASYMMETRIC may be returned at this point
                   return EqualCryptSet(publicArea,
                                        &(ObjectGet(parentHandle)->publicArea));
              }
        }
        else
        {
              // Non-storage keys must have TPM_ALG_NULL for the symmetric algorithm
              if(    publicArea->parameters.asymDetail.symmetric.algorithm
                  != TPM_ALG_NULL)
                   return TPM_RC_SYMMETRIC;
       }// End of asymmetric decryption key checks
   } // End of asymmetric checks
   // Check for bit attributes
   else if(publicArea->type == TPM_ALG_KEYEDHASH)
   {
       TPMT_KEYEDHASH_SCHEME    *scheme
           = &publicArea->parameters.keyedHashDetail.scheme;
       // If both sign and decrypt are set the scheme must be TPM_ALG_NULL
       // and the scheme selected when the key is used.
       // If neither sign nor decrypt is set, the scheme must be TPM_ALG_NULL
       // because this is a data object.
       if(      publicArea->objectAttributes.sign
           == publicArea->objectAttributes.decrypt)
       {
           if(scheme->scheme != TPM_ALG_NULL)
                return TPM_RC_SCHEME;
           return TPM_RC_SUCCESS;
       }
       // If this is a decryption key, make sure that is is XOR and that there
       // is a KDF
       else if(publicArea->objectAttributes.decrypt)
       {
           if(    scheme->scheme != TPM_ALG_XOR
               || scheme->details.xor_.hashAlg == TPM_ALG_NULL)
                return TPM_RC_SCHEME;
           if(scheme->details.xor_.kdf == TPM_ALG_NULL)
                return TPM_RC_KDF;
           return TPM_RC_SUCCESS;
        }
        // only supported signing scheme for keyedHash object is HMAC
        if(    scheme->scheme != TPM_ALG_HMAC
            || scheme->details.hmac.hashAlg == TPM_ALG_NULL)
             return TPM_RC_SCHEME;
         // end of the checks for keyedHash
         return TPM_RC_SUCCESS;
   }
   else if (publicArea->type == TPM_ALG_SYMCIPHER)
   {
       // Must be a decrypting key and may not be a signing key
       if(    publicArea->objectAttributes.decrypt == CLEAR
           || publicArea->objectAttributes.sign == SET
         )
            return TPM_RC_ATTRIBUTES;
   }
   else
       return TPM_RC_TYPE;
   return TPM_RC_SUCCESS;
}
//
//
//          PublicAttributesValidation()
//
//      This function validates the values in the public area of an object. This function is called by
//      TPM2_Create(), TPM2_Load(), and TPM2_CreatePrimary()
//
//      Error Returns                     Meaning
//
//      TPM_RC_ASYMMETRIC                 non-duplicable storage key and its parent have different public
//                                        parameters
//      TPM_RC_ATTRIBUTES                 fixedTPM, fixedParent, or encryptedDuplication attributes are
//                                        inconsistent between themselves or with those of the parent object;
//                                        inconsistent restricted, decrypt and sign attributes; attempt to inject
//                                        sensitive data for an asymmetric key; attempt to create a symmetric
//                                        cipher key that is not a decryption key
//      TPM_RC_HASH                       non-duplicable storage key and its parent have different name
//                                        algorithm
//      TPM_RC_KDF                        incorrect KDF specified for decrypting keyed hash object
//      TPM_RC_KEY                        invalid key size values in an asymmetric key public area
//      TPM_RC_SCHEME                     inconsistent attributes decrypt, sign, restricted and key's scheme ID;
//                                        or hash algorithm is inconsistent with the scheme ID for keyed hash
//                                        object
//      TPM_RC_SIZE                       authPolicy size does not match digest size of the name algorithm in
//                                        publicArea
//      TPM_RC_SYMMETRIC                  a storage key with no symmetric algorithm specified; or non-storage
//                                        key with symmetric algorithm different from TPM_ALG_NULL
//      TPM_RC_TYPE                       unexpected object type; or non-duplicable storage key and its parent
//                                        have different types
//
TPM_RC
PublicAttributesValidation(
   BOOL                load,                 // IN: TRUE if load checks, FALSE if
                                             //     TPM2_Create()
   TPMI_DH_OBJECT      parentHandle,         // IN: input parent handle
   TPMT_PUBLIC        *publicArea            // IN: public area of the object
   )
{
   OBJECT                  *parentObject = NULL;
   if(HandleGetType(parentHandle) != TPM_HT_PERMANENT)
       parentObject = ObjectGet(parentHandle);
    // Check authPolicy digest consistency
    if(   publicArea->authPolicy.t.size != 0
       && (    publicArea->authPolicy.t.size
            != CryptGetHashDigestSize(publicArea->nameAlg)
          )
      )
        return TPM_RC_SIZE;
    // If the parent is fixedTPM (including a Primary Object) the object must have
    // the same value for fixedTPM and fixedParent
    if(     parentObject == NULL
        || parentObject->publicArea.objectAttributes.fixedTPM == SET)
    {
        if(    publicArea->objectAttributes.fixedParent
            != publicArea->objectAttributes.fixedTPM
          )
             return TPM_RC_ATTRIBUTES;
    }
    else
        // The parent is not fixedTPM so the object can't be fixedTPM
        if(publicArea->objectAttributes.fixedTPM == SET)
             return TPM_RC_ATTRIBUTES;
    // A restricted object cannot be both sign and decrypt and it can't be neither
    // sign nor decrypt
    if (    publicArea->objectAttributes.restricted == SET
         && (    publicArea->objectAttributes.decrypt
              == publicArea->objectAttributes.sign)
       )
         return TPM_RC_ATTRIBUTES;
    // A fixedTPM object can not have encryptedDuplication bit SET
    if(    publicArea->objectAttributes.fixedTPM == SET
        && publicArea->objectAttributes.encryptedDuplication == SET)
        return TPM_RC_ATTRIBUTES;
    // If a parent object has fixedTPM CLEAR, the child must have the
    // same encryptedDuplication value as its parent.
    // Primary objects are considered to have a fixedTPM parent (the seeds).
   if(       (   parentObject != NULL
              && parentObject->publicArea.objectAttributes.fixedTPM == CLEAR)
       // Get here if parent is not fixed TPM
       && (     publicArea->objectAttributes.encryptedDuplication
             != parentObject->publicArea.objectAttributes.encryptedDuplication
           )
      )
        return TPM_RC_ATTRIBUTES;
   return SchemeChecks(load, parentHandle, publicArea);
}
//
//
//            FillInCreationData()
//
//      Fill in creation data for an object.
//
void
FillInCreationData(
    TPMI_DH_OBJECT                     parentHandle,    //   IN: handle of parent
    TPMI_ALG_HASH                      nameHashAlg,     //   IN: name hash algorithm
    TPML_PCR_SELECTION                *creationPCR,     //   IN: PCR selection
    TPM2B_DATA                        *outsideData,     //   IN: outside data
    TPM2B_CREATION_DATA               *outCreation,     //   OUT: creation data for output
    TPM2B_DIGEST                      *creationDigest   //   OUT: creation digest
//
   )
{
   BYTE                     creationBuffer[sizeof(TPMS_CREATION_DATA)];
   BYTE                    *buffer;
   INT32                    bufferSize;
   HASH_STATE               hashState;
   // Fill in TPMS_CREATION_DATA in outCreation
   // Compute PCR digest
   PCRComputeCurrentDigest(nameHashAlg, creationPCR,
                           &outCreation->t.creationData.pcrDigest);
   // Put back PCR selection list
   outCreation->t.creationData.pcrSelect = *creationPCR;
   // Get locality
   outCreation->t.creationData.locality
       = LocalityGetAttributes(_plat__LocalityGet());
   outCreation->t.creationData.parentNameAlg = TPM_ALG_NULL;
   // If the parent is is either a primary seed or TPM_ALG_NULL, then the Name
   // and QN of the parent are the parent's handle.
   if(HandleGetType(parentHandle) == TPM_HT_PERMANENT)
   {
       BYTE         *buffer = &outCreation->t.creationData.parentName.t.name[0];
       INT32         bufferSize = sizeof(TPM_HANDLE);
       outCreation->t.creationData.parentName.t.size =
            TPM_HANDLE_Marshal(&parentHandle, &buffer, &bufferSize);
         // Parent qualified name of a Temporary Object is the same as parent's
         // name
         MemoryCopy2B(&outCreation->t.creationData.parentQualifiedName.b,
                      &outCreation->t.creationData.parentName.b,
                     sizeof(outCreation->t.creationData.parentQualifiedName.t.name));
   }
   else           // Regular object
   {
       OBJECT              *parentObject = ObjectGet(parentHandle);
         // Set name algorithm
         outCreation->t.creationData.parentNameAlg =
             parentObject->publicArea.nameAlg;
         // Copy parent name
         outCreation->t.creationData.parentName = parentObject->name;
         // Copy parent qualified name
         outCreation->t.creationData.parentQualifiedName =
             parentObject->qualifiedName;
   }
   // Copy outside information
   outCreation->t.creationData.outsideInfo = *outsideData;
   // Marshal creation data to canonical form
   buffer = creationBuffer;
   bufferSize = sizeof(TPMS_CREATION_DATA);
   outCreation->t.size = TPMS_CREATION_DATA_Marshal(&outCreation->t.creationData,
                         &buffer, &bufferSize);
   // Compute hash for creation field in public template
   creationDigest->t.size = CryptStartHash(nameHashAlg, &hashState);
   CryptUpdateDigest(&hashState, outCreation->t.size, creationBuffer);
   CryptCompleteHash2B(&hashState, &creationDigest->b);
   return;
}
//           GetSeedForKDF()
//
//      Get a seed for KDF. The KDF for encryption and HMAC key use the same seed. It returns a pointer to
//      the seed
//
TPM2B_SEED*
GetSeedForKDF(
    TPM_HANDLE           protectorHandle,          // IN: the protector handle
    TPM2B_SEED          *seedIn                    // IN: the optional input seed
    )
{
    OBJECT                   *protector = NULL; // Pointer to the protector
    // Get seed for encryption key. Use input seed if provided.
    // Otherwise, using protector object's seedValue. TPM_RH_NULL is the only
    // exception that we may not have a loaded object as protector. In such a
    // case, use nullProof as seed.
    if(seedIn != NULL)
    {
        return seedIn;
    }
    else
    {
        if(protectorHandle == TPM_RH_NULL)
        {
             return (TPM2B_SEED *) &gr.nullProof;
        }
        else
        {
             protector = ObjectGet(protectorHandle);
             return (TPM2B_SEED *) &protector->sensitive.seedValue;
        }
    }
}
//
//
//           ProduceOuterWrap()
//
//      This function produce outer wrap for a buffer containing the sensitive data. It requires the sensitive data
//      being marshaled to the outerBuffer, with the leading bytes reserved for integrity hash. If iv is used, iv
//      space should be reserved at the beginning of the buffer. It assumes the sensitive data starts at address
//      (outerBuffer + integrity size {+ iv size}). This function performs:
//      a) Add IV before sensitive area if required
//      b) encrypt sensitive data, if iv is required, encrypt by iv. otherwise, encrypted by a NULL iv
//      c) add HMAC integrity at the beginning of the buffer It returns the total size of blob with outer wrap
//
UINT16
ProduceOuterWrap(
    TPM_HANDLE           protector,          //   IN: The handle of the object that provides
                                             //       protection. For object, it is parent
                                             //       handle. For credential, it is the handle
                                             //       of encrypt object.
    TPM2B_NAME          *name,               //   IN: the name of the object
    TPM_ALG_ID           hashAlg,            //   IN: hash algorithm for outer wrap
    TPM2B_SEED          *seed,               //   IN: an external seed may be provided for
                                             //       duplication blob. For non duplication
                                             //       blob, this parameter should be NULL
    BOOL                 useIV,              //   IN: indicate if an IV is used
    UINT16               dataSize,           //   IN: the size of sensitive data, excluding the
                                             //       leading integrity buffer size or the
                                             //       optional iv size
    BYTE                *outerBuffer         //   IN/OUT: outer buffer with sensitive data in
                                       //     it
   )
{
   TPM_ALG_ID         symAlg;
   UINT16             keyBits;
   TPM2B_SYM_KEY      symKey;
   TPM2B_IV           ivRNG;           // IV from RNG
   TPM2B_IV           *iv = NULL;
   UINT16             ivSize = 0;      // size of iv area, including the size field
   BYTE               *sensitiveData; // pointer to the sensitive data
   TPM2B_DIGEST       integrity;
   UINT16             integritySize;
   BYTE               *buffer;         // Auxiliary buffer pointer
   INT32              bufferSize;
   // Compute the beginning of sensitive data. The outer integrity should
   // always exist if this function function is called to make an outer wrap
   integritySize = sizeof(UINT16) + CryptGetHashDigestSize(hashAlg);
   sensitiveData = outerBuffer + integritySize;
   // If iv is used, adjust the pointer of sensitive data and add iv before it
   if(useIV)
   {
       ivSize = GetIV2BSize(protector);
         // Generate IV from RNG. The iv data size should be the total IV area
         // size minus the size of size field
         ivRNG.t.size = ivSize - sizeof(UINT16);
         CryptGenerateRandom(ivRNG.t.size, ivRNG.t.buffer);
         // Marshal IV to buffer
         buffer = sensitiveData;
         bufferSize = sizeof(TPM2B_IV);
         TPM2B_IV_Marshal(&ivRNG, &buffer, &bufferSize);
         // adjust sensitive data starting after IV area
         sensitiveData += ivSize;
         // Use iv for encryption
         iv = &ivRNG;
   }
   // Compute symmetric key parameters for outer buffer encryption
   ComputeProtectionKeyParms(protector, hashAlg, name, seed,
                             &symAlg, &keyBits, &symKey);
   // Encrypt inner buffer in place
   CryptSymmetricEncrypt(sensitiveData, symAlg, keyBits,
                         TPM_ALG_CFB, symKey.t.buffer, iv, dataSize,
                         sensitiveData);
   // Compute outer integrity. Integrity computation includes the optional IV
   // area
   ComputeOuterIntegrity(name, protector, hashAlg, seed, dataSize + ivSize,
                         outerBuffer + integritySize, &integrity);
   // Add integrity at the beginning of outer buffer
   buffer = outerBuffer;
   bufferSize = sizeof(TPM2B_DIGEST);
   TPM2B_DIGEST_Marshal(&integrity, &buffer, &bufferSize);
   // return the total size in outer wrap
   return dataSize + integritySize + ivSize;
}
//
//
//
//           UnwrapOuter()
//
//      This function remove the outer wrap of a blob containing sensitive data This function performs:
//      a) check integrity of outer blob
//      b) decrypt outer blob
//
//      Error Returns                      Meaning
//
//      TPM_RC_INSUFFICIENT                error during sensitive data unmarshaling
//      TPM_RC_INTEGRITY                   sensitive data integrity is broken
//      TPM_RC_SIZE                        error during sensitive data unmarshaling
//      TPM_RC_VALUE                       IV size for CFB does not match the encryption algorithm block size
//
TPM_RC
UnwrapOuter(
   TPM_HANDLE           protector,             //   IN: The handle of the object that provides
                                               //       protection. For object, it is parent
                                               //       handle. For credential, it is the handle
                                               //       of encrypt object.
   TPM2B_NAME          *name,                  //   IN: the name of the object
   TPM_ALG_ID           hashAlg,               //   IN: hash algorithm for outer wrap
   TPM2B_SEED          *seed,                  //   IN: an external seed may be provided for
                                               //       duplication blob. For non duplication
                                               //       blob, this parameter should be NULL.
   BOOL                 useIV,                 //   IN: indicates if an IV is used
   UINT16               dataSize,              //   IN: size of sensitive data in outerBuffer,
                                               //       including the leading integrity buffer
                                               //       size, and an optional iv area
   BYTE                *outerBuffer            //   IN/OUT: sensitive data
   )
{
   TPM_RC              result;
   TPM_ALG_ID          symAlg = TPM_ALG_NULL;
   TPM2B_SYM_KEY       symKey;
   UINT16              keyBits = 0;
   TPM2B_IV            ivIn;               // input IV retrieved from input buffer
   TPM2B_IV            *iv = NULL;
   BYTE                *sensitiveData;               // pointer to the sensitive data
   TPM2B_DIGEST        integrityToCompare;
   TPM2B_DIGEST        integrity;
   INT32               size;
   // Unmarshal integrity
   sensitiveData = outerBuffer;
   size = (INT32) dataSize;
   result = TPM2B_DIGEST_Unmarshal(&integrity, &sensitiveData, &size);
   if(result == TPM_RC_SUCCESS)
   {
       // Compute integrity to compare
       ComputeOuterIntegrity(name, protector, hashAlg, seed,
                             (UINT16) size, sensitiveData,
                             &integrityToCompare);
         // Compare outer blob integrity
         if(!Memory2BEqual(&integrity.b, &integrityToCompare.b))
             return TPM_RC_INTEGRITY;
         // Get the symmetric algorithm parameters used for encryption
         ComputeProtectionKeyParms(protector, hashAlg, name, seed,
                                          &symAlg, &keyBits, &symKey);
         // Retrieve IV if it is used
         if(useIV)
         {
             result = TPM2B_IV_Unmarshal(&ivIn, &sensitiveData, &size);
             if(result == TPM_RC_SUCCESS)
             {
                 // The input iv size for CFB must match the encryption algorithm
                 // block size
                 if(ivIn.t.size != CryptGetSymmetricBlockSize(symAlg, keyBits))
                     result = TPM_RC_VALUE;
                 else
                     iv = &ivIn;
             }
         }
    }
    // If no errors, decrypt private in place
    if(result == TPM_RC_SUCCESS)
        CryptSymmetricDecrypt(sensitiveData, symAlg, keyBits,
                              TPM_ALG_CFB, symKey.t.buffer, iv,
                              (UINT16) size, sensitiveData);
    return result;
}
//
//
//           SensitiveToPrivate()
//
//      This function prepare the private blob for off the chip storage The operations in this function:
//      a) marshal TPM2B_SENSITIVE structure into the buffer of TPM2B_PRIVATE
//      b) apply encryption to the sensitive area.
//      c) apply outer integrity computation.
//
void
SensitiveToPrivate(
    TPMT_SENSITIVE      *sensitive,         //   IN: sensitive structure
    TPM2B_NAME          *name,              //   IN: the name of the object
    TPM_HANDLE           parentHandle,      //   IN: The parent's handle
    TPM_ALG_ID           nameAlg,           //   IN: hash algorithm in public area. This
                                            //       parameter is used when parentHandle is
                                            //       NULL, in which case the object is
                                            //       temporary.
    TPM2B_PRIVATE       *outPrivate         //   OUT: output private structure
    )
{
    BYTE                     *buffer;                  //   Auxiliary buffer pointer
    INT32                    bufferSize;
    BYTE                     *sensitiveData;           //   pointer to the sensitive data
    UINT16                   dataSize;                 //   data blob size
    TPMI_ALG_HASH            hashAlg;                  //   hash algorithm for integrity
    UINT16                   integritySize;
    UINT16                   ivSize;
    pAssert(name != NULL && name->t.size != 0);
    // Find the hash algorithm for integrity computation
    if(parentHandle == TPM_RH_NULL)
    {
        // For Temporary Object, using self name algorithm
        hashAlg = nameAlg;
    }
    else
   {
         // Otherwise, using parent's name algorithm
         hashAlg = ObjectGetNameAlg(parentHandle);
   }
   // Starting of sensitive data without wrappers
   sensitiveData = outPrivate->t.buffer;
   // Compute the integrity size
   integritySize = sizeof(UINT16) + CryptGetHashDigestSize(hashAlg);
   // Reserve space for integrity
   sensitiveData += integritySize;
   // Get iv size
   ivSize = GetIV2BSize(parentHandle);
   // Reserve space for iv
   sensitiveData += ivSize;
   // Marshal sensitive area, leaving the leading 2 bytes for size
   buffer = sensitiveData + sizeof(UINT16);
   bufferSize = sizeof(TPMT_SENSITIVE);
   dataSize = TPMT_SENSITIVE_Marshal(sensitive, &buffer, &bufferSize);
   // Adding size before the data area
   buffer = sensitiveData;
   bufferSize = sizeof(UINT16);
   UINT16_Marshal(&dataSize, &buffer, &bufferSize);
   // Adjust the dataSize to include the size field
   dataSize += sizeof(UINT16);
   // Adjust the pointer to inner buffer including the iv
   sensitiveData = outPrivate->t.buffer + ivSize;
   //Produce outer wrap, including encryption and HMAC
   outPrivate->t.size = ProduceOuterWrap(parentHandle, name, hashAlg, NULL,
                                         TRUE, dataSize, outPrivate->t.buffer);
   return;
}
//
//
//           PrivateToSensitive()
//
//      Unwrap a input private area. Check the integrity, decrypt and retrieve data to a sensitive structure. The
//      operations in this function:
//      a) check the integrity HMAC of the input private area
//      b) decrypt the private buffer
//      c) unmarshal TPMT_SENSITIVE structure into the buffer of TPMT_SENSITIVE
//
//      Error Returns                   Meaning
//
//      TPM_RC_INTEGRITY                if the private area integrity is bad
//      TPM_RC_SENSITIVE                unmarshal errors while unmarshaling TPMS_ENCRYPT from input
//                                      private
//      TPM_RC_VALUE                    outer wrapper does not have an iV of the correct size
//
TPM_RC
PrivateToSensitive(
   TPM2B_PRIVATE       *inPrivate,          // IN: input private structure
   TPM2B_NAME          *name,               // IN: the name of the object
   TPM_HANDLE          parentHandle,    // IN: The parent's handle
   TPM_ALG_ID          nameAlg,         // IN: hash algorithm in public area. It is
                                        //     passed separately because we only pass
                                        //     name, rather than the whole public area
                                        //     of the object. This parameter is used in
                                        //     the following two cases: 1. primary
                                        //     objects. 2. duplication blob with inner
                                        //     wrap. In other cases, this parameter
                                        //     will be ignored
   TPMT_SENSITIVE     *sensitive        // OUT: sensitive structure
   )
{
   TPM_RC             result;
   BYTE               *buffer;
   INT32              size;
   BYTE               *sensitiveData; // pointer to the sensitive data
   UINT16             dataSize;
   UINT16             dataSizeInput;
   TPMI_ALG_HASH      hashAlg;        // hash algorithm for integrity
   OBJECT             *parent = NULL;
   UINT16             integritySize;
   UINT16             ivSize;
   // Make sure that name is provided
   pAssert(name != NULL && name->t.size != 0);
   // Find the hash algorithm for integrity computation
   if(parentHandle == TPM_RH_NULL)
   {
       // For Temporary Object, using self name algorithm
       hashAlg = nameAlg;
   }
   else
   {
       // Otherwise, using parent's name algorithm
       hashAlg = ObjectGetNameAlg(parentHandle);
   }
   // unwrap outer
   result = UnwrapOuter(parentHandle, name, hashAlg, NULL, TRUE,
                        inPrivate->t.size, inPrivate->t.buffer);
   if(result != TPM_RC_SUCCESS)
       return result;
   // Compute the inner integrity size.
   integritySize = sizeof(UINT16) + CryptGetHashDigestSize(hashAlg);
   // Get iv size
   ivSize = GetIV2BSize(parentHandle);
   // The starting of sensitive data and data size without outer wrapper
   sensitiveData = inPrivate->t.buffer + integritySize + ivSize;
   dataSize = inPrivate->t.size - integritySize - ivSize;
   // Unmarshal input data size
   buffer = sensitiveData;
   size = (INT32) dataSize;
   result = UINT16_Unmarshal(&dataSizeInput, &buffer, &size);
   if(result == TPM_RC_SUCCESS)
   {
       if((dataSizeInput + sizeof(UINT16)) != dataSize)
            result = TPM_RC_SENSITIVE;
       else
       {
              // Unmarshal sensitive buffer to sensitive structure
              result = TPMT_SENSITIVE_Unmarshal(sensitive, &buffer, &size);
              if(result != TPM_RC_SUCCESS || size != 0)
              {
                  pAssert(    (parent == NULL)
                           || parent->publicArea.objectAttributes.fixedTPM == CLEAR);
                  result = TPM_RC_SENSITIVE;
              }
              else
              {
                  // Always remove trailing zeros at load so that it is not necessary
                  // to check
                  // each time auth is checked.
                  MemoryRemoveTrailingZeros(&(sensitive->authValue));
              }
        }
    }
    return result;
}
//
//
//          SensitiveToDuplicate()
//
//      This function prepare the duplication blob from the sensitive area. The operations in this function:
//      a) marshal TPMT_SENSITIVE structure into the buffer of TPM2B_PRIVATE
//      b) apply inner wrap to the sensitive area if required
//      c) apply outer wrap if required
//
void
SensitiveToDuplicate(
    TPMT_SENSITIVE                *sensitive,          //   IN: sensitive structure
    TPM2B_NAME                    *name,               //   IN: the name of the object
    TPM_HANDLE                     parentHandle,       //   IN: The new parent's handle
    TPM_ALG_ID                     nameAlg,            //   IN: hash algorithm in public area. It
                                                       //       is passed separately because we
                                                       //       only pass name, rather than the
                                                       //       whole public area of the object.
    TPM2B_SEED                    *seed,               //   IN: the external seed. If external
                                                       //       seed is provided with size of 0,
                                                       //       no outer wrap should be applied
                                                       //       to duplication blob.
    TPMT_SYM_DEF_OBJECT           *symDef,             //   IN: Symmetric key definition. If the
                                                       //       symmetric key algorithm is NULL,
                                                       //       no inner wrap should be applied.
    TPM2B_DATA                    *innerSymKey,        //   IN/OUT: a symmetric key may be
                                                       //       provided to encrypt the inner
                                                       //       wrap of a duplication blob. May
                                                       //       be generated here if needed.
    TPM2B_PRIVATE                 *outPrivate          //   OUT: output private structure
    )
{
    BYTE                *buffer;        // Auxiliary buffer pointer
    INT32               bufferSize;
    BYTE                *sensitiveData; // pointer to the sensitive data
    TPMI_ALG_HASH       outerHash = TPM_ALG_NULL;// The hash algorithm for outer wrap
    TPMI_ALG_HASH       innerHash = TPM_ALG_NULL;// The hash algorithm for inner wrap
    UINT16              dataSize;       // data blob size
    BOOL                doInnerWrap = FALSE;
    BOOL                doOuterWrap = FALSE;
    // Make sure that name is provided
    pAssert(name != NULL && name->t.size != 0);
    // Make sure symDef and innerSymKey are not NULL
   pAssert(symDef != NULL && innerSymKey != NULL);
   // Starting of sensitive data without wrappers
   sensitiveData = outPrivate->t.buffer;
   // Find out if inner wrap is required
   if(symDef->algorithm != TPM_ALG_NULL)
   {
       doInnerWrap = TRUE;
       // Use self nameAlg as inner hash algorithm
       innerHash = nameAlg;
       // Adjust sensitive data pointer
       sensitiveData += sizeof(UINT16) + CryptGetHashDigestSize(innerHash);
   }
   // Find out if outer wrap is required
   if(seed->t.size != 0)
   {
       doOuterWrap = TRUE;
       // Use parent nameAlg as outer hash algorithm
       outerHash = ObjectGetNameAlg(parentHandle);
       // Adjust sensitive data pointer
       sensitiveData += sizeof(UINT16) + CryptGetHashDigestSize(outerHash);
   }
   // Marshal sensitive area, leaving the leading 2 bytes for size
   buffer = sensitiveData + sizeof(UINT16);
   bufferSize = sizeof(TPMT_SENSITIVE);
   dataSize = TPMT_SENSITIVE_Marshal(sensitive, &buffer, &bufferSize);
   // Adding size before the data area
   buffer = sensitiveData;
   bufferSize = sizeof(UINT16);
   UINT16_Marshal(&dataSize, &buffer, &bufferSize);
   // Adjust the dataSize to include the size field
   dataSize += sizeof(UINT16);
   // Apply inner wrap for duplication blob. It includes both integrity and
   // encryption
   if(doInnerWrap)
   {
       BYTE             *innerBuffer = NULL;
       BOOL             symKeyInput = TRUE;
       innerBuffer = outPrivate->t.buffer;
       // Skip outer integrity space
       if(doOuterWrap)
            innerBuffer += sizeof(UINT16) + CryptGetHashDigestSize(outerHash);
       dataSize = ProduceInnerIntegrity(name, innerHash, dataSize,
                                         innerBuffer);
        // Generate inner encryption key if needed
        if(innerSymKey->t.size == 0)
        {
            innerSymKey->t.size = (symDef->keyBits.sym + 7) / 8;
            CryptGenerateRandom(innerSymKey->t.size, innerSymKey->t.buffer);
             // TPM generates symmetric encryption.   Set the flag to FALSE
             symKeyInput = FALSE;
        }
        else
        {
             // assume the input key size should matches the symmetric definition
             pAssert(innerSymKey->t.size == (symDef->keyBits.sym + 7) / 8);
        }
        // Encrypt inner buffer in place
          CryptSymmetricEncrypt(innerBuffer, symDef->algorithm,
                                symDef->keyBits.sym, TPM_ALG_CFB,
                                innerSymKey->t.buffer, NULL, dataSize,
                                innerBuffer);
          // If the symmetric encryption key is imported, clear the buffer for
          // output
          if(symKeyInput)
              innerSymKey->t.size = 0;
   }
   // Apply outer wrap for duplication blob. It includes both integrity and
   // encryption
   if(doOuterWrap)
   {
       dataSize = ProduceOuterWrap(parentHandle, name, outerHash, seed, FALSE,
                                   dataSize, outPrivate->t.buffer);
   }
   // Data size for output
   outPrivate->t.size = dataSize;
   return;
}
//
//
//           DuplicateToSensitive()
//
//       Unwrap a duplication blob. Check the integrity, decrypt and retrieve data to a sensitive structure. The
//       operations in this function:
//       a) check the integrity HMAC of the input private area
//       b) decrypt the private buffer
//       c) unmarshal TPMT_SENSITIVE structure into the buffer of TPMT_SENSITIVE
//
//       Error Returns                   Meaning
//
//       TPM_RC_INSUFFICIENT             unmarshaling sensitive data from inPrivate failed
//       TPM_RC_INTEGRITY                inPrivate data integrity is broken
//       TPM_RC_SIZE                     unmarshaling sensitive data from inPrivate failed
//
TPM_RC
DuplicateToSensitive(
   TPM2B_PRIVATE                 *inPrivate,           //   IN: input private structure
   TPM2B_NAME                    *name,                //   IN: the name of the object
   TPM_HANDLE                     parentHandle,        //   IN: The parent's handle
   TPM_ALG_ID                     nameAlg,             //   IN: hash algorithm in public area.
   TPM2B_SEED                    *seed,                //   IN: an external seed may be provided.
                                                       //       If external seed is provided with
                                                       //       size of 0, no outer wrap is
                                                       //       applied
   TPMT_SYM_DEF_OBJECT           *symDef,              //   IN: Symmetric key definition. If the
                                                       //       symmetric key algorithm is NULL,
                                                       //       no inner wrap is applied
   TPM2B_DATA                    *innerSymKey,         //   IN: a symmetric key may be provided
                                                       //       to decrypt the inner wrap of a
                                                       //       duplication blob.
   TPMT_SENSITIVE                *sensitive            //   OUT: sensitive structure
   )
{
   TPM_RC              result;
   BYTE               *buffer;
   INT32              size;
   BYTE               *sensitiveData; // pointer to the sensitive data
   UINT16             dataSize;
   UINT16             dataSizeInput;
   // Make sure that name is provided
   pAssert(name != NULL && name->t.size != 0);
   // Make sure symDef and innerSymKey are not NULL
   pAssert(symDef != NULL && innerSymKey != NULL);
   // Starting of sensitive data
   sensitiveData = inPrivate->t.buffer;
   dataSize = inPrivate->t.size;
   // Find out if outer wrap is applied
   if(seed->t.size != 0)
   {
       TPMI_ALG_HASH   outerHash = TPM_ALG_NULL;
        // Use parent nameAlg as outer hash algorithm
        outerHash = ObjectGetNameAlg(parentHandle);
        result = UnwrapOuter(parentHandle, name, outerHash, seed, FALSE,
                             dataSize, sensitiveData);
        if(result != TPM_RC_SUCCESS)
            return result;
        // Adjust sensitive data pointer and size
        sensitiveData += sizeof(UINT16) + CryptGetHashDigestSize(outerHash);
        dataSize -= sizeof(UINT16) + CryptGetHashDigestSize(outerHash);
   }
   // Find out if inner wrap is applied
   if(symDef->algorithm != TPM_ALG_NULL)
   {
       TPMI_ALG_HASH   innerHash = TPM_ALG_NULL;
        // assume the input key size should matches the symmetric definition
        pAssert(innerSymKey->t.size == (symDef->keyBits.sym + 7) / 8);
        // Decrypt inner buffer in place
        CryptSymmetricDecrypt(sensitiveData, symDef->algorithm,
                              symDef->keyBits.sym, TPM_ALG_CFB,
                              innerSymKey->t.buffer, NULL, dataSize,
                              sensitiveData);
        // Use self nameAlg as inner hash algorithm
        innerHash = nameAlg;
        // Check inner integrity
        result = CheckInnerIntegrity(name, innerHash, dataSize, sensitiveData);
        if(result != TPM_RC_SUCCESS)
            return result;
        // Adjust sensitive data pointer and size
        sensitiveData += sizeof(UINT16) + CryptGetHashDigestSize(innerHash);
        dataSize -= sizeof(UINT16) + CryptGetHashDigestSize(innerHash);
   }
   // Unmarshal input data size
   buffer = sensitiveData;
   size = (INT32) dataSize;
   result = UINT16_Unmarshal(&dataSizeInput, &buffer, &size);
   if(result == TPM_RC_SUCCESS)
   {
       if((dataSizeInput + sizeof(UINT16)) != dataSize)
              result = TPM_RC_SIZE;
          else
          {
              // Unmarshal sensitive buffer to sensitive structure
              result = TPMT_SENSITIVE_Unmarshal(sensitive, &buffer, &size);
              // if the results is OK make sure that all the data was unmarshaled
              if(result == TPM_RC_SUCCESS && size != 0)
                  result = TPM_RC_SIZE;
       }
   }
   // Always remove trailing zeros at load so that it is not necessary to check
   // each time auth is checked.
   if(result == TPM_RC_SUCCESS)
       MemoryRemoveTrailingZeros(&(sensitive->authValue));
   return result;
}
//
//
//           SecretToCredential()
//
//       This function prepare the credential blob from a secret (a TPM2B_DIGEST) The operations in this
//       function:
//       a) marshal TPM2B_DIGEST structure into the buffer of TPM2B_ID_OBJECT
//       b) encrypt the private buffer, excluding the leading integrity HMAC area
//       c) compute integrity HMAC and append to the beginning of the buffer.
//       d) Set the total size of TPM2B_ID_OBJECT buffer
//
void
SecretToCredential(
   TPM2B_DIGEST              *secret,          //   IN: secret information
   TPM2B_NAME                *name,            //   IN: the name of the object
   TPM2B_SEED                *seed,            //   IN: an external seed.
   TPM_HANDLE                 protector,       //   IN: The protector's handle
   TPM2B_ID_OBJECT           *outIDObject      //   OUT: output credential
   )
{
   BYTE                      *buffer;          //   Auxiliary buffer pointer
   INT32                      bufferSize;
   BYTE                      *sensitiveData;   //   pointer to the sensitive data
   TPMI_ALG_HASH              outerHash;       //   The hash algorithm for outer wrap
   UINT16                     dataSize;        //   data blob size
   pAssert(secret != NULL && outIDObject != NULL);
   // use protector's name algorithm as outer hash
   outerHash = ObjectGetNameAlg(protector);
   // Marshal secret area to credential buffer, leave space for integrity
   sensitiveData = outIDObject->t.credential
                   + sizeof(UINT16) + CryptGetHashDigestSize(outerHash);
   // Marshal secret area
   buffer = sensitiveData;
   bufferSize = sizeof(TPM2B_DIGEST);
   dataSize = TPM2B_DIGEST_Marshal(secret, &buffer, &bufferSize);
   // Apply outer wrap
   outIDObject->t.size = ProduceOuterWrap(protector,
                                          name,
                                          outerHash,
                                          seed,
                                          FALSE,
                                          dataSize,
                                          outIDObject->t.credential);
   return;
}
//
//
//            CredentialToSecret()
//
//       Unwrap a credential. Check the integrity, decrypt and retrieve data to a TPM2B_DIGEST structure. The
//       operations in this function:
//       a) check the integrity HMAC of the input credential area
//       b) decrypt the credential buffer
//       c) unmarshal TPM2B_DIGEST structure into the buffer of TPM2B_DIGEST
//
//       Error Returns                      Meaning
//
//       TPM_RC_INSUFFICIENT                error during credential unmarshaling
//       TPM_RC_INTEGRITY                   credential integrity is broken
//       TPM_RC_SIZE                        error during credential unmarshaling
//       TPM_RC_VALUE                       IV size does not match the encryption algorithm block size
//
TPM_RC
CredentialToSecret(
   TPM2B_ID_OBJECT          *inIDObject,             //   IN: input credential blob
   TPM2B_NAME               *name,                   //   IN: the name of the object
   TPM2B_SEED               *seed,                   //   IN: an external seed.
   TPM_HANDLE                protector,              //   IN: The protector's handle
   TPM2B_DIGEST             *secret                  //   OUT: secret information
   )
{
   TPM_RC                           result;
   BYTE                            *buffer;
   INT32                            size;
   TPMI_ALG_HASH                    outerHash;     // The hash algorithm for outer wrap
   BYTE                            *sensitiveData; // pointer to the sensitive data
   UINT16                           dataSize;
   // use protector's name algorithm as outer hash
   outerHash = ObjectGetNameAlg(protector);
   // Unwrap outer, a TPM_RC_INTEGRITY error may be returned at this point
   result = UnwrapOuter(protector, name, outerHash, seed, FALSE,
                        inIDObject->t.size, inIDObject->t.credential);
   if(result == TPM_RC_SUCCESS)
   {
       // Compute the beginning of sensitive data
       sensitiveData = inIDObject->t.credential
                       + sizeof(UINT16) + CryptGetHashDigestSize(outerHash);
       dataSize = inIDObject->t.size
                  - (sizeof(UINT16) + CryptGetHashDigestSize(outerHash));
          // Unmarshal secret buffer to TPM2B_DIGEST structure
          buffer = sensitiveData;
          size = (INT32) dataSize;
          result = TPM2B_DIGEST_Unmarshal(secret, &buffer, &size);
          // If there were no other unmarshaling errors, make sure that the
          // expected amount of data was recovered
          if(result == TPM_RC_SUCCESS && size != 0)
              return TPM_RC_SIZE;
   }
   return result;
}
