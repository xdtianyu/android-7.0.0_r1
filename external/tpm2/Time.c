// This file was extracted from the TCG Published
// Trusted Platform Module Library
// Part 4: Supporting Routines
// Family "2.0"
// Level 00 Revision 01.16
// October 30, 2014

#include "InternalRoutines.h"
#include "Platform.h"
//          Functions
//
//           TimePowerOn()
//
//     This function initialize time info at _TPM_Init().
//
void
TimePowerOn(
    void
    )
{
    TPM_SU               orderlyShutDown;
    // Read orderly data info from NV memory
    NvReadReserved(NV_ORDERLY_DATA, &go);
    // Read orderly shut down state flag
    NvReadReserved(NV_ORDERLY, &orderlyShutDown);
    // If the previous cycle is orderly shut down, the value of the safe bit
    // the same as previously saved. Otherwise, it is not safe.
    if(orderlyShutDown == SHUTDOWN_NONE)
        go.clockSafe= NO;
    else
        go.clockSafe = YES;
    // Set the initial state of the DRBG
    CryptDrbgGetPutState(PUT_STATE);
    // Clear time since TPM power on
    g_time = 0;
    return;
}
//
//
//           TimeStartup()
//
//     This function updates the resetCount and restartCount components of TPMS_CLOCK_INFO structure at
//     TPM2_Startup().
//
void
TimeStartup(
    STARTUP_TYPE          type                // IN: start up type
    )
{
    if(type == SU_RESUME)
    {
        // Resume sequence
        gr.restartCount++;
    }
    else
    {
        if(type == SU_RESTART)
        {
             // Hibernate sequence
             gr.clearCount++;
             gr.restartCount++;
        }
        else
        {
             // Reset sequence
             // Increase resetCount
             gp.resetCount++;
              // Write resetCount to NV
              NvWriteReserved(NV_RESET_COUNT, &gp.resetCount);
              gp.totalResetCount++;
              // We do not expect the total reset counter overflow during the life
              // time of TPM. if it ever happens, TPM will be put to failure mode
              // and there is no way to recover it.
              // The reason that there is no recovery is that we don't increment
              // the NV totalResetCount when incrementing would make it 0. When the
              // TPM starts up again, the old value of totalResetCount will be read
              // and we will get right back to here with the increment failing.
              if(gp.totalResetCount == 0)
                  FAIL(FATAL_ERROR_INTERNAL);
              // Write total reset counter to NV
              NvWriteReserved(NV_TOTAL_RESET_COUNT, &gp.totalResetCount);
              // Reset restartCount
              gr.restartCount = 0;
         }
   }
   return;
}
//
//
//             TimeUpdateToCurrent()
//
//      This function updates the Time and Clock in the global TPMS_TIME_INFO structure.
//      In this implementation, Time and Clock are updated at the beginning of each command and the values
//      are unchanged for the duration of the command.
//      Because Clock updates may require a write to NV memory, Time and Clock are not allowed to advance if
//      NV is not available. When clock is not advancing, any function that uses Clock will fail and return
//      TPM_RC_NV_UNAVAILABLE or TPM_RC_NV_RATE.
//      This implementations does not do rate limiting. If the implementation does do rate limiting, then the Clock
//      update should not be inhibited even when doing rather limiting.
//
void
TimeUpdateToCurrent(
   void
   )
{
   UINT64          oldClock;
   UINT64          elapsed;
#define CLOCK_UPDATE_MASK ((1ULL << NV_CLOCK_UPDATE_INTERVAL)- 1)
   // Can't update time during the dark interval or when rate limiting.
   if(NvIsAvailable() != TPM_RC_SUCCESS)
       return;
   // Save the old clock value
   oldClock = go.clock;
   // Update the time info to current
   elapsed = _plat__ClockTimeElapsed();
   go.clock += elapsed;
   g_time += elapsed;
   // Check to see if the update has caused a need for an nvClock update
   // CLOCK_UPDATE_MASK is measured by second, while the value in go.clock is
   // recorded by millisecond. Align the clock value to second before the bit
//
   // operations
   if( ((go.clock/1000) | CLOCK_UPDATE_MASK)
           > ((oldClock/1000) | CLOCK_UPDATE_MASK))
   {
       // Going to update the time state so the safe flag
       // should be set
       go.clockSafe = YES;
         // Get the DRBG state before updating orderly data
         CryptDrbgGetPutState(GET_STATE);
         NvWriteReserved(NV_ORDERLY_DATA, &go);
   }
   // Call self healing logic for dictionary attack parameters
   DASelfHeal();
   return;
}
//
//
//           TimeSetAdjustRate()
//
//      This function is used to perform rate adjustment on Time and Clock.
//
void
TimeSetAdjustRate(
   TPM_CLOCK_ADJUST          adjust            // IN: adjust constant
   )
{
   switch(adjust)
   {
       case TPM_CLOCK_COARSE_SLOWER:
           _plat__ClockAdjustRate(CLOCK_ADJUST_COARSE);
           break;
       case TPM_CLOCK_COARSE_FASTER:
           _plat__ClockAdjustRate(-CLOCK_ADJUST_COARSE);
           break;
       case TPM_CLOCK_MEDIUM_SLOWER:
           _plat__ClockAdjustRate(CLOCK_ADJUST_MEDIUM);
           break;
       case TPM_CLOCK_MEDIUM_FASTER:
           _plat__ClockAdjustRate(-CLOCK_ADJUST_MEDIUM);
           break;
       case TPM_CLOCK_FINE_SLOWER:
           _plat__ClockAdjustRate(CLOCK_ADJUST_FINE);
           break;
       case TPM_CLOCK_FINE_FASTER:
           _plat__ClockAdjustRate(-CLOCK_ADJUST_FINE);
           break;
       case TPM_CLOCK_NO_CHANGE:
           break;
       default:
           pAssert(FALSE);
           break;
   }
   return;
}
//
//
//           TimeGetRange()
//
//      This function is used to access TPMS_TIME_INFO. The TPMS_TIME_INFO structure is treaded as an
//      array of bytes, and a byte offset and length determine what bytes are returned.
//
//      Error Returns                   Meaning
//
//      TPM_RC_RANGE                    invalid data range
//
TPM_RC
TimeGetRange(
   UINT16              offset,             // IN: offset in TPMS_TIME_INFO
   UINT16              size,               // IN: size of data
   TIME_INFO          *dataBuffer          // OUT: result buffer
   )
{
   TPMS_TIME_INFO            timeInfo;
   UINT16                    infoSize;
   BYTE                      infoData[sizeof(TPMS_TIME_INFO)];
   BYTE                      *buffer;
   INT32                     bufferSize;
   // Fill TPMS_TIME_INFO structure
   timeInfo.time = g_time;
   TimeFillInfo(&timeInfo.clockInfo);
   // Marshal TPMS_TIME_INFO to canonical form
   buffer = infoData;
   bufferSize = sizeof(TPMS_TIME_INFO);
   infoSize = TPMS_TIME_INFO_Marshal(&timeInfo, &buffer, &bufferSize);
   // Check if the input range is valid
   if(offset + size > infoSize) return TPM_RC_RANGE;
   // Copy info data to output buffer
   MemoryCopy(dataBuffer, infoData + offset, size, sizeof(TIME_INFO));
   return TPM_RC_SUCCESS;
}
//
//
//          TimeFillInfo
//
//      This function gathers information to fill in a TPMS_CLOCK_INFO structure.
//
void
TimeFillInfo(
   TPMS_CLOCK_INFO           *clockInfo
   )
{
   clockInfo->clock = go.clock;
   clockInfo->resetCount = gp.resetCount;
   clockInfo->restartCount = gr.restartCount;
   // If NV is not available, clock stopped advancing and the value reported is
   // not "safe".
   if(NvIsAvailable() == TPM_RC_SUCCESS)
       clockInfo->safe = go.clockSafe;
   else
       clockInfo->safe = NO;
   return;
}
