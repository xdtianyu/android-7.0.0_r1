// This file was extracted from the TCG Published
// Trusted Platform Module Library
// Part 4: Supporting Routines
// Family "2.0"
// Level 00 Revision 01.16
// October 30, 2014

#include "InternalRoutines.h"
//
//
//
//          Functions
//
//          EntityGetLoadStatus()
//
//     This function will indicate if the entity associated with a handle is present in TPM memory. If the handle is
//     a persistent object handle, and the object exists, the persistent object is moved from NV memory into a
//     RAM object slot and the persistent handle is replaced with the transient object handle for the slot.
//
//     Error Returns                     Meaning
//
//     TPM_RC_HANDLE                     handle type does not match
//     TPM_RC_REFERENCE_H0               entity is not present
//     TPM_RC_HIERARCHY                  entity belongs to a disabled hierarchy
//     TPM_RC_OBJECT_MEMORY              handle is an evict object but there is no space to load it to RAM
//
TPM_RC
EntityGetLoadStatus(
    TPM_HANDLE          *handle,              // IN/OUT: handle of the entity
    TPM_CC               commandCode          // IN: the commmandCode
    )
{
    TPM_RC              result = TPM_RC_SUCCESS;
    switch(HandleGetType(*handle))
    {
        // For handles associated with hierarchies, the entity is present
        // only if the associated enable is SET.
        case TPM_HT_PERMANENT:
            switch(*handle)
            {
                case TPM_RH_OWNER:
                    if(!gc.shEnable)
                        result = TPM_RC_HIERARCHY;
                    break;
#ifdef    VENDOR_PERMANENT
                 case VENDOR_PERMANENT:
#endif
                   case TPM_RH_ENDORSEMENT:
                       if(!gc.ehEnable)
                            result = TPM_RC_HIERARCHY;
                       break;
                   case TPM_RH_PLATFORM:
                       if(!g_phEnable)
                            result = TPM_RC_HIERARCHY;
                       break;
                       // null handle, PW session handle and lockout
                       // handle are always available
                   case TPM_RH_NULL:
                   case TPM_RS_PW:
                   case TPM_RH_LOCKOUT:
                       break;
                   default:
                       // handling of the manufacture_specific handles
                       if(      ((TPM_RH)*handle >= TPM_RH_FIRST)
                            && ((TPM_RH)*handle <= TPM_RH_LAST))
                            // use the value that would have been returned from
                            // unmarshaling if it did the handle filtering
                                result = TPM_RC_VALUE;
                       else
                            pAssert(FALSE);
                       break;
            }
            break;
        case TPM_HT_TRANSIENT:
            // For a transient object, check if the handle is associated
            // with a loaded object.
            if(!ObjectIsPresent(*handle))
                 result = TPM_RC_REFERENCE_H0;
            break;
        case TPM_HT_PERSISTENT:
            // Persistent object
            // Copy the persistent object to RAM and replace the handle with the
            // handle of the assigned slot. A TPM_RC_OBJECT_MEMORY,
            // TPM_RC_HIERARCHY or TPM_RC_REFERENCE_H0 error may be returned by
            // ObjectLoadEvict()
            result = ObjectLoadEvict(handle, commandCode);
            break;
        case TPM_HT_HMAC_SESSION:
            // For an HMAC session, see if the session is loaded
            // and if the session in the session slot is actually
            // an HMAC session.
            if(SessionIsLoaded(*handle))
            {
                 SESSION             *session;
                 session = SessionGet(*handle);
                 // Check if the session is a HMAC session
                 if(session->attributes.isPolicy == SET)
                     result = TPM_RC_HANDLE;
            }
            else
                 result = TPM_RC_REFERENCE_H0;
            break;
        case TPM_HT_POLICY_SESSION:
            // For a policy session, see if the session is loaded
            // and if the session in the session slot is actually
            // a policy session.
            if(SessionIsLoaded(*handle))
            {
                 SESSION             *session;
                 session = SessionGet(*handle);
                 // Check if the session is a policy session
                 if(session->attributes.isPolicy == CLEAR)
                     result = TPM_RC_HANDLE;
            }
            else
                 result = TPM_RC_REFERENCE_H0;
            break;
        case TPM_HT_NV_INDEX:
            // For an NV Index, use the platform-specific routine
            // to search the IN Index space.
            result = NvIndexIsAccessible(*handle, commandCode);
            break;
        case TPM_HT_PCR:
            // Any PCR handle that is unmarshaled successfully referenced
            // a PCR that is defined.
            break;
        default:
            // Any other handle type is a defect in the unmarshaling code.
            pAssert(FALSE);
            break;
   }
   return result;
}
//
//
//
//           EntityGetAuthValue()
//
//      This function is used to access the authValue associated with a handle. This function assumes that the
//      handle references an entity that is accessible and the handle is not for a persistent objects. That is
//      EntityGetLoadStatus() should have been called. Also, the accessibility of the authValue should have been
//      verified by IsAuthValueAvailable().
//      This function copies the authorization value of the entity to auth.
//      Return value is the number of octets copied to auth.
//
UINT16
EntityGetAuthValue(
    TPMI_DH_ENTITY       handle,             // IN: handle of entity
    AUTH_VALUE          *auth                // OUT: authValue of the entity
    )
{
    TPM2B_AUTH           authValue = {};
   switch(HandleGetType(handle))
   {
       case TPM_HT_PERMANENT:
           switch(handle)
           {
               case TPM_RH_OWNER:
                   // ownerAuth for TPM_RH_OWNER
                   authValue = gp.ownerAuth;
                   break;
               case TPM_RH_ENDORSEMENT:
                   // endorsementAuth for TPM_RH_ENDORSEMENT
                   authValue = gp.endorsementAuth;
                   break;
               case TPM_RH_PLATFORM:
                   // platformAuth for TPM_RH_PLATFORM
                   authValue = gc.platformAuth;
                   break;
               case TPM_RH_LOCKOUT:
                   // lockoutAuth for TPM_RH_LOCKOUT
                   authValue = gp.lockoutAuth;
                   break;
               case TPM_RH_NULL:
                   // nullAuth for TPM_RH_NULL. Return 0 directly here
                   return 0;
                   break;
#ifdef VENDOR_PERMANENT
               case VENDOR_PERMANENT:
                   // vendor auth value
                   authValue = g_platformUniqueDetails;
#endif
               default:
                   // If any other permanent handle is present it is
                   // a code defect.
                   pAssert(FALSE);
                   break;
           }
           break;
       case TPM_HT_TRANSIENT:
           // authValue for an object
           // A persistent object would have been copied into RAM
           // and would have an transient object handle here.
           {
               OBJECT          *object;
               object = ObjectGet(handle);
               // special handling if this is a sequence object
               if(ObjectIsSequence(object))
                   {
                       authValue = ((HASH_OBJECT *)object)->auth;
                   }
                   else
                   {
                       // Auth value is available only when the private portion of
                       // the object is loaded. The check should be made before
                       // this function is called
                       pAssert(object->attributes.publicOnly == CLEAR);
                       authValue = object->sensitive.authValue;
                   }
             }
             break;
         case TPM_HT_NV_INDEX:
             // authValue for an NV index
             {
                  NV_INDEX        nvIndex;
                  NvGetIndexInfo(handle, &nvIndex);
                  authValue = nvIndex.authValue;
             }
             break;
         case TPM_HT_PCR:
             // authValue for PCR
             PCRGetAuthValue(handle, &authValue);
             break;
         default:
             // If any other handle type is present here, then there is a defect
             // in the unmarshaling code.
             pAssert(FALSE);
             break;
    }
    // Copy the authValue
    pAssert(authValue.t.size <= sizeof(authValue.t.buffer));
    MemoryCopy(auth, authValue.t.buffer, authValue.t.size, sizeof(TPMU_HA));
    return authValue.t.size;
}
//
//
//           EntityGetAuthPolicy()
//
//      This function is used to access the authPolicy associated with a handle. This function assumes that the
//      handle references an entity that is accessible and the handle is not for a persistent objects. That is
//      EntityGetLoadStatus() should have been called. Also, the accessibility of the authPolicy should have
//      been verified by IsAuthPolicyAvailable().
//      This function copies the authorization policy of the entity to authPolicy.
//      The return value is the hash algorithm for the policy.
//
TPMI_ALG_HASH
EntityGetAuthPolicy(
    TPMI_DH_ENTITY       handle,             // IN: handle of entity
    TPM2B_DIGEST        *authPolicy          // OUT: authPolicy of the entity
    )
{
    TPMI_ALG_HASH            hashAlg = TPM_ALG_NULL;
    switch(HandleGetType(handle))
    {
        case TPM_HT_PERMANENT:
            switch(handle)
            {
                case TPM_RH_OWNER:
//
                      // ownerPolicy for TPM_RH_OWNER
                      *authPolicy = gp.ownerPolicy;
                      hashAlg = gp.ownerAlg;
                      break;
                  case TPM_RH_ENDORSEMENT:
                      // endorsementPolicy for TPM_RH_ENDORSEMENT
                      *authPolicy = gp.endorsementPolicy;
                      hashAlg = gp.endorsementAlg;
                      break;
                  case TPM_RH_PLATFORM:
                      // platformPolicy for TPM_RH_PLATFORM
                      *authPolicy = gc.platformPolicy;
                      hashAlg = gc.platformAlg;
                      break;
                  case TPM_RH_LOCKOUT:
                      // lockoutPolicy for TPM_RH_LOCKOUT
                      *authPolicy = gp.lockoutPolicy;
                      hashAlg = gp.lockoutAlg;
                      break;
                  default:
                      // If any other permanent handle is present it is
                      // a code defect.
                      pAssert(FALSE);
                      break;
             }
             break;
         case TPM_HT_TRANSIENT:
             // authPolicy for an object
             {
                  OBJECT *object = ObjectGet(handle);
                  *authPolicy = object->publicArea.authPolicy;
                  hashAlg = object->publicArea.nameAlg;
             }
             break;
         case TPM_HT_NV_INDEX:
             // authPolicy for a NV index
             {
                  NV_INDEX        nvIndex;
                  NvGetIndexInfo(handle, &nvIndex);
                  *authPolicy = nvIndex.publicArea.authPolicy;
                  hashAlg = nvIndex.publicArea.nameAlg;
             }
             break;
         case TPM_HT_PCR:
             // authPolicy for a PCR
             hashAlg = PCRGetAuthPolicy(handle, authPolicy);
             break;
         default:
             // If any other handle type is present it is a code defect.
             pAssert(FALSE);
             break;
   }
   return hashAlg;
}
//
//
//           EntityGetName()
//
//      This function returns the Name associated with a handle. It will set name to the Name and return the size
//      of the Name string.
//
UINT16
EntityGetName(
   TPMI_DH_ENTITY       handle,           // IN: handle of entity
   NAME                *name              // OUT: name of entity
    )
{
    UINT16              nameSize;
    INT32 bufferSize = sizeof(TPM_HANDLE);
    switch(HandleGetType(handle))
    {
        case TPM_HT_TRANSIENT:
            // Name for an object
            nameSize = ObjectGetName(handle, name);
            break;
        case TPM_HT_NV_INDEX:
            // Name for a NV index
            nameSize = NvGetName(handle, name);
            break;
        default:
            // For all other types, the handle is the Name
            nameSize = TPM_HANDLE_Marshal(&handle, (BYTE **)&name, &bufferSize);
            break;
    }
    return nameSize;
}
//
//
//           EntityGetHierarchy()
//
//      This function returns the hierarchy handle associated with an entity.
//      a) A handle that is a hierarchy handle is associated with itself.
//      b) An NV index belongs to TPM_RH_PLATFORM if TPMA_NV_PLATFORMCREATE, is SET,
//         otherwise it belongs to TPM_RH_OWNER
//      c) An object handle belongs to its hierarchy. All other handles belong to the platform hierarchy. or an NV
//         Index.
//
TPMI_RH_HIERARCHY
EntityGetHierarchy(
    TPMI_DH_ENTITY       handle             // IN :handle of entity
    )
{
    TPMI_RH_HIERARCHY             hierarcy = TPM_RH_NULL;
    switch(HandleGetType(handle))
    {
        case TPM_HT_PERMANENT:
            // hierarchy for a permanent handle
            switch(handle)
            {
                case TPM_RH_PLATFORM:
                case TPM_RH_ENDORSEMENT:
                case TPM_RH_NULL:
                    hierarcy = handle;
                    break;
                // all other permanent handles are associated with the owner
                // hierarchy. (should only be TPM_RH_OWNER and TPM_RH_LOCKOUT)
                default:
                    hierarcy = TPM_RH_OWNER;
                    break;
            }
            break;
        case TPM_HT_NV_INDEX:
            // hierarchy for NV index
            {
                NV_INDEX        nvIndex;
                NvGetIndexInfo(handle, &nvIndex);
                // If only the platform can delete the index, then it is
                  // considered to be in the platform hierarchy, otherwise it
                  // is in the owner hierarchy.
                  if(nvIndex.publicArea.attributes.TPMA_NV_PLATFORMCREATE == SET)
                      hierarcy = TPM_RH_PLATFORM;
                  else
                      hierarcy = TPM_RH_OWNER;
             }
             break;
         case TPM_HT_TRANSIENT:
             // hierarchy for an object
             {
                 OBJECT          *object;
                 object = ObjectGet(handle);
                 if(object->attributes.ppsHierarchy)
                 {
                     hierarcy = TPM_RH_PLATFORM;
                 }
                 else if(object->attributes.epsHierarchy)
                 {
                     hierarcy = TPM_RH_ENDORSEMENT;
                 }
                 else if(object->attributes.spsHierarchy)
                 {
                     hierarcy = TPM_RH_OWNER;
                 }
             }
             break;
         case TPM_HT_PCR:
             hierarcy = TPM_RH_OWNER;
             break;
         default:
             pAssert(0);
             break;
     }
     // this is unreachable but it provides a return value for the default
     // case which makes the complier happy
     return hierarcy;
}
