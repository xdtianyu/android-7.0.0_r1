// This file was extracted from the TCG Published
// Trusted Platform Module Library
// Part 4: Supporting Routines
// Family "2.0"
// Level 00 Revision 01.16
// October 30, 2014

#define NV_C
#include "InternalRoutines.h"
#include "Platform.h"
//
//      NV Index/evict object iterator value
//
typedef        UINT32              NV_ITER;              // type of a NV iterator
#define        NV_ITER_INIT        0xFFFFFFFF            // initial value to start an
                                                        // iterator
//
//
//           NV Utility Functions
//
//           NvCheckState()
//
//     Function to check the NV state by accessing the platform-specific function to get the NV state. The result
//     state is registered in s_NvIsAvailable that will be reported by NvIsAvailable().
//     This function is called at the beginning of ExecuteCommand() before any potential call to NvIsAvailable().
//
void
NvCheckState(void)
{
    int        func_return;
    func_return = _plat__IsNvAvailable();
    if(func_return == 0)
    {
        s_NvStatus = TPM_RC_SUCCESS;
    }
    else if(func_return == 1)
    {
        s_NvStatus = TPM_RC_NV_UNAVAILABLE;
    }
    else
    {
        s_NvStatus = TPM_RC_NV_RATE;
    }
    return;
}
//
//
//           NvIsAvailable()
//
//     This function returns the NV availability parameter.
//
//     Error Returns                     Meaning
//
//     TPM_RC_SUCCESS                    NV is available
//     TPM_RC_NV_RATE                    NV is unavailable because of rate limit
//     TPM_RC_NV_UNAVAILABLE             NV is inaccessible
//
TPM_RC
NvIsAvailable(
    void
    )
{
    return s_NvStatus;
}
//
//
//           NvCommit
//
//     This is a wrapper for the platform function to commit pending NV writes.
//
BOOL
NvCommit(
    void
    )
{
    BOOL    success = (_plat__NvCommit() == 0);
    return success;
}
//
//
//          NvReadMaxCount()
//
//     This function returns the max NV counter value.
//
static UINT64
NvReadMaxCount(
    void
    )
{
    UINT64      countValue;
    _plat__NvMemoryRead(s_maxCountAddr, sizeof(UINT64), &countValue);
    return countValue;
}
//
//
//          NvWriteMaxCount()
//
//     This function updates the max counter value to NV memory.
//
static void
NvWriteMaxCount(
    UINT64               maxCount
    )
{
    _plat__NvMemoryWrite(s_maxCountAddr, sizeof(UINT64), &maxCount);
    return;
}
//
//
//          NV Index and Persistent Object Access Functions
//
//          Introduction
//
//     These functions are used to access an NV Index and persistent object memory. In this implementation,
//     the memory is simulated with RAM. The data in dynamic area is organized as a linked list, starting from
//     address s_evictNvStart. The first 4 bytes of a node in this link list is the offset of next node, followed by
//     the data entry. A 0-valued offset value indicates the end of the list. If the data entry area of the last node
//     happens to reach the end of the dynamic area without space left for an additional 4 byte end marker, the
//     end address, s_evictNvEnd, should serve as the mark of list end
//
//          NvNext()
//
//     This function provides a method to traverse every data entry in NV dynamic area.
//     To begin with, parameter iter should be initialized to NV_ITER_INIT indicating the first element. Every
//     time this function is called, the value in iter would be adjusted pointing to the next element in traversal. If
//     there is no next element, iter value would be 0. This function returns the address of the 'data entry'
//     pointed by the iter. If there is no more element in the set, a 0 value is returned indicating the end of
//     traversal.
//
static UINT32
NvNext(
    NV_ITER             *iter
    )
{
   NV_ITER        currentIter;
   // If iterator is at the beginning of list
   if(*iter == NV_ITER_INIT)
   {
       // Initialize iterator
       *iter = s_evictNvStart;
   }
   // If iterator reaches the end of NV space, or iterator indicates list end
   if(*iter + sizeof(UINT32) > s_evictNvEnd || *iter == 0)
       return 0;
   // Save the current iter offset
   currentIter = *iter;
   // Adjust iter pointer pointing to next entity
   // Read pointer value
   _plat__NvMemoryRead(*iter, sizeof(UINT32), iter);
   if(*iter == 0) return 0;
   return currentIter + sizeof(UINT32);                // entity stores after the pointer
}
//
//
//           NvGetEnd()
//
//      Function to find the end of the NV dynamic data list
//
static UINT32
NvGetEnd(
   void
   )
{
   NV_ITER             iter = NV_ITER_INIT;
   UINT32              endAddr = s_evictNvStart;
   UINT32              currentAddr;
   while((currentAddr = NvNext(&iter)) != 0)
       endAddr = currentAddr;
   if(endAddr != s_evictNvStart)
   {
       // Read offset
       endAddr -= sizeof(UINT32);
       _plat__NvMemoryRead(endAddr, sizeof(UINT32), &endAddr);
   }
   return endAddr;
}
//
//
//           NvGetFreeByte
//
//      This function returns the number of free octets in NV space.
//
static UINT32
NvGetFreeByte(
   void
   )
{
   return s_evictNvEnd - NvGetEnd();
}
//
//           NvGetEvictObjectSize
//
//      This function returns the size of an evict object in NV space
//
static UINT32
NvGetEvictObjectSize(
    void
    )
{
    return sizeof(TPM_HANDLE) + sizeof(OBJECT) + sizeof(UINT32);
}
//
//
//           NvGetCounterSize
//
//      This function returns the size of a counter index in NV space.
//
static UINT32
NvGetCounterSize(
    void
    )
{
    // It takes an offset field, a handle and the sizeof(NV_INDEX) and
    // sizeof(UINT64) for counter data
    return sizeof(TPM_HANDLE) + sizeof(NV_INDEX) + sizeof(UINT64) + sizeof(UINT32);
}
//
//
//           NvTestSpace()
//
//      This function will test if there is enough space to add a new entity.
//
//      Return Value                      Meaning
//
//      TRUE                              space available
//      FALSE                             no enough space
//
static BOOL
NvTestSpace(
    UINT32               size,               // IN: size of the entity to be added
    BOOL                 isIndex             // IN: TRUE if the entity is an index
    )
{
    UINT32         remainByte = NvGetFreeByte();
    // For NV Index, need to make sure that we do not allocate and Index if this
    // would mean that the TPM cannot allocate the minimum number of evict
    // objects.
    if(isIndex)
    {
        // Get the number of persistent objects allocated
        UINT32      persistentNum = NvCapGetPersistentNumber();
         // If we have not allocated the requisite number of evict objects, then we
         // need to reserve space for them.
         // NOTE: some of this is not written as simply as it might seem because
         // the values are all unsigned and subtracting needs to be done carefully
         // so that an underflow doesn't cause problems.
         if(persistentNum < MIN_EVICT_OBJECTS)
         {
             UINT32      needed = (MIN_EVICT_OBJECTS - persistentNum)
                                 * NvGetEvictObjectSize();
             if(needed > remainByte)
                 remainByte = 0;
             else
                 remainByte -= needed;
         }
         // if the requisite number of evict objects have been allocated then
         // no need to reserve additional space
   }
   // This checks for the size of the value being added plus the index value.
   // NOTE: This does not check to see if the end marker can be placed in
   // memory because the end marker will not be written if it will not fit.
   return (size + sizeof(UINT32) <= remainByte);
}
//
//
//           NvAdd()
//
//      This function adds a new entity to NV.
//      This function requires that there is enough space to add a new entity (i.e., that NvTestSpace() has been
//      called and the available space is at least as large as the required space).
//
static void
NvAdd(
   UINT32                totalSize,       // IN: total size needed for this        entity For
                                          //     evict object, totalSize is        the same as
                                          //     bufferSize. For NV Index,         totalSize is
                                          //     bufferSize plus index data        size
   UINT32                bufferSize,      // IN: size of initial buffer
   BYTE                 *entity           // IN: initial buffer
   )
{
   UINT32               endAddr;
   UINT32               nextAddr;
   UINT32               listEnd = 0;
   // Get the end of data list
   endAddr = NvGetEnd();
   // Calculate the value of next pointer, which is the size of a pointer +
   // the entity data size
   nextAddr = endAddr + sizeof(UINT32) + totalSize;
   // Write next pointer
   _plat__NvMemoryWrite(endAddr, sizeof(UINT32), &nextAddr);
   // Write entity data
   _plat__NvMemoryWrite(endAddr + sizeof(UINT32), bufferSize, entity);
   // Write the end of list if it is not going to exceed the NV space
   if(nextAddr + sizeof(UINT32) <= s_evictNvEnd)
       _plat__NvMemoryWrite(nextAddr, sizeof(UINT32), &listEnd);
   // Set the flag so that NV changes are committed before the command completes.
   g_updateNV = TRUE;
}
//
//
//           NvDelete()
//
//      This function is used to delete an NV Index or persistent object from NV memory.
//
static void
NvDelete(
   UINT32                entityAddr       // IN: address of entity to be deleted
   )
{
   UINT32              next;
   UINT32              entrySize;
   UINT32              entryAddr = entityAddr - sizeof(UINT32);
   UINT32              listEnd = 0;
   // Get the offset of the next entry.
   _plat__NvMemoryRead(entryAddr, sizeof(UINT32), &next);
   // The size of this entry is the difference between the current entry and the
   // next entry.
   entrySize = next - entryAddr;
   //    Move each entry after the current one to fill the freed space.
   //    Stop when we have reached the end of all the indexes. There are two
   //    ways to detect the end of the list. The first is to notice that there
   //    is no room for anything else because we are at the end of NV. The other
   //    indication is that we find an end marker.
   // The loop condition checks for the end of NV.
   while(next + sizeof(UINT32) <= s_evictNvEnd)
   {
       UINT32      size, oldAddr, newAddr;
         // Now check for the end marker
         _plat__NvMemoryRead(next, sizeof(UINT32), &oldAddr);
         if(oldAddr == 0)
             break;
         size = oldAddr - next;
         // Move entry
         _plat__NvMemoryMove(next, next - entrySize, size);
         // Update forward link
         newAddr = oldAddr - entrySize;
         _plat__NvMemoryWrite(next - entrySize, sizeof(UINT32), &newAddr);
         next = oldAddr;
   }
   // Mark the end of list
   _plat__NvMemoryWrite(next - entrySize, sizeof(UINT32), &listEnd);
   // Set the flag so that NV changes are committed before the command completes.
   g_updateNV = TRUE;
}
//
//
//           RAM-based NV Index Data Access Functions
//
//           Introduction
//
//      The data layout in ram buffer is {size of(NV_handle() + data), NV_handle(), data} for each NV Index data
//      stored in RAM.
//      NV storage is updated when a NV Index is added or deleted. We do NOT updated NV storage when the
//      data is updated/
//
//           NvTestRAMSpace()
//
//      This function indicates if there is enough RAM space to add a data for a new NV Index.
//
//
//
//
//      Return Value                      Meaning
//
//      TRUE                              space available
//      FALSE                             no enough space
//
static BOOL
NvTestRAMSpace(
   UINT32                size                // IN: size of the data to be added to RAM
   )
{
   BOOL           success = (       s_ramIndexSize
                                  + size
                                  + sizeof(TPM_HANDLE) + sizeof(UINT32)
                                  <= RAM_INDEX_SPACE);
   return success;
}
//
//
//           NvGetRamIndexOffset
//
//      This function returns the offset of NV data in the RAM buffer
//      This function requires that NV Index is in RAM. That is, the index must be known to exist.
//
static UINT32
NvGetRAMIndexOffset(
   TPMI_RH_NV_INDEX           handle               // IN: NV handle
   )
{
   UINT32         currAddr = 0;
   while(currAddr < s_ramIndexSize)
   {
       TPMI_RH_NV_INDEX    currHandle;
       UINT32              currSize;
       memcpy(&currHandle, &s_ramIndex[currAddr + sizeof(UINT32)],
              sizeof(currHandle));
         // Found a match
         if(currHandle == handle)
              // data buffer follows the handle and size field
              break;
         memcpy(&currSize, &s_ramIndex[currAddr], sizeof(currSize));
         currAddr += sizeof(UINT32) + currSize;
   }
   // We assume the index data is existing in RAM space
   pAssert(currAddr < s_ramIndexSize);
   return currAddr + sizeof(TPMI_RH_NV_INDEX) + sizeof(UINT32);
}
//
//
//           NvAddRAM()
//
//      This function adds a new data area to RAM.
//      This function requires that enough free RAM space is available to add the new data.
//
static void
NvAddRAM(
   TPMI_RH_NV_INDEX           handle,              // IN: NV handle
   UINT32                     size                 // IN: size of data
   )
{
   // Add data space at the end of reserved RAM buffer
   UINT32 value = size + sizeof(TPMI_RH_NV_INDEX);
   memcpy(&s_ramIndex[s_ramIndexSize], &value,
          sizeof(s_ramIndex[s_ramIndexSize]));
   memcpy(&s_ramIndex[s_ramIndexSize + sizeof(UINT32)], &handle,
          sizeof(s_ramIndex[s_ramIndexSize + sizeof(UINT32)]));
   s_ramIndexSize += sizeof(UINT32) + sizeof(TPMI_RH_NV_INDEX) + size;
   pAssert(s_ramIndexSize <= RAM_INDEX_SPACE);
   // Update NV version of s_ramIndexSize
   _plat__NvMemoryWrite(s_ramIndexSizeAddr, sizeof(UINT32), &s_ramIndexSize);
   // Write reserved RAM space to NV to reflect the newly added NV Index
   _plat__NvMemoryWrite(s_ramIndexAddr, RAM_INDEX_SPACE, s_ramIndex);
   return;
}
//
//
//          NvDeleteRAM()
//
//      This function is used to delete a RAM-backed NV Index data area.
//      This function assumes the data of NV Index exists in RAM
//
static void
NvDeleteRAM(
   TPMI_RH_NV_INDEX          handle           // IN: NV handle
   )
{
   UINT32             nodeOffset;
   UINT32             nextNode;
   UINT32             size;
   nodeOffset = NvGetRAMIndexOffset(handle);
   // Move the pointer back to get the size field of this node
   nodeOffset -= sizeof(UINT32) + sizeof(TPMI_RH_NV_INDEX);
   // Get node size
   memcpy(&size, &s_ramIndex[nodeOffset], sizeof(size));
   // Get the offset of next node
   nextNode = nodeOffset + sizeof(UINT32) + size;
   // Move data
   MemoryMove(s_ramIndex + nodeOffset, s_ramIndex + nextNode,
              s_ramIndexSize - nextNode, s_ramIndexSize - nextNode);
   // Update RAM size
   s_ramIndexSize -= size + sizeof(UINT32);
   // Update NV version of s_ramIndexSize
   _plat__NvMemoryWrite(s_ramIndexSizeAddr, sizeof(UINT32), &s_ramIndexSize);
   // Write reserved RAM space to NV to reflect the newly delete NV Index
   _plat__NvMemoryWrite(s_ramIndexAddr, RAM_INDEX_SPACE, s_ramIndex);
   return;
}
//
//
//
//           Utility Functions
//
//           NvInitStatic()
//
//      This function initializes the static variables used in the NV subsystem.
//
static void
NvInitStatic(
    void
    )
{
    UINT16         i;
    UINT32         reservedAddr;
    s_reservedSize[NV_DISABLE_CLEAR] = sizeof(gp.disableClear);
    s_reservedSize[NV_OWNER_ALG] = sizeof(gp.ownerAlg);
    s_reservedSize[NV_ENDORSEMENT_ALG] = sizeof(gp.endorsementAlg);
    s_reservedSize[NV_LOCKOUT_ALG] = sizeof(gp.lockoutAlg);
    s_reservedSize[NV_OWNER_POLICY] = sizeof(gp.ownerPolicy);
    s_reservedSize[NV_ENDORSEMENT_POLICY] = sizeof(gp.endorsementPolicy);
    s_reservedSize[NV_LOCKOUT_POLICY] = sizeof(gp.lockoutPolicy);
    s_reservedSize[NV_OWNER_AUTH] = sizeof(gp.ownerAuth);
    s_reservedSize[NV_ENDORSEMENT_AUTH] = sizeof(gp.endorsementAuth);
    s_reservedSize[NV_LOCKOUT_AUTH] = sizeof(gp.lockoutAuth);
    s_reservedSize[NV_EP_SEED] = sizeof(gp.EPSeed);
    s_reservedSize[NV_SP_SEED] = sizeof(gp.SPSeed);
    s_reservedSize[NV_PP_SEED] = sizeof(gp.PPSeed);
    s_reservedSize[NV_PH_PROOF] = sizeof(gp.phProof);
    s_reservedSize[NV_SH_PROOF] = sizeof(gp.shProof);
    s_reservedSize[NV_EH_PROOF] = sizeof(gp.ehProof);
    s_reservedSize[NV_TOTAL_RESET_COUNT] = sizeof(gp.totalResetCount);
    s_reservedSize[NV_RESET_COUNT] = sizeof(gp.resetCount);
    s_reservedSize[NV_PCR_POLICIES] = sizeof(gp.pcrPolicies);
    s_reservedSize[NV_PCR_ALLOCATED] = sizeof(gp.pcrAllocated);
    s_reservedSize[NV_PP_LIST] = sizeof(gp.ppList);
    s_reservedSize[NV_FAILED_TRIES] = sizeof(gp.failedTries);
    s_reservedSize[NV_MAX_TRIES] = sizeof(gp.maxTries);
    s_reservedSize[NV_RECOVERY_TIME] = sizeof(gp.recoveryTime);
    s_reservedSize[NV_LOCKOUT_RECOVERY] = sizeof(gp.lockoutRecovery);
    s_reservedSize[NV_LOCKOUT_AUTH_ENABLED] = sizeof(gp.lockOutAuthEnabled);
    s_reservedSize[NV_ORDERLY] = sizeof(gp.orderlyState);
    s_reservedSize[NV_AUDIT_COMMANDS] = sizeof(gp.auditComands);
    s_reservedSize[NV_AUDIT_HASH_ALG] = sizeof(gp.auditHashAlg);
    s_reservedSize[NV_AUDIT_COUNTER] = sizeof(gp.auditCounter);
    s_reservedSize[NV_ALGORITHM_SET] = sizeof(gp.algorithmSet);
    s_reservedSize[NV_FIRMWARE_V1] = sizeof(gp.firmwareV1);
    s_reservedSize[NV_FIRMWARE_V2] = sizeof(gp.firmwareV2);
    s_reservedSize[NV_ORDERLY_DATA] = sizeof(go);
    s_reservedSize[NV_STATE_CLEAR] = sizeof(gc);
    s_reservedSize[NV_STATE_RESET] = sizeof(gr);
    // Initialize reserved data address. In this implementation, reserved data
    // is stored at the start of NV memory
    reservedAddr = 0;
    for(i = 0; i < NV_RESERVE_LAST; i++)
    {
        s_reservedAddr[i] = reservedAddr;
        reservedAddr += s_reservedSize[i];
    }
    // Initialize auxiliary variable space for index/evict implementation.
    // Auxiliary variables are stored after reserved data area
    // RAM index copy starts at the beginning
    s_ramIndexSizeAddr = reservedAddr;
    s_ramIndexAddr = s_ramIndexSizeAddr + sizeof(UINT32);
    // Maximum counter value
    s_maxCountAddr = s_ramIndexAddr + RAM_INDEX_SPACE;
    // dynamic memory start
    s_evictNvStart = s_maxCountAddr + sizeof(UINT64);
    // dynamic memory ends at the end of NV memory
    s_evictNvEnd = NV_MEMORY_SIZE;
    return;
}
//
//
//           NvInit()
//
//      This function initializes the NV system at pre-install time.
//      This function should only be called in a manufacturing environment or in a simulation.
//      The layout of NV memory space is an implementation choice.
//
void
NvInit(
    void
    )
{
    UINT32         nullPointer = 0;
    UINT64         zeroCounter = 0;
    // Initialize static variables
    NvInitStatic();
    // Initialize RAM index space as unused
    _plat__NvMemoryWrite(s_ramIndexSizeAddr, sizeof(UINT32), &nullPointer);
    // Initialize max counter value to 0
    _plat__NvMemoryWrite(s_maxCountAddr, sizeof(UINT64), &zeroCounter);
    // Initialize the next offset of the first entry in evict/index list to 0
    _plat__NvMemoryWrite(s_evictNvStart, sizeof(TPM_HANDLE), &nullPointer);
    return;
}
//
//
//           NvReadReserved()
//
//      This function is used to move reserved data from NV memory to RAM.
//
void
NvReadReserved(
    NV_RESERVE           type,               // IN: type of reserved data
    void                *buffer              // OUT: buffer receives the data.
    )
{
    // Input type should be valid
    pAssert(type >= 0 && type < NV_RESERVE_LAST);
    _plat__NvMemoryRead(s_reservedAddr[type], s_reservedSize[type], buffer);
    return;
}
//
//
//           NvWriteReserved()
//
//      This function is used to post a reserved data for writing to NV memory. Before the TPM completes the
//      operation, the value will be written.
//
void
NvWriteReserved(
   NV_RESERVE           type,              // IN: type of reserved data
   void                *buffer             // IN: data buffer
   )
{
   // Input type should be valid
   pAssert(type >= 0 && type < NV_RESERVE_LAST);
   _plat__NvMemoryWrite(s_reservedAddr[type], s_reservedSize[type], buffer);
   // Set the flag that a NV write happens
   g_updateNV = TRUE;
   return;
}
//
//
//           NvReadPersistent()
//
//      This function reads persistent data to the RAM copy of the gp structure.
//
void
NvReadPersistent(
   void
   )
{
   // Hierarchy persistent data
   NvReadReserved(NV_DISABLE_CLEAR, &gp.disableClear);
   NvReadReserved(NV_OWNER_ALG, &gp.ownerAlg);
   NvReadReserved(NV_ENDORSEMENT_ALG, &gp.endorsementAlg);
   NvReadReserved(NV_LOCKOUT_ALG, &gp.lockoutAlg);
   NvReadReserved(NV_OWNER_POLICY, &gp.ownerPolicy);
   NvReadReserved(NV_ENDORSEMENT_POLICY, &gp.endorsementPolicy);
   NvReadReserved(NV_LOCKOUT_POLICY, &gp.lockoutPolicy);
   NvReadReserved(NV_OWNER_AUTH, &gp.ownerAuth);
   NvReadReserved(NV_ENDORSEMENT_AUTH, &gp.endorsementAuth);
   NvReadReserved(NV_LOCKOUT_AUTH, &gp.lockoutAuth);
   NvReadReserved(NV_EP_SEED, &gp.EPSeed);
   NvReadReserved(NV_SP_SEED, &gp.SPSeed);
   NvReadReserved(NV_PP_SEED, &gp.PPSeed);
   NvReadReserved(NV_PH_PROOF, &gp.phProof);
   NvReadReserved(NV_SH_PROOF, &gp.shProof);
   NvReadReserved(NV_EH_PROOF, &gp.ehProof);
   // Time persistent data
   NvReadReserved(NV_TOTAL_RESET_COUNT, &gp.totalResetCount);
   NvReadReserved(NV_RESET_COUNT, &gp.resetCount);
   // PCR persistent data
   NvReadReserved(NV_PCR_POLICIES, &gp.pcrPolicies);
   NvReadReserved(NV_PCR_ALLOCATED, &gp.pcrAllocated);
   // Physical Presence persistent data
   NvReadReserved(NV_PP_LIST, &gp.ppList);
   // Dictionary attack values persistent data
   NvReadReserved(NV_FAILED_TRIES, &gp.failedTries);
   NvReadReserved(NV_MAX_TRIES, &gp.maxTries);
   NvReadReserved(NV_RECOVERY_TIME, &gp.recoveryTime);
//
    NvReadReserved(NV_LOCKOUT_RECOVERY, &gp.lockoutRecovery);
    NvReadReserved(NV_LOCKOUT_AUTH_ENABLED, &gp.lockOutAuthEnabled);
    // Orderly State persistent data
    NvReadReserved(NV_ORDERLY, &gp.orderlyState);
    // Command audit values persistent data
    NvReadReserved(NV_AUDIT_COMMANDS, &gp.auditComands);
    NvReadReserved(NV_AUDIT_HASH_ALG, &gp.auditHashAlg);
    NvReadReserved(NV_AUDIT_COUNTER, &gp.auditCounter);
    // Algorithm selection persistent data
    NvReadReserved(NV_ALGORITHM_SET, &gp.algorithmSet);
    // Firmware version persistent data
    NvReadReserved(NV_FIRMWARE_V1, &gp.firmwareV1);
    NvReadReserved(NV_FIRMWARE_V2, &gp.firmwareV2);
    return;
}
//
//
//           NvIsPlatformPersistentHandle()
//
//      This function indicates if a handle references a persistent object in the range belonging to the platform.
//
//      Return Value                      Meaning
//
//      TRUE                              handle references a platform persistent object
//      FALSE                             handle does not reference platform persistent object and may
//                                        reference an owner persistent object either
//
BOOL
NvIsPlatformPersistentHandle(
    TPM_HANDLE           handle              // IN: handle
    )
{
    return (handle >= PLATFORM_PERSISTENT && handle <= PERSISTENT_LAST);
}
//
//
//           NvIsOwnerPersistentHandle()
//
//      This function indicates if a handle references a persistent object in the range belonging to the owner.
//
//      Return Value                      Meaning
//
//      TRUE                              handle is owner persistent handle
//      FALSE                             handle is not owner persistent handle and may not be a persistent
//                                        handle at all
//
BOOL
NvIsOwnerPersistentHandle(
    TPM_HANDLE           handle              // IN: handle
    )
{
    return (handle >= PERSISTENT_FIRST && handle < PLATFORM_PERSISTENT);
}
//
//
//           NvNextIndex()
//
//      This function returns the offset in NV of the next NV Index entry. A value of 0 indicates the end of the list.
//      Family "2.0"                                   TCG Published                                          Page 131
//      Level 00 Revision 01.16               Copyright © TCG 2006-2014                            October 30, 2014
//      Trusted Platform Module Library                                                Part 4: Supporting Routines
//
static UINT32
NvNextIndex(
   NV_ITER             *iter
   )
{
   UINT32         addr;
   TPM_HANDLE     handle;
   while((addr = NvNext(iter)) != 0)
   {
       // Read handle
       _plat__NvMemoryRead(addr, sizeof(TPM_HANDLE), &handle);
       if(HandleGetType(handle) == TPM_HT_NV_INDEX)
           return addr;
   }
   pAssert(addr == 0);
   return addr;
}
//
//
//           NvNextEvict()
//
//      This function returns the offset in NV of the next evict object entry. A value of 0 indicates the end of the
//      list.
//
static UINT32
NvNextEvict(
   NV_ITER             *iter
   )
{
   UINT32         addr;
   TPM_HANDLE     handle;
   while((addr = NvNext(iter)) != 0)
   {
       // Read handle
       _plat__NvMemoryRead(addr, sizeof(TPM_HANDLE), &handle);
       if(HandleGetType(handle) == TPM_HT_PERSISTENT)
           return addr;
   }
   pAssert(addr == 0);
   return addr;
}
//
//
//          NvFindHandle()
//
//      this function returns the offset in NV memory of the entity associated with the input handle. A value of
//      zero indicates that handle does not exist reference an existing persistent object or defined NV Index.
//
static UINT32
NvFindHandle(
   TPM_HANDLE            handle
   )
{
   UINT32              addr;
   NV_ITER             iter = NV_ITER_INIT;
   while((addr = NvNext(&iter)) != 0)
   {
       TPM_HANDLE          entityHandle;
       // Read handle
//
          _plat__NvMemoryRead(addr, sizeof(TPM_HANDLE), &entityHandle);
          if(entityHandle == handle)
              return addr;
    }
    pAssert(addr == 0);
    return addr;
}
//
//
//          NvPowerOn()
//
//      This function is called at _TPM_Init() to initialize the NV environment.
//
//      Return Value                      Meaning
//
//      TRUE                              all NV was initialized
//      FALSE                             the NV     containing saved     state    had   an   error   and
//                                        TPM2_Startup(CLEAR) is required
//
BOOL
NvPowerOn(
    void
    )
{
    int          nvError = 0;
    // If power was lost, need to re-establish the RAM data that is loaded from
    // NV and initialize the static variables
    if(_plat__WasPowerLost(TRUE))
    {
        if((nvError = _plat__NVEnable(0)) < 0)
            FAIL(FATAL_ERROR_NV_UNRECOVERABLE);
          NvInitStatic();
    }
    return nvError == 0;
}
//
//
//          NvStateSave()
//
//      This function is used to cause the memory containing the RAM backed NV Indices to be written to NV.
//
void
NvStateSave(
    void
    )
{
    // Write RAM backed NV Index info to NV
    // No need to save s_ramIndexSize because we save it to NV whenever it is
    // updated.
    _plat__NvMemoryWrite(s_ramIndexAddr, RAM_INDEX_SPACE, s_ramIndex);
    // Set the flag so that an NV write happens before the command completes.
    g_updateNV = TRUE;
    return;
}
//
//
//
//           NvEntityStartup()
//
//      This function is called at TPM_Startup(). If the startup completes a TPM Resume cycle, no action is
//      taken. If the startup is a TPM Reset or a TPM Restart, then this function will:
//      a) clear read/write lock;
//      b) reset NV Index data that has TPMA_NV_CLEAR_STCLEAR SET; and
//      c) set the lower bits in orderly counters to 1 for a non-orderly startup
//      It is a prerequisite that NV be available for writing before this function is called.
//
void
NvEntityStartup(
    STARTUP_TYPE           type               // IN: start up type
    )
{
    NV_ITER                   iter = NV_ITER_INIT;
    UINT32                    currentAddr;         // offset points to the current entity
    // Restore RAM index data
    _plat__NvMemoryRead(s_ramIndexSizeAddr, sizeof(UINT32), &s_ramIndexSize);
    _plat__NvMemoryRead(s_ramIndexAddr, RAM_INDEX_SPACE, s_ramIndex);
    // If recovering from state save, do nothing
    if(type == SU_RESUME)
        return;
    // Iterate all the NV Index to clear the locks
    while((currentAddr = NvNextIndex(&iter)) != 0)
    {
        NV_INDEX    nvIndex;
        UINT32      indexAddr;              // NV address points to index info
        TPMA_NV     attributes;
        UINT32      attributesValue;
        UINT32      publicAreaAttributesValue;
          indexAddr = currentAddr + sizeof(TPM_HANDLE);
          // Read NV Index info structure
          _plat__NvMemoryRead(indexAddr, sizeof(NV_INDEX), &nvIndex);
          attributes = nvIndex.publicArea.attributes;
          // Clear read/write lock
          if(attributes.TPMA_NV_READLOCKED == SET)
              attributes.TPMA_NV_READLOCKED = CLEAR;
          if(         attributes.TPMA_NV_WRITELOCKED == SET
                 &&   (   attributes.TPMA_NV_WRITTEN == CLEAR
                      || attributes.TPMA_NV_WRITEDEFINE == CLEAR
                      )
                )
                 attributes.TPMA_NV_WRITELOCKED = CLEAR;
          // Reset NV data for TPMA_NV_CLEAR_STCLEAR
          if(attributes.TPMA_NV_CLEAR_STCLEAR == SET)
          {
              attributes.TPMA_NV_WRITTEN = CLEAR;
              attributes.TPMA_NV_WRITELOCKED = CLEAR;
          }
          // Reset NV data for orderly values that are not counters
          // NOTE: The function has already exited on a TPM Resume, so the only
          // things being processed are TPM Restart and TPM Reset
          if(     type == SU_RESET
              && attributes.TPMA_NV_ORDERLY == SET
              && attributes.TPMA_NV_COUNTER == CLEAR
             )
                 attributes.TPMA_NV_WRITTEN = CLEAR;
         // Write NV Index info back if it has changed
         memcpy(&attributesValue, &attributes, sizeof(attributesValue));
         memcpy(&publicAreaAttributesValue, &nvIndex.publicArea.attributes,
                sizeof(publicAreaAttributesValue));
         if(attributesValue != publicAreaAttributesValue)
         {
             nvIndex.publicArea.attributes = attributes;
             _plat__NvMemoryWrite(indexAddr, sizeof(NV_INDEX), &nvIndex);
                 // Set the flag that a NV write happens
                 g_updateNV = TRUE;
         }
         // Set the lower bits in an orderly counter to 1 for a non-orderly startup
         if(    g_prevOrderlyState == SHUTDOWN_NONE
             && attributes.TPMA_NV_WRITTEN == SET)
         {
              if(    attributes.TPMA_NV_ORDERLY == SET
                  && attributes.TPMA_NV_COUNTER == SET)
              {
                   TPMI_RH_NV_INDEX    nvHandle;
                   UINT64              counter;
                     // Read NV handle
                     _plat__NvMemoryRead(currentAddr, sizeof(TPM_HANDLE), &nvHandle);
                     // Read the counter value saved to NV upon the last roll over.
                     // Do not use RAM backed storage for this once.
                     nvIndex.publicArea.attributes.TPMA_NV_ORDERLY = CLEAR;
                     NvGetIntIndexData(nvHandle, &nvIndex, &counter);
                     nvIndex.publicArea.attributes.TPMA_NV_ORDERLY = SET;
                     // Set the lower bits of counter to 1's
                     counter |= MAX_ORDERLY_COUNT;
                     // Write back to RAM
                     NvWriteIndexData(nvHandle, &nvIndex, 0, sizeof(counter), &counter);
                     // No write to NV because an orderly shutdown will update the
                     // counters.
                 }
         }
   }
   return;
}
//
//
//           NV Access Functions
//
//             Introduction
//
//      This set of functions provide accessing NV Index and persistent objects based using a handle for
//      reference to the entity.
//
//             NvIsUndefinedIndex()
//
//      This function is used to verify that an NV Index is not defined. This is only used by
//      TPM2_NV_DefineSpace().
//
//
//
//
//      Return Value                      Meaning
//
//      TRUE                              the handle points to an existing NV Index
//      FALSE                             the handle points to a non-existent Index
//
BOOL
NvIsUndefinedIndex(
   TPMI_RH_NV_INDEX         handle                 // IN: handle
   )
{
   UINT32             entityAddr;                  // offset points to the entity
   pAssert(HandleGetType(handle) == TPM_HT_NV_INDEX);
   // Find the address of index
   entityAddr = NvFindHandle(handle);
   // If handle is not found, return TPM_RC_SUCCESS
   if(entityAddr == 0)
       return TPM_RC_SUCCESS;
   // NV Index is defined
   return TPM_RC_NV_DEFINED;
}
//
//
//          NvIndexIsAccessible()
//
//      This function validates that a handle references a defined NV Index and that the Index is currently
//      accessible.
//
//      Error Returns                     Meaning
//
//      TPM_RC_HANDLE                     the handle points to an undefined NV Index If shEnable is CLEAR,
//                                        this would include an index created using ownerAuth. If phEnableNV
//                                        is CLEAR, this would include and index created using platform auth
//      TPM_RC_NV_READLOCKED              Index is present but locked for reading and command does not write
//                                        to the index
//      TPM_RC_NV_WRITELOCKED             Index is present but locked for writing and command writes to the
//                                        index
//
TPM_RC
NvIndexIsAccessible(
   TPMI_RH_NV_INDEX         handle,                // IN: handle
   TPM_CC                   commandCode            // IN: the command
   )
{
   UINT32                  entityAddr;             // offset points to the entity
   NV_INDEX                nvIndex;                //
   pAssert(HandleGetType(handle) == TPM_HT_NV_INDEX);
   // Find the address of index
   entityAddr = NvFindHandle(handle);
   // If handle is not found, return TPM_RC_HANDLE
   if(entityAddr == 0)
       return TPM_RC_HANDLE;
   // Read NV Index info structure
   _plat__NvMemoryRead(entityAddr + sizeof(TPM_HANDLE), sizeof(NV_INDEX),
                           &nvIndex);
   if(gc.shEnable == FALSE || gc.phEnableNV == FALSE)
   {
       // if shEnable is CLEAR, an ownerCreate NV Index should not be
       // indicated as present
       if(nvIndex.publicArea.attributes.TPMA_NV_PLATFORMCREATE == CLEAR)
       {
           if(gc.shEnable == FALSE)
               return TPM_RC_HANDLE;
       }
       // if phEnableNV is CLEAR, a platform created Index should not
       // be visible
       else if(gc.phEnableNV == FALSE)
           return TPM_RC_HANDLE;
   }
   // If the Index is write locked and this is an NV Write operation...
   if(     nvIndex.publicArea.attributes.TPMA_NV_WRITELOCKED
       && IsWriteOperation(commandCode))
   {
       // then return a locked indication unless the command is TPM2_NV_WriteLock
       if(commandCode != TPM_CC_NV_WriteLock)
           return TPM_RC_NV_LOCKED;
       return TPM_RC_SUCCESS;
   }
   // If the Index is read locked and this is an NV Read operation...
   if(     nvIndex.publicArea.attributes.TPMA_NV_READLOCKED
       && IsReadOperation(commandCode))
   {
       // then return a locked indication unless the command is TPM2_NV_ReadLock
       if(commandCode != TPM_CC_NV_ReadLock)
           return TPM_RC_NV_LOCKED;
       return TPM_RC_SUCCESS;
   }
   // NV Index is accessible
   return TPM_RC_SUCCESS;
}
//
//
//           NvIsUndefinedEvictHandle()
//
//      This function indicates if a handle does not reference an existing persistent object. This function requires
//      that the handle be in the proper range for persistent objects.
//
//      Return Value                     Meaning
//
//      TRUE                             handle does not reference an existing persistent object
//      FALSE                            handle does reference an existing persistent object
//
static BOOL
NvIsUndefinedEvictHandle(
   TPM_HANDLE            handle             // IN: handle
   )
{
   UINT32           entityAddr;    // offset points to the entity
   pAssert(HandleGetType(handle) == TPM_HT_PERSISTENT);
   // Find the address of evict object
   entityAddr = NvFindHandle(handle);
   // If handle is not found, return TRUE
   if(entityAddr == 0)
       return TRUE;
    else
        return FALSE;
}
//
//
//           NvGetEvictObject()
//
//      This function is used to dereference an evict object handle and get a pointer to the object.
//
//      Error Returns                     Meaning
//
//      TPM_RC_HANDLE                     the handle does not point to an existing persistent object
//
TPM_RC
NvGetEvictObject(
    TPM_HANDLE           handle,              // IN: handle
    OBJECT              *object               // OUT: object data
    )
{
    UINT32              entityAddr;         // offset points to the entity
    TPM_RC              result = TPM_RC_SUCCESS;
    pAssert(HandleGetType(handle) == TPM_HT_PERSISTENT);
    // Find the address of evict object
    entityAddr = NvFindHandle(handle);
    // If handle is not found, return an error
    if(entityAddr == 0)
        result = TPM_RC_HANDLE;
    else
        // Read evict object
        _plat__NvMemoryRead(entityAddr + sizeof(TPM_HANDLE),
                             sizeof(OBJECT),
                             object);
    // whether there is an error or not, make sure that the evict
    // status of the object is set so that the slot will get freed on exit
    object->attributes.evict = SET;
    return result;
}
//
//
//           NvGetIndexInfo()
//
//      This function is used to retrieve the contents of an NV Index.
//      An implementation is allowed to save the NV Index in a vendor-defined format. If the format is different
//      from the default used by the reference code, then this function would be changed to reformat the data into
//      the default format.
//      A prerequisite to calling this function is that the handle must be known to reference a defined NV Index.
//
void
NvGetIndexInfo(
    TPMI_RH_NV_INDEX          handle,              // IN: handle
    NV_INDEX                 *nvIndex              // OUT: NV index structure
    )
{
    UINT32                    entityAddr;          // offset points to the entity
    pAssert(HandleGetType(handle) == TPM_HT_NV_INDEX);
    // Find the address of NV index
    entityAddr = NvFindHandle(handle);
    pAssert(entityAddr != 0);
    // This implementation uses the default format so just
    // read the data in
    _plat__NvMemoryRead(entityAddr + sizeof(TPM_HANDLE), sizeof(NV_INDEX),
                        nvIndex);
    return;
}
//
//
//           NvInitialCounter()
//
//      This function returns the value to be used when a counter index is initialized. It will scan the NV counters
//      and find the highest value in any active counter. It will use that value as the starting point. If there are no
//      active counters, it will use the value of the previous largest counter.
//
UINT64
NvInitialCounter(
    void
    )
{
    UINT64              maxCount;
    NV_ITER             iter = NV_ITER_INIT;
    UINT32              currentAddr;
    // Read the maxCount value
    maxCount = NvReadMaxCount();
    // Iterate all existing counters
    while((currentAddr = NvNextIndex(&iter)) != 0)
    {
        TPMI_RH_NV_INDEX    nvHandle;
        NV_INDEX            nvIndex;
         // Read NV handle
         _plat__NvMemoryRead(currentAddr, sizeof(TPM_HANDLE), &nvHandle);
         // Get NV Index
         NvGetIndexInfo(nvHandle, &nvIndex);
         if(    nvIndex.publicArea.attributes.TPMA_NV_COUNTER == SET
             && nvIndex.publicArea.attributes.TPMA_NV_WRITTEN == SET)
         {
             UINT64      countValue;
             // Read counter value
             NvGetIntIndexData(nvHandle, &nvIndex, &countValue);
             if(countValue > maxCount)
                 maxCount = countValue;
         }
    }
    // Initialize the new counter value to be maxCount + 1
    // A counter is only initialized the first time it is written. The
    // way to write a counter is with TPM2_NV_INCREMENT(). Since the
    // "initial" value of a defined counter is the largest count value that
    // may have existed in this index previously, then the first use would
    // add one to that value.
    return maxCount;
}
//
//
//           NvGetIndexData()
//
//      This function is used to access the data in an NV Index. The data is returned as a byte sequence. Since
//      counter values are kept in native format, they are converted to canonical form before being returned.
//      Family "2.0"                                  TCG Published                                         Page 139
//      Level 00 Revision 01.16               Copyright © TCG 2006-2014                            October 30, 2014
//      Trusted Platform Module Library                                                Part 4: Supporting Routines
//
//
//      This function requires that the NV Index be defined, and that the required data is within the data range. It
//      also requires that TPMA_NV_WRITTEN of the Index is SET.
//
void
NvGetIndexData(
    TPMI_RH_NV_INDEX          handle,            //   IN: handle
    NV_INDEX                 *nvIndex,           //   IN: RAM image of index header
    UINT32                    offset,            //   IN: offset of NV data
    UINT16                    size,              //   IN: size of NV data
    void                     *data               //   OUT: data buffer
    )
{
    pAssert(nvIndex->publicArea.attributes.TPMA_NV_WRITTEN == SET);
    if(   nvIndex->publicArea.attributes.TPMA_NV_BITS == SET
       || nvIndex->publicArea.attributes.TPMA_NV_COUNTER == SET)
    {
        // Read bit or counter data in canonical form
        UINT64      dataInInt;
        NvGetIntIndexData(handle, nvIndex, &dataInInt);
        UINT64_TO_BYTE_ARRAY(dataInInt, (BYTE *)data);
    }
    else
    {
        if(nvIndex->publicArea.attributes.TPMA_NV_ORDERLY == SET)
        {
            UINT32      ramAddr;
              // Get data from RAM buffer
              ramAddr = NvGetRAMIndexOffset(handle);
              MemoryCopy(data, s_ramIndex + ramAddr + offset, size, size);
         }
         else
         {
              UINT32      entityAddr;
              entityAddr = NvFindHandle(handle);
              // Get data from NV
              // Skip NV Index info, read data buffer
              entityAddr += sizeof(TPM_HANDLE) + sizeof(NV_INDEX) + offset;
              // Read the data
              _plat__NvMemoryRead(entityAddr, size, data);
        }
    }
    return;
}
//
//
//           NvGetIntIndexData()
//
//      Get data in integer format of a bit or counter NV Index.
//      This function requires that the NV Index is defined and that the NV Index previously has been written.
//
void
NvGetIntIndexData(
    TPMI_RH_NV_INDEX          handle,            // IN: handle
    NV_INDEX                 *nvIndex,           // IN: RAM image of NV Index header
    UINT64                   *data               // IN: UINT64 pointer for counter or bit
    )
{
    // Validate that index has been written and is the right type
    pAssert(   nvIndex->publicArea.attributes.TPMA_NV_WRITTEN == SET
            && (   nvIndex->publicArea.attributes.TPMA_NV_BITS == SET
                || nvIndex->publicArea.attributes.TPMA_NV_COUNTER == SET
                   )
              );
    // bit and counter value is store in native format for TPM CPU.                  So we directly
    // copy the contents of NV to output data buffer
    if(nvIndex->publicArea.attributes.TPMA_NV_ORDERLY == SET)
    {
        UINT32      ramAddr;
          // Get data from RAM buffer
          ramAddr = NvGetRAMIndexOffset(handle);
          MemoryCopy(data, s_ramIndex + ramAddr, sizeof(*data), sizeof(*data));
    }
    else
    {
        UINT32      entityAddr;
        entityAddr = NvFindHandle(handle);
          // Get data from NV
          // Skip NV Index info, read data buffer
          _plat__NvMemoryRead(
              entityAddr + sizeof(TPM_HANDLE) + sizeof(NV_INDEX),
              sizeof(UINT64), data);
    }
    return;
}
//
//
//           NvWriteIndexInfo()
//
//       This function is called to queue the write of NV Index data to persistent memory.
//       This function requires that NV Index is defined.
//
//       Error Returns                        Meaning
//
//       TPM_RC_NV_RATE                       NV is rate limiting so retry
//       TPM_RC_NV_UNAVAILABLE                NV is not available
//
TPM_RC
NvWriteIndexInfo(
    TPMI_RH_NV_INDEX            handle,                // IN: handle
    NV_INDEX                   *nvIndex                // IN: NV Index info to be written
    )
{
    UINT32             entryAddr;
    TPM_RC             result;
    // Get the starting offset for the index in the RAM image of NV
    entryAddr = NvFindHandle(handle);
    pAssert(entryAddr != 0);
    // Step over the link value
    entryAddr = entryAddr + sizeof(TPM_HANDLE);
    // If the index data is actually changed, then a write to NV is required
    if(_plat__NvIsDifferent(entryAddr, sizeof(NV_INDEX),nvIndex))
    {
        // Make sure that NV is available
        result = NvIsAvailable();
        if(result != TPM_RC_SUCCESS)
            return result;
        _plat__NvMemoryWrite(entryAddr, sizeof(NV_INDEX), nvIndex);
        g_updateNV = TRUE;
    }
    return TPM_RC_SUCCESS;
}
//
//
//            NvWriteIndexData()
//
//       This function is used to write NV index data.
//       This function requires that the NV Index is defined, and the data is within the defined data range for the
//       index.
//
//       Error Returns                     Meaning
//
//       TPM_RC_NV_RATE                    NV is rate limiting so retry
//       TPM_RC_NV_UNAVAILABLE             NV is not available
//
TPM_RC
NvWriteIndexData(
    TPMI_RH_NV_INDEX          handle,               //   IN: handle
    NV_INDEX                 *nvIndex,              //   IN: RAM copy of NV Index
    UINT32                    offset,               //   IN: offset of NV data
    UINT32                    size,                 //   IN: size of NV data
    void                     *data                  //   OUT: data buffer
    )
{
    TPM_RC               result;
    // Validate that write falls within range of the index
    pAssert(nvIndex->publicArea.dataSize >= offset + size);
    // Update TPMA_NV_WRITTEN bit if necessary
    if(nvIndex->publicArea.attributes.TPMA_NV_WRITTEN == CLEAR)
    {
        nvIndex->publicArea.attributes.TPMA_NV_WRITTEN = SET;
        result = NvWriteIndexInfo(handle, nvIndex);
        if(result != TPM_RC_SUCCESS)
            return result;
    }
    // Check to see if process for an orderly index is required.
    if(nvIndex->publicArea.attributes.TPMA_NV_ORDERLY == SET)
    {
        UINT32      ramAddr;
          // Write data to RAM buffer
          ramAddr = NvGetRAMIndexOffset(handle);
          MemoryCopy(s_ramIndex + ramAddr + offset, data, size,
                     sizeof(s_ramIndex) - ramAddr - offset);
          // NV update does not happen for orderly index. Have
          // to clear orderlyState to reflect that we have changed the
          // NV and an orderly shutdown is required. Only going to do this if we
          // are not processing a counter that has just rolled over
          if(g_updateNV == FALSE)
              g_clearOrderly = TRUE;
    }
    // Need to process this part if the Index isn't orderly or if it is
    // an orderly counter that just rolled over.
    if(g_updateNV || nvIndex->publicArea.attributes.TPMA_NV_ORDERLY == CLEAR)
    {
        // Processing for an index with TPMA_NV_ORDERLY CLEAR
        UINT32      entryAddr = NvFindHandle(handle);
          pAssert(entryAddr != 0);
//
          // Offset into the index to the first byte of the data to be written
          entryAddr += sizeof(TPM_HANDLE) + sizeof(NV_INDEX) + offset;
          // If the data is actually changed, then a write to NV is required
          if(_plat__NvIsDifferent(entryAddr, size, data))
          {
              // Make sure that NV is available
              result = NvIsAvailable();
              if(result != TPM_RC_SUCCESS)
                  return result;
              _plat__NvMemoryWrite(entryAddr, size, data);
              g_updateNV = TRUE;
          }
    }
    return TPM_RC_SUCCESS;
}
//
//
//            NvGetName()
//
//       This function is used to compute the Name of an NV Index.
//       The name buffer receives the bytes of the Name and the return value is the number of octets in the
//       Name.
//       This function requires that the NV Index is defined.
//
UINT16
NvGetName(
    TPMI_RH_NV_INDEX          handle,            // IN: handle of the index
    NAME                     *name               // OUT: name of the index
    )
{
    UINT16                    dataSize, digestSize;
    NV_INDEX                  nvIndex;
    BYTE                      marshalBuffer[sizeof(TPMS_NV_PUBLIC)];
    BYTE                     *buffer;
    INT32                     bufferSize;
    HASH_STATE                hashState;
    // Get NV public info
    NvGetIndexInfo(handle, &nvIndex);
    // Marshal public area
    buffer = marshalBuffer;
    bufferSize = sizeof(TPMS_NV_PUBLIC);
    dataSize = TPMS_NV_PUBLIC_Marshal(&nvIndex.publicArea, &buffer, &bufferSize);
    // hash public area
    digestSize = CryptStartHash(nvIndex.publicArea.nameAlg, &hashState);
    CryptUpdateDigest(&hashState, dataSize, marshalBuffer);
    // Complete digest leaving room for the nameAlg
    CryptCompleteHash(&hashState, digestSize, &((BYTE *)name)[2]);
    // Include the nameAlg
    UINT16_TO_BYTE_ARRAY(nvIndex.publicArea.nameAlg, (BYTE *)name);
    return digestSize + 2;
}
//
//
//            NvDefineIndex()
//
//       This function is used to assign NV memory to an NV Index.
//
//
//
//       Error Returns                     Meaning
//
//       TPM_RC_NV_SPACE                   insufficient NV space
//
TPM_RC
NvDefineIndex(
   TPMS_NV_PUBLIC      *publicArea,          // IN: A template for an area to create.
   TPM2B_AUTH          *authValue            // IN: The initial authorization value
   )
{
   // The buffer to be written to NV memory
   BYTE            nvBuffer[sizeof(TPM_HANDLE) + sizeof(NV_INDEX)];
   NV_INDEX            *nvIndex;                  // a pointer to the NV_INDEX data in
                                                  //   nvBuffer
   UINT16              entrySize;                 // size of entry
   entrySize = sizeof(TPM_HANDLE) + sizeof(NV_INDEX) + publicArea->dataSize;
   // Check if we have enough space to create the NV Index
   // In this implementation, the only resource limitation is the available NV
   // space. Other implementation may have other limitation on counter or on
   // NV slot
   if(!NvTestSpace(entrySize, TRUE)) return TPM_RC_NV_SPACE;
   // if the index to be defined is RAM backed, check RAM space availability
   // as well
   if(publicArea->attributes.TPMA_NV_ORDERLY == SET
           && !NvTestRAMSpace(publicArea->dataSize))
       return TPM_RC_NV_SPACE;
   // Copy input value to nvBuffer
       // Copy handle
   memcpy(nvBuffer, &publicArea->nvIndex, sizeof(TPM_HANDLE));
       // Copy NV_INDEX
   nvIndex = (NV_INDEX *) (nvBuffer + sizeof(TPM_HANDLE));
   nvIndex->publicArea = *publicArea;
   nvIndex->authValue = *authValue;
   // Add index to NV memory
   NvAdd(entrySize, sizeof(TPM_HANDLE) + sizeof(NV_INDEX), nvBuffer);
   // If the data of NV Index is RAM backed, add the data area in RAM as well
   if(publicArea->attributes.TPMA_NV_ORDERLY == SET)
       NvAddRAM(publicArea->nvIndex, publicArea->dataSize);
   return TPM_RC_SUCCESS;
}
//
//
//           NvAddEvictObject()
//
//       This function is used to assign NV memory to a persistent object.
//
//       Error Returns                     Meaning
//
//       TPM_RC_NV_HANDLE                  the requested handle is already in use
//       TPM_RC_NV_SPACE                   insufficient NV space
//
TPM_RC
NvAddEvictObject(
   TPMI_DH_OBJECT       evictHandle,         // IN: new evict handle
//
    OBJECT              *object              // IN: object to be added
    )
{
    // The buffer to be written to NV memory
    BYTE            nvBuffer[sizeof(TPM_HANDLE) + sizeof(OBJECT)];
    OBJECT              *nvObject;                // a pointer to the OBJECT data in
                                                  // nvBuffer
    UINT16              entrySize;                // size of entry
    // evict handle type should match the object hierarchy
    pAssert(   (   NvIsPlatformPersistentHandle(evictHandle)
                && object->attributes.ppsHierarchy == SET)
            || (   NvIsOwnerPersistentHandle(evictHandle)
                && (   object->attributes.spsHierarchy == SET
                    || object->attributes.epsHierarchy == SET)));
    // An evict needs 4 bytes of handle + sizeof OBJECT
    entrySize = sizeof(TPM_HANDLE) + sizeof(OBJECT);
    // Check if we have enough space to add the evict object
    // An evict object needs 8 bytes in index table + sizeof OBJECT
    // In this implementation, the only resource limitation is the available NV
    // space. Other implementation may have other limitation on evict object
    // handle space
    if(!NvTestSpace(entrySize, FALSE)) return TPM_RC_NV_SPACE;
    // Allocate a new evict handle
    if(!NvIsUndefinedEvictHandle(evictHandle))
        return TPM_RC_NV_DEFINED;
    // Copy evict object to nvBuffer
        // Copy handle
    memcpy(nvBuffer, &evictHandle, sizeof(TPM_HANDLE));
        // Copy OBJECT
    nvObject = (OBJECT *) (nvBuffer + sizeof(TPM_HANDLE));
    *nvObject = *object;
    // Set evict attribute and handle
    nvObject->attributes.evict = SET;
    nvObject->evictHandle = evictHandle;
    // Add evict to NV memory
    NvAdd(entrySize, entrySize, nvBuffer);
    return TPM_RC_SUCCESS;
}
//
//
//           NvDeleteEntity()
//
//       This function will delete a NV Index or an evict object.
//       This function requires that the index/evict object has been defined.
//
void
NvDeleteEntity(
    TPM_HANDLE           handle              // IN: handle of entity to be deleted
    )
{
    UINT32         entityAddr;         // pointer to entity
    entityAddr = NvFindHandle(handle);
    pAssert(entityAddr != 0);
    if(HandleGetType(handle) == TPM_HT_NV_INDEX)
    {
        NV_INDEX    nvIndex;
          // Read the NV Index info
          _plat__NvMemoryRead(entityAddr + sizeof(TPM_HANDLE), sizeof(NV_INDEX),
                              &nvIndex);
          // If the entity to be deleted is a counter with the maximum counter
          // value, record it in NV memory
          if(nvIndex.publicArea.attributes.TPMA_NV_COUNTER == SET
                  && nvIndex.publicArea.attributes.TPMA_NV_WRITTEN == SET)
          {
              UINT64      countValue;
              UINT64      maxCount;
              NvGetIntIndexData(handle, &nvIndex, &countValue);
              maxCount = NvReadMaxCount();
              if(countValue > maxCount)
                  NvWriteMaxCount(countValue);
          }
          // If the NV Index is RAM back, delete the RAM data as well
          if(nvIndex.publicArea.attributes.TPMA_NV_ORDERLY == SET)
              NvDeleteRAM(handle);
    }
    NvDelete(entityAddr);
    return;
}
//
//
//            NvFlushHierarchy()
//
//       This function will delete persistent objects belonging to the indicated If the storage hierarchy is selected,
//       the function will also delete any NV Index define using ownerAuth.
//
void
NvFlushHierarchy(
    TPMI_RH_HIERARCHY         hierarchy          // IN: hierarchy to be flushed.
    )
{
    NV_ITER             iter = NV_ITER_INIT;
    UINT32              currentAddr;
    while((currentAddr = NvNext(&iter)) != 0)
    {
        TPM_HANDLE      entityHandle;
          // Read handle information.
          _plat__NvMemoryRead(currentAddr, sizeof(TPM_HANDLE), &entityHandle);
          if(HandleGetType(entityHandle) == TPM_HT_NV_INDEX)
          {
              // Handle NV Index
              NV_INDEX    nvIndex;
              // If flush endorsement or platform hierarchy, no NV Index would be
              // flushed
              if(hierarchy == TPM_RH_ENDORSEMENT || hierarchy == TPM_RH_PLATFORM)
                  continue;
              _plat__NvMemoryRead(currentAddr + sizeof(TPM_HANDLE),
                                  sizeof(NV_INDEX), &nvIndex);
              // For storage hierarchy, flush OwnerCreated index
               if(    nvIndex.publicArea.attributes.TPMA_NV_PLATFORMCREATE == CLEAR)
               {
                     // Delete the NV Index
                     NvDelete(currentAddr);
                     // Re-iterate from beginning after a delete
                     iter = NV_ITER_INIT;
                     // If the NV Index is RAM back, delete the RAM data as well
                     if(nvIndex.publicArea.attributes.TPMA_NV_ORDERLY == SET)
                         NvDeleteRAM(entityHandle);
              }
          }
          else if(HandleGetType(entityHandle) == TPM_HT_PERSISTENT)
          {
              OBJECT          object;
               // Get evict object
               NvGetEvictObject(entityHandle, &object);
               // If the evict object belongs to the hierarchy to be flushed
               if(     (    hierarchy == TPM_RH_PLATFORM
                        && object.attributes.ppsHierarchy == SET)
                   || (     hierarchy == TPM_RH_OWNER
                        && object.attributes.spsHierarchy == SET)
                   || (     hierarchy == TPM_RH_ENDORSEMENT
                        && object.attributes.epsHierarchy == SET)
                   )
               {
                     // Delete the evict object
                     NvDelete(currentAddr);
                     // Re-iterate from beginning after a delete
                     iter = NV_ITER_INIT;
               }
          }
          else
          {
               pAssert(FALSE);
          }
   }
   return;
}
//
//
//              NvSetGlobalLock()
//
//       This function is used to SET the TPMA_NV_WRITELOCKED attribute for all NV Indices that have
//       TPMA_NV_GLOBALLOCK SET. This function is use by TPM2_NV_GlobalWriteLock().
//
void
NvSetGlobalLock(
   void
   )
{
   NV_ITER               iter = NV_ITER_INIT;
   UINT32                currentAddr;
   // Check all Indices
   while((currentAddr = NvNextIndex(&iter)) != 0)
   {
       NV_INDEX    nvIndex;
          // Read the index data
          _plat__NvMemoryRead(currentAddr + sizeof(TPM_HANDLE),
                              sizeof(NV_INDEX), &nvIndex);
          // See if it should be locked
          if(nvIndex.publicArea.attributes.TPMA_NV_GLOBALLOCK == SET)
          {
                // if so, lock it
                nvIndex.publicArea.attributes.TPMA_NV_WRITELOCKED = SET;
                _plat__NvMemoryWrite(currentAddr + sizeof(TPM_HANDLE),
                                     sizeof(NV_INDEX), &nvIndex);
                // Set the flag that a NV write happens
                g_updateNV = TRUE;
          }
   }
   return;
}
//
//
//              InsertSort()
//
//       Sort a handle into handle list in ascending order. The total handle number in the list should not exceed
//       MAX_CAP_HANDLES
//
static void
InsertSort(
   TPML_HANDLE           *handleList,     // IN/OUT: sorted handle list
   UINT32                 count,          // IN: maximum count in the handle list
   TPM_HANDLE             entityHandle    // IN: handle to be inserted
   )
{
   UINT32                i, j;
   UINT32                originalCount;
   // For a corner case that the maximum count is 0, do nothing
   if(count == 0) return;
   // For empty list, add the handle at the beginning and return
   if(handleList->count == 0)
   {
       handleList->handle[0] = entityHandle;
       handleList->count++;
       return;
   }
   // Check if the maximum of the list has been reached
   originalCount = handleList->count;
   if(originalCount < count)
       handleList->count++;
   // Insert the handle to the list
   for(i = 0; i < originalCount; i++)
   {
       if(handleList->handle[i] > entityHandle)
       {
           for(j = handleList->count - 1; j > i; j--)
           {
               handleList->handle[j] = handleList->handle[j-1];
           }
           break;
       }
   }
     // If a slot was found, insert the handle in this position
     if(i < originalCount || handleList->count > originalCount)
         handleList->handle[i] = entityHandle;
     return;
}
//
//
//            NvCapGetPersistent()
//
//       This function is used to get a list of handles of the persistent objects, starting at handle.
//       Handle must be in valid persistent object handle range, but does not have to reference an existing
//       persistent object.
//
//       Return Value                      Meaning
//
//       YES                               if there are more handles available
//       NO                                all the available handles has been returned
//
TPMI_YES_NO
NvCapGetPersistent(
     TPMI_DH_OBJECT       handle,            // IN: start handle
     UINT32               count,             // IN: maximum number of returned handle
     TPML_HANDLE         *handleList         // OUT: list of handle
     )
{
     TPMI_YES_NO               more = NO;
     NV_ITER                   iter = NV_ITER_INIT;
     UINT32                    currentAddr;
     pAssert(HandleGetType(handle) == TPM_HT_PERSISTENT);
     // Initialize output handle list
     handleList->count = 0;
     // The maximum count of handles we may return is MAX_CAP_HANDLES
     if(count > MAX_CAP_HANDLES) count = MAX_CAP_HANDLES;
     while((currentAddr = NvNextEvict(&iter)) != 0)
     {
         TPM_HANDLE      entityHandle;
          // Read handle information.
          _plat__NvMemoryRead(currentAddr, sizeof(TPM_HANDLE), &entityHandle);
          // Ignore persistent handles that have values less than the input handle
          if(entityHandle < handle)
              continue;
          // if the handles in the list have reached the requested count, and there
          // are still handles need to be inserted, indicate that there are more.
          if(handleList->count == count)
              more = YES;
          // A handle with a value larger than start handle is a candidate
          // for return. Insert sort it to the return list. Insert sort algorithm
          // is chosen here for simplicity based on the assumption that the total
          // number of NV Indices is small. For an implementation that may allow
          // large number of NV Indices, a more efficient sorting algorithm may be
          // used here.
          InsertSort(handleList, count, entityHandle);
//
     }
     return more;
}
//
//
//            NvCapGetIndex()
//
//       This function returns a list of handles of NV Indices, starting from handle. Handle must be in the range of
//       NV Indices, but does not have to reference an existing NV Index.
//
//       Return Value                      Meaning
//
//       YES                               if there are more handles to report
//       NO                                all the available handles has been reported
//
TPMI_YES_NO
NvCapGetIndex(
     TPMI_DH_OBJECT     handle,              // IN: start handle
     UINT32             count,               // IN: maximum number of returned handle
     TPML_HANDLE       *handleList           // OUT: list of handle
     )
{
     TPMI_YES_NO             more = NO;
     NV_ITER                 iter = NV_ITER_INIT;
     UINT32                  currentAddr;
     pAssert(HandleGetType(handle) == TPM_HT_NV_INDEX);
     // Initialize output handle list
     handleList->count = 0;
     // The maximum count of handles we may return is MAX_CAP_HANDLES
     if(count > MAX_CAP_HANDLES) count = MAX_CAP_HANDLES;
     while((currentAddr = NvNextIndex(&iter)) != 0)
     {
         TPM_HANDLE      entityHandle;
          // Read handle information.
          _plat__NvMemoryRead(currentAddr, sizeof(TPM_HANDLE), &entityHandle);
          // Ignore index handles that have values less than the 'handle'
          if(entityHandle < handle)
              continue;
          // if the count of handles in the list has reached the requested count,
          // and there are still handles to report, set more.
          if(handleList->count == count)
              more = YES;
          // A handle with a value larger than start handle is a candidate
          // for return. Insert sort it to the return list. Insert sort algorithm
          // is chosen here for simplicity based on the assumption that the total
          // number of NV Indices is small. For an implementation that may allow
          // large number of NV Indices, a more efficient sorting algorithm may be
          // used here.
          InsertSort(handleList, count, entityHandle);
     }
     return more;
}
//
//
//
//           NvCapGetIndexNumber()
//
//       This function returns the count of NV Indexes currently defined.
//
UINT32
NvCapGetIndexNumber(
   void
   )
{
   UINT32              num = 0;
   NV_ITER             iter = NV_ITER_INIT;
   while(NvNextIndex(&iter) != 0) num++;
   return num;
}
//
//
//           NvCapGetPersistentNumber()
//
//       Function returns the count of persistent objects currently in NV memory.
//
UINT32
NvCapGetPersistentNumber(
   void
   )
{
   UINT32              num = 0;
   NV_ITER             iter = NV_ITER_INIT;
   while(NvNextEvict(&iter) != 0) num++;
   return num;
}
//
//
//           NvCapGetPersistentAvail()
//
//       This function returns an estimate of the number of additional persistent objects that could be loaded into
//       NV memory.
//
UINT32
NvCapGetPersistentAvail(
   void
   )
{
   UINT32              availSpace;
   UINT32              objectSpace;
   // Compute the available space in NV storage
   availSpace = NvGetFreeByte();
   // Get the space needed to add a persistent object to NV storage
   objectSpace = NvGetEvictObjectSize();
   return availSpace / objectSpace;
}
//
//
//           NvCapGetCounterNumber()
//
//       Get the number of defined NV Indexes that have NV TPMA_NV_COUNTER attribute SET.
//
//
UINT32
NvCapGetCounterNumber(
   void
   )
{
   NV_ITER             iter = NV_ITER_INIT;
   UINT32              currentAddr;
   UINT32              num = 0;
   while((currentAddr = NvNextIndex(&iter)) != 0)
   {
       NV_INDEX    nvIndex;
          // Get NV Index info
          _plat__NvMemoryRead(currentAddr + sizeof(TPM_HANDLE),
                               sizeof(NV_INDEX), &nvIndex);
          if(nvIndex.publicArea.attributes.TPMA_NV_COUNTER == SET) num++;
   }
   return num;
}
//
//
//            NvCapGetCounterAvail()
//
//       This function returns an estimate of the number of additional counter type NV Indices that can be defined.
//
UINT32
NvCapGetCounterAvail(
   void
   )
{
   UINT32              availNVSpace;
   UINT32              availRAMSpace;
   UINT32              counterNVSpace;
   UINT32              counterRAMSpace;
   UINT32              persistentNum = NvCapGetPersistentNumber();
   // Get the available space in NV storage
   availNVSpace = NvGetFreeByte();
   if (persistentNum < MIN_EVICT_OBJECTS)
   {
       // Some space have to be reserved for evict object. Adjust availNVSpace.
       UINT32       reserved = (MIN_EVICT_OBJECTS - persistentNum)
                              * NvGetEvictObjectSize();
       if (reserved > availNVSpace)
            availNVSpace = 0;
       else
            availNVSpace -= reserved;
   }
   // Get the space needed to add a counter index to NV storage
   counterNVSpace = NvGetCounterSize();
   // Compute the available space in RAM
   availRAMSpace = RAM_INDEX_SPACE - s_ramIndexSize;
   // Compute the space needed to add a counter index to RAM storage
   // It takes an size field, a handle and sizeof(UINT64) for counter data
   counterRAMSpace = sizeof(UINT32) + sizeof(TPM_HANDLE) + sizeof(UINT64);
   // Return the min of counter number in NV and in RAM
   if(availNVSpace / counterNVSpace > availRAMSpace / counterRAMSpace)
       return availRAMSpace / counterRAMSpace;
     else
         return availNVSpace / counterNVSpace;
}
