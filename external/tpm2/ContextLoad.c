// This file was extracted from the TCG Published
// Trusted Platform Module Library
// Part 3: Commands
// Family "2.0"
// Level 00 Revision 01.16
// October 30, 2014

#include "InternalRoutines.h"
#include "ContextLoad_fp.h"
#include "Context_spt_fp.h"
//
//
//     Error Returns                 Meaning
//
//     TPM_RC_CONTEXT_GAP            there is only one available slot and this is not the oldest saved
//                                   session context
//     TPM_RC_HANDLE                 'context. savedHandle' does not reference a saved session
//     TPM_RC_HIERARCHY              'context.hierarchy' is disabled
//     TPM_RC_INTEGRITY              context integrity check fail
//     TPM_RC_OBJECT_MEMORY          no free slot for an object
//     TPM_RC_SESSION_MEMORY         no free session slots
//     TPM_RC_SIZE                   incorrect context blob size
//
TPM_RC
TPM2_ContextLoad(
   ContextLoad_In     *in,                  // IN: input parameter list
   ContextLoad_Out    *out                  // OUT: output parameter list
   )
{
// Local Variables
   TPM_RC      result = TPM_RC_SUCCESS;

   TPM2B_DIGEST       integrityToCompare;
   TPM2B_DIGEST       integrity;
   UINT16             integritySize;
   UINT64             fingerprint;
   BYTE               *buffer;
   INT32              size;

   TPM_HT             handleType;
   TPM2B_SYM_KEY      symKey;
   TPM2B_IV           iv;

// Input Validation

   // Check context blob size
   handleType = HandleGetType(in->context.savedHandle);

   // Check integrity
   // In this implementation, the same routine is used for both sessions
   // and objects.
   integritySize = CryptGetHashDigestSize(CONTEXT_INTEGRITY_HASH_ALG);

   // Get integrity from context blob
   buffer = in->context.contextBlob.t.buffer;
   size = (INT32) in->context.contextBlob.t.size;
   result = TPM2B_DIGEST_Unmarshal(&integrity, &buffer, &size);
   if(result != TPM_RC_SUCCESS)
       return result;
   if(integrity.t.size != integritySize)
       return TPM_RC_SIZE;

   integritySize += sizeof(integrity.t.size);
//

   // Compute context integrity
   ComputeContextIntegrity(&in->context, &integrityToCompare);

   // Compare integrity
   if(!Memory2BEqual(&integrity.b, &integrityToCompare.b))
       return TPM_RC_INTEGRITY + RC_ContextLoad_context;

   // Compute context encryption key
   ComputeContextProtectionKey(&in->context, &symKey, &iv);

   // Decrypt context data in place
   CryptSymmetricDecrypt(in->context.contextBlob.t.buffer + integritySize,
                         CONTEXT_ENCRYPT_ALG, CONTEXT_ENCRYPT_KEY_BITS,
                         TPM_ALG_CFB, symKey.t.buffer, &iv,
                         in->context.contextBlob.t.size - integritySize,
                         in->context.contextBlob.t.buffer + integritySize);

   // Read the fingerprint value, skip the leading integrity size
   MemoryCopy(&fingerprint, in->context.contextBlob.t.buffer + integritySize,
              sizeof(fingerprint), sizeof(fingerprint));
   // Check fingerprint. If the check fails, TPM should be put to failure mode
   if(fingerprint != in->context.sequence)
       FAIL(FATAL_ERROR_INTERNAL);

   // Perform object or session specific input check
   switch(handleType)
   {
   case TPM_HT_TRANSIENT:
   {
       // Get a pointer to the object in the context blob
       OBJECT      *outObject = (OBJECT *)(in->context.contextBlob.t.buffer
                               + integritySize + sizeof(fingerprint));

       // Discard any changes to the handle that the TRM might have made
       in->context.savedHandle = TRANSIENT_FIRST;

       // If hierarchy is disabled, no object context can be loaded in this
       // hierarchy
       if(!HierarchyIsEnabled(in->context.hierarchy))
           return TPM_RC_HIERARCHY + RC_ContextLoad_context;

       // Restore object. A TPM_RC_OBJECT_MEMORY error may be returned at
       // this point
       result = ObjectContextLoad(outObject, &out->loadedHandle);
       if(result != TPM_RC_SUCCESS)
           return result;

       // If this is a sequence object, the crypto library may need to
       // reformat the data into an internal format
       if(ObjectIsSequence(outObject))
           SequenceDataImportExport(ObjectGet(out->loadedHandle),
                                    outObject, IMPORT_STATE);

       break;
   }
   case TPM_HT_POLICY_SESSION:
   case TPM_HT_HMAC_SESSION:
   {

       SESSION      *session = (SESSION *)(in->context.contextBlob.t.buffer
                                        + integritySize + sizeof(fingerprint));

       // This command may cause the orderlyState to be cleared due to
       // the update of state reset data. If this is the case, check if NV is
       // available first
      if(gp.orderlyState != SHUTDOWN_NONE)
      {
          // The command needs NV update. Check if NV is available.
          // A TPM_RC_NV_UNAVAILABLE or TPM_RC_NV_RATE error may be returned
          // at this point
          result = NvIsAvailable();
          if(result != TPM_RC_SUCCESS)
              return result;
      }

      // Check if input handle points to a valid saved session
      if(!SessionIsSaved(in->context.savedHandle))
          return TPM_RC_HANDLE + RC_ContextLoad_context;

      // Restore session. A TPM_RC_SESSION_MEMORY, TPM_RC_CONTEXT_GAP error
      // may be returned at this point
      result = SessionContextLoad(session, &in->context.savedHandle);
      if(result != TPM_RC_SUCCESS)
          return result;

      out->loadedHandle = in->context.savedHandle;

      // orderly state should be cleared because of the update of state
      // reset and state clear data
      g_clearOrderly = TRUE;

      break;
  }
  default:
      // Context blob may only have an object handle or a session handle.
      // All the other handle type should be filtered out at unmarshal
      pAssert(FALSE);
      break;
  }

   return TPM_RC_SUCCESS;
}
