// This file was extracted from the TCG Published
// Trusted Platform Module Library
// Part 3: Commands
// Family "2.0"
// Level 00 Revision 01.16
// October 30, 2014

#include "InternalRoutines.h"
#include "ReadClock_fp.h"
TPM_RC
TPM2_ReadClock(
   ReadClock_Out    *out            // OUT: output parameter list
   )
{
// Command Output
   out->currentTime.time = g_time;
   TimeFillInfo(&out->currentTime.clockInfo);

#ifndef EMBEDDED_MODE
   {
       UINT64 start_time = _plat__ClockTimeFromStart();
       // When running on a simulator, some tests fail, because two commands
       // invoked back to back happen to run within the same millisecond, but
       // the test expects time readings to be different. Modifying the tests
       // is more involved, let's just wait a couple of milliseconds here to
       // avoid those tests' false negatives.
       while ((_plat__ClockTimeFromStart() - start_time) < 2)
           ;
   }
#endif
   return TPM_RC_SUCCESS;
}
