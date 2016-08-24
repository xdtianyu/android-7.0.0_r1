// This file was extracted from the TCG Published
// Trusted Platform Module Library
// Part 3: Commands
// Family "2.0"
// Level 00 Revision 01.16
// October 30, 2014

#include "InternalRoutines.h"
#include "NV_DefineSpace_fp.h"
//
//
//     Error Returns                 Meaning
//
//     TPM_RC_NV_ATTRIBUTES          attributes of the index are not consistent
//     TPM_RC_NV_DEFINED             index already exists
//     TPM_RC_HIERARCHY              for authorizations using TPM_RH_PLATFORM phEnable_NV is
//                                   clear.
//     TPM_RC_NV_SPACE               Insufficient space for the index
//     TPM_RC_SIZE                   'auth->size' or 'publicInfo->authPolicy.size' is larger than the digest
//                                   size of 'publicInfo->nameAlg', or 'publicInfo->dataSize' is not
//                                   consistent with 'publicInfo->attributes'.
//
TPM_RC
TPM2_NV_DefineSpace(
   NV_DefineSpace_In   *in                  // IN: input parameter list
   )
{
   TPM_RC          result;
   TPMA_NV         attributes;
   UINT16          nameSize;

   nameSize = CryptGetHashDigestSize(in->publicInfo.t.nvPublic.nameAlg);

   // Check if NV is available. NvIsAvailable may return TPM_RC_NV_UNAVAILABLE
   // TPM_RC_NV_RATE or TPM_RC_SUCCESS.
   result = NvIsAvailable();
   if(result != TPM_RC_SUCCESS)
       return result;

// Input Validation
   // If an index is being created by the owner and shEnable is
   // clear, then we would not reach this point because ownerAuth
   // can't be given when shEnable is CLEAR. However, if phEnable
   // is SET but phEnableNV is CLEAR, we have to check here
   if(in->authHandle == TPM_RH_PLATFORM && gc.phEnableNV == CLEAR)
       return TPM_RC_HIERARCHY + RC_NV_DefineSpace_authHandle;

   attributes = in->publicInfo.t.nvPublic.attributes;

   //TPMS_NV_PUBLIC validation.
   // Counters and bit fields must have a size of 8
   if (   (attributes.TPMA_NV_COUNTER == SET || attributes.TPMA_NV_BITS == SET)
       && (in->publicInfo.t.nvPublic.dataSize != 8))
       return TPM_RC_SIZE + RC_NV_DefineSpace_publicInfo;

   // check that the authPolicy consistent with hash algorithm
   if(   in->publicInfo.t.nvPublic.authPolicy.t.size != 0
      && in->publicInfo.t.nvPublic.authPolicy.t.size != nameSize)
       return TPM_RC_SIZE + RC_NV_DefineSpace_publicInfo;

   // make sure that the authValue is not too large
   MemoryRemoveTrailingZeros(&in->auth);
   if(in->auth.t.size > nameSize)
       return TPM_RC_SIZE + RC_NV_DefineSpace_auth;

   //TPMA_NV validation.
   // Locks may not be SET and written cannot be SET
   if(   attributes.TPMA_NV_WRITTEN == SET
      || attributes.TPMA_NV_WRITELOCKED == SET
      || attributes.TPMA_NV_READLOCKED == SET)
       return TPM_RC_ATTRIBUTES + RC_NV_DefineSpace_publicInfo;

   // There must be a way to read the index
   if(   attributes.TPMA_NV_OWNERREAD == CLEAR
      && attributes.TPMA_NV_PPREAD == CLEAR
      && attributes.TPMA_NV_AUTHREAD == CLEAR
      && attributes.TPMA_NV_POLICYREAD == CLEAR)
       return TPM_RC_ATTRIBUTES + RC_NV_DefineSpace_publicInfo;

   // There must be a way to write the index
   if(   attributes.TPMA_NV_OWNERWRITE == CLEAR
      && attributes.TPMA_NV_PPWRITE == CLEAR
      && attributes.TPMA_NV_AUTHWRITE == CLEAR
      && attributes.TPMA_NV_POLICYWRITE == CLEAR)
       return TPM_RC_ATTRIBUTES + RC_NV_DefineSpace_publicInfo;

   // Make sure that no attribute is used that is not supported by the proper
   // command
#if CC_NV_Increment == NO
   if( attributes.TPMA_NV_COUNTER == SET)
       return TPM_RC_ATTRIBUTES + RC_NV_DefineSpace_publicInfo;
#endif
#if CC_NV_SetBits == NO
      if( attributes.TPMA_NV_BITS == SET)
          return TPM_RC_ATTRIBUTES + RC_NV_DefineSpace_publicInfo;
#endif
#if CC_NV_Extend == NO
     if( attributes.TPMA_NV_EXTEND == SET)
         return TPM_RC_ATTRIBUTES + RC_NV_DefineSpace_publicInfo;
#endif
#if CC_NV_UndefineSpaceSpecial == NO
    if( attributes.TPMA_NV_POLICY_DELETE == SET)
        return TPM_RC_ATTRIBUTES + RC_NV_DefineSpace_publicInfo;
#endif

   // Can be COUNTER or BITS or EXTEND but not more than one
   if( attributes.TPMA_NV_COUNTER == SET
      && attributes.TPMA_NV_BITS == SET)
       return TPM_RC_ATTRIBUTES + RC_NV_DefineSpace_publicInfo;
   if(   attributes.TPMA_NV_COUNTER == SET
      && attributes.TPMA_NV_EXTEND == SET)
       return TPM_RC_ATTRIBUTES + RC_NV_DefineSpace_publicInfo;
   if(   attributes.TPMA_NV_BITS == SET
      && attributes.TPMA_NV_EXTEND == SET)
       return TPM_RC_ATTRIBUTES + RC_NV_DefineSpace_publicInfo;

   // An index with TPMA_NV_CLEAR_STCLEAR can't be a counter and can't have
   // TPMA_NV_WRITEDEFINE SET
   if(     attributes.TPMA_NV_CLEAR_STCLEAR == SET
       && (    attributes.TPMA_NV_COUNTER == SET
            || attributes.TPMA_NV_WRITEDEFINE == SET)
      )
       return TPM_RC_ATTRIBUTES + RC_NV_DefineSpace_publicInfo;

   // Make sure that the creator of the index can delete the index
   if( (    in->publicInfo.t.nvPublic.attributes.TPMA_NV_PLATFORMCREATE == SET
          && in->authHandle == TPM_RH_OWNER
         )
      || (   in->publicInfo.t.nvPublic.attributes.TPMA_NV_PLATFORMCREATE == CLEAR
          && in->authHandle == TPM_RH_PLATFORM
         )
     )
         return TPM_RC_ATTRIBUTES + RC_NV_DefineSpace_authHandle;

  // If TPMA_NV_POLICY_DELETE is SET, then the index must be defined by
  // the platform
  if(    in->publicInfo.t.nvPublic.attributes.TPMA_NV_POLICY_DELETE == SET
     && TPM_RH_PLATFORM != in->authHandle
    )
      return TPM_RC_ATTRIBUTES + RC_NV_DefineSpace_publicInfo;

  // If the NV index is used as a PCR, the data size must match the digest
  // size
  if(   in->publicInfo.t.nvPublic.attributes.TPMA_NV_EXTEND == SET
     && in->publicInfo.t.nvPublic.dataSize != nameSize
    )
      return TPM_RC_ATTRIBUTES + RC_NV_DefineSpace_publicInfo;

  // See if the index is already defined.
  if(NvIsUndefinedIndex(in->publicInfo.t.nvPublic.nvIndex))
      return TPM_RC_NV_DEFINED;

// Internal Data Update
   // define the space. A TPM_RC_NV_SPACE error may be returned at this point
   result = NvDefineIndex(&in->publicInfo.t.nvPublic, &in->auth);
   if(result != TPM_RC_SUCCESS)
       return result;

  return TPM_RC_SUCCESS;

}
