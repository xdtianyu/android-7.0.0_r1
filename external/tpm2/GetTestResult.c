// This file was extracted from the TCG Published
// Trusted Platform Module Library
// Part 3: Commands
// Family "2.0"
// Level 00 Revision 01.16
// October 30, 2014

#include "InternalRoutines.h"
#include "GetTestResult_fp.h"
//
//     In the reference implementation, this function is only reachable if the TPM is not in failure mode meaning
//     that all tests that have been run have completed successfully. There is not test data and the test result is
//     TPM_RC_SUCCESS.
//
TPM_RC
TPM2_GetTestResult(
   GetTestResult_Out      *out               // OUT: output parameter list
   )
{
// Command Output

   // Call incremental self test function in crypt module
   out->testResult = CryptGetTestResult(&out->outData);

   return TPM_RC_SUCCESS;
}
