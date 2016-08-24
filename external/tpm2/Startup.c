// This file was extracted from the TCG Published
// Trusted Platform Module Library
// Part 3: Commands
// Family "2.0"
// Level 00 Revision 01.16
// October 30, 2014

#include "InternalRoutines.h"
#include "Startup_fp.h"
#include "Unique_fp.h"
//
//
//     Error Returns                     Meaning
//
//     TPM_RC_LOCALITY                   a Startup(STATE) does not have the same H-CRTM state as the
//                                       previous Startup() or the locality of the startup is not 0 pr 3
//     TPM_RC_NV_UNINITIALIZED           the saved state cannot be recovered and a Startup(CLEAR) is
//                                       requried.
//     TPM_RC_VALUE                      start up type is not compatible with previous shutdown sequence
//
TPM_RC
TPM2_Startup(
   Startup_In        *in                 // IN: input parameter list
   )
{
   STARTUP_TYPE            startup;
   TPM_RC                  result;
   BOOL                    prevDrtmPreStartup;
   BOOL                    prevStartupLoc3;
   BYTE                    locality = _plat__LocalityGet();

   // In the PC Client specification, only locality 0 and 3 are allowed
   if(locality != 0 && locality != 3)
       return TPM_RC_LOCALITY;
   // Indicate that the locality was 3 unless there was an H-CRTM
   if(g_DrtmPreStartup)
       locality = 0;
   g_StartupLocality3 = (locality == 3);

   // The command needs NV update. Check if NV is available.
   // A TPM_RC_NV_UNAVAILABLE or TPM_RC_NV_RATE error may be returned at
   // this point
   result = NvIsAvailable();
   if(result != TPM_RC_SUCCESS)
       return result;
// Input Validation

   // Read orderly shutdown states from previous power cycle
   NvReadReserved(NV_ORDERLY, &g_prevOrderlyState);

   // See if the orderly state indicates that state was saved
   if(     (g_prevOrderlyState & ~(PRE_STARTUP_FLAG | STARTUP_LOCALITY_3))
       == TPM_SU_STATE)
   {
       // If so, extrat the saved flags (HACK)
       prevDrtmPreStartup = (g_prevOrderlyState & PRE_STARTUP_FLAG) != 0;
       prevStartupLoc3 = (g_prevOrderlyState & STARTUP_LOCALITY_3) != 0;
       g_prevOrderlyState = TPM_SU_STATE;
   }
   else
   {
       prevDrtmPreStartup = 0;
       prevStartupLoc3 = 0;
   }
   // if this startup is a TPM Resume, then the H-CRTM states have to match.
   if(in->startupType == TPM_SU_STATE)
  {
         if(g_DrtmPreStartup != prevDrtmPreStartup)
             return TPM_RC_VALUE + RC_Startup_startupType;
         if(g_StartupLocality3 != prevStartupLoc3)
             return TPM_RC_LOCALITY;
  }
  // if the previous power cycle was shut down with no StateSave command, or
  // with StateSave command for CLEAR, or the part of NV used for TPM_SU_STATE
  // cannot be recovered, then this cycle can not startup up with STATE
  if(in->startupType == TPM_SU_STATE)
  {
      if(     g_prevOrderlyState == SHUTDOWN_NONE
         ||   g_prevOrderlyState == TPM_SU_CLEAR)
          return TPM_RC_VALUE + RC_Startup_startupType;

         if(g_nvOk == FALSE)
             return TPM_RC_NV_UNINITIALIZED;
  }

// Internal Date Update

  // Translate the TPM2_ShutDown and TPM2_Startup sequence into the startup
  // types. Will only be a SU_RESTART if the NV is OK
  if(     in->startupType == TPM_SU_CLEAR
      && g_prevOrderlyState == TPM_SU_STATE
      && g_nvOk == TRUE)
  {
      startup = SU_RESTART;
      // Read state reset data
      NvReadReserved(NV_STATE_RESET, &gr);
  }
  // In this check, we don't need to look at g_nvOk because that was checked
  // above
  else if(in->startupType == TPM_SU_STATE && g_prevOrderlyState == TPM_SU_STATE)
  {
      // Read state clear and state reset data
      NvReadReserved(NV_STATE_CLEAR, &gc);
      NvReadReserved(NV_STATE_RESET, &gr);
      startup = SU_RESUME;
  }
  else
  {
      startup = SU_RESET;
  }

  // Read persistent data from NV
  NvReadPersistent();

  // Crypto Startup
  CryptUtilStartup(startup);

  // Read the platform unique value that is used as VENDOR_PERMANENT auth value
  g_platformUniqueDetails.t.size = (UINT16)_plat__GetUnique(1,
                                     sizeof(g_platformUniqueDetails.t.buffer),
                                     g_platformUniqueDetails.t.buffer);

  // Start up subsystems
  // Start counters and timers
  TimeStartup(startup);

  // Start dictionary attack subsystem
  DAStartup(startup);

  // Enable hierarchies
  HierarchyStartup(startup);

   // Restore/Initialize PCR
   PCRStartup(startup, locality);

   // Restore/Initialize command audit information
   CommandAuditStartup(startup);

   // Object context variables
   if(startup == SU_RESET)
   {
       // Reset object context ID to 0
       gr.objectContextID = 0;
       // Reset clearCount to 0
       gr.clearCount= 0;
   }

   // Initialize session table
   SessionStartup(startup);

   // Initialize index/evict data.   This function clear read/write locks
   // in NV index
   NvEntityStartup(startup);

   // Initialize the orderly shut down flag for this cycle to SHUTDOWN_NONE.
   gp.orderlyState = SHUTDOWN_NONE;
   NvWriteReserved(NV_ORDERLY, &gp.orderlyState);

   // Update TPM internal states if command succeeded.
   // Record a TPM2_Startup command has been received.
   TPMRegisterStartup();

   // The H-CRTM state no longer matters
   g_DrtmPreStartup = FALSE;

   return TPM_RC_SUCCESS;
}
