// This file was extracted from the TCG Published
// Trusted Platform Module Library
// Part 3: Commands
// Family "2.0"
// Level 00 Revision 01.16
// October 30, 2014

#include "InternalRoutines.h"
#include "Attest_spt_fp.h"
#include "GetTime_fp.h"
//
//
//     Error Returns               Meaning
//
//     TPM_RC_KEY                  key referenced by signHandle is not a signing key
//     TPM_RC_SCHEME               inScheme is incompatible with signHandle type; or both scheme and
//                                 key's default scheme are empty; or scheme is empty while key's
//                                 default scheme requires explicit input scheme (split signing); or non-
//                                 empty default key scheme differs from scheme
//     TPM_RC_VALUE                digest generated for the given scheme is greater than the modulus of
//                                 signHandle (for an RSA key); invalid commit status or failed to
//                                 generate r value (for an ECC key)
//
TPM_RC
TPM2_GetTime(
   GetTime_In      *in,             // IN: input parameter list
   GetTime_Out     *out             // OUT: output parameter list
   )
{
   TPM_RC                 result;
   TPMS_ATTEST            timeInfo;

// Command Output

   // Filling in attest information
   // Common fields
   result = FillInAttestInfo(in->signHandle,
                             &in->inScheme,
                             &in->qualifyingData,
                             &timeInfo);
   if(result != TPM_RC_SUCCESS)
   {
       if(result == TPM_RC_KEY)
           return TPM_RC_KEY + RC_GetTime_signHandle;
       else
           return RcSafeAddToResult(result, RC_GetTime_inScheme);
   }

   // GetClock specific fields
   // Attestation type
   timeInfo.type = TPM_ST_ATTEST_TIME;

   // current clock in plain text
   timeInfo.attested.time.time.time = g_time;
   TimeFillInfo(&timeInfo.attested.time.time.clockInfo);

   // Firmware version in plain text
   timeInfo.attested.time.firmwareVersion
       = ((UINT64) gp.firmwareV1) << 32;
   timeInfo.attested.time.firmwareVersion += gp.firmwareV2;

   // Sign attestation structure. A NULL signature will be returned if
   // signHandle is TPM_RH_NULL. A TPM_RC_NV_UNAVAILABLE, TPM_RC_NV_RATE,
   // TPM_RC_VALUE, TPM_RC_SCHEME or TPM_RC_ATTRIBUTES error may be returned at
   // this point
   result = SignAttestInfo(in->signHandle,
                           &in->inScheme,
                           &timeInfo,
                           &in->qualifyingData,
                           &out->timeInfo,
                           &out->signature);
   if(result != TPM_RC_SUCCESS)
       return result;

   // orderly state should be cleared because of the reporting of clock info
   // if signing happens
   if(in->signHandle != TPM_RH_NULL)
       g_clearOrderly = TRUE;

   return TPM_RC_SUCCESS;
}
