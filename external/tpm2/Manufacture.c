// This file was extracted from the TCG Published
// Trusted Platform Module Library
// Part 4: Supporting Routines
// Family "2.0"
// Level 00 Revision 01.16
// October 30, 2014

#define MANUFACTURE_C
#include "InternalRoutines.h"
#include "Global.h"
//
//
//          Functions
//
//         TPM_Manufacture()
//
//     This function initializes the TPM values in preparation for the TPM's first use. This function will fail if
//     previously called. The TPM can be re-manufactured by calling TPM_Teardown() first and then calling this
//     function again.
//
//     Return Value                      Meaning
//
//     0                                 success
//     1                                 manufacturing process previously performed
//
LIB_EXPORT int
TPM_Manufacture(
   BOOL                 firstTime           // IN: indicates if this is the first call from
                                            //     main()
   )
{
   TPM_SU              orderlyShutdown;
   UINT64              totalResetCount = 0;
   // If TPM has been manufactured, return indication.
   if(!firstTime && g_manufactured)
       return 1;
   // initialize crypto units
   //CryptInitUnits();
   //
   s_selfHealTimer = 0;
   s_lockoutTimer = 0;
   s_DAPendingOnNV = FALSE;
   // initialize NV
   NvInit();
#ifdef _DRBG_STATE_SAVE
   // Initialize the drbg. This needs to come before the install
   // of the hierarchies
   if(!_cpri__Startup())               // Have to start the crypto units first
       FAIL(FATAL_ERROR_INTERNAL);
   _cpri__DrbgGetPutState(PUT_STATE, 0, NULL);
#endif
   // default configuration for PCR
   PCRSimStart();
   // initialize pre-installed hierarchy data
   // This should happen after NV is initialized because hierarchy data is
   // stored in NV.
   HierarchyPreInstall_Init();
   // initialize dictionary attack parameters
   DAPreInstall_Init();
   // initialize PP list
   PhysicalPresencePreInstall_Init();
   // initialize command audit list
   CommandAuditPreInstall_Init();
   // first start up is required to be Startup(CLEAR)
    orderlyShutdown = TPM_SU_CLEAR;
    NvWriteReserved(NV_ORDERLY, &orderlyShutdown);
   // initialize the firmware version
   gp.firmwareV1 = FIRMWARE_V1;
#ifdef FIRMWARE_V2
   gp.firmwareV2 = FIRMWARE_V2;
#else
   gp.firmwareV2 = 0;
#endif
   NvWriteReserved(NV_FIRMWARE_V1, &gp.firmwareV1);
   NvWriteReserved(NV_FIRMWARE_V2, &gp.firmwareV2);
    // initialize the total reset counter to 0
    NvWriteReserved(NV_TOTAL_RESET_COUNT, &totalResetCount);
    // initialize the clock stuff
    go.clock = 0;
    go.clockSafe = YES;
#ifdef _DRBG_STATE_SAVE
   // initialize the current DRBG state in NV
   _cpri__DrbgGetPutState(GET_STATE, sizeof(go.drbgState), (BYTE *)&go.drbgState);
#endif
    NvWriteReserved(NV_ORDERLY_DATA, &go);
    // Commit NV writes. Manufacture process is an artificial process existing
    // only in simulator environment and it is not defined in the specification
    // that what should be the expected behavior if the NV write fails at this
    // point. Therefore, it is assumed the NV write here is always success and
    // no return code of this function is checked.
    NvCommit();
    g_manufactured = TRUE;
    return 0;
}
//
//
//          TPM_TearDown()
//
//      This function prepares the TPM for re-manufacture. It should not be implemented in anything other than a
//      simulated TPM.
//      In this implementation, all that is needs is to stop the cryptographic units and set a flag to indicate that the
//      TPM can be re-manufactured. This should be all that is necessary to start the manufacturing process
//      again.
//
//      Return Value                      Meaning
//
//      0                                 success
//      1                                 TPM not previously manufactured
//
LIB_EXPORT int
TPM_TearDown(
    void
    )
{
    // stop crypt units
    CryptStopUnits();
    g_manufactured = FALSE;
      return 0;
}
