// This file was extracted from the TCG Published
// Trusted Platform Module Library
// Part 3: Commands
// Family "2.0"
// Level 00 Revision 01.16
// October 30, 2014

#include "InternalRoutines.h"
#include "EvictControl_fp.h"
//
//
//     Error Returns                     Meaning
//
//     TPM_RC_ATTRIBUTES                 an object with temporary, stClear or publicOnly attribute SET cannot
//                                       be made persistent
//     TPM_RC_HIERARCHY                  auth cannot authorize the operation in the hierarchy of evictObject
//     TPM_RC_HANDLE                     evictHandle of the persistent object to be evicted is not the same as
//                                       the persistentHandle argument
//     TPM_RC_NV_HANDLE                  persistentHandle is unavailable
//     TPM_RC_NV_SPACE                   no space in NV to make evictHandle persistent
//     TPM_RC_RANGE                      persistentHandle is not in the range corresponding to the hierarchy of
//                                       evictObject
//
TPM_RC
TPM2_EvictControl(
   EvictControl_In       *in                   // IN: input parameter list
   )
{
   TPM_RC       result;
   OBJECT       *evictObject;

   // The command needs NV update. Check if NV is available.
   // A TPM_RC_NV_UNAVAILABLE or TPM_RC_NV_RATE error may be returned at
   // this point
   result = NvIsAvailable();
   if(result != TPM_RC_SUCCESS) return result;

// Input Validation

   // Get internal object pointer
   evictObject = ObjectGet(in->objectHandle);

   // Temporary, stClear or public only objects can not be made persistent
   if(   evictObject->attributes.temporary == SET
      || evictObject->attributes.stClear == SET
      || evictObject->attributes.publicOnly == SET
     )
       return TPM_RC_ATTRIBUTES + RC_EvictControl_objectHandle;

   // If objectHandle refers to a persistent object, it should be the same as
   // input persistentHandle
   if(   evictObject->attributes.evict == SET
      && evictObject->evictHandle != in->persistentHandle
     )
       return TPM_RC_HANDLE + RC_EvictControl_objectHandle;

   // Additional auth validation
   if(in->auth == TPM_RH_PLATFORM)
   {
       // To make persistent
       if(evictObject->attributes.evict == CLEAR)
       {
           // Platform auth can not set evict object in storage or endorsement
           // hierarchy
          if(evictObject->attributes.ppsHierarchy == CLEAR)
              return TPM_RC_HIERARCHY + RC_EvictControl_objectHandle;

          // Platform cannot use a handle outside of platform persistent range.
          if(!NvIsPlatformPersistentHandle(in->persistentHandle))
              return TPM_RC_RANGE + RC_EvictControl_persistentHandle;
      }
      // Platform auth can delete any persistent object
  }
  else if(in->auth == TPM_RH_OWNER)
  {
      // Owner auth can not set or clear evict object in platform hierarchy
      if(evictObject->attributes.ppsHierarchy == SET)
          return TPM_RC_HIERARCHY + RC_EvictControl_objectHandle;

      // Owner cannot use a handle outside of owner persistent range.
      if(   evictObject->attributes.evict == CLEAR
         && !NvIsOwnerPersistentHandle(in->persistentHandle)
        )
          return TPM_RC_RANGE + RC_EvictControl_persistentHandle;
  }
  else
  {
      // Other auth is not allowed in this command and should be filtered out
      // at unmarshal process
      pAssert(FALSE);
  }

// Internal Data Update

  // Change evict state
  if(evictObject->attributes.evict == CLEAR)
  {
      // Make object persistent
      // A TPM_RC_NV_HANDLE or TPM_RC_NV_SPACE error may be returned at this
      // point
      result = NvAddEvictObject(in->persistentHandle, evictObject);
      if(result != TPM_RC_SUCCESS) return result;
  }
  else
  {
      // Delete the persistent object in NV
      NvDeleteEntity(evictObject->evictHandle);
  }

  return TPM_RC_SUCCESS;

}
