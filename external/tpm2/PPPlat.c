// This file was extracted from the TCG Published
// Trusted Platform Module Library
// Part 4: Supporting Routines
// Family "2.0"
// Level 00 Revision 01.16
// October 30, 2014

#include "PlatformData.h"
//
//
//      Functions
//
//      _plat__PhysicalPresenceAsserted()
//
//     Check if physical presence is signaled
//
//     Return Value                      Meaning
//
//     TRUE                              if physical presence is signaled
//     FALSE                             if physical presence is not signaled
//
LIB_EXPORT BOOL
_plat__PhysicalPresenceAsserted(
   void
   )
{
   // Do not know how to check physical presence without real hardware.
   // so always return TRUE;
   return s_physicalPresence;
}
//
//
//      _plat__Signal_PhysicalPresenceOn()
//
//     Signal physical presence on
//
LIB_EXPORT void
_plat__Signal_PhysicalPresenceOn(
   void
   )
{
   s_physicalPresence = TRUE;
   return;
}
//
//
//      _plat__Signal_PhysicalPresenceOff()
//
//     Signal physical presence off
//
LIB_EXPORT void
_plat__Signal_PhysicalPresenceOff(
   void
   )
{
   s_physicalPresence = FALSE;
   return;
}
