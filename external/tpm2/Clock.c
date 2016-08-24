// This file was extracted from the TCG Published
// Trusted Platform Module Library
// Part 4: Supporting Routines
// Family "2.0"
// Level 00 Revision 01.16
// October 30, 2014

#include "PlatformData.h"
#include "Platform.h"

#ifdef __linux__

#include <sys/time.h>
// Function clock() does not provide accurate wall clock time on linux, let's
// substitite it with our own caclulations.
//
// Return current wall clock modulo milliseconds.
static UINT64 clock(void)
{
  struct timeval tv;
  gettimeofday(&tv, NULL);
  return (UINT64)tv.tv_sec * 1000 + tv.tv_usec / 1000;
}
#else
#include <time.h>
#endif
//
//
//          Functions
//
//          _plat__ClockReset()
//
//     Set the current clock time as initial time. This function is called at a power on event to reset the clock
//
LIB_EXPORT void
_plat__ClockReset(
     void
     )
{
     // Implementation specific: Microsoft C set CLOCKS_PER_SEC to be 1/1000,
     // so here the measurement of clock() is in millisecond.
     s_initClock = clock();
     s_adjustRate = CLOCK_NOMINAL;
     return;
}
//
//
//          _plat__ClockTimeFromStart()
//
//     Function returns the compensated                time    from    the    start    of   the    command      when
//     _plat__ClockTimeFromStart() was called.
//
unsigned long long
_plat__ClockTimeFromStart(
     void
     )
{
     unsigned long long currentClock = clock();
     return ((currentClock - s_initClock) * CLOCK_NOMINAL) / s_adjustRate;
}
//
//
//          _plat__ClockTimeElapsed()
//
//     Get the time elapsed from current to the last time the _plat__ClockTimeElapsed() is called. For the first
//     _plat__ClockTimeElapsed() call after a power on event, this call report the elapsed time from power on to
//     the current call
//
LIB_EXPORT unsigned long long
_plat__ClockTimeElapsed(
     void
//
    )
{
    unsigned long long elapsed;
    unsigned long long currentClock = clock();
    elapsed = ((currentClock - s_initClock) * CLOCK_NOMINAL) / s_adjustRate;
    s_initClock += (elapsed * s_adjustRate) / CLOCK_NOMINAL;
#ifdef DEBUGGING_TIME
   // Put this in so that TPM time will pass much faster than real time when
   // doing debug.
   // A value of 1000 for DEBUG_TIME_MULTIPLER will make each ms into a second
   // A good value might be 100
   elapsed *= DEBUG_TIME_MULTIPLIER
#endif
              return elapsed;
}
//
//
//        _plat__ClockAdjustRate()
//
//     Adjust the clock rate
//
LIB_EXPORT void
_plat__ClockAdjustRate(
    int                adjust         // IN: the adjust number.   It could be positive
                                      //     or negative
    )
{
    // We expect the caller should only use a fixed set of constant values to
    // adjust the rate
    switch(adjust)
    {
        case CLOCK_ADJUST_COARSE:
            s_adjustRate += CLOCK_ADJUST_COARSE;
            break;
        case -CLOCK_ADJUST_COARSE:
            s_adjustRate -= CLOCK_ADJUST_COARSE;
            break;
        case CLOCK_ADJUST_MEDIUM:
            s_adjustRate += CLOCK_ADJUST_MEDIUM;
            break;
        case -CLOCK_ADJUST_MEDIUM:
            s_adjustRate -= CLOCK_ADJUST_MEDIUM;
            break;
        case CLOCK_ADJUST_FINE:
            s_adjustRate += CLOCK_ADJUST_FINE;
            break;
        case -CLOCK_ADJUST_FINE:
            s_adjustRate -= CLOCK_ADJUST_FINE;
            break;
        default:
            // ignore any other values;
            break;
    }
    if(s_adjustRate > (CLOCK_NOMINAL + CLOCK_ADJUST_LIMIT))
        s_adjustRate = CLOCK_NOMINAL + CLOCK_ADJUST_LIMIT;
    if(s_adjustRate < (CLOCK_NOMINAL - CLOCK_ADJUST_LIMIT))
        s_adjustRate = CLOCK_NOMINAL-CLOCK_ADJUST_LIMIT;
    return;
}
