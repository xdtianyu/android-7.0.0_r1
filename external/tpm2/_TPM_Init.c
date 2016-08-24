// This file was extracted from the TCG Published
// Trusted Platform Module Library
// Part 3: Commands
// Family "2.0"
// Level 00 Revision 01.16
// October 30, 2014

#include "InternalRoutines.h"
LIB_EXPORT void
_TPM_Init(
   void
   )
{
   // Clear the failure mode flags
   g_inFailureMode = FALSE;
   g_forceFailureMode = FALSE;

   // Initialize the NvEnvironment.
   g_nvOk = NvPowerOn();

   // Initialize crypto engine
   CryptInitUnits();

   // Start clock
   TimePowerOn();

   // Set initialization state
   TPMInit();

   // Initialize object table
   ObjectStartup();

   // Set g_DRTMHandle as unassigned
   g_DRTMHandle = TPM_RH_UNASSIGNED;

   // No H-CRTM, yet.
   g_DrtmPreStartup = FALSE;

   return;
}
