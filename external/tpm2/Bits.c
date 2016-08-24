// This file was extracted from the TCG Published
// Trusted Platform Module Library
// Part 4: Supporting Routines
// Family "2.0"
// Level 00 Revision 01.16
// October 30, 2014

#include "InternalRoutines.h"
//
//
//           Functions
//
//            BitIsSet()
//
//      This function is used to check the setting of a bit in an array of bits.
//
//      Return Value                          Meaning
//
//      TRUE                                  bit is set
//      FALSE                                 bit is not set
//
BOOL
BitIsSet(
     unsigned int          bitNum,                    // IN: number of the bit in 'bArray'
     BYTE                 *bArray,                    // IN: array containing the bit
     unsigned int          arraySize                  // IN: size in bytes of 'bArray'
     )
{
     pAssert(arraySize > (bitNum >> 3));
     return((bArray[bitNum >> 3] & (1 << (bitNum & 7))) != 0);
}
//
//
//            BitSet()
//
//      This function will set the indicated bit in bArray.
//
void
BitSet(
     unsigned int          bitNum,                    // IN: number of the bit in 'bArray'
     BYTE                 *bArray,                    // IN: array containing the bit
     unsigned int          arraySize                  // IN: size in bytes of 'bArray'
     )
{
     pAssert(arraySize > bitNum/8);
     bArray[bitNum >> 3] |= (1 << (bitNum & 7));
}
//
//
//           BitClear()
//
//     This function will clear the indicated bit in bArray.
//
void
BitClear(
     unsigned int         bitNum,             // IN: number of the bit in 'bArray'.
     BYTE                *bArray,             // IN: array containing the bit
     unsigned int         arraySize           // IN: size in bytes of 'bArray'
     )
{
     pAssert(arraySize > bitNum/8);
     bArray[bitNum >> 3] &= ~(1 << (bitNum & 7));
}
