// This file was extracted from the TCG Published
// Trusted Platform Module Library
// Part 3: Commands
// Family "2.0"
// Level 00 Revision 01.16
// October 30, 2014

#include "InternalRoutines.h"
#include "Attest_spt_fp.h"
#include "Quote_fp.h"
//
//
//     Error Returns               Meaning
//
//     TPM_RC_KEY                  signHandle does not reference a signing key;
//     TPM_RC_SCHEME               the scheme is not compatible with sign key type, or input scheme is
//                                 not compatible with default scheme, or the chosen scheme is not a
//                                 valid sign scheme
//
TPM_RC
TPM2_Quote(
   Quote_In        *in,             // IN: input parameter list
   Quote_Out       *out             // OUT: output parameter list
   )
{
   TPM_RC                  result;
   TPMI_ALG_HASH           hashAlg;
   TPMS_ATTEST             quoted;

// Command Output

   // Filling in attest information
   // Common fields
   // FillInAttestInfo may return TPM_RC_SCHEME or TPM_RC_KEY
   result = FillInAttestInfo(in->signHandle,
                             &in->inScheme,
                             &in->qualifyingData,
                             &quoted);
   if(result != TPM_RC_SUCCESS)
   {
       if(result == TPM_RC_KEY)
           return TPM_RC_KEY + RC_Quote_signHandle;
       else
           return RcSafeAddToResult(result, RC_Quote_inScheme);
   }

   // Quote specific fields
   // Attestation type
   quoted.type = TPM_ST_ATTEST_QUOTE;

   // Get hash algorithm in sign scheme. This hash algorithm is used to
   // compute PCR digest. If there is no algorithm, then the PCR cannot
   // be digested and this command returns TPM_RC_SCHEME
   hashAlg = in->inScheme.details.any.hashAlg;

   if(hashAlg == TPM_ALG_NULL)
       return TPM_RC_SCHEME + RC_Quote_inScheme;

   // Compute PCR digest
   PCRComputeCurrentDigest(hashAlg,
                           &in->PCRselect,
                           &quoted.attested.quote.pcrDigest);

   // Copy PCR select. "PCRselect" is modified in PCRComputeCurrentDigest
   // function
   quoted.attested.quote.pcrSelect = in->PCRselect;

   // Sign attestation structure. A NULL signature will be returned if
   // signHandle is TPM_RH_NULL. TPM_RC_VALUE, TPM_RC_SCHEME or TPM_RC_ATTRIBUTES
   // error may be returned by SignAttestInfo.
   // NOTE: TPM_RC_ATTRIBUTES means that the key is not a signing key but that
   // was checked above and TPM_RC_KEY was returned. TPM_RC_VALUE means that the
   // value to sign is too large but that means that the digest is too big and
   // that can't happen.
   result = SignAttestInfo(in->signHandle,
                           &in->inScheme,
                           &quoted,
                           &in->qualifyingData,
                           &out->quoted,
                           &out->signature);
   if(result != TPM_RC_SUCCESS)
       return result;

   // orderly state should be cleared because of the reporting of clock info
   // if signing happens
   if(in->signHandle != TPM_RH_NULL)
       g_clearOrderly = TRUE;

   return TPM_RC_SUCCESS;
}
