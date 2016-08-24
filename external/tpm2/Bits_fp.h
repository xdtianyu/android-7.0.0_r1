/*
 * Copyright 2015 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef __BITS_FP_H_
#define __BITS_FP_H_

BOOL BitIsSet(unsigned int bitNum,    // IN: number of the bit in 'bArray'
              BYTE *bArray,           // IN: array containing the bit
              unsigned int arraySize  // IN: size in bytes of 'bArray'
              );
//
//
//            BitSet()
//
//      This function will set the indicated bit in bArray.
//
void BitSet(unsigned int bitNum,    // IN: number of the bit in 'bArray'
            BYTE *bArray,           // IN: array containing the bit
            unsigned int arraySize  // IN: size in bytes of 'bArray'
            );

//
//
//           BitClear()
//
//     This function will clear the indicated bit in bArray.
//
void BitClear(unsigned int bitNum,    // IN: number of the bit in 'bArray'.
              BYTE *bArray,           // IN: array containing the bit
              unsigned int arraySize  // IN: size in bytes of 'bArray'
              );

#endif
