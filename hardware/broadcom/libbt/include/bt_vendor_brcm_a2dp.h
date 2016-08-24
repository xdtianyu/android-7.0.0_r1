/******************************************************************************
 *
 *  Copyright (C) 2015 Motorola Corporation
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at:
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 ******************************************************************************/

/******************************************************************************
 *
 *  Filename:      bt_vendor_brcm_a2dp.h
 *
 *  Description:   Contains definitions specific for interfacing with Broadcom
 *                 Bluetooth chipsets for A2DP Offload implementation.
 *
 ******************************************************************************/

#ifndef BT_VENDOR_BRCM_A2DP_H
#define BT_VENDOR_BRCM_A2DP_H

#include "bt_vendor_brcm.h"
#include "bt_target.h"
#include "uipc_msg.h"

/******************************************************************************
**  Constants & Macros
******************************************************************************/

#define HCI_VSC_WRITE_PCM_PINS  0xFC61
#define HCI_VSC_READ_PCM_PINS   0xFC62
#define HCI_VSC_UIPC_OVER_HCI   0xFC8B

/* pinmux for I2S pins */
#define PCM_PIN_FCN_GPIO 0x00
#define PCM_PIN_FCN_PCM  0x01
#define PCM_PIN_FCN_I2S_MASTER 0x05
#define PCM_PIN_FCN_I2S_SLAVE  0x07
#define PCM_PIN_FCN_INVALID    0xFF

/* PADCONF for I2S pins */
/* From LSB, byte map to DIN, DOUT, WS, CLK */
/*
bit 0:   0 OUTPUT, 1 INPUT
bit 1:   0 NO-PULL,1 PULL-UP
bit 2:   0 NO-PULL,1 PULL-DN
bit 3:   1 SHMITT Trigger Enable
bit 4-7:   Drive Strength
*/
/* Define standard Master & Slave I2S PADCONFs */
#define PCM_PIN_PADCNF_I2S_SLAVE  0x19191819
#define PCM_PIN_PADCNF_I2S_MASTER 0x18181819

#define HCI_EVT_CMD_CMPL_LEN    1
#define HCI_EVT_CMD_CMPL_VSC    3
#define HCI_CMD_PREAMBLE_SIZE   3
#define HCI_CMD_MAX_LEN         258

#define UNUSED(x) (void)(x)

#if (BRCM_A2DP_OFFLOAD != TRUE)
#define BRCM_A2DP_OFFLOAD    FALSE
#endif

/* A2DP offload parameters from vnd_<prod>.txt */

#ifndef BRCM_A2DP_OFFLOAD_SRC
#define BRCM_A2DP_OFFLOAD_SRC  AUDIO_ROUTE_SRC_I2S
#endif

#ifndef BRCM_A2DP_OFFLOAD_SRC_SF
#define BRCM_A2DP_OFFLOAD_SRC_SF  AUDIO_ROUTE_SF_48K
#endif

#ifndef BRCM_A2DP_OFFLOAD_MAX_BITPOOL
/* High quality setting @ 44.1 kHz */
#define BRCM_A2DP_OFFLOAD_MAX_BITPOOL 53
#endif

#ifndef BRCM_A2DP_OFFLOAD_PCM_PIN_FCN
#define BRCM_A2DP_OFFLOAD_PCM_PIN_FCN PCM_PIN_FCN_I2S_SLAVE
#endif

#ifndef BRCM_A2DP_OFFLOAD_PCM_PIN_PADCNF
#if (BRCM_A2DP_OFFLOAD_PCM_PIN_FCN == PCM_PIN_FCN_I2S_MASTER)
#define BRCM_A2DP_OFFLOAD_PCM_PIN_PADCNF  PCM_PIN_PADCNF_I2S_MASTER
#else
#define BRCM_A2DP_OFFLOAD_PCM_PIN_PADCNF  PCM_PIN_PADCNF_I2S_SLAVE
#endif
#endif

#define MULTI_BIT_SET(x) !!(x & (x - 1))

void brcm_vnd_a2dp_init();
int brcm_vnd_a2dp_execute(bt_vendor_opcode_t, void *ev_data);

#endif /*BT_VENDOR_BRCM_A2DP_H*/
