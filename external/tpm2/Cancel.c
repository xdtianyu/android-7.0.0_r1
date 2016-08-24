// This file was extracted from the TCG Published
// Trusted Platform Module Library
// Part 4: Supporting Routines
// Family "2.0"
// Level 00 Revision 01.16
// October 30, 2014

#include "PlatformData.h"
//
//
//          Functions
//
//          _plat__IsCanceled()
//
//     Check if the cancel flag is set
//
//     Return Value                      Meaning
//
//     TRUE                              if cancel flag is set
//     FALSE                             if cancel flag is not set
//
LIB_EXPORT BOOL
_plat__IsCanceled(
     void
     )
{
     // return cancel flag
     return s_isCanceled;
}
//
//
//          _plat__SetCancel()
//
//     Set cancel flag.
//
LIB_EXPORT void
_plat__SetCancel(
     void
     )
{
     s_isCanceled = TRUE;
     return;
}
//
//
//
//        _plat__ClearCancel()
//
//     Clear cancel flag
//
LIB_EXPORT void
_plat__ClearCancel(
   void
   )
{
   s_isCanceled = FALSE;
   return;
}
