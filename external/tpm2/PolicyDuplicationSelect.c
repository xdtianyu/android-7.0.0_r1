// This file was extracted from the TCG Published
// Trusted Platform Module Library
// Part 3: Commands
// Family "2.0"
// Level 00 Revision 01.16
// October 30, 2014

#include "InternalRoutines.h"
#include "PolicyDuplicationSelect_fp.h"
//
//
//     Error Returns                     Meaning
//
//     TPM_RC_COMMAND_CODE               commandCode of 'policySession; is not empty
//     TPM_RC_CPHASH                     cpHash of policySession is not empty
//
TPM_RC
TPM2_PolicyDuplicationSelect(
   PolicyDuplicationSelect_In       *in                 // IN: input parameter list
   )
{
   SESSION           *session;
   HASH_STATE        hashState;
   TPM_CC            commandCode = TPM_CC_PolicyDuplicationSelect;

// Input Validation

   // Get pointer to the session structure
   session = SessionGet(in->policySession);

   // cpHash in session context must be empty
   if(session->u1.cpHash.t.size != 0)
       return TPM_RC_CPHASH;

   // commandCode in session context must be empty
   if(session->commandCode != 0)
       return TPM_RC_COMMAND_CODE;

// Internal Data Update

   // Update name hash
   session->u1.cpHash.t.size = CryptStartHash(session->authHashAlg, &hashState);

   // add objectName
   CryptUpdateDigest2B(&hashState, &in->objectName.b);

   // add new parent name
   CryptUpdateDigest2B(&hashState, &in->newParentName.b);

   // complete hash
   CryptCompleteHash2B(&hashState, &session->u1.cpHash.b);

   // update policy hash
   // Old policyDigest size should be the same as the new policyDigest size since
   // they are using the same hash algorithm
   session->u2.policyDigest.t.size
           = CryptStartHash(session->authHashAlg, &hashState);

   // add old policy
   CryptUpdateDigest2B(&hashState, &session->u2.policyDigest.b);

   // add command code
   CryptUpdateDigestInt(&hashState, sizeof(TPM_CC), &commandCode);

   // add objectName
   if(in->includeObject == YES)
       CryptUpdateDigest2B(&hashState, &in->objectName.b);

  // add new parent name
  CryptUpdateDigest2B(&hashState, &in->newParentName.b);

  // add includeObject
  CryptUpdateDigestInt(&hashState, sizeof(TPMI_YES_NO), &in->includeObject);

  // complete digest
  CryptCompleteHash2B(&hashState, &session->u2.policyDigest.b);

  // clear iscpHashDefined bit to indicate now this field contains a nameHash
  session->attributes.iscpHashDefined = CLEAR;

  // set commandCode in session context
  session->commandCode = TPM_CC_Duplicate;

   return TPM_RC_SUCCESS;
}
