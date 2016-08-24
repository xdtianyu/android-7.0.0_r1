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
 *  This is the stream state machine for the BRCM offloaded advanced audio.
 *
 ******************************************************************************/

#define LOG_TAG "bt_vnd_a2dp"
#define LOG_NDEBUG 0

#include <string.h>
#include <pthread.h>
#include <utils/Log.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <signal.h>
#include <time.h>
#include <errno.h>
#include <fcntl.h>
#include <dirent.h>
#include <ctype.h>
#include <cutils/properties.h>
#include <stdlib.h>
#include "bt_hci_bdroid.h"
#include "bt_vendor_brcm_a2dp.h"
#include "a2d_api.h"
#include "a2d_sbc.h"

#if (BTA2DP_DEBUG == TRUE)
#define BTA2DPDBG(param, ...) {ALOGD(param, ## __VA_ARGS__);}
#else
#define BTA2DPDBG(param, ...) {}
#endif

/*****************************************************************************
** Constants and types
*****************************************************************************/

typedef void (*hci_cback)(void *);

typedef enum
{
    BRCM_VND_A2DP_OFFLOAD_INIT_REQ,
    BRCM_VND_A2DP_OFFLOAD_START_REQ,
    BRCM_VND_A2DP_OFFLOAD_STOP_REQ,
    BRCM_VND_UIPC_OPEN_RSP,
    BRCM_VND_L2C_SYNC_TO_LITE_RSP,
    BRCM_VND_SYNC_TO_BTC_LITE_RSP,
    BRCM_VND_AUDIO_CODEC_CONFIG_RSP,
    BRCM_VND_AUDIO_ROUTE_CONFIG_RSP,
    BRCM_VND_UIPC_CLOSE_RSP,
    BRCM_VND_L2C_REMOVE_TO_LITE_RSP,
    BRCM_VND_A2DP_START_RSP,
    BRCM_VND_A2DP_SUSPEND_RSP,
    BRCM_VND_STREAM_STOP_RSP,
    BRCM_VND_A2DP_CLEANUP_RSP,
    BRCM_VND_A2DP_OFFLOAD_FAILED_ABORT,
} tBRCM_VND_A2DP_EVENT;

/* state machine states */
typedef enum
{
    BRCM_VND_A2DP_INVALID_SST = -1,
    BRCM_VND_A2DP_IDLE_SST,
    BRCM_VND_A2DP_STARTING_SST,
    BRCM_VND_A2DP_STREAM_SST,
}
tBRCM_VND_A2DP_SST_STATES;

static uint8_t brcm_vnd_a2dp_offload_configure();
static uint8_t brcm_vnd_a2dp_offload_cleanup();
static uint8_t brcm_vnd_a2dp_offload_suspend();
static tBRCM_VND_A2DP_SST_STATES brcm_vnd_a2dp_sm_idle_process_ev(tBRCM_VND_A2DP_EVENT event, void *ev_data);
static tBRCM_VND_A2DP_SST_STATES brcm_vnd_a2dp_sm_starting_process_ev(tBRCM_VND_A2DP_EVENT event, void *ev_data);
static tBRCM_VND_A2DP_SST_STATES brcm_vnd_a2dp_sm_stream_process_ev(tBRCM_VND_A2DP_EVENT event, void *ev_data);
static void brcm_vnd_a2dp_hci_uipc_cback(void *pmem);

typedef struct {
    uint8_t     fcn;
    uint32_t    pad_conf;
}
tBRCM_VND_PCM_CONF;

typedef struct {
    tBRCM_VND_A2DP_SST_STATES state;
    tCODEC_INFO_SBC codec_info;
    tBRCM_VND_PCM_CONF pcmi2s_pinmux;
    bt_vendor_op_a2dp_offload_t offload_params;
}
tBRCM_VND_A2DP_PDATA;

typedef struct {
    tBRCM_VND_A2DP_SST_STATES (*enter)(tBRCM_VND_A2DP_EVENT event);
    tBRCM_VND_A2DP_SST_STATES (*process_event)(tBRCM_VND_A2DP_EVENT event, void *ev_data);
}
tBRCM_VND_A2DP_SST_STATE;

/* state table */
static tBRCM_VND_A2DP_SST_STATE brcm_vnd_a2dp_sst_tbl[] =
{
    {NULL, brcm_vnd_a2dp_sm_idle_process_ev},
    {NULL, brcm_vnd_a2dp_sm_starting_process_ev},
    {NULL, brcm_vnd_a2dp_sm_stream_process_ev},
};

static pthread_mutex_t g_mutex = PTHREAD_MUTEX_INITIALIZER;
static tBRCM_VND_A2DP_PDATA brcm_vnd_a2dp_pdata = { .state = BRCM_VND_A2DP_INVALID_SST };


/*******************************************************************************
** Local Utility Functions
*******************************************************************************/

static void log_bin_to_hexstr(uint8_t *bin, uint8_t binsz, const char *log_tag)
{
#if (BTA2DP_DEBUG == TRUE)
  char     *str, hex_str[]= "0123456789abcdef";
  uint8_t  i;

  str = (char *)malloc(binsz * 3);
  if (!binsz) {
    ALOGE("%s alloc failed", __FUNCTION__);
    return;
  }

  for (i = 0; i < binsz; i++) {
      str[(i * 3) + 0] = hex_str[(bin[i] >> 4) & 0x0F];
      str[(i * 3) + 1] = hex_str[(bin[i]     ) & 0x0F];
      str[(i * 3) + 2] = ' ';
  }
  str[(binsz * 3) - 1] = 0x00;
  BTA2DPDBG("%s %s", log_tag, str);
#endif
}

static uint8_t brcm_vnd_a2dp_send_hci_vsc(uint16_t cmd, uint8_t *payload, uint8_t len, hci_cback cback)
{
    HC_BT_HDR   *p_buf;
    uint8_t     *p, status;
    uint16_t    opcode;

    // Perform Opening configure cmds. //
    if (bt_vendor_cbacks) {
        p_buf = (HC_BT_HDR *)bt_vendor_cbacks->alloc(
            BT_HC_HDR_SIZE + HCI_CMD_PREAMBLE_SIZE + len);
        if (p_buf)
        {
            p_buf->event = MSG_STACK_TO_HC_HCI_CMD;
            p_buf->offset = 0;
            p_buf->layer_specific = 0;
            p_buf->len = HCI_CMD_PREAMBLE_SIZE + len;
            p = (uint8_t *)(p_buf + 1);

            UINT16_TO_STREAM(p, cmd);
            *p++ = len;
            memcpy(p, payload, len);

            //BTA2DPDBG("%s Cmd %04x UIPC Event %02x%02x UIPC Op %02x Len %d", __FUNCTION__, cmd, event, payload[1], payload[0], payload[2], len);    
            log_bin_to_hexstr((uint8_t *)(p_buf + 1), HCI_CMD_PREAMBLE_SIZE + len, __FUNCTION__);

            if (bt_vendor_cbacks->xmit_cb(cmd, p_buf, cback))
            {
                return BT_VND_OP_RESULT_SUCCESS;
            }
            bt_vendor_cbacks->dealloc(p_buf);
        }
    }
    return BT_VND_OP_RESULT_FAIL;
}

static void brcm_vnd_map_a2d_uipc_codec_info(tCODEC_INFO_SBC *codec_info)
{
    switch(codec_info->sampling_freq) {
        case A2D_SBC_IE_SAMP_FREQ_16:
            codec_info->sampling_freq = CODEC_INFO_SBC_SF_16K; break;
        case A2D_SBC_IE_SAMP_FREQ_32:
            codec_info->sampling_freq = CODEC_INFO_SBC_SF_32K; break;
        case A2D_SBC_IE_SAMP_FREQ_44:
            codec_info->sampling_freq = CODEC_INFO_SBC_SF_44K; break;
        case A2D_SBC_IE_SAMP_FREQ_48:
            codec_info->sampling_freq = CODEC_INFO_SBC_SF_48K; break;

    }
    switch(codec_info->channel_mode) {
        case A2D_SBC_IE_CH_MD_MONO:
            codec_info->channel_mode = CODEC_INFO_SBC_CH_MONO; break;
        case A2D_SBC_IE_CH_MD_DUAL:
            codec_info->channel_mode = CODEC_INFO_SBC_CH_DUAL; break;
        case A2D_SBC_IE_CH_MD_STEREO:
            codec_info->channel_mode = CODEC_INFO_SBC_CH_STEREO; break;
        case A2D_SBC_IE_CH_MD_JOINT:
            codec_info->channel_mode = CODEC_INFO_SBC_CH_JS; break;
    }
    switch(codec_info->block_length) {
        case A2D_SBC_IE_BLOCKS_4:
            codec_info->block_length = CODEC_INFO_SBC_BLOCK_4; break;
        case A2D_SBC_IE_BLOCKS_8:
            codec_info->block_length = CODEC_INFO_SBC_BLOCK_8; break;
        case A2D_SBC_IE_BLOCKS_12:
            codec_info->block_length = CODEC_INFO_SBC_BLOCK_12; break;
        case A2D_SBC_IE_BLOCKS_16:
            codec_info->block_length = CODEC_INFO_SBC_BLOCK_16; break;
    }
    switch(codec_info->alloc_method) {
        case A2D_SBC_IE_ALLOC_MD_S:
            codec_info->alloc_method = CODEC_INFO_SBC_ALLOC_SNR; break;
        case A2D_SBC_IE_ALLOC_MD_L:
            codec_info->alloc_method = CODEC_INFO_SBC_ALLOC_LOUDNESS; break;
    }
    switch(codec_info->num_subbands) {
        case A2D_SBC_IE_SUBBAND_4:
            codec_info->num_subbands = CODEC_INFO_SBC_SUBBAND_4; break;
        case A2D_SBC_IE_SUBBAND_8:
            codec_info->num_subbands = CODEC_INFO_SBC_SUBBAND_8; break;
    }
}

static tA2D_STATUS bcrm_vnd_a2dp_parse_codec_info(tCODEC_INFO_SBC *parsed_info, uint8_t *codec_info)
{
    tA2D_STATUS status = A2D_SUCCESS;
    UINT8   losc;
    UINT8   mt;

    BTA2DPDBG("%s", __FUNCTION__);

    if( parsed_info == NULL || codec_info == NULL)
        status = A2D_FAIL;
    else
    {
        losc    = *codec_info++;
        mt      = *codec_info++;
        /* If the function is called for the wrong Media Type or Media Codec Type */
        if(losc != A2D_SBC_INFO_LEN || *codec_info != A2D_MEDIA_CT_SBC)
            status = A2D_WRONG_CODEC;
        else
        {
            codec_info++;
            parsed_info->sampling_freq = *codec_info & A2D_SBC_IE_SAMP_FREQ_MSK;
            parsed_info->channel_mode  = *codec_info & A2D_SBC_IE_CH_MD_MSK;
            codec_info++;
            parsed_info->block_length  = *codec_info & A2D_SBC_IE_BLOCKS_MSK;
            parsed_info->num_subbands  = *codec_info & A2D_SBC_IE_SUBBAND_MSK;
            parsed_info->alloc_method  = *codec_info & A2D_SBC_IE_ALLOC_MD_MSK;
            codec_info += 2; /* MAX Bitpool */
            parsed_info->bitpool_size  = (*codec_info > BRCM_A2DP_OFFLOAD_MAX_BITPOOL) ?
                                         BRCM_A2DP_OFFLOAD_MAX_BITPOOL : (*codec_info);

            if(MULTI_BIT_SET(parsed_info->sampling_freq))
                status = A2D_BAD_SAMP_FREQ;
            if(MULTI_BIT_SET(parsed_info->channel_mode))
                status = A2D_BAD_CH_MODE;
            if(MULTI_BIT_SET(parsed_info->block_length))
                status = A2D_BAD_BLOCK_LEN;
            if(MULTI_BIT_SET(parsed_info->num_subbands))
                status = A2D_BAD_SUBBANDS;
            if(MULTI_BIT_SET(parsed_info->alloc_method))
                status = A2D_BAD_ALLOC_MTHD;
            if(parsed_info->bitpool_size < A2D_SBC_IE_MIN_BITPOOL || parsed_info->bitpool_size > A2D_SBC_IE_MAX_BITPOOL )
                status = A2D_BAD_MIN_BITPOOL;

            if(status == A2D_SUCCESS)
                brcm_vnd_map_a2d_uipc_codec_info(parsed_info);

            BTA2DPDBG("%s STATUS %d parsed info : SampF %02x, ChnMode %02x, BlockL %02x, NSubB %02x, alloc %02x, bitpool %02x",
                __FUNCTION__, status, parsed_info->sampling_freq, parsed_info->channel_mode, parsed_info->block_length,
                parsed_info->num_subbands, parsed_info->alloc_method, parsed_info->bitpool_size);

        }
    }
    return status;
}

/*******************************************************************************
** State Machine Functions
*******************************************************************************/

/*******************************************************************************
**
** Function         brcm_vnd_a2dp_ssm_execute
**
** Description      Stream state machine event handling function for AV
**
**
** Returns          void
**
*******************************************************************************/
int brcm_vnd_a2dp_ssm_execute(tBRCM_VND_A2DP_EVENT event, void *ev_data)
{
    tBRCM_VND_A2DP_SST_STATE *state_table;
    tBRCM_VND_A2DP_SST_STATES next_state;

    pthread_mutex_lock(&g_mutex);

    BTA2DPDBG("%s ev %d state %d", __FUNCTION__, event, brcm_vnd_a2dp_pdata.state);

    if (brcm_vnd_a2dp_pdata.state != BRCM_VND_A2DP_INVALID_SST) {
        state_table = &brcm_vnd_a2dp_sst_tbl[brcm_vnd_a2dp_pdata.state];
        /* process event */
        next_state = state_table->process_event(event, ev_data);
    } else if (BRCM_VND_A2DP_OFFLOAD_INIT_REQ == event) {
        next_state = BRCM_VND_A2DP_IDLE_SST;
    }
    else {
        pthread_mutex_unlock(&g_mutex);
        return BT_VND_OP_RESULT_FAIL;
    }

    /* transition stae */
    while (next_state != brcm_vnd_a2dp_pdata.state) {
        brcm_vnd_a2dp_pdata.state = next_state;
        state_table = &brcm_vnd_a2dp_sst_tbl[next_state];
        if (state_table->enter)
            next_state = state_table->enter(event);
    }

    pthread_mutex_unlock(&g_mutex);
    return BT_VND_OP_RESULT_SUCCESS;
}

/* state machine actions */

static tBRCM_VND_A2DP_SST_STATES brcm_vnd_a2dp_sm_idle_process_ev(tBRCM_VND_A2DP_EVENT event, void *ev_data)
{
    tBRCM_VND_A2DP_SST_STATES next_state = brcm_vnd_a2dp_pdata.state;

    switch (event) {
        case BRCM_VND_A2DP_OFFLOAD_START_REQ:
            brcm_vnd_a2dp_pdata.offload_params = *(bt_vendor_op_a2dp_offload_t*)ev_data;
            if (A2D_SUCCESS != bcrm_vnd_a2dp_parse_codec_info( &brcm_vnd_a2dp_pdata.codec_info,
                    (uint8_t *)brcm_vnd_a2dp_pdata.offload_params.codec_info)) {
                ALOGE("%s CodecConfig BT_VND_OP_A2DP_OFFLOAD_START FAILED", __FUNCTION__);
                bt_vendor_cbacks->a2dp_offload_cb(BT_VND_OP_RESULT_FAIL, BT_VND_OP_A2DP_OFFLOAD_START,
                                                  brcm_vnd_a2dp_pdata.offload_params.bta_av_handle);
            } else {
                brcm_vnd_a2dp_offload_configure();
                next_state = BRCM_VND_A2DP_STARTING_SST;
            }
            break;

        default:
            ALOGV("%s Unexpected Event %d in State %d, IGNORE", __FUNCTION__, event, brcm_vnd_a2dp_pdata.state);
            break;
    }
    return next_state;
}

static tBRCM_VND_A2DP_SST_STATES brcm_vnd_a2dp_sm_starting_process_ev(tBRCM_VND_A2DP_EVENT event, void *ev_data)
{
    tBRCM_VND_A2DP_SST_STATES next_state = brcm_vnd_a2dp_pdata.state;
    uint8_t status, *p;

    switch (event) {
        case BRCM_VND_A2DP_OFFLOAD_START_REQ:
            brcm_vnd_a2dp_offload_cleanup();
            brcm_vnd_a2dp_pdata.offload_params = *(bt_vendor_op_a2dp_offload_t*)ev_data;
            if (A2D_SUCCESS != bcrm_vnd_a2dp_parse_codec_info(
                    &brcm_vnd_a2dp_pdata.codec_info, (uint8_t *)brcm_vnd_a2dp_pdata.offload_params.codec_info)) {
                ALOGE("%s CodecConfig BT_VND_OP_A2DP_OFFLOAD_START FAILED", __FUNCTION__);
                bt_vendor_cbacks->a2dp_offload_cb(BT_VND_OP_RESULT_FAIL, BT_VND_OP_A2DP_OFFLOAD_START,
                                                  brcm_vnd_a2dp_pdata.offload_params.bta_av_handle);
                next_state = BRCM_VND_A2DP_IDLE_SST;
            } else {
                brcm_vnd_a2dp_offload_configure();
            }
            break;

        case BRCM_VND_A2DP_OFFLOAD_STOP_REQ:
            brcm_vnd_a2dp_offload_cleanup();
            next_state = BRCM_VND_A2DP_IDLE_SST;
            break;

        case BRCM_VND_UIPC_OPEN_RSP: {
                uint8_t num_streams;
                uint16_t maj_ver, min_ver;
                p = (uint8_t*)ev_data + offsetof(tUIPC_OPEN_RSP, status);
                STREAM_TO_UINT8(status,p);
                STREAM_TO_UINT16(maj_ver,p);
                STREAM_TO_UINT16(min_ver,p);
                STREAM_TO_UINT8(num_streams,p);
                // TODO Verify Params //
                if (status) {
                    ALOGE("%s BRCM_VND_UIPC_OPEN_RSP %02x FAILED", __FUNCTION__, status);
                    brcm_vnd_a2dp_offload_cleanup();
                    bt_vendor_cbacks->a2dp_offload_cb(BT_VND_OP_RESULT_FAIL, BT_VND_OP_A2DP_OFFLOAD_START,
                                                      brcm_vnd_a2dp_pdata.offload_params.bta_av_handle);
                    next_state = BRCM_VND_A2DP_IDLE_SST;
                }
            }
            break;

        case BRCM_VND_L2C_SYNC_TO_LITE_RSP:
            status = *((uint8_t*)ev_data + offsetof(tL2C_SYNC_TO_LITE_RESP, stream.status));
            if (status) {
                ALOGE("%s L2C_SYNC_TO_LITE_RESP %02x FAILED", __FUNCTION__, status);
                brcm_vnd_a2dp_offload_cleanup();
                bt_vendor_cbacks->a2dp_offload_cb(BT_VND_OP_RESULT_FAIL, BT_VND_OP_A2DP_OFFLOAD_START,
                                                  brcm_vnd_a2dp_pdata.offload_params.bta_av_handle);
                next_state = BRCM_VND_A2DP_IDLE_SST;
            }
            break;

        case BRCM_VND_SYNC_TO_BTC_LITE_RSP:
            status = *((uint8_t*)ev_data + offsetof(tAVDT_SYNC_TO_BTC_LITE_RESP, status));
            if (status) {
                ALOGE("%s AVDT_SYNC_TO_BTC_LITE_RESP %02x FAILED", __FUNCTION__, status);
                brcm_vnd_a2dp_offload_cleanup();
                bt_vendor_cbacks->a2dp_offload_cb(BT_VND_OP_RESULT_FAIL, BT_VND_OP_A2DP_OFFLOAD_START,
                                                  brcm_vnd_a2dp_pdata.offload_params.bta_av_handle);
                next_state = BRCM_VND_A2DP_IDLE_SST;
            }
            break;

        case BRCM_VND_AUDIO_ROUTE_CONFIG_RSP:
            status = *((uint8_t*)ev_data + offsetof(tAUDIO_ROUTE_CONFIG_RESP, status));
            if (status) {
                ALOGE("%s AUDIO_ROUTE_CONFIG_RESP %02x FAILED", __FUNCTION__, status);
                brcm_vnd_a2dp_offload_cleanup();
                bt_vendor_cbacks->a2dp_offload_cb(BT_VND_OP_RESULT_FAIL, BT_VND_OP_A2DP_OFFLOAD_START,
                                                  brcm_vnd_a2dp_pdata.offload_params.bta_av_handle);
                next_state = BRCM_VND_A2DP_IDLE_SST;
            }
            break;

        case BRCM_VND_AUDIO_CODEC_CONFIG_RSP:
            status = *((uint8_t*)ev_data + offsetof(tAUDIO_CODEC_CONFIG_RESP, status));
            if (status) {
                ALOGE("%s BRCM_VND_AUDIO_CODEC_CONFIG_RSP %02x FAILED", __FUNCTION__, status);
                brcm_vnd_a2dp_offload_cleanup();
                bt_vendor_cbacks->a2dp_offload_cb(BT_VND_OP_RESULT_FAIL, BT_VND_OP_A2DP_OFFLOAD_START,
                                                  brcm_vnd_a2dp_pdata.offload_params.bta_av_handle);
                next_state = BRCM_VND_A2DP_IDLE_SST;
            }
            break;

        case BRCM_VND_A2DP_START_RSP:
            /* status = *((uint8_t*)ev_data + offsetof(tA2DP_GENERIC_RESP, status)); */
            bt_vendor_cbacks->a2dp_offload_cb(BT_VND_OP_RESULT_SUCCESS, BT_VND_OP_A2DP_OFFLOAD_START,
                                              brcm_vnd_a2dp_pdata.offload_params.bta_av_handle);
            next_state = BRCM_VND_A2DP_STREAM_SST;
            break;

        case BRCM_VND_A2DP_OFFLOAD_FAILED_ABORT:
            ALOGE("%s BRCM_VND_A2DP_OFFLOAD_FAILED_ABORT", __FUNCTION__);
            brcm_vnd_a2dp_offload_cleanup();
            bt_vendor_cbacks->a2dp_offload_cb(BT_VND_OP_RESULT_FAIL, BT_VND_OP_A2DP_OFFLOAD_START,
                                              brcm_vnd_a2dp_pdata.offload_params.bta_av_handle);
            next_state = BRCM_VND_A2DP_IDLE_SST;
            break;

        default:
            ALOGE("%s Unexpected Event %d in State %d, IGNORE", __FUNCTION__, event, brcm_vnd_a2dp_pdata.state);
            break;
    }
    return next_state;
}

static tBRCM_VND_A2DP_SST_STATES brcm_vnd_a2dp_sm_stream_process_ev(tBRCM_VND_A2DP_EVENT event, void *ev_data)
{
    tBRCM_VND_A2DP_SST_STATES next_state = brcm_vnd_a2dp_pdata.state;
    switch (event) {
        case BRCM_VND_A2DP_OFFLOAD_START_REQ:
            brcm_vnd_a2dp_offload_cleanup();
            brcm_vnd_a2dp_pdata.offload_params = *(bt_vendor_op_a2dp_offload_t*)ev_data;
            if (A2D_SUCCESS != bcrm_vnd_a2dp_parse_codec_info(
                    &brcm_vnd_a2dp_pdata.codec_info, (uint8_t *)brcm_vnd_a2dp_pdata.offload_params.codec_info)) {
                ALOGE("%s CodecConfig BT_VND_OP_A2DP_OFFLOAD_START FAILED", __FUNCTION__);
                bt_vendor_cbacks->a2dp_offload_cb(BT_VND_OP_RESULT_FAIL, BT_VND_OP_A2DP_OFFLOAD_START,
                                                  brcm_vnd_a2dp_pdata.offload_params.bta_av_handle);
                next_state = BRCM_VND_A2DP_IDLE_SST;
            } else {
                brcm_vnd_a2dp_offload_configure();
                next_state = BRCM_VND_A2DP_STARTING_SST;
            }
            break;

        case BRCM_VND_A2DP_OFFLOAD_STOP_REQ:
        case BRCM_VND_A2DP_OFFLOAD_FAILED_ABORT:
            ALOGE("%s BRCM_VND_A2DP_OFFLOAD_STOP ABORT %d.", __FUNCTION__,
                  (event == BRCM_VND_A2DP_OFFLOAD_FAILED_ABORT));
            brcm_vnd_a2dp_offload_cleanup();
            next_state = BRCM_VND_A2DP_IDLE_SST;
            break;

        default:
            ALOGE("%s Unexpected Event %d in State %d, IGNORE", __FUNCTION__, event, brcm_vnd_a2dp_pdata.state);
            break;
    }
    return next_state;
}

static uint8_t brcm_vnd_a2dp_offload_configure()
{
    uint8_t *p, msg_req[HCI_CMD_MAX_LEN];

    BTA2DPDBG("%s", __FUNCTION__);

    p = msg_req;
    brcm_vnd_a2dp_send_hci_vsc(HCI_VSC_READ_PCM_PINS, msg_req, (uint8_t)(p - msg_req), brcm_vnd_a2dp_hci_uipc_cback);

    p = msg_req;
    UINT8_TO_STREAM(p, BRCM_A2DP_OFFLOAD_PCM_PIN_FCN);
    UINT32_TO_STREAM(p, BRCM_A2DP_OFFLOAD_PCM_PIN_PADCNF);
    brcm_vnd_a2dp_send_hci_vsc(HCI_VSC_WRITE_PCM_PINS, msg_req, (uint8_t)(p - msg_req), brcm_vnd_a2dp_hci_uipc_cback);

    p = msg_req;
    UINT16_TO_STREAM(p, BT_EVT_BTU_IPC_MGMT_EVT);
    UINT8_TO_STREAM(p, UIPC_OPEN_REQ);
    brcm_vnd_a2dp_send_hci_vsc(HCI_VSC_UIPC_OVER_HCI, msg_req, (uint8_t)(p - msg_req), brcm_vnd_a2dp_hci_uipc_cback);

    p = msg_req;
    UINT16_TO_STREAM(p, BT_EVT_BTU_IPC_L2C_EVT);
    UINT8_TO_STREAM(p, L2C_SYNC_TO_LITE_REQ);
    UINT16_TO_STREAM(p, brcm_vnd_a2dp_pdata.offload_params.xmit_quota);
    UINT16_TO_STREAM(p, brcm_vnd_a2dp_pdata.offload_params.acl_data_size);
    UINT16_TO_STREAM(p, !(brcm_vnd_a2dp_pdata.offload_params.is_flushable));
    UINT8_TO_STREAM(p, 0x02); //multi_av_data_cong_start
    UINT8_TO_STREAM(p, 0x00); //multi_av_data_cong_end
    UINT8_TO_STREAM(p, 0x04); //multi_av_data_cong_discard
    UINT8_TO_STREAM(p, 1); //num_stream
    UINT16_TO_STREAM(p, brcm_vnd_a2dp_pdata.offload_params.local_cid);
    UINT16_TO_STREAM(p, brcm_vnd_a2dp_pdata.offload_params.remote_cid);
    UINT16_TO_STREAM(p, brcm_vnd_a2dp_pdata.offload_params.stream_mtu);
    UINT16_TO_STREAM(p, brcm_vnd_a2dp_pdata.offload_params.lm_handle);
    UINT16_TO_STREAM(p, brcm_vnd_a2dp_pdata.offload_params.xmit_quota);
    UINT8_TO_STREAM(p, brcm_vnd_a2dp_pdata.offload_params.is_flushable);
    brcm_vnd_a2dp_send_hci_vsc(HCI_VSC_UIPC_OVER_HCI, msg_req, (uint8_t)(p - msg_req), brcm_vnd_a2dp_hci_uipc_cback);

    p = msg_req;
    UINT16_TO_STREAM(p, BT_EVT_BTU_IPC_AVDT_EVT);
    UINT8_TO_STREAM(p, AVDT_SYNC_TO_BTC_LITE_REQ);
    UINT8_TO_STREAM(p, 1); //num_stream
    UINT16_TO_STREAM(p, brcm_vnd_a2dp_pdata.offload_params.local_cid);
    UINT32_TO_STREAM(p, brcm_vnd_a2dp_pdata.offload_params.stream_source);
    brcm_vnd_a2dp_send_hci_vsc(HCI_VSC_UIPC_OVER_HCI, msg_req, (uint8_t)(p - msg_req), brcm_vnd_a2dp_hci_uipc_cback);

    p = msg_req;
    UINT16_TO_STREAM(p, BT_EVT_BTU_IPC_BTM_EVT);
    UINT8_TO_STREAM(p, AUDIO_ROUTE_CONFIG_REQ);
    UINT8_TO_STREAM(p, BRCM_A2DP_OFFLOAD_SRC);
    UINT8_TO_STREAM(p, BRCM_A2DP_OFFLOAD_SRC_SF);
    UINT8_TO_STREAM(p, AUDIO_ROUTE_OUT_BTA2DP);
    UINT8_TO_STREAM(p, BRCM_A2DP_OFFLOAD_SRC_SF);
    UINT8_TO_STREAM(p, AUDIO_ROUTE_SF_NA);
    UINT8_TO_STREAM(p, AUDIO_ROUTE_EQ_BYPASS);
    brcm_vnd_a2dp_send_hci_vsc(HCI_VSC_UIPC_OVER_HCI, msg_req, (uint8_t)(p - msg_req), brcm_vnd_a2dp_hci_uipc_cback);

    p = msg_req;
    UINT16_TO_STREAM(p, BT_EVT_BTU_IPC_BTM_EVT);
    UINT8_TO_STREAM(p, AUDIO_CODEC_CONFIG_REQ);
    UINT16_TO_STREAM(p, AUDIO_CODEC_SBC_ENC);
    UINT8_TO_STREAM(p, brcm_vnd_a2dp_pdata.codec_info.sampling_freq);
    UINT8_TO_STREAM(p, brcm_vnd_a2dp_pdata.codec_info.channel_mode);
    UINT8_TO_STREAM(p, brcm_vnd_a2dp_pdata.codec_info.block_length);
    UINT8_TO_STREAM(p, brcm_vnd_a2dp_pdata.codec_info.num_subbands);
    UINT8_TO_STREAM(p, brcm_vnd_a2dp_pdata.codec_info.alloc_method);
    UINT8_TO_STREAM(p, brcm_vnd_a2dp_pdata.codec_info.bitpool_size);
    brcm_vnd_a2dp_send_hci_vsc(HCI_VSC_UIPC_OVER_HCI, msg_req, (uint8_t)(p - msg_req), brcm_vnd_a2dp_hci_uipc_cback);

    p = msg_req;
    UINT16_TO_STREAM(p, BT_EVT_BTU_IPC_BTM_EVT);
    UINT8_TO_STREAM(p, A2DP_START_REQ);
    UINT16_TO_STREAM(p, brcm_vnd_a2dp_pdata.offload_params.local_cid);
    UINT16_TO_STREAM(p, brcm_vnd_a2dp_pdata.offload_params.stream_mtu);
    brcm_vnd_a2dp_send_hci_vsc(HCI_VSC_UIPC_OVER_HCI, msg_req, (uint8_t)(p - msg_req), brcm_vnd_a2dp_hci_uipc_cback);

    return 0;
}

static uint8_t brcm_vnd_a2dp_offload_cleanup()
{
    uint8_t *p, msg_req[HCI_CMD_MAX_LEN];

    BTA2DPDBG("%s", __FUNCTION__);

    p = msg_req;
    UINT16_TO_STREAM(p, BT_EVT_BTU_IPC_BTM_EVT);
    UINT8_TO_STREAM(p, A2DP_CLEANUP_REQ);
    UINT16_TO_STREAM(p, brcm_vnd_a2dp_pdata.offload_params.local_cid);
    UINT16_TO_STREAM(p, brcm_vnd_a2dp_pdata.offload_params.stream_mtu);
    brcm_vnd_a2dp_send_hci_vsc(HCI_VSC_UIPC_OVER_HCI, msg_req, (uint8_t)(p - msg_req), brcm_vnd_a2dp_hci_uipc_cback);

    p = msg_req;
    UINT16_TO_STREAM(p, BT_EVT_BTU_IPC_L2C_EVT);
    UINT8_TO_STREAM(p, L2C_REMOVE_TO_LITE_REQ);
    UINT16_TO_STREAM(p, brcm_vnd_a2dp_pdata.offload_params.xmit_quota);
    UINT8_TO_STREAM(p, 1); //num_stream
    UINT16_TO_STREAM(p, brcm_vnd_a2dp_pdata.offload_params.local_cid);
    brcm_vnd_a2dp_send_hci_vsc(HCI_VSC_UIPC_OVER_HCI, msg_req, (uint8_t)(p - msg_req), brcm_vnd_a2dp_hci_uipc_cback);

    p = msg_req;
    UINT16_TO_STREAM(p, BT_EVT_BTU_IPC_MGMT_EVT);
    UINT8_TO_STREAM(p, UIPC_CLOSE_REQ);
    brcm_vnd_a2dp_send_hci_vsc(HCI_VSC_UIPC_OVER_HCI, msg_req, (uint8_t)(p - msg_req), brcm_vnd_a2dp_hci_uipc_cback);

    if (PCM_PIN_FCN_INVALID != brcm_vnd_a2dp_pdata.pcmi2s_pinmux.fcn) {
        p = msg_req;
        UINT8_TO_STREAM(p, brcm_vnd_a2dp_pdata.pcmi2s_pinmux.fcn);
        UINT32_TO_STREAM(p, brcm_vnd_a2dp_pdata.pcmi2s_pinmux.pad_conf);
        brcm_vnd_a2dp_send_hci_vsc(HCI_VSC_WRITE_PCM_PINS, msg_req, (uint8_t)(p - msg_req), brcm_vnd_a2dp_hci_uipc_cback);
        brcm_vnd_a2dp_pdata.pcmi2s_pinmux.fcn = PCM_PIN_FCN_INVALID;
    }

    return 0;
}

static uint8_t brcm_vnd_a2dp_offload_suspend()
{
    uint8_t *p, msg_req[HCI_CMD_MAX_LEN];

    BTA2DPDBG("%s", __FUNCTION__);

    p = msg_req;
    UINT16_TO_STREAM(p, BT_EVT_BTU_IPC_BTM_EVT);
    UINT8_TO_STREAM(p, A2DP_SUSPEND_REQ);
    UINT16_TO_STREAM(p, brcm_vnd_a2dp_pdata.offload_params.local_cid);
    brcm_vnd_a2dp_send_hci_vsc(HCI_VSC_UIPC_OVER_HCI, msg_req, (uint8_t)(p - msg_req), brcm_vnd_a2dp_hci_uipc_cback);

    return 0;
}

void brcm_vnd_a2dp_hci_uipc_cback(void *pmem)
{
    HC_BT_HDR    *p_evt_buf = (HC_BT_HDR *)pmem;
    uint8_t     *p, len, vsc_result, uipc_opcode;
    uint16_t    vsc_opcode, uipc_event;
    HC_BT_HDR    *p_buf = NULL;
    bt_vendor_op_result_t status = BT_VND_OP_RESULT_SUCCESS;
    tBRCM_VND_A2DP_EVENT   ssm_event;

    p = (uint8_t *)(p_evt_buf + 1) + HCI_EVT_CMD_CMPL_LEN;
    len = *p;
    p = (uint8_t *)(p_evt_buf + 1) + HCI_EVT_CMD_CMPL_VSC;
    STREAM_TO_UINT16(vsc_opcode,p);
    vsc_result = *p++;

    log_bin_to_hexstr(((uint8_t *)(p_evt_buf + 1) + HCI_EVT_CMD_CMPL_VSC), len-1, __FUNCTION__);

    if (vsc_result != 0) {
        ALOGE("%s Failed VSC Op %04x", __FUNCTION__, vsc_opcode);
        status = BT_VND_OP_RESULT_FAIL;
    }
    else if (vsc_opcode == HCI_VSC_UIPC_OVER_HCI) {
        STREAM_TO_UINT16(uipc_event,p);
        uipc_opcode = *p;
        BTA2DPDBG("%s UIPC Event %04x UIPC Op %02x", __FUNCTION__, uipc_event, uipc_opcode);

        switch (uipc_event) {
            case BT_EVT_BTU_IPC_MGMT_EVT :
                switch (uipc_opcode) {
                    case UIPC_OPEN_RSP    : ssm_event = BRCM_VND_UIPC_OPEN_RSP; break;
                    case UIPC_CLOSE_RSP    : ssm_event = BRCM_VND_UIPC_CLOSE_RSP; break;
                    default: status = BT_VND_OP_RESULT_FAIL;
                }
                break;

            case BT_EVT_BTU_IPC_BTM_EVT     :
                switch (uipc_opcode) {
                    case A2DP_START_RESP:   ssm_event = BRCM_VND_A2DP_START_RSP; break;
                    case A2DP_SUSPEND_RESP: ssm_event = BRCM_VND_A2DP_SUSPEND_RSP; break;
                    case A2DP_CLEANUP_RESP: ssm_event = BRCM_VND_A2DP_CLEANUP_RSP; break;
                    case AUDIO_CODEC_CONFIG_RESP: ssm_event = BRCM_VND_AUDIO_CODEC_CONFIG_RSP; break;
                    case AUDIO_ROUTE_CONFIG_RESP: ssm_event = BRCM_VND_AUDIO_ROUTE_CONFIG_RSP; break;
                    default: status = BT_VND_OP_RESULT_FAIL;
                }
                break;

            case BT_EVT_BTU_IPC_L2C_EVT  :
                switch (uipc_opcode) {
                    case L2C_REMOVE_TO_LITE_RESP: ssm_event = BRCM_VND_L2C_REMOVE_TO_LITE_RSP; break;
                    case L2C_SYNC_TO_LITE_RESP:   ssm_event = BRCM_VND_L2C_SYNC_TO_LITE_RSP; break;
                    default: status = BT_VND_OP_RESULT_FAIL;
                }
                break;

            case BT_EVT_BTU_IPC_AVDT_EVT :
                if (uipc_opcode == AVDT_SYNC_TO_BTC_LITE_RESP) {
                    ssm_event = BRCM_VND_SYNC_TO_BTC_LITE_RSP;
                    break;
                }

            default:
                status = BT_VND_OP_RESULT_FAIL;
                break;
        }
        if (status == BT_VND_OP_RESULT_SUCCESS)
            brcm_vnd_a2dp_ssm_execute(ssm_event, p);
    }
    else if (vsc_opcode == HCI_VSC_READ_PCM_PINS) {
        STREAM_TO_UINT8(brcm_vnd_a2dp_pdata.pcmi2s_pinmux.fcn, p);
        STREAM_TO_UINT32(brcm_vnd_a2dp_pdata.pcmi2s_pinmux.pad_conf, p);
        BTA2DPDBG("%s HCI_VSC_READ_PCM_PINS %02x %08x", __FUNCTION__,
                  brcm_vnd_a2dp_pdata.pcmi2s_pinmux.fcn, brcm_vnd_a2dp_pdata.pcmi2s_pinmux.pad_conf);
    }

    if (status != BT_VND_OP_RESULT_SUCCESS)
        brcm_vnd_a2dp_ssm_execute(BRCM_VND_A2DP_OFFLOAD_FAILED_ABORT, NULL);

    /* Free the RX event buffer */
    bt_vendor_cbacks->dealloc(p_evt_buf);
}

void brcm_vnd_a2dp_init()
{
    if (!bt_vendor_cbacks)
        return;

    ALOGD("%s ", __FUNCTION__);
    brcm_vnd_a2dp_ssm_execute(BRCM_VND_A2DP_OFFLOAD_INIT_REQ, NULL);
}

int brcm_vnd_a2dp_execute(bt_vendor_opcode_t opcode, void *ev_data)
{
    tBRCM_VND_A2DP_EVENT ssm_event = (opcode == BT_VND_OP_A2DP_OFFLOAD_START)?
        BRCM_VND_A2DP_OFFLOAD_START_REQ:BRCM_VND_A2DP_OFFLOAD_STOP_REQ;

    ALOGD("%s opcode %d , state %d", __FUNCTION__, opcode, brcm_vnd_a2dp_pdata.state);

    return brcm_vnd_a2dp_ssm_execute(ssm_event, ev_data);
}


