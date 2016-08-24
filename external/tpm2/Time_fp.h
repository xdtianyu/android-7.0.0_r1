/*
 * Copyright 2015 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef __TPM2_TIME_FP_H
#define __TPM2_TIME_FP_H

void TimeFillInfo(TPMS_CLOCK_INFO *clockInfo);
TPM_RC TimeGetRange(UINT16 offset,         // IN: offset in TPMS_TIME_INFO
                    UINT16 size,           // IN: size of data
                    TIME_INFO *dataBuffer  // OUT: result buffer
                    );
void TimePowerOn(void);
void TimeSetAdjustRate(TPM_CLOCK_ADJUST adjust  // IN: adjust constant
                       );
void TimeStartup(STARTUP_TYPE type  // IN: start up type
                 );
void TimeUpdateToCurrent(void);

#endif  // __TPM2_TIME_FP_H
