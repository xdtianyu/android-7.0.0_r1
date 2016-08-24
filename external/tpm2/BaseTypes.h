// This file was extracted from the TCG Published
// Trusted Platform Module Library
// Part 4: Supporting Routines
// Family "2.0"
// Level 00 Revision 01.16
// October 30, 2014

#ifndef _BASETYPES_H
#define _BASETYPES_H
#include "stdint.h"
//
//     NULL definition
//
#ifndef          NULL
#define          NULL        (0)
#endif
typedef uint8_t              UINT8;
typedef uint8_t              BYTE;
typedef int8_t               INT8;
typedef int                   BOOL;
typedef uint16_t             UINT16;
typedef int16_t              INT16;
typedef uint32_t             UINT32;
typedef int32_t              INT32;
typedef uint64_t             UINT64;
typedef int64_t              INT64;
typedef struct {
   UINT16         size;
   BYTE           buffer[1];
} TPM2B;
#endif
