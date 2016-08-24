// This file was extracted from the TCG Published
// Trusted Platform Module Library
// Part 4: Supporting Routines
// Family "2.0"
// Level 00 Revision 01.16
// October 30, 2014

#ifndef        PLATFORM_H
#define        PLATFORM_H
//
//
//          Includes and Defines
//
#include "bool.h"
#include "stdint.h"
#include "TpmError.h"
#include "TpmBuildSwitches.h"
#define UNREFERENCED(a) ((void)(a))
//
//
//          Power Functions
//
//          _plat__Signal_PowerOn
//
//     Signal power on This signal is simulate by a RPC call
//
LIB_EXPORT int
_plat__Signal_PowerOn(void);
//
//
//          _plat__Signal_Reset
//
//     Signal reset This signal is simulate by a RPC call
//
LIB_EXPORT int
_plat__Signal_Reset(void);
//
//
//          _plat__WasPowerLost()
//
//     Indicates if the power was lost before a _TPM__Init().
//
LIB_EXPORT BOOL
_plat__WasPowerLost(BOOL clear);
//
//
//          _plat__Signal_PowerOff()
//
//     Signal power off This signal is simulate by a RPC call
//
LIB_EXPORT void
_plat__Signal_PowerOff(void);
//
//
//          Physical Presence Functions
//
//          _plat__PhysicalPresenceAsserted()
//
//     Check if physical presence is signaled
//
//
//
//
//     Return Value                      Meaning
//
//     TRUE                              if physical presence is signaled
//     FALSE                             if physical presence is not signaled
//
LIB_EXPORT BOOL
_plat__PhysicalPresenceAsserted(void);
//
//
//         _plat__Signal_PhysicalPresenceOn
//
//     Signal physical presence on This signal is simulate by a RPC call
//
LIB_EXPORT void
_plat__Signal_PhysicalPresenceOn(void);
//
//
//         _plat__Signal_PhysicalPresenceOff()
//
//     Signal physical presence off This signal is simulate by a RPC call
//
LIB_EXPORT void
_plat__Signal_PhysicalPresenceOff(void);
//
//
//          Command Canceling Functions
//
//         _plat__IsCanceled()
//
//     Check if the cancel flag is set
//
//     Return Value                      Meaning
//
//     TRUE                              if cancel flag is set
//     FALSE                             if cancel flag is not set
//
LIB_EXPORT BOOL
_plat__IsCanceled(void);
//
//
//         _plat__SetCancel()
//
//     Set cancel flag.
//
LIB_EXPORT void
_plat__SetCancel(void);
//
//
//         _plat__ClearCancel()
//
//     Clear cancel flag
//
LIB_EXPORT void
_plat__ClearCancel( void);
//
//
//
//          NV memory functions
//
//         _plat__NvErrors()
//
//     This function is used by the simulator to set the error flags in the NV subsystem to simulate an error in the
//     NV loading process
//
LIB_EXPORT void
_plat__NvErrors(
    BOOL           recoverable,
    BOOL           unrecoverable
    );
//
//
//         _plat__NVEnable()
//
//     Enable platform NV memory NV memory is automatically enabled at power on event. This function is
//     mostly for TPM_Manufacture() to access NV memory without a power on event
//
//     Return Value                     Meaning
//
//     0                                if success
//     non-0                            if fail
//
LIB_EXPORT int
_plat__NVEnable(
    void      *platParameter                       // IN: platform specific parameters
);
//
//
//         _plat__NVDisable()
//
//     Disable platform NV memory NV memory is automatically disabled at power off event. This function is
//     mostly for TPM_Manufacture() to disable NV memory without a power off event
//
LIB_EXPORT void
_plat__NVDisable(void);
//
//
//         _plat__IsNvAvailable()
//
//     Check if NV is available
//
//     Return Value                     Meaning
//
//     0                                NV is available
//     1                                NV is not available due to write failure
//     2                                NV is not available due to rate limit
//
LIB_EXPORT int
_plat__IsNvAvailable(void);
//
//
//         _plat__NvCommit()
//
//     Update NV chip
//
//
//
//
//     Return Value                      Meaning
//
//     0                                 NV write success
//     non-0                             NV write fail
//
LIB_EXPORT int
_plat__NvCommit(void);
//
//
//         _plat__NvMemoryRead()
//
//     Read a chunk of NV memory
//
LIB_EXPORT void
_plat__NvMemoryRead(
    unsigned int              startOffset,                 // IN: read start
    unsigned int              size,                        // IN: size of bytes to read
    void                      *data                        // OUT: data buffer
);
//
//
//         _plat__NvIsDifferent()
//
//     This function checks to see if the NV is different from the test value. This is so that NV will not be written if
//     it has not changed.
//
//     Return Value                      Meaning
//
//     TRUE                              the NV location is different from the test value
//     FALSE                             the NV location is the same as the test value
//
LIB_EXPORT BOOL
_plat__NvIsDifferent(
    unsigned int               startOffset,                 // IN: read start
    unsigned int               size,                        // IN: size of bytes to compare
    void                      *data                         // IN: data buffer
    );
//
//
//         _plat__NvMemoryWrite()
//
//     Write a chunk of NV memory
//
LIB_EXPORT void
_plat__NvMemoryWrite(
    unsigned int              startOffset,                 // IN: read start
    unsigned int              size,                        // IN: size of bytes to read
    void                      *data                        // OUT: data buffer
);
//
//
//         _plat__NvMemoryMove()
//
//     Move a chunk of NV memory from source to destination This function should ensure that if there overlap,
//     the original data is copied before it is written
//
LIB_EXPORT void
_plat__NvMemoryMove(
    unsigned int              sourceOffset,                 // IN: source offset
    unsigned int              destOffset,                   // IN: destination offset
    unsigned int              size                          // IN: size of data being moved
);
//
//
//      _plat__SetNvAvail()
//
//     Set the current NV state to available. This function is for testing purposes only. It is not part of the
//     platform NV logic
//
LIB_EXPORT void
_plat__SetNvAvail(void);
//
//
//      _plat__ClearNvAvail()
//
//     Set the current NV state to unavailable. This function is for testing purposes only. It is not part of the
//     platform NV logic
//
LIB_EXPORT void
_plat__ClearNvAvail(void);
//
//
//          Locality Functions
//
//          _plat__LocalityGet()
//
//     Get the most recent command locality in locality value form
//
LIB_EXPORT unsigned char
_plat__LocalityGet(void);
//
//
//          _plat__LocalitySet()
//
//     Set the most recent command locality in locality value form
//
LIB_EXPORT void
_plat__LocalitySet(
    unsigned char      locality
);
//
//
//          _plat__IsRsaKeyCacheEnabled()
//
//     This function is used to check if the RSA key cache is enabled or not.
//
LIB_EXPORT int
_plat__IsRsaKeyCacheEnabled(
    void
    );
//
//
//          Clock Constants and Functions
//
//     Assume that the nominal divisor is 30000
//
#define        CLOCK_NOMINAL                30000
//
//     A 1% change in rate is 300 counts
//
#define        CLOCK_ADJUST_COARSE          300
//
//
//     A .1 change in rate is 30 counts
//
#define        CLOCK_ADJUST_MEDIUM            30
//
//     A minimum change in rate is 1 count
//
#define        CLOCK_ADJUST_FINE              1
//
//     The clock tolerance is +/-15% (4500 counts) Allow some guard band (16.7%)
//
#define        CLOCK_ADJUST_LIMIT             5000
//
//
//         _plat__ClockReset()
//
//     This function sets the current clock time as initial time. This function is called at a power on event to reset
//     the clock
//
LIB_EXPORT void
_plat__ClockReset(void);
//
//
//         _plat__ClockTimeFromStart()
//
//     Function returns the compensated                  time   from   the    start   of     the   command      when
//     _plat__ClockTimeFromStart() was called.
//
LIB_EXPORT unsigned long long
_plat__ClockTimeFromStart(
    void
    );
//
//
//         _plat__ClockTimeElapsed()
//
//     Get the time elapsed from current to the last time the _plat__ClockTimeElapsed() is called. For the first
//     _plat__ClockTimeElapsed() call after a power on event, this call report the elapsed time from power on to
//     the current call
//
LIB_EXPORT unsigned long long
_plat__ClockTimeElapsed(void);
//
//
//         _plat__ClockAdjustRate()
//
//     Adjust the clock rate
//
LIB_EXPORT void
_plat__ClockAdjustRate(
    int            adjust                    // IN: the adjust number.         It could be
                                             // positive or negative
    );
//
//
//
//           Single Function Files
//
//           _plat__GetEntropy()
//
//      This function is used to get available hardware entropy. In a hardware implementation of this function,
//      there would be no call to the system to get entropy. If the caller does not ask for any entropy, then this is
//      a startup indication and firstValue should be reset.
//
//      Return Value                     Meaning
//
//      <0                               hardware failure of the entropy generator, this is sticky
//      >= 0                             the returned amount of entropy (bytes)
//
LIB_EXPORT int32_t
_plat__GetEntropy(
      unsigned char          *entropy,                  // output buffer
      uint32_t                amount                    // amount requested
);

int uart_printf(const char *format, ...);
#define ecprintf(format, args...) uart_printf(format, ## args);

#endif
