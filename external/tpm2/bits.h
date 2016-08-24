// This file was extracted from the TCG Published
// Trusted Platform Module Library
// Part 4: Supporting Routines
// Family "2.0"
// Level 00 Revision 01.16
// October 30, 2014

#ifndef     _BITS_H
#define     _BITS_H
#define CLEAR_BIT(bit, vector)    BitClear((bit), (BYTE *)&(vector), sizeof(vector))
#define SET_BIT(bit, vector)      BitSet((bit), (BYTE *)&(vector), sizeof(vector))
#define TEST_BIT(bit, vector)     BitIsSet((bit), (BYTE *)&(vector), sizeof(vector))
#endif
