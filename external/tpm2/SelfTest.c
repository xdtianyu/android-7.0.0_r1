// This file was extracted from the TCG Published
// Trusted Platform Module Library
// Part 3: Commands
// Family "2.0"
// Level 00 Revision 01.16
// October 30, 2014

#include "InternalRoutines.h"
#include "SelfTest_fp.h"
//
//
//     Error Returns                     Meaning
//
//     TPM_RC_CANCELED                   the command was canceled (some incremental process may have
//                                       been made)
//     TPM_RC_TESTING                    self test in process
//
TPM_RC
TPM2_SelfTest(
   SelfTest_In      *in                   // IN: input parameter list
   )
{
// Command Output

   // Call self test function in crypt module
   return CryptSelfTest(in->fullTest);
}
