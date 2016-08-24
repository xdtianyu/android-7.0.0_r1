// This file was extracted from the TCG Published
// Trusted Platform Module Library
// Part 4: Supporting Routines
// Family "2.0"
// Level 00 Revision 01.16
// October 30, 2014

#ifndef _TPMB_H
#define _TPMB_H
//
//     This macro helps avoid having to type in the structure in order to create a new TPM2B type that is used in
//     a function.
//
#define TPM2B_TYPE(name, bytes)                           \
   typedef union {                                       \
       struct {                                          \
            UINT16 size;                                 \
            BYTE    buffer[(bytes)];                     \
       } t;                                              \
       TPM2B     b;                                      \
   } TPM2B_##name
//
//     Macro to instance and initialize a TPM2B value
//
#define TPM2B_INIT(TYPE, name) \
   TPM2B_##TYPE    name = {sizeof(name.t.buffer), {0}}
#define TPM2B_BYTE_VALUE(bytes) TPM2B_TYPE(bytes##_BYTE_VALUE, bytes)
#endif
