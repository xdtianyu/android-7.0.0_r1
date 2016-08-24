// This file was extracted from the TCG Published
// Trusted Platform Module Library
// Part 4: Supporting Routines
// Family "2.0"
// Level 00 Revision 01.16
// October 30, 2014

#define OBJECT_C
#include "InternalRoutines.h"
#include "Platform.h"
//
//
//            Functions
//
//             ObjectStartup()
//
//       This function is called at TPM2_Startup() to initialize the object subsystem.
//
void
ObjectStartup(
     void
     )
{
     UINT32        i;
     // object slots initialization
     for(i = 0; i < MAX_LOADED_OBJECTS; i++)
     {
         //Set the slot to not occupied
         s_objects[i].occupied = FALSE;
     }
     return;
}
//
//
//             ObjectCleanupEvict()
//
//       In this implementation, a persistent object is moved from NV into an object slot for processing. It is
//       flushed after command execution. This function is called from ExecuteCommand().
//
void
ObjectCleanupEvict(
     void
     )
{
     UINT32        i;
     // This has to be iterated because a command may have two handles
     // and they may both be persistent.
     // This could be made to be more efficient so that a search is not needed.
     for(i = 0; i < MAX_LOADED_OBJECTS; i++)
     {
         // If an object is a temporary evict object, flush it from slot
         if(s_objects[i].object.entity.attributes.evict == SET)
             s_objects[i].occupied = FALSE;
     }
   return;
}
//
//
//          ObjectIsPresent()
//
//     This function checks to see if a transient handle references a loaded object. This routine should not be
//     called if the handle is not a transient handle. The function validates that the handle is in the
//     implementation-dependent allowed in range for loaded transient objects.
//
//     Return Value                      Meaning
//
//     TRUE                              if the handle references a loaded object
//     FALSE                             if the handle is not an object handle, or it does not reference to a
//                                       loaded object
//
BOOL
ObjectIsPresent(
   TPMI_DH_OBJECT        handle              // IN: handle to be checked
   )
{
   UINT32              slotIndex;                  // index of object slot
   pAssert(HandleGetType(handle) == TPM_HT_TRANSIENT);
   // The index in the loaded object array is found by subtracting the first
   // object handle number from the input handle number. If the indicated
   // slot is occupied, then indicate that there is already is a loaded
   // object associated with the handle.
   slotIndex = handle - TRANSIENT_FIRST;
   if(slotIndex >= MAX_LOADED_OBJECTS)
       return FALSE;
   return s_objects[slotIndex].occupied;
}
//
//
//          ObjectIsSequence()
//
//     This function is used to check if the object is a sequence object. This function should not be called if the
//     handle does not reference a loaded object.
//
//     Return Value                      Meaning
//
//     TRUE                              object is an HMAC, hash, or event sequence object
//     FALSE                             object is not an HMAC, hash, or event sequence object
//
BOOL
ObjectIsSequence(
   OBJECT              *object               // IN: handle to be checked
   )
{
   pAssert (object != NULL);
   if(   object->attributes.hmacSeq == SET
      || object->attributes.hashSeq == SET
      || object->attributes.eventSeq == SET)
       return TRUE;
   else
       return FALSE;
}
//
//
//           ObjectGet()
//
//      This function is used to find the object structure associated with a handle.
//      This function requires that handle references a loaded object.
//
OBJECT*
ObjectGet(
    TPMI_DH_OBJECT       handle             // IN: handle of the object
    )
{
    pAssert(   handle >= TRANSIENT_FIRST
            && handle - TRANSIENT_FIRST < MAX_LOADED_OBJECTS);
    pAssert(s_objects[handle - TRANSIENT_FIRST].occupied == TRUE);
    // In this implementation, the handle is determined by the slot occupied by the
    // object.
    return &s_objects[handle - TRANSIENT_FIRST].object.entity;
}
//
//
//           ObjectGetName()
//
//      This function is used to access the Name of the object. In this implementation, the Name is computed
//      when the object is loaded and is saved in the internal representation of the object. This function copies
//      the Name data from the object into the buffer at name and returns the number of octets copied.
//      This function requires that handle references a loaded object.
//
UINT16
ObjectGetName(
    TPMI_DH_OBJECT       handle,            // IN: handle of the object
    NAME                *name               // OUT: name of the object
    )
{
    OBJECT      *object = ObjectGet(handle);
    if(object->publicArea.nameAlg == TPM_ALG_NULL)
        return 0;
    // Copy the Name data to the output
    MemoryCopy(name, object->name.t.name, object->name.t.size, sizeof(NAME));
    return object->name.t.size;
}
//
//
//           ObjectGetNameAlg()
//
//      This function is used to get the Name algorithm of a object.
//      This function requires that handle references a loaded object.
//
TPMI_ALG_HASH
ObjectGetNameAlg(
    TPMI_DH_OBJECT       handle             // IN: handle of the object
    )
{
    OBJECT                   *object = ObjectGet(handle);
    return object->publicArea.nameAlg;
}
//
//
//
//           ObjectGetQualifiedName()
//
//      This function returns the Qualified Name of the object. In this implementation, the Qualified Name is
//      computed when the object is loaded and is saved in the internal representation of the object. The
//      alternative would be to retain the Name of the parent and compute the QN when needed. This would take
//      the same amount of space so it is not recommended that the alternate be used.
//      This function requires that handle references a loaded object.
//
void
ObjectGetQualifiedName(
    TPMI_DH_OBJECT       handle,            // IN: handle of the object
    TPM2B_NAME          *qualifiedName      // OUT: qualified name of the object
    )
{
    OBJECT      *object = ObjectGet(handle);
    if(object->publicArea.nameAlg == TPM_ALG_NULL)
        qualifiedName->t.size = 0;
    else
        // Copy the name
        *qualifiedName = object->qualifiedName;
    return;
}
//
//
//           ObjectDataGetHierarchy()
//
//      This function returns the handle for the hierarchy of an object.
//
TPMI_RH_HIERARCHY
ObjectDataGetHierarchy(
    OBJECT              *object             // IN :object
    )
{
    if(object->attributes.spsHierarchy)
    {
        return TPM_RH_OWNER;
    }
    else if(object->attributes.epsHierarchy)
    {
        return TPM_RH_ENDORSEMENT;
    }
    else if(object->attributes.ppsHierarchy)
    {
        return TPM_RH_PLATFORM;
    }
    else
    {
        return TPM_RH_NULL;
    }
}
//
//
//          ObjectGetHierarchy()
//
//      This function returns the handle of the hierarchy to which a handle belongs. This function is similar to
//      ObjectDataGetHierarchy() but this routine takes a handle but ObjectDataGetHierarchy() takes an pointer
//      to an object.
//      This function requires that handle references a loaded object.
//
TPMI_RH_HIERARCHY
ObjectGetHierarchy(
    TPMI_DH_OBJECT        handle              // IN :object handle
    )
{
    OBJECT               *object = ObjectGet(handle);
    return ObjectDataGetHierarchy(object);
}
//
//
//           ObjectAllocateSlot()
//
//      This function is used to allocate a slot in internal object array.
//
//      Return Value                       Meaning
//
//      TRUE                               allocate success
//      FALSE                              do not have free slot
//
static BOOL
ObjectAllocateSlot(
    TPMI_DH_OBJECT       *handle,             // OUT: handle of allocated object
    OBJECT               **object             // OUT: points to the allocated object
    )
{
    UINT32          i;
    // find an unoccupied handle slot
    for(i = 0; i < MAX_LOADED_OBJECTS; i++)
    {
        if(!s_objects[i].occupied)          // If found a free slot
        {
            // Mark the slot as occupied
            s_objects[i].occupied = TRUE;
            break;
        }
    }
    // If we reach the end of object slot without finding a free one, return
    // error.
    if(i == MAX_LOADED_OBJECTS) return FALSE;
    *handle = i + TRANSIENT_FIRST;
    *object = &s_objects[i].object.entity;
    // Initialize the object attributes
    MemorySet(&((*object)->attributes), 0, sizeof(OBJECT_ATTRIBUTES));
    return TRUE;
}
//
//
//           ObjectLoad()
//
//      This function loads an object into an internal object structure. If an error is returned, the internal state is
//      unchanged.
//
//
//
//
//      Error Returns                     Meaning
//
//      TPM_RC_BINDING                    if the public and sensitive parts of the object are not matched
//      TPM_RC_KEY                        if the parameters in the public area of the object are not consistent
//      TPM_RC_OBJECT_MEMORY              if there is no free slot for an object
//      TPM_RC_TYPE                       the public and private parts are not the same type
//
TPM_RC
ObjectLoad(
   TPMI_RH_HIERARCHY        hierarchy,               //   IN: hierarchy to which the object belongs
   TPMT_PUBLIC             *publicArea,              //   IN: public area
   TPMT_SENSITIVE          *sensitive,               //   IN: sensitive area (may be null)
   TPM2B_NAME              *name,                    //   IN: object's name (may be null)
   TPM_HANDLE               parentHandle,            //   IN: handle of parent
   BOOL                     skipChecks,              //   IN: flag to indicate if it is OK to skip
                                                     //       consistency checks.
   TPMI_DH_OBJECT          *handle                   //   OUT: object handle
   )
{
   OBJECT                   *object = NULL;
   OBJECT                   *parent = NULL;
   TPM_RC                    result = TPM_RC_SUCCESS;
   TPM2B_NAME                parentQN;         // Parent qualified name
   // Try to allocate a slot for new object
   if(!ObjectAllocateSlot(handle, &object))
       return TPM_RC_OBJECT_MEMORY;
   // Initialize public
   object->publicArea = *publicArea;
   if(sensitive != NULL)
       object->sensitive = *sensitive;
   // Are the consistency checks needed
   if(!skipChecks)
   {
       // Check if key size matches
       if(!CryptObjectIsPublicConsistent(&object->publicArea))
       {
           result = TPM_RC_KEY;
           goto ErrorExit;
       }
       if(sensitive != NULL)
       {
           // Check if public type matches sensitive type
           result = CryptObjectPublicPrivateMatch(object);
           if(result != TPM_RC_SUCCESS)
               goto ErrorExit;
       }
   }
   object->attributes.publicOnly = (sensitive == NULL);
   // If 'name' is NULL, then there is nothing left to do for this
   // object as it has no qualified name and it is not a member of any
   // hierarchy and it is temporary
   if(name == NULL || name->t.size == 0)
   {
       object->qualifiedName.t.size = 0;
       object->name.t.size = 0;
       object->attributes.temporary = SET;
       return TPM_RC_SUCCESS;
   }
   // If parent handle is a permanent handle, it is a primary or temporary
   // object
   if(HandleGetType(parentHandle) == TPM_HT_PERMANENT)
   {
       // initialize QN
       parentQN.t.size = 4;
        // for a primary key, parent qualified name is the handle of hierarchy
        UINT32_TO_BYTE_ARRAY(parentHandle, parentQN.t.name);
   }
   else
   {
       // Get hierarchy and qualified name of parent
       ObjectGetQualifiedName(parentHandle, &parentQN);
        // Check for stClear object
        parent = ObjectGet(parentHandle);
        if(    publicArea->objectAttributes.stClear == SET
            || parent->attributes.stClear == SET)
             object->attributes.stClear = SET;
   }
   object->name = *name;
   // Compute object qualified name
   ObjectComputeQualifiedName(&parentQN, publicArea->nameAlg,
                              name, &object->qualifiedName);
   // Any object in TPM_RH_NULL hierarchy is temporary
   if(hierarchy == TPM_RH_NULL)
   {
       object->attributes.temporary = SET;
   }
   else if(parentQN.t.size == sizeof(TPM_HANDLE))
   {
       // Otherwise, if the size of parent's qualified name is the size of a
       // handle, this object is a primary object
       object->attributes.primary = SET;
   }
   switch(hierarchy)
   {
       case TPM_RH_PLATFORM:
           object->attributes.ppsHierarchy = SET;
           break;
       case TPM_RH_OWNER:
           object->attributes.spsHierarchy = SET;
           break;
       case TPM_RH_ENDORSEMENT:
           object->attributes.epsHierarchy = SET;
           break;
       case TPM_RH_NULL:
           break;
       default:
           pAssert(FALSE);
           break;
   }
   return TPM_RC_SUCCESS;
ErrorExit:
   ObjectFlush(*handle);
   return result;
}
//
//
//
//          AllocateSequenceSlot()
//
//      This function allocates a sequence slot and initializes the parts that are used by the normal objects so
//      that a sequence object is not inadvertently used for an operation that is not appropriate for a sequence.
//
static BOOL
AllocateSequenceSlot(
   TPM_HANDLE          *newHandle,             // OUT: receives the allocated handle
   HASH_OBJECT         **object,               // OUT: receives pointer to allocated object
   TPM2B_AUTH          *auth                   // IN: the authValue for the slot
   )
{
   OBJECT                   *objectHash;                   // the hash as an object
   if(!ObjectAllocateSlot(newHandle, &objectHash))
       return FALSE;
   *object = (HASH_OBJECT *)objectHash;
   // Validate that the proper location of the hash state data relative to the
   // object state data.
   pAssert(&((*object)->auth) == &objectHash->publicArea.authPolicy);
   // Set the common values that a sequence object shares with an ordinary object
   // The type is TPM_ALG_NULL
   (*object)->type = TPM_ALG_NULL;
   // This has no name algorithm and the name is the Empty Buffer
   (*object)->nameAlg = TPM_ALG_NULL;
   // Clear the attributes
   MemorySet(&((*object)->objectAttributes), 0, sizeof(TPMA_OBJECT));
   // A sequence object is considered to be in the NULL hierarchy so it should
   // be marked as temporary so that it can't be persisted
   (*object)->attributes.temporary = SET;
   // A sequence object is DA exempt.
   (*object)->objectAttributes.noDA = SET;
   if(auth != NULL)
   {
       MemoryRemoveTrailingZeros(auth);
       (*object)->auth = *auth;
   }
   else
       (*object)->auth.t.size = 0;
   return TRUE;
}
//
//
//          ObjectCreateHMACSequence()
//
//      This function creates an internal HMAC sequence object.
//
//      Error Returns                     Meaning
//
//      TPM_RC_OBJECT_MEMORY              if there is no free slot for an object
//
TPM_RC
ObjectCreateHMACSequence(
   TPMI_ALG_HASH        hashAlg,               // IN: hash algorithm
   TPM_HANDLE           handle,                // IN: the handle associated with sequence
                                               //     object
   TPM2B_AUTH         *auth,                 // IN: authValue
   TPMI_DH_OBJECT     *newHandle             // OUT: HMAC sequence object handle
   )
{
   HASH_OBJECT               *hmacObject;
   OBJECT                    *keyObject;
   // Try to allocate a slot for new object
   if(!AllocateSequenceSlot(newHandle, &hmacObject, auth))
       return TPM_RC_OBJECT_MEMORY;
   // Set HMAC sequence bit
   hmacObject->attributes.hmacSeq = SET;
   // Get pointer to the HMAC key object
   keyObject = ObjectGet(handle);
   CryptStartHMACSequence2B(hashAlg, &keyObject->sensitive.sensitive.bits.b,
                            &hmacObject->state.hmacState);
   return TPM_RC_SUCCESS;
}
//
//
//         ObjectCreateHashSequence()
//
//      This function creates a hash sequence object.
//
//      Error Returns                   Meaning
//
//      TPM_RC_OBJECT_MEMORY            if there is no free slot for an object
//
TPM_RC
ObjectCreateHashSequence(
   TPMI_ALG_HASH       hashAlg,              // IN: hash algorithm
   TPM2B_AUTH         *auth,                 // IN: authValue
   TPMI_DH_OBJECT     *newHandle             // OUT: sequence object handle
   )
{
   HASH_OBJECT               *hashObject;
   // Try to allocate a slot for new object
   if(!AllocateSequenceSlot(newHandle, &hashObject, auth))
       return TPM_RC_OBJECT_MEMORY;
   // Set hash sequence bit
   hashObject->attributes.hashSeq = SET;
   // Start hash for hash sequence
   CryptStartHashSequence(hashAlg, &hashObject->state.hashState[0]);
   return TPM_RC_SUCCESS;
}
//
//
//         ObjectCreateEventSequence()
//
//      This function creates an event sequence object.
//
//      Error Returns                   Meaning
//
//      TPM_RC_OBJECT_MEMORY            if there is no free slot for an object
//
TPM_RC
ObjectCreateEventSequence(
   TPM2B_AUTH          *auth,              // IN: authValue
   TPMI_DH_OBJECT      *newHandle          // OUT: sequence object handle
   )
{
   HASH_OBJECT              *hashObject;
   UINT32                    count;
   TPM_ALG_ID                hash;
   // Try to allocate a slot for new object
   if(!AllocateSequenceSlot(newHandle, &hashObject, auth))
       return TPM_RC_OBJECT_MEMORY;
   // Set the event sequence attribute
   hashObject->attributes.eventSeq = SET;
   // Initialize hash states for each implemented PCR algorithms
   for(count = 0; (hash = CryptGetHashAlgByIndex(count)) != TPM_ALG_NULL; count++)
   {
       // If this is a _TPM_Init or _TPM_HashStart, the sequence object will
       // not leave the TPM so it doesn't need the sequence handling
       if(auth == NULL)
            CryptStartHash(hash, &hashObject->state.hashState[count]);
       else
            CryptStartHashSequence(hash, &hashObject->state.hashState[count]);
   }
   return TPM_RC_SUCCESS;
}
//
//
//          ObjectTerminateEvent()
//
//      This function is called to close out the event sequence and clean up the hash context states.
//
void
ObjectTerminateEvent(
   void
   )
{
   HASH_OBJECT         *hashObject;
   int                  count;
   BYTE                 buffer[MAX_DIGEST_SIZE];
   hashObject = (HASH_OBJECT *)ObjectGet(g_DRTMHandle);
   // Don't assume that this is a proper sequence object
   if(hashObject->attributes.eventSeq)
   {
       // If it is, close any open hash contexts. This is done in case
       // the crypto implementation has some context values that need to be
       // cleaned up (hygiene).
       //
       for(count = 0; CryptGetHashAlgByIndex(count) != TPM_ALG_NULL; count++)
       {
           CryptCompleteHash(&hashObject->state.hashState[count], 0, buffer);
       }
       // Flush sequence object
       ObjectFlush(g_DRTMHandle);
   }
   g_DRTMHandle = TPM_RH_UNASSIGNED;
}
//
//
//
//          ObjectContextLoad()
//
//      This function loads an object from a saved object context.
//
//      Error Returns                     Meaning
//
//      TPM_RC_OBJECT_MEMORY              if there is no free slot for an object
//
TPM_RC
ObjectContextLoad(
    OBJECT              *object,               // IN: object structure from saved context
    TPMI_DH_OBJECT      *handle                // OUT: object handle
    )
{
    OBJECT         *newObject;
    // Try to allocate a slot for new object
    if(!ObjectAllocateSlot(handle, &newObject))
        return TPM_RC_OBJECT_MEMORY;
    // Copy input object data to internal structure
    *newObject = *object;
    return TPM_RC_SUCCESS;
}
//
//
//          ObjectFlush()
//
//      This function frees an object slot.
//      This function requires that the object is loaded.
//
void
ObjectFlush(
    TPMI_DH_OBJECT        handle               // IN: handle to be freed
    )
{
    UINT32      index = handle - TRANSIENT_FIRST;
    pAssert(ObjectIsPresent(handle));
    // Mark the handle slot as unoccupied
    s_objects[index].occupied = FALSE;
    // With no attributes
    MemorySet((BYTE*)&(s_objects[index].object.entity.attributes),
               0, sizeof(OBJECT_ATTRIBUTES));
    return;
}
//
//
//          ObjectFlushHierarchy()
//
//      This function is called to flush all the loaded transient objects associated with a hierarchy when the
//      hierarchy is disabled.
//
void
ObjectFlushHierarchy(
    TPMI_RH_HIERARCHY          hierarchy             // IN: hierarchy to be flush
    )
{
    UINT16              i;
    // iterate object slots
    for(i = 0; i < MAX_LOADED_OBJECTS; i++)
    {
        if(s_objects[i].occupied)           // If found an occupied slot
        {
            switch(hierarchy)
            {
                case TPM_RH_PLATFORM:
                    if(s_objects[i].object.entity.attributes.ppsHierarchy == SET)
                         s_objects[i].occupied = FALSE;
                    break;
                case TPM_RH_OWNER:
                    if(s_objects[i].object.entity.attributes.spsHierarchy == SET)
                         s_objects[i].occupied = FALSE;
                    break;
                case TPM_RH_ENDORSEMENT:
                    if(s_objects[i].object.entity.attributes.epsHierarchy == SET)
                         s_objects[i].occupied = FALSE;
                    break;
                default:
                    pAssert(FALSE);
                    break;
            }
        }
    }
    return;
}
//
//
//           ObjectLoadEvict()
//
//      This function loads a persistent object into a transient object slot.
//      This function requires that handle is associated with a persistent object.
//
//      Error Returns                     Meaning
//
//      TPM_RC_HANDLE                     the persistent object does not exist or the associated hierarchy is
//                                        disabled.
//      TPM_RC_OBJECT_MEMORY              no object slot
//
TPM_RC
ObjectLoadEvict(
    TPM_HANDLE           *handle,             // IN:OUT: evict object handle. If success, it
                                              // will be replace by the loaded object handle
    TPM_CC                commandCode         // IN: the command being processed
    )
{
    TPM_RC               result;
    TPM_HANDLE           evictHandle = *handle;           // Save the evict handle
    OBJECT               *object;
    // If this is an index that references a persistent object created by
    // the platform, then return TPM_RH_HANDLE if the phEnable is FALSE
    if(*handle >= PLATFORM_PERSISTENT)
    {
        // belongs to platform
        if(g_phEnable == CLEAR)
            return TPM_RC_HANDLE;
    }
    // belongs to owner
    else if(gc.shEnable == CLEAR)
        return TPM_RC_HANDLE;
   // Try to allocate a slot for an object
   if(!ObjectAllocateSlot(handle, &object))
       return TPM_RC_OBJECT_MEMORY;
   // Copy persistent object to transient object slot. A TPM_RC_HANDLE
   // may be returned at this point. This will mark the slot as containing
   // a transient object so that it will be flushed at the end of the
   // command
   result = NvGetEvictObject(evictHandle, object);
   // Bail out if this failed
   if(result != TPM_RC_SUCCESS)
       return result;
   // check the object to see if it is in the endorsement hierarchy
   // if it is and this is not a TPM2_EvictControl() command, indicate
   // that the hierarchy is disabled.
   // If the associated hierarchy is disabled, make it look like the
   // handle is not defined
   if(     ObjectDataGetHierarchy(object) == TPM_RH_ENDORSEMENT
        && gc.ehEnable == CLEAR
        && commandCode != TPM_CC_EvictControl
      )
        return TPM_RC_HANDLE;
   return result;
}
//
//
//          ObjectComputeName()
//
//      This function computes the Name of an object from its public area.
//
void
ObjectComputeName(
   TPMT_PUBLIC         *publicArea,       // IN: public area of an object
   TPM2B_NAME          *name              // OUT: name of the object
   )
{
   TPM2B_PUBLIC               marshalBuffer;
   BYTE                      *buffer;               // auxiliary marshal buffer pointer
   INT32                      bufferSize;
   HASH_STATE                 hashState;            // hash state
   // if the nameAlg is NULL then there is no name.
   if(publicArea->nameAlg == TPM_ALG_NULL)
   {
       name->t.size = 0;
       return;
   }
   // Start hash stack
   name->t.size = CryptStartHash(publicArea->nameAlg, &hashState);
   // Marshal the public area into its canonical form
   buffer = marshalBuffer.b.buffer;
   bufferSize = sizeof(TPMT_PUBLIC);
   marshalBuffer.t.size = TPMT_PUBLIC_Marshal(publicArea, &buffer, &bufferSize);
   // Adding public area
   CryptUpdateDigest2B(&hashState, &marshalBuffer.b);
   // Complete hash leaving room for the name algorithm
   CryptCompleteHash(&hashState, name->t.size, &name->t.name[2]);
   // set the nameAlg
   UINT16_TO_BYTE_ARRAY(publicArea->nameAlg, name->t.name);
//
   name->t.size += 2;
   return;
}
//
//
//          ObjectComputeQualifiedName()
//
//      This function computes the qualified name of an object.
//
void
ObjectComputeQualifiedName(
   TPM2B_NAME          *parentQN,             //   IN: parent's qualified name
   TPM_ALG_ID           nameAlg,              //   IN: name hash
   TPM2B_NAME          *name,                 //   IN: name of the object
   TPM2B_NAME          *qualifiedName         //   OUT: qualified name of the object
   )
{
   HASH_STATE          hashState;         // hash state
   //         QN_A = hash_A (QN of parent || NAME_A)
   // Start hash
   qualifiedName->t.size = CryptStartHash(nameAlg, &hashState);
   // Add parent's qualified name
   CryptUpdateDigest2B(&hashState, &parentQN->b);
   // Add self name
   CryptUpdateDigest2B(&hashState, &name->b);
   // Complete hash leaving room for the name algorithm
   CryptCompleteHash(&hashState, qualifiedName->t.size,
                     &qualifiedName->t.name[2]);
   UINT16_TO_BYTE_ARRAY(nameAlg, qualifiedName->t.name);
   qualifiedName->t.size += 2;
   return;
}
//
//
//          ObjectDataIsStorage()
//
//      This function determines if a public area has the attributes associated with a storage key. A storage key is
//      an asymmetric object that has its restricted and decrypt attributes SET, and sign CLEAR.
//
//      Return Value                      Meaning
//
//      TRUE                              if the object is a storage key
//      FALSE                             if the object is not a storage key
//
BOOL
ObjectDataIsStorage(
   TPMT_PUBLIC         *publicArea            // IN: public area of the object
   )
{
   if(   CryptIsAsymAlgorithm(publicArea->type)                          //   must be asymmetric,
      && publicArea->objectAttributes.restricted == SET                  //   restricted,
      && publicArea->objectAttributes.decrypt == SET                     //   decryption key
      && publicArea->objectAttributes.sign == CLEAR                      //   can not be sign key
     )
       return TRUE;
   else
       return FALSE;
}
//
//          ObjectIsStorage()
//
//      This function determines if an object has the attributes associated with a storage key. A storage key is an
//      asymmetric object that has its restricted and decrypt attributes SET, and sign CLEAR.
//
//      Return Value                    Meaning
//
//      TRUE                            if the object is a storage key
//      FALSE                           if the object is not a storage key
//
BOOL
ObjectIsStorage(
     TPMI_DH_OBJECT     handle              // IN: object handle
     )
{
     OBJECT           *object = ObjectGet(handle);
     return ObjectDataIsStorage(&object->publicArea);
}
//
//
//          ObjectCapGetLoaded()
//
//      This function returns a a list of handles of loaded object, starting from handle. Handle must be in the
//      range of valid transient object handles, but does not have to be the handle of a loaded transient object.
//
//      Return Value                    Meaning
//
//      YES                             if there are more handles available
//      NO                              all the available handles has been returned
//
TPMI_YES_NO
ObjectCapGetLoaded(
     TPMI_DH_OBJECT     handle,             // IN: start handle
     UINT32             count,              // IN: count of returned handles
     TPML_HANDLE       *handleList          // OUT: list of handle
     )
{
     TPMI_YES_NO             more = NO;
     UINT32                  i;
     pAssert(HandleGetType(handle) == TPM_HT_TRANSIENT);
     // Initialize output handle list
     handleList->count = 0;
     // The maximum count of handles we may return is MAX_CAP_HANDLES
     if(count > MAX_CAP_HANDLES) count = MAX_CAP_HANDLES;
     // Iterate object slots to get loaded object handles
     for(i = handle - TRANSIENT_FIRST; i < MAX_LOADED_OBJECTS; i++)
     {
         if(s_objects[i].occupied == TRUE)
         {
             // A valid transient object can not be the copy of a persistent object
             pAssert(s_objects[i].object.entity.attributes.evict == CLEAR);
              if(handleList->count < count)
              {
                  // If we have not filled up the return list, add this object
                  // handle to it
                  handleList->handle[handleList->count] = i + TRANSIENT_FIRST;
                  handleList->count++;
//
                 }
                 else
                 {
                     // If the return list is full but we still have loaded object
                     // available, report this and stop iterating
                     more = YES;
                     break;
                 }
          }
     }
     return more;
}
//
//
//             ObjectCapGetTransientAvail()
//
//      This function returns an estimate of the number of additional transient objects that could be loaded into
//      the TPM.
//
UINT32
ObjectCapGetTransientAvail(
     void
     )
{
     UINT32          i;
     UINT32          num = 0;
     // Iterate object slot to get the number of unoccupied slots
     for(i = 0; i < MAX_LOADED_OBJECTS; i++)
     {
         if(s_objects[i].occupied == FALSE) num++;
     }
     return num;
}
