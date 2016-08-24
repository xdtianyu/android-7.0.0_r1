// This file was extracted from the TCG Published
// Trusted Platform Module Library
// Part 4: Supporting Routines
// Family "2.0"
// Level 00 Revision 01.16
// October 30, 2014

#include "Tpm.h"
#include "InternalRoutines.h"
//
//
//          Functions
//
//          HandleGetType()
//
//     This function returns the type of a handle which is the MSO of the handle.
//
TPM_HT
HandleGetType(
     TPM_HANDLE           handle             // IN: a handle to be checked
     )
{
     // return the upper bytes of input data
     return (TPM_HT) ((handle & HR_RANGE_MASK) >> HR_SHIFT);
}
//
//
//          NextPermanentHandle()
//
//     This function returns the permanent handle that is equal to the input value or is the next higher value. If
//     there is no handle with the input value and there is no next higher value, it returns 0:
//
//     Return Value                      Meaning
//
TPM_HANDLE
NextPermanentHandle(
     TPM_HANDLE           inHandle           // IN: the handle to check
     )
{
     // If inHandle is below the start of the range of permanent handles
     // set it to the start and scan from there
     if(inHandle < TPM_RH_FIRST)
         inHandle = TPM_RH_FIRST;
     // scan from input value untill we find an implemented permanent handle
     // or go out of range
     for(; inHandle <= TPM_RH_LAST; inHandle++)
     {
         switch (inHandle)
         {
             case TPM_RH_OWNER:
             case TPM_RH_NULL:
             case TPM_RS_PW:
             case TPM_RH_LOCKOUT:
             case TPM_RH_ENDORSEMENT:
             case TPM_RH_PLATFORM:
             case TPM_RH_PLATFORM_NV:
     #ifdef VENDOR_PERMANENT
             case VENDOR_PERMANENT:
     #endif
                 return inHandle;
                  break;
              default:
                  break;
         }
     }
     // Out of range on the top
     return 0;
}
//
//
//          PermanentCapGetHandles()
//
//     This function returns a list of the permanent handles of PCR, started from handle. If handle is larger than
//     the largest permanent handle, an empty list will be returned with more set to NO.
//
//     Return Value                      Meaning
//
//     YES                               if there are more handles available
//     NO                                all the available handles has been returned
//
TPMI_YES_NO
PermanentCapGetHandles(
     TPM_HANDLE         handle,              // IN: start handle
     UINT32             count,               // IN: count of returned handle
     TPML_HANDLE       *handleList           // OUT: list of handle
     )
{
     TPMI_YES_NO       more = NO;
     UINT32            i;
     pAssert(HandleGetType(handle) == TPM_HT_PERMANENT);
     // Initialize output handle list
     handleList->count = 0;
     // The maximum count of handles we may return is MAX_CAP_HANDLES
     if(count > MAX_CAP_HANDLES) count = MAX_CAP_HANDLES;
     // Iterate permanent handle range
     for(i = NextPermanentHandle(handle);
              i != 0; i = NextPermanentHandle(i+1))
     {
         if(handleList->count < count)
         {
              // If we have not filled up the return list, add this permanent
              // handle to it
              handleList->handle[handleList->count] = i;
              handleList->count++;
         }
         else
         {
              // If the return list is full but we still have permanent handle
              // available, report this and stop iterating
              more = YES;
              break;
         }
     }
     return more;
}
