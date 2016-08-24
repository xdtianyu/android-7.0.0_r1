// This file was extracted from the TCG Published
// Trusted Platform Module Library
// Part 3: Commands
// Family "2.0"
// Level 00 Revision 01.16
// October 30, 2014

#include "InternalRoutines.h"
#include "PolicyCounterTimer_fp.h"
#include "Policy_spt_fp.h"
//
//
//     Error Returns                  Meaning
//
//     TPM_RC_POLICY                  the comparison of the selected portion of the TPMS_TIME_INFO with
//                                    operandB failed
//     TPM_RC_RANGE                   offset + size exceed size of TPMS_TIME_INFO structure
//
TPM_RC
TPM2_PolicyCounterTimer(
   PolicyCounterTimer_In      *in              // IN: input parameter list
   )
{
   TPM_RC                result;
   SESSION              *session;
   TIME_INFO             infoData;      // data buffer of TPMS_TIME_INFO
   TPM_CC                commandCode = TPM_CC_PolicyCounterTimer;
   HASH_STATE            hashState;
   TPM2B_DIGEST          argHash;

// Input Validation

   // If the command is going to use any part of the counter or timer, need
   // to verify that time is advancing.
   // The time and clock vales are the first two 64-bit values in the clock
   if(in->offset < sizeof(UINT64) + sizeof(UINT64))
   {
       // Using Clock or Time so see if clock is running. Clock doesn't run while
       // NV is unavailable.
       // TPM_RC_NV_UNAVAILABLE or TPM_RC_NV_RATE error may be returned here.
       result = NvIsAvailable();
       if(result != TPM_RC_SUCCESS)
           return result;
   }
   // Get pointer to the session structure
   session = SessionGet(in->policySession);

   //If this is a trial policy, skip all validations and the operation
   if(session->attributes.isTrialPolicy == CLEAR)
   {
       // Get time data info. The size of time info data equals the input
       // operand B size. A TPM_RC_RANGE error may be returned at this point
       result = TimeGetRange(in->offset, in->operandB.t.size, &infoData);
       if(result != TPM_RC_SUCCESS) return result;

         // Arithmetic Comparison
         switch(in->operation)
         {
             case TPM_EO_EQ:
                 // compare A = B
                 if(CryptCompare(in->operandB.t.size, infoData,
                                 in->operandB.t.size, in->operandB.t.buffer) != 0)
                     return TPM_RC_POLICY;
                 break;
             case TPM_EO_NEQ:
                 // compare A != B
                 if(CryptCompare(in->operandB.t.size, infoData,
                                in->operandB.t.size, in->operandB.t.buffer)   == 0)
                    return TPM_RC_POLICY;
                break;
            case TPM_EO_SIGNED_GT:
                // compare A > B signed
                if(CryptCompareSigned(in->operandB.t.size, infoData,
                                in->operandB.t.size, in->operandB.t.buffer)   <= 0)
                    return TPM_RC_POLICY;
                break;
            case TPM_EO_UNSIGNED_GT:
                // compare A > B unsigned
                if(CryptCompare(in->operandB.t.size, infoData,
                                in->operandB.t.size, in->operandB.t.buffer)   <= 0)
                    return TPM_RC_POLICY;
                break;
            case TPM_EO_SIGNED_LT:
                // compare A < B signed
                if(CryptCompareSigned(in->operandB.t.size, infoData,
                                in->operandB.t.size, in->operandB.t.buffer)   >= 0)
                    return TPM_RC_POLICY;
                break;
            case TPM_EO_UNSIGNED_LT:
                // compare A < B unsigned
                if(CryptCompare(in->operandB.t.size, infoData,
                                in->operandB.t.size, in->operandB.t.buffer)   >= 0)
                    return TPM_RC_POLICY;
                break;
            case TPM_EO_SIGNED_GE:
                // compare A >= B signed
                if(CryptCompareSigned(in->operandB.t.size, infoData,
                                in->operandB.t.size, in->operandB.t.buffer)   < 0)
                    return TPM_RC_POLICY;
                break;
            case TPM_EO_UNSIGNED_GE:
                // compare A >= B unsigned
                if(CryptCompare(in->operandB.t.size, infoData,
                                in->operandB.t.size, in->operandB.t.buffer)   < 0)
                    return TPM_RC_POLICY;
                break;
            case TPM_EO_SIGNED_LE:
                // compare A <= B signed
                if(CryptCompareSigned(in->operandB.t.size, infoData,
                                in->operandB.t.size, in->operandB.t.buffer)   > 0)
                    return TPM_RC_POLICY;
                break;
            case TPM_EO_UNSIGNED_LE:
                // compare A <= B unsigned
                if(CryptCompare(in->operandB.t.size, infoData,
                                in->operandB.t.size, in->operandB.t.buffer)   > 0)
                    return TPM_RC_POLICY;
                break;
            case TPM_EO_BITSET:
                // All bits SET in B are SET in A. ((A&B)=B)
            {
                UINT32 i;
                for (i = 0; i < in->operandB.t.size; i++)
                    if(   (infoData[i] & in->operandB.t.buffer[i])
                       != in->operandB.t.buffer[i])
                        return TPM_RC_POLICY;
            }
            break;
            case TPM_EO_BITCLEAR:
                // All bits SET in B are CLEAR in A. ((A&B)=0)
            {
                UINT32 i;
                for (i = 0; i < in->operandB.t.size; i++)
                  if((infoData[i] & in->operandB.t.buffer[i]) != 0)
                      return TPM_RC_POLICY;
          }
          break;
          default:
              pAssert(FALSE);
              break;
      }
  }

// Internal Data Update

  // Start argument list hash
  argHash.t.size = CryptStartHash(session->authHashAlg, &hashState);
  // add operandB
  CryptUpdateDigest2B(&hashState, &in->operandB.b);
  // add offset
  CryptUpdateDigestInt(&hashState, sizeof(UINT16), &in->offset);
  // add operation
  CryptUpdateDigestInt(&hashState, sizeof(TPM_EO), &in->operation);
  // complete argument hash
  CryptCompleteHash2B(&hashState, &argHash.b);

  // update policyDigest
  // start hash
  CryptStartHash(session->authHashAlg, &hashState);

  // add old digest
  CryptUpdateDigest2B(&hashState, &session->u2.policyDigest.b);

  // add commandCode
  CryptUpdateDigestInt(&hashState, sizeof(TPM_CC), &commandCode);

  // add argument digest
  CryptUpdateDigest2B(&hashState, &argHash.b);

  // complete the digest
  CryptCompleteHash2B(&hashState, &session->u2.policyDigest.b);

   return TPM_RC_SUCCESS;
}
