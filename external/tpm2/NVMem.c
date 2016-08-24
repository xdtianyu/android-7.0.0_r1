// This file was extracted from the TCG Published
// Trusted Platform Module Library
// Part 4: Supporting Routines
// Family "2.0"
// Level 00 Revision 01.16
// October 30, 2014

#include     <memory.h>
#include     <stdio.h>
#include     <string.h>

#include     "PlatformData.h"
#include     "TpmError.h"
#include     "assert.h"

#ifndef EMBEDDED_MODE
#define FILE_BACKED_NV
#endif

#if defined FILE_BACKED_NV
static   FILE*                  s_NVFile;
#endif
static   unsigned char          s_NV[NV_MEMORY_SIZE];
static   BOOL                   s_NvIsAvailable;
static   BOOL                   s_NV_unrecoverable;
static   BOOL                   s_NV_recoverable;
//
//
//          Functions
//
//          _plat__NvErrors()
//
//     This function is used by the simulator to set the error flags in the NV subsystem to simulate an error in the
//     NV loading process
//
LIB_EXPORT void
_plat__NvErrors(
     BOOL                 recoverable,
     BOOL                 unrecoverable
     )
{
     s_NV_unrecoverable = unrecoverable;
     s_NV_recoverable = recoverable;
}
//
//
//          _plat__NVEnable()
//
//     Enable NV memory.
//     This version just pulls in data from a file. In a real TPM, with NV on chip, this function would verify the
//     integrity of the saved context. If the NV memory was not on chip but was in something like RPMB, the NV
//     state would be read in, decrypted and integrity checked.
//     The recovery from an integrity failure depends on where the error occurred. It it was in the state that is
//     discarded by TPM Reset, then the error is recoverable if the TPM is reset. Otherwise, the TPM must go
//     into failure mode.
//
//     Return Value                      Meaning
//
//     0                                 if success
//     >0                                if receive recoverable error
//     <0                                if unrecoverable error
//
LIB_EXPORT int
_plat__NVEnable(
     void                *platParameter       // IN: platform specific parameter
     )
{
     // Start assuming everything is OK
   s_NV_unrecoverable = FALSE;
   s_NV_recoverable = FALSE;
#ifdef FILE_BACKED_NV
   if(s_NVFile != NULL) return 0;
   // Try to open an exist NVChip file for read/write
   s_NVFile = fopen("NVChip", "r+b");
   if(NULL != s_NVFile)
   {
       // See if the NVChip file is empty
       fseek(s_NVFile, 0, SEEK_END);
       if(0 == ftell(s_NVFile))
           s_NVFile = NULL;
   }
   if(s_NVFile == NULL)
   {
       // Initialize all the byte in the new file to 0
       memset(s_NV, 0, NV_MEMORY_SIZE);
          // If NVChip file does not exist, try to create it for read/write
          s_NVFile = fopen("NVChip", "w+b");
          // Start initialize at the end of new file
          fseek(s_NVFile, 0, SEEK_END);
          // Write 0s to NVChip file
          fwrite(s_NV, 1, NV_MEMORY_SIZE, s_NVFile);
   }
   else
   {
       // If NVChip file exist, assume the size is correct
       fseek(s_NVFile, 0, SEEK_END);
       assert(ftell(s_NVFile) == NV_MEMORY_SIZE);
       // read NV file data to memory
       fseek(s_NVFile, 0, SEEK_SET);
       assert(1 == fread(s_NV, NV_MEMORY_SIZE, 1, s_NVFile));
   }
#endif
   // NV contents have been read and the error checks have been performed. For
   // simulation purposes, use the signaling interface to indicate if an error is
   // to be simulated and the type of the error.
   if(s_NV_unrecoverable)
       return -1;
   return s_NV_recoverable;
}
//
//
//         _plat__NVDisable()
//
//     Disable NV memory
//
LIB_EXPORT void
_plat__NVDisable(
   void
   )
{
#ifdef     FILE_BACKED_NV
   assert(s_NVFile != NULL);
   // Close NV file
   fclose(s_NVFile);
   // Set file handle to NULL
//
    s_NVFile = NULL;
#endif
    return;
}
//
//
//          _plat__IsNvAvailable()
//
//      Check if NV is available
//
//      Return Value                      Meaning
//
//      0                                 NV is available
//      1                                 NV is not available due to write failure
//      2                                 NV is not available due to rate limit
//
LIB_EXPORT int
_plat__IsNvAvailable(
    void
    )
{
    // NV is not available if the TPM is in failure mode
    if(!s_NvIsAvailable)
        return 1;
#ifdef FILE_BACKED_NV
   if(s_NVFile == NULL)
       return 1;
#endif
    return 0;
}
//
//
//          _plat__NvMemoryRead()
//
//      Function: Read a chunk of NV memory
//
LIB_EXPORT void
_plat__NvMemoryRead(
    unsigned int           startOffset,       // IN: read start
    unsigned int           size,              // IN: size of bytes to read
    void                  *data               // OUT: data buffer
    )
{
    assert(startOffset + size <= NV_MEMORY_SIZE);
    // Copy data from RAM
    memcpy(data, &s_NV[startOffset], size);
    return;
}
//
//
//          _plat__NvIsDifferent()
//
//      This function checks to see if the NV is different from the test value. This is so that NV will not be written if
//      it has not changed.
//
//
//
//
//      Return Value                  Meaning
//
//      TRUE                          the NV location is different from the test value
//      FALSE                         the NV location is the same as the test value
//
LIB_EXPORT BOOL
_plat__NvIsDifferent(
   unsigned int        startOffset,       // IN: read start
   unsigned int        size,              // IN: size of bytes to read
   void               *data               // IN: data buffer
   )
{
   return (memcmp(&s_NV[startOffset], data, size) != 0);
}
//
//
//         _plat__NvMemoryWrite()
//
//      This function is used to update NV memory. The write is to a memory copy of NV. At the end of the
//      current command, any changes are written to the actual NV memory.
//
LIB_EXPORT void
_plat__NvMemoryWrite(
   unsigned int        startOffset,       // IN: write start
   unsigned int        size,              // IN: size of bytes to write
   void               *data               // OUT: data buffer
   )
{
   assert(startOffset + size <= NV_MEMORY_SIZE);
   // Copy the data to the NV image
   memcpy(&s_NV[startOffset], data, size);
}
//
//
//         _plat__NvMemoryMove()
//
//      Function: Move a chunk of NV memory from source to destination This function should ensure that if
//      there overlap, the original data is copied before it is written
//
LIB_EXPORT void
_plat__NvMemoryMove(
   unsigned int        sourceOffset,      // IN: source offset
   unsigned int        destOffset,        // IN: destination offset
   unsigned int        size               // IN: size of data being moved
   )
{
   assert(sourceOffset + size <= NV_MEMORY_SIZE);
   assert(destOffset + size <= NV_MEMORY_SIZE);
   // Move data in RAM
   memmove(&s_NV[destOffset], &s_NV[sourceOffset], size);
   return;
}
//
//
//         _plat__NvCommit()
//
//      Update NV chip
//
//
//
//      Return Value                      Meaning
//
//      0                                 NV write success
//      non-0                             NV write fail
//
LIB_EXPORT int
_plat__NvCommit(
   void
   )
{
#ifdef FILE_BACKED_NV
   // If NV file is not available, return failure
   if(s_NVFile == NULL)
       return 1;
   // Write RAM data to NV
   fseek(s_NVFile, 0, SEEK_SET);
   fwrite(s_NV, 1, NV_MEMORY_SIZE, s_NVFile);
   return 0;
#else
   return 0;
#endif
}
//
//
//       _plat__SetNvAvail()
//
//      Set the current NV state to available. This function is for testing purpose only. It is not part of the
//      platform NV logic
//
LIB_EXPORT void
_plat__SetNvAvail(
   void
   )
{
   s_NvIsAvailable = TRUE;
   return;
}
//
//
//       _plat__ClearNvAvail()
//
//      Set the current NV state to unavailable. This function is for testing purpose only. It is not part of the
//      platform NV logic
//
LIB_EXPORT void
_plat__ClearNvAvail(
   void
   )
{
   s_NvIsAvailable = FALSE;
   return;
}
