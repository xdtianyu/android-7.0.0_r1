// This file was extracted from the TCG Published
// Trusted Platform Module Library
// Part 4: Supporting Routines
// Family "2.0"
// Level 00 Revision 01.16
// October 30, 2014

#define POWER_C
#include "InternalRoutines.h"
//          Functions
//
//           TPMInit()
//
//     This function is used to process a power on event.
//
void
TPMInit(
      void
      )
{
      // Set state as not initialized. This means that Startup is required
      s_initialized = FALSE;
      return;
}
//
//
//           TPMRegisterStartup()
//
//     This function registers the fact that the TPM has been initialized (a TPM2_Startup() has completed
//     successfully).
//
void
TPMRegisterStartup(
      void
      )
{
      s_initialized = TRUE;
      return;
}
//
//
//           TPMIsStarted()
//
//     Indicates if the TPM has been initialized (a TPM2_Startup() has completed successfully after a
//     _TPM_Init()).
//
//     Return Value                    Meaning
//
//     TRUE                            TPM has been initialized
//     FALSE                           TPM has not been initialized
//
BOOL
TPMIsStarted(
      void
      )
{
      return s_initialized;
}
