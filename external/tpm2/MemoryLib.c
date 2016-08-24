// This file was extracted from the TCG Published
// Trusted Platform Module Library
// Part 4: Supporting Routines
// Family "2.0"
// Level 00 Revision 01.16
// October 30, 2014

#define MEMORY_LIB_C
#include "MemoryLib_fp.h"
//
//     These buffers are set aside to hold command and response values. In this implementation, it is not
//     guaranteed that the code will stop accessing the s_actionInputBuffer before starting to put values in the
//     s_actionOutputBuffer so different buffers are required. However, the s_actionInputBuffer and
//     s_responseBuffer are not needed at the same time and they could be the same buffer.
//
//          Functions on BYTE Arrays
//
//          MemoryMove()
//
//     This function moves data from one place in memory to another. No safety checks of any type are
//     performed. If source and data buffer overlap, then the move is done as if an intermediate buffer were
//     used.
//
//     NOTE:           This function is used by MemoryCopy(), MemoryCopy2B(), and MemoryConcat2b() and requires that the caller
//                     know the maximum size of the destination buffer so that there is no possibility of buffer overrun.
//
LIB_EXPORT void
MemoryMove(
      void              *destination,          //   OUT: move destination
      const void        *source,               //   IN: move source
      UINT32             size,                 //   IN: number of octets to moved
      UINT32             dSize                 //   IN: size of the receive buffer
      )
{
      const BYTE *p = (BYTE *)source;
      BYTE *q = (BYTE *)destination;
      if(destination == NULL || source == NULL)
          return;
      pAssert(size <= dSize);
      // if the destination buffer has a lower address than the
      // source, then moving bytes in ascending order is safe.
      dSize -= size;
      if (p>q || (p+size <= q))
      {
          while(size--)
              *q++ = *p++;
      }
      // If the destination buffer has a higher address than the
      // source, then move bytes from the end to the beginning.
      else if (p < q)
      {
          p += size;
          q += size;
//
          while (size--)
              *--q = *--p;
      }
      // If the source and destination address are the same, nothing to move.
      return;
}
//
//         MemoryEqual()
//
//     This function indicates if two buffers have the same values in the indicated number of bytes.
//
//     Return Value                     Meaning
//
//     TRUE                             all octets are the same
//     FALSE                            all octets are not the same
//
LIB_EXPORT BOOL
MemoryEqual(
      const void       *buffer1,             // IN: compare buffer1
      const void       *buffer2,             // IN: compare buffer2
      UINT32            size                 // IN: size of bytes being compared
      )
{
      BOOL          equal = TRUE;
      const BYTE   *b1, *b2;
      b1 = (BYTE *)buffer1;
      b2 = (BYTE *)buffer2;
      // Compare all bytes so that there is no leakage of information
      // due to timing differences.
      for(; size > 0; size--)
          equal = (*b1++ == *b2++) && equal;
      return equal;
}
//
//
//         MemoryCopy2B()
//
//     This function copies a TPM2B. This can be used when the TPM2B types are the same or different. No
//     size checking is done on the destination so the caller should make sure that the destination is large
//     enough.
//
//      This function returns the number of octets in the data buffer of the TPM2B.
//
LIB_EXPORT INT16
MemoryCopy2B(
   TPM2B               *dest,                // OUT: receiving TPM2B
   const TPM2B         *source,              // IN: source TPM2B
   UINT16               dSize                // IN: size of the receiving buffer
   )
{
   if(dest == NULL)
       return 0;
   if(source == NULL)
       dest->size = 0;
   else
   {
       dest->size = source->size;
       MemoryMove(dest->buffer, source->buffer, dest->size, dSize);
   }
   return dest->size;
}
//
//
//          MemoryConcat2B()
//
//      This function will concatenate the buffer contents of a TPM2B to an the buffer contents of another TPM2B
//      and adjust the size accordingly (a := (a | b)).
//
LIB_EXPORT void
MemoryConcat2B(
   TPM2B               *aInOut,              // IN/OUT: destination 2B
   TPM2B               *bIn,                 // IN: second 2B
   UINT16               aSize                // IN: The size of aInOut.buffer (max values for
                                             //     aInOut.size)
   )
{
   MemoryMove(&aInOut->buffer[aInOut->size],
              bIn->buffer,
              bIn->size,
              aSize - aInOut->size);
   aInOut->size = aInOut->size + bIn->size;
   return;
}
//
//
//          Memory2BEqual()
//
//      This function will compare two TPM2B structures. To be equal, they need to be the same size and the
//      buffer contexts need to be the same in all octets.
//
//      Return Value                      Meaning
//
//      TRUE                              size and buffer contents are the same
//      FALSE                             size or buffer contents are not the same
//
LIB_EXPORT BOOL
Memory2BEqual(
   const TPM2B         *aIn,                 // IN: compare value
   const TPM2B         *bIn                  // IN: compare value
   )
{
   if(aIn->size != bIn->size)
       return FALSE;
    return MemoryEqual(aIn->buffer, bIn->buffer, aIn->size);
}
//
//
//          MemorySet()
//
//      This function will set all the octets in the specified memory range to the specified octet value.
//
//      NOTE:            the dSize parameter forces the caller to know how big the receiving buffer is to make sure that there is no
//                       possibility that the caller will inadvertently run over the end of the buffer.
//
LIB_EXPORT void
MemorySet(
    void                 *destination,           // OUT: memory destination
    char                  value,                 // IN: fill value
    UINT32                size                   // IN: number of octets to fill
    )
{
    char *p = (char *)destination;
    while (size--)
        *p++ = value;
    return;
}
#ifndef EMBEDDED_MODE
//
//
//          MemoryGetActionInputBuffer()
//
//      This function returns the address of the buffer into which the command parameters will be unmarshaled in
//      preparation for calling the command actions.
//
BYTE *
MemoryGetActionInputBuffer(
    UINT32                 size                  // Size, in bytes, required for the input
                                                 // unmarshaling
    )
{
    BYTE           *buf = NULL;
    if(size > 0)
    {
        // In this implementation, a static buffer is set aside for action output.
        // Other implementations may apply additional optimization based on command
        // code or other factors.
        UINT32      *p = s_actionInputBuffer;
        buf = (BYTE *)p;
        pAssert(size < sizeof(s_actionInputBuffer));
       // size of an element in the buffer
#define SZ      sizeof(s_actionInputBuffer[0])
       for(size = (size + SZ - 1) / SZ; size > 0; size--)
           *p++ = 0;
#undef SZ
   }
   return buf;
}
//
//
//          MemoryGetActionOutputBuffer()
//
//      This function returns the address of the buffer into which the command action code places its output
//      values.
//
void *
MemoryGetActionOutputBuffer(
      TPM_CC             command            // Command that requires the buffer
      )
{
      // In this implementation, a static buffer is set aside for action output.
      // Other implementations may apply additional optimization based on the command
      // code or other factors.
      command = 0;        // Unreferenced parameter
      return s_actionOutputBuffer;
}
#endif // EMBEDDED_MODE  ^^^^^ not defined.

//
//
//       MemoryGetResponseBuffer()
//
//      This function returns the address into which the command response is marshaled from values in the
//      action output buffer.
//
BYTE*
MemoryGetResponseBuffer(
      TPM_CC             command            // Command that requires the buffer
      )
{
      // In this implementation, a static buffer is set aside for responses.
      // Other implementation may apply additional optimization based on the command
      // code or other factors.
      command = 0;        // Unreferenced parameter
      return s_responseBuffer;
}
//
//
//       MemoryRemoveTrailingZeros()
//
//      This function is used to adjust the length of an authorization value. It adjusts the size of the TPM2B so
//      that it does not include octets at the end of the buffer that contain zero. The function returns the number
//      of non-zero octets in the buffer.
//
UINT16
MemoryRemoveTrailingZeros (
      TPM2B_AUTH        *auth               // IN/OUT: value to adjust
      )
{
      BYTE         *a = &auth->t.buffer[auth->t.size-1];
      for(; auth->t.size > 0; auth->t.size--)
      {
          if(*a--)
              break;
      }
      return auth->t.size;
}
