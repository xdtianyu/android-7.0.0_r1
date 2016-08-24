// This file was extracted from the TCG Published
// Trusted Platform Module Library
// Part 3: Commands
// Family "2.0"
// Level 00 Revision 01.16
// October 30, 2014

#include "InternalRoutines.h"
#include "ClockSet_fp.h"
//
//     Read the current TPMS_TIMER_INFO structure settings
//
//     Error Returns                 Meaning
//
//     TPM_RC_VALUE                  invalid new clock
//
TPM_RC
TPM2_ClockSet(
   ClockSet_In       *in              // IN: input parameter list
   )
{
#define CLOCK_UPDATE_MASK    ((1ULL << NV_CLOCK_UPDATE_INTERVAL)- 1)
   UINT64      clockNow;

// Input Validation

   // new time can not be bigger than 0xFFFF000000000000 or smaller than
   // current clock
   if(in->newTime > 0xFFFF000000000000ULL
           || in->newTime < go.clock)
       return TPM_RC_VALUE + RC_ClockSet_newTime;

// Internal Data Update

   // Internal Data Update
   clockNow = go.clock;    // grab the old value
   go.clock = in->newTime;       // set the new value
   // Check to see if the update has caused a need for an nvClock update
   if((in->newTime & CLOCK_UPDATE_MASK) > (clockNow & CLOCK_UPDATE_MASK))
   {
       CryptDrbgGetPutState(GET_STATE);
       NvWriteReserved(NV_ORDERLY_DATA, &go);

       // Now the time state is safe
       go.clockSafe = YES;
   }

   return TPM_RC_SUCCESS;
}
