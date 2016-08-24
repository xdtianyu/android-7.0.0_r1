// This file was extracted from the TCG Published
// Trusted Platform Module Library
// Part 3: Commands
// Family "2.0"
// Level 00 Revision 01.16
// October 30, 2014

#include "InternalRoutines.h"
#include "ContextSave_fp.h"
#include "Context_spt_fp.h"
//
//
//     Error Returns                     Meaning
//
//     TPM_RC_CONTEXT_GAP                a contextID could not be assigned for a session context save
//     TPM_RC_TOO_MANY_CONTEXTS          no more contexts can be saved as the counter has maxed out
//
TPM_RC
TPM2_ContextSave(
   ContextSave_In        *in,              // IN: input parameter list
   ContextSave_Out       *out              // OUT: output parameter list
   )
{
   TPM_RC            result;
   UINT16            fingerprintSize;      // The size of fingerprint in context
   // blob.
   UINT64            contextID = 0;        // session context ID
   TPM2B_SYM_KEY     symKey;
   TPM2B_IV          iv;

   TPM2B_DIGEST      integrity;
   UINT16            integritySize;
   BYTE              *buffer;
   INT32             bufferSize;

   // This command may cause the orderlyState to be cleared due to
   // the update of state reset data. If this is the case, check if NV is
   // available first
   if(gp.orderlyState != SHUTDOWN_NONE)
   {
       // The command needs NV update. Check if NV is available.
       // A TPM_RC_NV_UNAVAILABLE or TPM_RC_NV_RATE error may be returned at
       // this point
       result = NvIsAvailable();
       if(result != TPM_RC_SUCCESS) return result;
   }

// Internal Data Update

   // Initialize output handle. At the end of command action, the output
   // handle of an object will be replaced, while the output handle
   // for a session will be the same as input
   out->context.savedHandle = in->saveHandle;

   // Get the size of fingerprint in context blob. The sequence value in
   // TPMS_CONTEXT structure is used as the fingerprint
   fingerprintSize = sizeof(out->context.sequence);

   // Compute the integrity size at the beginning of context blob
   integritySize = sizeof(integrity.t.size)
                   + CryptGetHashDigestSize(CONTEXT_INTEGRITY_HASH_ALG);

   // Perform object or session specific context save
   switch(HandleGetType(in->saveHandle))
   {
   case TPM_HT_TRANSIENT:
   {
       OBJECT          *object = ObjectGet(in->saveHandle);
      OBJECT         *outObject =
                             (OBJECT *)(out->context.contextBlob.t.buffer
                                         + integritySize + fingerprintSize);

      // Set size of the context data. The contents of context blob is vendor
      // defined. In this implementation, the size is size of integrity
      // plus fingerprint plus the whole internal OBJECT structure
      out->context.contextBlob.t.size = integritySize +
                                        fingerprintSize + sizeof(OBJECT);
      // Make sure things fit
      pAssert(out->context.contextBlob.t.size
              < sizeof(out->context.contextBlob.t.buffer));

      // Copy the whole internal OBJECT structure to context blob, leave
      // the size for fingerprint
      *outObject = *object;

      // Increment object context ID
      gr.objectContextID++;
      // If object context ID overflows, TPM should be put in failure mode
      if(gr.objectContextID == 0)
          FAIL(FATAL_ERROR_INTERNAL);

      // Fill in other return values for an object.
      out->context.sequence = gr.objectContextID;
      // For regular object, savedHandle is 0x80000000. For sequence object,
      // savedHandle is 0x80000001. For object with stClear, savedHandle
      // is 0x80000002
      if(ObjectIsSequence(object))
      {
          out->context.savedHandle = 0x80000001;
          SequenceDataImportExport(object, outObject, EXPORT_STATE);
      }
      else if(object->attributes.stClear == SET)
      {
          out->context.savedHandle = 0x80000002;
      }
      else
      {
          out->context.savedHandle = 0x80000000;
      }

      // Get object hierarchy
      out->context.hierarchy = ObjectDataGetHierarchy(object);

      break;
  }
  case TPM_HT_HMAC_SESSION:
  case TPM_HT_POLICY_SESSION:
  {
      SESSION         *session = SessionGet(in->saveHandle);

      // Set size of the context data. The contents of context blob is vendor
      // defined. In this implementation, the size of context blob is the
      // size of a internal session structure plus the size of
      // fingerprint plus the size of integrity
      out->context.contextBlob.t.size = integritySize +
                                        fingerprintSize + sizeof(*session);

      // Make sure things fit
      pAssert(out->context.contextBlob.t.size
              < sizeof(out->context.contextBlob.t.buffer));

      // Copy the whole internal SESSION structure to context blob.
      // Save space for fingerprint at the beginning of the buffer
      // This is done before anything else so that the actual context
       // can be reclaimed after this call
       MemoryCopy(out->context.contextBlob.t.buffer
                  + integritySize + fingerprintSize,
                  session, sizeof(*session),
                  sizeof(out->context.contextBlob.t.buffer)
                           - integritySize - fingerprintSize);

       // Fill in the other return parameters for a session
       // Get a context ID and set the session tracking values appropriately
       // TPM_RC_CONTEXT_GAP is a possible error.
       // SessionContextSave() will flush the in-memory context
       // so no additional errors may occur after this call.
       result = SessionContextSave(out->context.savedHandle, &contextID);
       if(result != TPM_RC_SUCCESS) return result;

       // sequence number is the current session contextID
       out->context.sequence = contextID;

       // use TPM_RH_NULL as hierarchy for session context
       out->context.hierarchy = TPM_RH_NULL;

       break;
   }
   default:
       // SaveContext may only take an object handle or a session handle.
       // All the other handle type should be filtered out at unmarshal
       pAssert(FALSE);
       break;
   }

   // Save fingerprint at the beginning of encrypted area of context blob.
   // Reserve the integrity space
   MemoryCopy(out->context.contextBlob.t.buffer + integritySize,
              &out->context.sequence, sizeof(out->context.sequence),
              sizeof(out->context.contextBlob.t.buffer) - integritySize);

   // Compute context encryption key
   ComputeContextProtectionKey(&out->context, &symKey, &iv);

   // Encrypt context blob
   CryptSymmetricEncrypt(out->context.contextBlob.t.buffer + integritySize,
                         CONTEXT_ENCRYPT_ALG, CONTEXT_ENCRYPT_KEY_BITS,
                         TPM_ALG_CFB, symKey.t.buffer, &iv,
                         out->context.contextBlob.t.size - integritySize,
                         out->context.contextBlob.t.buffer + integritySize);

   // Compute integrity hash for the object
   // In this implementation, the same routine is used for both sessions
   // and objects.
   ComputeContextIntegrity(&out->context, &integrity);

   // add integrity at the beginning of context blob
   buffer = out->context.contextBlob.t.buffer;
   bufferSize = sizeof(TPM2B_DIGEST);
   TPM2B_DIGEST_Marshal(&integrity, &buffer, &bufferSize);

   // orderly state should be cleared because of the update of state reset and
   // state clear data
   g_clearOrderly = TRUE;

   return TPM_RC_SUCCESS;
}
