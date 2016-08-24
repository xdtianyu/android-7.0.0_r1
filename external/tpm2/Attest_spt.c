// This file was extracted from the TCG Published
// Trusted Platform Module Library
// Part 4: Supporting Routines
// Family "2.0"
// Level 00 Revision 01.16
// October 30, 2014

#include "InternalRoutines.h"
#include "Attest_spt_fp.h"
//
//
//          Functions
//
//          FillInAttestInfo()
//
//     Fill in common fields of TPMS_ATTEST structure.
//
//     Error Returns                     Meaning
//
//     TPM_RC_KEY                        key referenced by signHandle is not a signing key
//     TPM_RC_SCHEME                     both scheme and key's default scheme are empty; or scheme is
//                                       empty while key's default scheme requires explicit input scheme (split
//                                       signing); or non-empty default key scheme differs from scheme
//
TPM_RC
FillInAttestInfo(
     TPMI_DH_OBJECT         signHandle,            //   IN: handle of signing object
     TPMT_SIG_SCHEME       *scheme,                //   IN/OUT: scheme to be used for signing
     TPM2B_DATA            *data,                  //   IN: qualifying data
     TPMS_ATTEST           *attest                 //   OUT: attest structure
     )
{
     TPM_RC                         result;
     TPMI_RH_HIERARCHY              signHierarhcy;
     result = CryptSelectSignScheme(signHandle, scheme);
     if(result != TPM_RC_SUCCESS)
         return result;
     // Magic number
     attest->magic = TPM_GENERATED_VALUE;
     if(signHandle == TPM_RH_NULL)
     {
         BYTE     *buffer;
         INT32     bufferSize;
         // For null sign handle, the QN is TPM_RH_NULL
         buffer = attest->qualifiedSigner.t.name;
         bufferSize = sizeof(TPM_HANDLE);
         attest->qualifiedSigner.t.size =
              TPM_HANDLE_Marshal(&signHandle, &buffer, &bufferSize);
     }
     else
     {
         // Certifying object qualified name
         // if the scheme is anonymous, this is an empty buffer
         if(CryptIsSchemeAnonymous(scheme->scheme))
              attest->qualifiedSigner.t.size = 0;
         else
              ObjectGetQualifiedName(signHandle, &attest->qualifiedSigner);
   }
   // current clock in plain text
   TimeFillInfo(&attest->clockInfo);
   // Firmware version in plain text
   attest->firmwareVersion = ((UINT64) gp.firmwareV1 << (sizeof(UINT32) * 8));
   attest->firmwareVersion += gp.firmwareV2;
   // Get the hierarchy of sign object. For NULL sign handle, the hierarchy
   // will be TPM_RH_NULL
   signHierarhcy = EntityGetHierarchy(signHandle);
   if(signHierarhcy != TPM_RH_PLATFORM && signHierarhcy != TPM_RH_ENDORSEMENT)
   {
       // For sign object is not in platform or endorsement hierarchy,
       // obfuscate the clock and firmwereVersion information
       UINT64          obfuscation[2];
       TPMI_ALG_HASH   hashAlg;
         // Get hash algorithm
         if(signHandle == TPM_RH_NULL || signHandle == TPM_RH_OWNER)
         {
              hashAlg = CONTEXT_INTEGRITY_HASH_ALG;
         }
         else
         {
              OBJECT          *signObject = NULL;
              signObject = ObjectGet(signHandle);
              hashAlg = signObject->publicArea.nameAlg;
         }
         KDFa(hashAlg, &gp.shProof.b, "OBFUSCATE",
               &attest->qualifiedSigner.b, NULL, 128, (BYTE *)&obfuscation[0], NULL);
         // Obfuscate data
         attest->firmwareVersion += obfuscation[0];
         attest->clockInfo.resetCount += (UINT32)(obfuscation[1] >> 32);
         attest->clockInfo.restartCount += (UINT32)obfuscation[1];
   }
   // External data
   if(CryptIsSchemeAnonymous(scheme->scheme))
       attest->extraData.t.size = 0;
   else
   {
       // If we move the data to the attestation structure, then we will not use
       // it in the signing operation except as part of the signed data
       attest->extraData = *data;
       data->t.size = 0;
   }
   return TPM_RC_SUCCESS;
}
//
//
//          SignAttestInfo()
//
//     Sign a TPMS_ATTEST structure. If signHandle is TPM_RH_NULL, a null signature is returned.
//
//
//
//
//      Error Returns                     Meaning
//
//      TPM_RC_ATTRIBUTES                 signHandle references not a signing key
//      TPM_RC_SCHEME                     scheme is not compatible with signHandle type
//      TPM_RC_VALUE                      digest generated for the given scheme is greater than the modulus of
//                                        signHandle (for an RSA key); invalid commit status or failed to
//                                        generate r value (for an ECC key)
//
TPM_RC
SignAttestInfo(
   TPMI_DH_OBJECT           signHandle,                //   IN: handle of sign object
   TPMT_SIG_SCHEME         *scheme,                    //   IN: sign scheme
   TPMS_ATTEST             *certifyInfo,               //   IN: the data to be signed
   TPM2B_DATA              *qualifyingData,            //   IN: extra data for the signing proce
   TPM2B_ATTEST            *attest,                    //   OUT: marshaled attest blob to be
                                                       //       signed
   TPMT_SIGNATURE          *signature                  //   OUT: signature
   )
{
   TPM_RC                         result;
   TPMI_ALG_HASH                  hashAlg;
   BYTE                           *buffer;
   INT32                          bufferSize;
   HASH_STATE                     hashState;
   TPM2B_DIGEST                   digest;
   // Marshal TPMS_ATTEST structure for hash
   buffer = attest->t.attestationData;
   bufferSize = sizeof(TPMS_ATTEST);
   attest->t.size = TPMS_ATTEST_Marshal(certifyInfo, &buffer, &bufferSize);
   if(signHandle == TPM_RH_NULL)
   {
       signature->sigAlg = TPM_ALG_NULL;
   }
   else
   {
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
         // Compute hash
         hashAlg = scheme->details.any.hashAlg;
         digest.t.size = CryptStartHash(hashAlg, &hashState);
         CryptUpdateDigest(&hashState, attest->t.size, attest->t.attestationData);
         CryptCompleteHash2B(&hashState, &digest.b);
         // If there is qualifying data, need to rehash the the data
         // hash(qualifyingData || hash(attestationData))
         if(qualifyingData->t.size != 0)
         {
             CryptStartHash(hashAlg, &hashState);
             CryptUpdateDigest(&hashState,
                               qualifyingData->t.size,
                               qualifyingData->t.buffer);
             CryptUpdateDigest(&hashState, digest.t.size, digest.t.buffer);
             CryptCompleteHash2B(&hashState, &digest.b);
          }
          // Sign the hash. A TPM_RC_VALUE, TPM_RC_SCHEME, or
          // TPM_RC_ATTRIBUTES error may be returned at this point
          return CryptSign(signHandle,
                           scheme,
                           &digest,
                           signature);
     }
     return TPM_RC_SUCCESS;
}
