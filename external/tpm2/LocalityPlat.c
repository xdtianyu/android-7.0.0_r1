// This file was extracted from the TCG Published
// Trusted Platform Module Library
// Part 4: Supporting Routines
// Family "2.0"
// Level 00 Revision 01.16
// October 30, 2014

#include "PlatformData.h"
#include "TpmError.h"

static BOOL s_RsaKeyCacheEnabled;

//
//
//          Functions
//
//          _plat__LocalityGet()
//
//     Get the most recent command locality in locality value form. This is an integer value for locality and not a
//     locality structure The locality can be 0-4 or 32-255. 5-31 is not allowed.
//
LIB_EXPORT unsigned char
_plat__LocalityGet(
     void
     )
{
     return s_locality;
}
//
//
//          _plat__LocalitySet()
//
//     Set the most recent command locality in locality value form
//
LIB_EXPORT void
_plat__LocalitySet(
     unsigned char       locality
     )
{
     if(locality > 4 && locality < 32)
         locality = 0;
     s_locality = locality;
     return;
}
//
//
//          _plat__IsRsaKeyCacheEnabled()
//
//     This function is used to check if the RSA key cache is enabled or not.
//
LIB_EXPORT int
_plat__IsRsaKeyCacheEnabled(
     void
     )
{
     return s_RsaKeyCacheEnabled;
}
