// This file was extracted from the TCG Published
// Trusted Platform Module Library
// Part 4: Supporting Routines
// Family "2.0"
// Level 00 Revision 01.16
// October 30, 2014

#include       "InternalRoutines.h"
//
//          Functions
//
//           Signal_Hash_Start()
//
//     This function interfaces between the platform code and _TPM_Hash_Start().
//
LIB_EXPORT void
Signal_Hash_Start(
     void
     )
{
     _TPM_Hash_Start();
     return;
}
//
//
//           Signal_Hash_Data()
//
//     This function interfaces between the platform code and _TPM_Hash_Data().
//
LIB_EXPORT void
Signal_Hash_Data(
     unsigned int        size,
     unsigned char      *buffer
     )
{
     _TPM_Hash_Data(size, buffer);
     return;
}
//
//
//           Signal_Hash_End()
//
//     This function interfaces between the platform code and _TPM_Hash_End().
//
LIB_EXPORT void
Signal_Hash_End(
     void
     )
{
     _TPM_Hash_End();
     return;
}
