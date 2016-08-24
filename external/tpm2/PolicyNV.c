// This file was extracted from the TCG Published
// Trusted Platform Module Library
// Part 3: Commands
// Family "2.0"
// Level 00 Revision 01.16
// October 30, 2014

#include "InternalRoutines.h"
#include "PolicyNV_fp.h"
#include "Policy_spt_fp.h"
#include "NV_spt_fp.h"         // Include NV support routine for read access check
//
//
//     Error Returns                     Meaning
//
//     TPM_RC_AUTH_TYPE                  NV index authorization type is not correct
//     TPM_RC_NV_LOCKED                  NV index read locked
//     TPM_RC_NV_UNINITIALIZED           the NV index has not been initialized
//     TPM_RC_POLICY                     the comparison to the NV contents failed
//     TPM_RC_SIZE                       the size of nvIndex data starting at offset is less than the size of
//                                       operandB
//
TPM_RC
TPM2_PolicyNV(
   PolicyNV_In       *in                  // IN: input parameter list
   )
{
   TPM_RC                   result;
   SESSION                 *session;
   NV_INDEX                 nvIndex;
   BYTE                     nvBuffer[sizeof(in->operandB.t.buffer)];
   TPM2B_NAME               nvName;
   TPM_CC                   commandCode = TPM_CC_PolicyNV;
   HASH_STATE               hashState;
   TPM2B_DIGEST             argHash;

// Input Validation

   // Get NV index information
   NvGetIndexInfo(in->nvIndex, &nvIndex);

   // Get pointer to the session structure
   session = SessionGet(in->policySession);

   //If this is a trial policy, skip all validations and the operation
   if(session->attributes.isTrialPolicy == CLEAR)
   {
       // NV Read access check. NV index should be allowed for read. A
       // TPM_RC_AUTH_TYPE or TPM_RC_NV_LOCKED error may be return at this
       // point
       result = NvReadAccessChecks(in->authHandle, in->nvIndex);
       if(result != TPM_RC_SUCCESS) return result;

       // Valid NV data size should not be smaller than input operandB size
       if((nvIndex.publicArea.dataSize - in->offset) < in->operandB.t.size)
           return TPM_RC_SIZE + RC_PolicyNV_operandB;

       // Arithmetic Comparison

       // Get NV data. The size of NV data equals the input operand B size
       NvGetIndexData(in->nvIndex, &nvIndex, in->offset,
                      in->operandB.t.size, nvBuffer);

       switch(in->operation)
       {
          case TPM_EO_EQ:
              // compare A = B
              if(CryptCompare(in->operandB.t.size, nvBuffer,
                              in->operandB.t.size, in->operandB.t.buffer)   != 0)
                  return TPM_RC_POLICY;
              break;
          case TPM_EO_NEQ:
              // compare A != B
              if(CryptCompare(in->operandB.t.size, nvBuffer,
                              in->operandB.t.size, in->operandB.t.buffer)   == 0)
                  return TPM_RC_POLICY;
              break;
          case TPM_EO_SIGNED_GT:
              // compare A > B signed
              if(CryptCompareSigned(in->operandB.t.size, nvBuffer,
                              in->operandB.t.size, in->operandB.t.buffer)   <= 0)
                  return TPM_RC_POLICY;
              break;
          case TPM_EO_UNSIGNED_GT:
              // compare A > B unsigned
              if(CryptCompare(in->operandB.t.size, nvBuffer,
                              in->operandB.t.size, in->operandB.t.buffer)   <= 0)
                  return TPM_RC_POLICY;
              break;
          case TPM_EO_SIGNED_LT:
              // compare A < B signed
              if(CryptCompareSigned(in->operandB.t.size, nvBuffer,
                              in->operandB.t.size, in->operandB.t.buffer)   >= 0)
                  return TPM_RC_POLICY;
              break;
          case TPM_EO_UNSIGNED_LT:
              // compare A < B unsigned
              if(CryptCompare(in->operandB.t.size, nvBuffer,
                              in->operandB.t.size, in->operandB.t.buffer)   >= 0)
                  return TPM_RC_POLICY;
              break;
          case TPM_EO_SIGNED_GE:
              // compare A >= B signed
              if(CryptCompareSigned(in->operandB.t.size, nvBuffer,
                              in->operandB.t.size, in->operandB.t.buffer)   < 0)
                  return TPM_RC_POLICY;
              break;
          case TPM_EO_UNSIGNED_GE:
              // compare A >= B unsigned
              if(CryptCompare(in->operandB.t.size, nvBuffer,
                              in->operandB.t.size, in->operandB.t.buffer)   < 0)
                  return TPM_RC_POLICY;
              break;
          case TPM_EO_SIGNED_LE:
              // compare A <= B signed
              if(CryptCompareSigned(in->operandB.t.size, nvBuffer,
                              in->operandB.t.size, in->operandB.t.buffer)   > 0)
                  return TPM_RC_POLICY;
              break;
          case TPM_EO_UNSIGNED_LE:
              // compare A <= B unsigned
              if(CryptCompare(in->operandB.t.size, nvBuffer,
                              in->operandB.t.size, in->operandB.t.buffer)   > 0)
                  return TPM_RC_POLICY;
              break;
          case TPM_EO_BITSET:
              // All bits SET in B are SET in A. ((A&B)=B)
          {
              UINT32 i;
              for (i = 0; i < in->operandB.t.size; i++)
                  if((nvBuffer[i] & in->operandB.t.buffer[i])
                             != in->operandB.t.buffer[i])
                         return TPM_RC_POLICY;
            }
            break;
            case TPM_EO_BITCLEAR:
                // All bits SET in B are CLEAR in A. ((A&B)=0)
            {
                UINT32 i;
                for (i = 0; i < in->operandB.t.size; i++)
                    if((nvBuffer[i] & in->operandB.t.buffer[i]) != 0)
                        return TPM_RC_POLICY;
            }
            break;
            default:
                pAssert(FALSE);
                break;
       }
   }

// Internal Data Update

   // Start argument hash
   argHash.t.size = CryptStartHash(session->authHashAlg, &hashState);

   // add operandB
   CryptUpdateDigest2B(&hashState, &in->operandB.b);

   // add offset
   CryptUpdateDigestInt(&hashState, sizeof(UINT16), &in->offset);

   // add operation
   CryptUpdateDigestInt(&hashState, sizeof(TPM_EO), &in->operation);

   // complete argument digest
   CryptCompleteHash2B(&hashState, &argHash.b);

   // Update policyDigest
   // Start digest
   CryptStartHash(session->authHashAlg, &hashState);

   // add old digest
   CryptUpdateDigest2B(&hashState, &session->u2.policyDigest.b);

   // add commandCode
   CryptUpdateDigestInt(&hashState, sizeof(TPM_CC), &commandCode);

   // add argument digest
   CryptUpdateDigest2B(&hashState, &argHash.b);

   // Adding nvName
   nvName.t.size = EntityGetName(in->nvIndex, &nvName.t.name);
   CryptUpdateDigest2B(&hashState, &nvName.b);

   // complete the digest
   CryptCompleteHash2B(&hashState, &session->u2.policyDigest.b);

   return TPM_RC_SUCCESS;
}
