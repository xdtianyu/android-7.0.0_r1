// This file was extracted from the TCG Published
// Trusted Platform Module Library
// Part 4: Supporting Routines
// Family "2.0"
// Level 00 Revision 01.16
// October 30, 2014

#include       "PlatformData.h"
#include       "Platform.h"
//
//
//          Functions
//
//          _plat__Signal_PowerOn()
//
//     Signal platform power on
//
LIB_EXPORT int
_plat__Signal_PowerOn(
     void
     )
{
     // Start clock
     _plat__ClockReset();
     // Initialize locality
     s_locality = 0;
     // Command cancel
      s_isCanceled = FALSE;
     // Need to indicate that we lost power
     s_powerLost = TRUE;
     return 0;
}
//
//
//          _plat__WasPowerLost()
//
//     Test whether power was lost before a _TPM_Init()
//
LIB_EXPORT BOOL
_plat__WasPowerLost(
     BOOL                 clear
     )
{
     BOOL        retVal = s_powerLost;
     if(clear)
         s_powerLost = FALSE;
     return retVal;
}
//
//
//          _plat_Signal_Reset()
//
//     This a TPM reset without a power loss.
//
LIB_EXPORT int
_plat__Signal_Reset(
     void
     )
{
     // Need to reset the clock
     _plat__ClockReset();
   // if we are doing reset but did not have a power failure, then we should
   // not need to reload NV ...
   return 0;
}
//
//
//        _plat__Signal_PowerOff()
//
//     Signal platform power off
//
LIB_EXPORT void
_plat__Signal_PowerOff(
   void
   )
{
   // Prepare NV memory for power off
   _plat__NVDisable();
   return;
}
