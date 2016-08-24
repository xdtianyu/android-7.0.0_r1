/*
 * Copyright 2015 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef __TPM2_MEMORYLIB_FP_H
#define __TPM2_MEMORYLIB_FP_H

#include "InternalRoutines.h"

BYTE *MemoryGetResponseBuffer(
    TPM_CC command  // Command that requires the buffer
    );
LIB_EXPORT BOOL Memory2BEqual(const TPM2B *aIn,  // IN: compare value
                              const TPM2B *bIn   // IN: compare value
                              );
#define MemoryCopy(destination, source, size, destSize) \
  MemoryMove((destination), (source), (size), (destSize))
LIB_EXPORT INT16 MemoryCopy2B(TPM2B *dest,          // OUT: receiving TPM2B
                              const TPM2B *source,  // IN: source TPM2B
                              UINT16 dSize  // IN: size of the receiving buffer
                              );
LIB_EXPORT void MemoryMove(void *destination,   //   OUT: move destination
                           const void *source,  //   IN: move source
                           UINT32 size,  //   IN: number of octets to moved
                           UINT32 dSize  //   IN: size of the receive buffer
                           );
UINT16 MemoryRemoveTrailingZeros(TPM2B_AUTH *auth  // IN/OUT: value to adjust
                                 );
LIB_EXPORT void MemorySet(void *destination,  // OUT: memory destination
                          char value,         // IN: fill value
                          UINT32 size         // IN: number of octets to fill
                          );
LIB_EXPORT void MemoryConcat2B(
    TPM2B *aInOut,  // IN/OUT: destination 2B
    TPM2B *bIn,     // IN: second 2B
    UINT16 aSize  // IN: The size of aInOut.buffer (max values for aInOut.size)
    );
LIB_EXPORT BOOL MemoryEqual(const void *buffer1,  // IN: compare buffer1
                            const void *buffer2,  // IN: compare buffer2
                            UINT32 size  // IN: size of bytes being compared
                            );

#endif  // __TPM2_MEMORYLIB_FP_H
