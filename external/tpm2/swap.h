// This file was extracted from the TCG Published
// Trusted Platform Module Library
// Part 4: Supporting Routines
// Family "2.0"
// Level 00 Revision 01.16
// October 30, 2014

#ifndef _SWAP_H
#define _SWAP_H
#include "Implementation.h"
#if    NO_AUTO_ALIGN == YES || LITTLE_ENDIAN_TPM == YES
//
//     The aggregation macros for machines that do not allow unaligned access or for little-endian machines.
//     Aggregate bytes into an UINT
//
#define BYTE_ARRAY_TO_UINT8(b)          (UINT8)((b)[0])
#define BYTE_ARRAY_TO_UINT16(b)         (UINT16)( ((b)[0] << 8) \
                                                + (b)[1])
#define BYTE_ARRAY_TO_UINT32(b)         (UINT32)( ((b)[0] << 24) \
                                                + ((b)[1] << 16) \
                                                + ((b)[2] << 8 ) \
                                                + (b)[3])
#define BYTE_ARRAY_TO_UINT64(b)         (UINT64)( ((UINT64)(b)[0] <<       56)   \
                                                + ((UINT64)(b)[1] <<      48)   \
                                                + ((UINT64)(b)[2] <<      40)   \
                                                + ((UINT64)(b)[3] <<      32)   \
                                                + ((UINT64)(b)[4] <<      24)   \
                                                + ((UINT64)(b)[5] <<      16)   \
                                                + ((UINT64)(b)[6] <<       8)   \
                                                + (UINT64)(b)[7])
//
//     Disaggregate a UINT into a byte array
//
#define UINT8_TO_BYTE_ARRAY(i, b)           {(b)[0]   = (BYTE)(i);}
#define UINT16_TO_BYTE_ARRAY(i, b)          {(b)[0]   = (BYTE)((i) >>   8); \
                                             (b)[1]   = (BYTE) (i);}
#define UINT32_TO_BYTE_ARRAY(i, b)          {(b)[0]   = (BYTE)((i) >> 24);  \
                                             (b)[1]   = (BYTE)((i) >> 16);  \
                                             (b)[2]   = (BYTE)((i) >> 8);   \
                                             (b)[3]   = (BYTE) (i);}
#define UINT64_TO_BYTE_ARRAY(i, b)          {(b)[0]   = (BYTE)((i) >>  56); \
                                             (b)[1]   = (BYTE)((i) >>  48); \
                                             (b)[2]   = (BYTE)((i) >>  40); \
                                             (b)[3]   = (BYTE)((i) >>  32); \
                                             (b)[4]   = (BYTE)((i) >>  24); \
                                             (b)[5]   = (BYTE)((i) >>  16); \
                                             (b)[6]   = (BYTE)((i) >>   8); \
                                             (b)[7]   = (BYTE) (i);}
#else
//
//     the big-endian macros for machines that allow unaligned memory access Aggregate a byte array into a
//     UINT
//
#define   BYTE_ARRAY_TO_UINT8(b)            *((UINT8      *)(b))
#define   BYTE_ARRAY_TO_UINT16(b)           *((UINT16     *)(b))
#define   BYTE_ARRAY_TO_UINT32(b)           *((UINT32     *)(b))
#define   BYTE_ARRAY_TO_UINT64(b)           *((UINT64     *)(b))
//
//     Disaggregate a UINT into a byte array
#define   UINT8_TO_BYTE_ARRAY(i, b)      (*((UINT8    *)(b))   =   (i))
#define   UINT16_TO_BYTE_ARRAY(i, b)     (*((UINT16   *)(b))   =   (i))
#define   UINT32_TO_BYTE_ARRAY(i, b)     (*((UINT32   *)(b))   =   (i))
#define   UINT64_TO_BYTE_ARRAY(i, b)     (*((UINT64   *)(b))   =   (i))
#endif    // NO_AUTO_ALIGN == YES
#endif    // _SWAP_H
