// This file was extracted from the TCG Published
// Trusted Platform Module Library
// Part 4: Supporting Routines
// Family "2.0"
// Level 00 Revision 01.16
// October 30, 2014

#define DA_C
#include "InternalRoutines.h"
//
//
//           Functions
//
//            DAPreInstall_Init()
//
//      This function initializes the DA parameters to their manufacturer-default values. The default values are
//      determined by a platform-specific specification.
//      This function should not be called outside of a manufacturing or simulation environment.
//      The DA parameters will be restored to these initial values by TPM2_Clear().
//
void
DAPreInstall_Init(
     void
     )
{
     gp.failedTries = 0;
     gp.maxTries = 3;
     gp.recoveryTime = 1000;                  // in seconds (~16.67 minutes)
     gp.lockoutRecovery = 1000;               // in seconds
     gp.lockOutAuthEnabled = TRUE;            // Use of lockoutAuth is enabled
     // Record persistent DA parameter changes to NV
     NvWriteReserved(NV_FAILED_TRIES, &gp.failedTries);
     NvWriteReserved(NV_MAX_TRIES, &gp.maxTries);
     NvWriteReserved(NV_RECOVERY_TIME, &gp.recoveryTime);
     NvWriteReserved(NV_LOCKOUT_RECOVERY, &gp.lockoutRecovery);
     NvWriteReserved(NV_LOCKOUT_AUTH_ENABLED, &gp.lockOutAuthEnabled);
    return;
}
//
//
//          DAStartup()
//
//     This function is called by TPM2_Startup() to initialize the DA parameters. In the case of Startup(CLEAR),
//     use of lockoutAuth will be enabled if the lockout recovery time is 0. Otherwise, lockoutAuth will not be
//     enabled until the TPM has been continuously powered for the lockoutRecovery time.
//     This function requires that NV be available and not rate limiting.
//
void
DAStartup(
    STARTUP_TYPE         type               // IN: startup type
    )
{
    // For TPM Reset, if lockoutRecovery is 0, enable use of lockoutAuth.
    if(type == SU_RESET)
    {
        if(gp.lockoutRecovery == 0)
        {
            gp.lockOutAuthEnabled = TRUE;
            // Record the changes to NV
            NvWriteReserved(NV_LOCKOUT_AUTH_ENABLED, &gp.lockOutAuthEnabled);
        }
    }
    // If DA has not been disabled and the previous shutdown is not orderly
    // failedTries is not already at its maximum then increment 'failedTries'
    if(    gp.recoveryTime != 0
        && g_prevOrderlyState == SHUTDOWN_NONE
        && gp.failedTries < gp.maxTries)
    {
        gp.failedTries++;
        // Record the change to NV
        NvWriteReserved(NV_FAILED_TRIES, &gp.failedTries);
    }
    // Reset self healing timers
    s_selfHealTimer = g_time;
    s_lockoutTimer = g_time;
    return;
}
//
//
//          DARegisterFailure()
//
//     This function is called when a authorization failure occurs on an entity that is subject to dictionary-attack
//     protection. When a DA failure is triggered, register the failure by resetting the relevant self-healing timer
//     to the current time.
//
void
DARegisterFailure(
    TPM_HANDLE           handle             // IN: handle for failure
    )
{
    // Reset the timer associated with lockout if the handle is the lockout auth.
    if(handle == TPM_RH_LOCKOUT)
         s_lockoutTimer = g_time;
    else
         s_selfHealTimer = g_time;
//
   return;
}
//
//
//             DASelfHeal()
//
//      This function is called to check if sufficient time has passed to allow decrement of failedTries or to re-
//      enable use of lockoutAuth.
//      This function should be called when the time interval is updated.
//
void
DASelfHeal(
   void
   )
{
   // Regular auth self healing logic
   // If no failed authorization tries, do nothing. Otherwise, try to
   // decrease failedTries
   if(gp.failedTries != 0)
   {
       // if recovery time is 0, DA logic has been disabled. Clear failed tries
       // immediately
       if(gp.recoveryTime == 0)
       {
            gp.failedTries = 0;
            // Update NV record
            NvWriteReserved(NV_FAILED_TRIES, &gp.failedTries);
       }
       else
       {
            UINT64          decreaseCount;
               // In the unlikely event that failedTries should become larger than
               // maxTries
               if(gp.failedTries > gp.maxTries)
                   gp.failedTries = gp.maxTries;
               // How much can failedTried be decreased
               decreaseCount = ((g_time - s_selfHealTimer) / 1000) / gp.recoveryTime;
               if(gp.failedTries <= (UINT32) decreaseCount)
                   // should not set failedTries below zero
                   gp.failedTries = 0;
               else
                   gp.failedTries -= (UINT32) decreaseCount;
               // the cast prevents overflow of the product
               s_selfHealTimer += (decreaseCount * (UINT64)gp.recoveryTime) * 1000;
               if(decreaseCount != 0)
                   // If there was a change to the failedTries, record the changes
                   // to NV
                   NvWriteReserved(NV_FAILED_TRIES, &gp.failedTries);
         }
   }
   // LockoutAuth self healing logic
   // If lockoutAuth is enabled, do nothing. Otherwise, try to see if we
   // may enable it
   if(!gp.lockOutAuthEnabled)
   {
       // if lockout authorization recovery time is 0, a reboot is required to
       // re-enable use of lockout authorization. Self-healing would not
       // apply in this case.
       if(gp.lockoutRecovery != 0)
//
           {
                 if(((g_time - s_lockoutTimer)/1000) >= gp.lockoutRecovery)
                 {
                     gp.lockOutAuthEnabled = TRUE;
                     // Record the changes to NV
                     NvWriteReserved(NV_LOCKOUT_AUTH_ENABLED, &gp.lockOutAuthEnabled);
                 }
           }
     }
     return;
}
