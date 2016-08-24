// This file was extracted from the TCG Published
// Trusted Platform Module Library
// Part 3: Commands
// Family "2.0"
// Level 00 Revision 01.16
// October 30, 2014

#include "InternalRoutines.h"
#include "IncrementalSelfTest_fp.h"
//
//
//     Error Returns                 Meaning
//
//     TPM_RC_CANCELED               the command was canceled (some tests may have completed)
//     TPM_RC_VALUE                  an algorithm in the toTest list is not implemented
//
TPM_RC
TPM2_IncrementalSelfTest(
   IncrementalSelfTest_In        *in,                // IN: input parameter list
   IncrementalSelfTest_Out       *out                // OUT: output parameter list
   )
{
   TPM_RC                         result;
// Command Output

   // Call incremental self test function in crypt module. If this function
   // returns TPM_RC_VALUE, it means that an algorithm on the 'toTest' list is
   // not implemented.
   result = CryptIncrementalSelfTest(&in->toTest, &out->toDoList);
   if(result == TPM_RC_VALUE)
       return TPM_RC_VALUE + RC_IncrementalSelfTest_toTest;
   return result;
}
