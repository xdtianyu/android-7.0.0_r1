// This file was extracted from the TCG Published
// Trusted Platform Module Library
// Part 3: Commands
// Family "2.0"
// Level 00 Revision 01.16
// October 30, 2014

#include "InternalRoutines.h"
#include "Attest_spt_fp.h"
#include "NV_spt_fp.h"
#include "NV_Certify_fp.h"
//
//
//     Error Returns                     Meaning
//
//     TPM_RC_NV_AUTHORIZATION           the authorization was valid but the authorizing entity (authHandle) is
//                                       not allowed to read from the Index referenced by nvIndex
//     TPM_RC_KEY                        signHandle does not reference a signing key
//     TPM_RC_NV_LOCKED                  Index referenced by nvIndex is locked for reading
//     TPM_RC_NV_RANGE                   offset plus size extends outside of the data range of the Index
//                                       referenced by nvIndex
//     TPM_RC_NV_UNINITIALIZED           Index referenced by nvIndex has not been written
//     TPM_RC_SCHEME                     inScheme is not an allowed value for the key definition
//
TPM_RC
TPM2_NV_Certify(
   NV_Certify_In     *in,                 // IN: input parameter list
   NV_Certify_Out    *out                 // OUT: output parameter list
   )
{
   TPM_RC                    result;
   NV_INDEX                  nvIndex;
   TPMS_ATTEST               certifyInfo;

   // Attestation command may cause the orderlyState to be cleared due to
   // the reporting of clock info. If this is the case, check if NV is
   // available first
   if(gp.orderlyState != SHUTDOWN_NONE)
   {
       // The command needs NV update. Check if NV is available.
       // A TPM_RC_NV_UNAVAILABLE or TPM_RC_NV_RATE error may be returned at
       // this point
       result = NvIsAvailable();
       if(result != TPM_RC_SUCCESS)
           return result;
   }

// Input Validation

   // Get NV index info
   NvGetIndexInfo(in->nvIndex, &nvIndex);

   // Common access checks. A TPM_RC_NV_AUTHORIZATION or TPM_RC_NV_LOCKED
   // error may be returned at this point
   result = NvReadAccessChecks(in->authHandle, in->nvIndex);
   if(result != TPM_RC_SUCCESS)
       return result;

   // See if the range to be certified is out of the bounds of the defined
   // Index
   if((in->size + in->offset) > nvIndex.publicArea.dataSize)
       return TPM_RC_NV_RANGE;

// Command Output

  // Filling in attest information
  // Common fields
  // FillInAttestInfo can return TPM_RC_SCHEME or TPM_RC_KEY
  result = FillInAttestInfo(in->signHandle,
                            &in->inScheme,
                            &in->qualifyingData,
                            &certifyInfo);
  if(result != TPM_RC_SUCCESS)
  {
      if(result == TPM_RC_KEY)
          return TPM_RC_KEY + RC_NV_Certify_signHandle;
      else
          return RcSafeAddToResult(result, RC_NV_Certify_inScheme);
  }
  // NV certify specific fields
  // Attestation type
  certifyInfo.type = TPM_ST_ATTEST_NV;

  // Get the name of the index
  certifyInfo.attested.nv.indexName.t.size =
      NvGetName(in->nvIndex, &certifyInfo.attested.nv.indexName.t.name);

  // Set the return size
  certifyInfo.attested.nv.nvContents.t.size = in->size;

  // Set the offset
  certifyInfo.attested.nv.offset = in->offset;

  // Perform the read
  NvGetIndexData(in->nvIndex, &nvIndex,
                 in->offset, in->size,
                 certifyInfo.attested.nv.nvContents.t.buffer);

  // Sign attestation structure. A NULL signature will be returned if
  // signHandle is TPM_RH_NULL. SignAttestInfo() may return TPM_RC_VALUE,
  // TPM_RC_SCHEME or TPM_RC_ATTRUBUTES.
  // Note: SignAttestInfo may return TPM_RC_ATTRIBUTES if the key is not a
  // signing key but that was checked above. TPM_RC_VALUE would mean that the
  // data to sign is too large but the data to sign is a digest
  result = SignAttestInfo(in->signHandle,
                          &in->inScheme,
                          &certifyInfo,
                          &in->qualifyingData,
                          &out->certifyInfo,
                          &out->signature);
  if(result != TPM_RC_SUCCESS)
      return result;

  // orderly state should be cleared because of the reporting of clock info
  // if signing happens
  if(in->signHandle != TPM_RH_NULL)
      g_clearOrderly = TRUE;

   return TPM_RC_SUCCESS;
}
