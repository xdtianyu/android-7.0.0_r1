/*
 * Copyright (C) 2014 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "sync.h"
#include <utils/Log.h>
#include "wifi_hal.h"
#include "nan_i.h"
#include "nancommand.h"


int NanCommand::isNanResponse()
{
    if (mNanVendorEvent == NULL) {
        ALOGE("NULL check failed");
        return WIFI_ERROR_INVALID_ARGS;
    }

    NanMsgHeader *pHeader = (NanMsgHeader *)mNanVendorEvent;

    switch (pHeader->msgId) {
    case NAN_MSG_ID_ERROR_RSP:
    case NAN_MSG_ID_CONFIGURATION_RSP:
    case NAN_MSG_ID_PUBLISH_SERVICE_CANCEL_RSP:
    case NAN_MSG_ID_PUBLISH_SERVICE_RSP:
    case NAN_MSG_ID_SUBSCRIBE_SERVICE_RSP:
    case NAN_MSG_ID_SUBSCRIBE_SERVICE_CANCEL_RSP:
    case NAN_MSG_ID_TRANSMIT_FOLLOWUP_RSP:
    case NAN_MSG_ID_STATS_RSP:
    case NAN_MSG_ID_ENABLE_RSP:
    case NAN_MSG_ID_DISABLE_RSP:
    case NAN_MSG_ID_TCA_RSP:
    case NAN_MSG_ID_BEACON_SDF_RSP:
    case NAN_MSG_ID_CAPABILITIES_RSP:
        return 1;
    default:
        return 0;
    }
}


int NanCommand::getNanResponse(transaction_id *id, NanResponseMsg *pRsp)
{
    if (mNanVendorEvent == NULL || pRsp == NULL) {
        ALOGE("NULL check failed");
        return WIFI_ERROR_INVALID_ARGS;
    }

    NanMsgHeader *pHeader = (NanMsgHeader *)mNanVendorEvent;

    switch (pHeader->msgId) {
        case NAN_MSG_ID_ERROR_RSP:
        {
            pNanErrorRspMsg pFwRsp = \
                (pNanErrorRspMsg)mNanVendorEvent;
            *id = (transaction_id)pFwRsp->fwHeader.transactionId;
            pRsp->status = (NanStatusType)pFwRsp->status;
            pRsp->value = pFwRsp->value;
            pRsp->response_type = NAN_RESPONSE_ERROR;
            break;
        }
        case NAN_MSG_ID_CONFIGURATION_RSP:
        {
            pNanConfigurationRspMsg pFwRsp = \
                (pNanConfigurationRspMsg)mNanVendorEvent;
            *id = (transaction_id)pFwRsp->fwHeader.transactionId;
            pRsp->status = (NanStatusType)pFwRsp->status;
            pRsp->value = pFwRsp->value;
            pRsp->response_type = NAN_RESPONSE_CONFIG;
        }
        break;
        case NAN_MSG_ID_PUBLISH_SERVICE_CANCEL_RSP:
        {
            pNanPublishServiceCancelRspMsg pFwRsp = \
                (pNanPublishServiceCancelRspMsg)mNanVendorEvent;
            *id = (transaction_id)pFwRsp->fwHeader.transactionId;
            pRsp->status = (NanStatusType)pFwRsp->status;
            pRsp->value = pFwRsp->value;
            pRsp->response_type = NAN_RESPONSE_PUBLISH_CANCEL;
            pRsp->body.publish_response.publish_id = \
                pFwRsp->fwHeader.handle;
            break;
        }
        case NAN_MSG_ID_PUBLISH_SERVICE_RSP:
        {
            pNanPublishServiceRspMsg pFwRsp = \
                (pNanPublishServiceRspMsg)mNanVendorEvent;
            *id = (transaction_id)pFwRsp->fwHeader.transactionId;
            pRsp->status = (NanStatusType)pFwRsp->status;
            pRsp->value = pFwRsp->value;
            pRsp->response_type = NAN_RESPONSE_PUBLISH;
            pRsp->body.publish_response.publish_id = \
                pFwRsp->fwHeader.handle;
            break;
        }
        case NAN_MSG_ID_SUBSCRIBE_SERVICE_RSP:
        {
            pNanSubscribeServiceRspMsg pFwRsp = \
                (pNanSubscribeServiceRspMsg)mNanVendorEvent;
            *id = (transaction_id)pFwRsp->fwHeader.transactionId;
            pRsp->status = (NanStatusType)pFwRsp->status;
            pRsp->value = pFwRsp->value;
            pRsp->response_type = NAN_RESPONSE_SUBSCRIBE;
            pRsp->body.subscribe_response.subscribe_id = \
                pFwRsp->fwHeader.handle;
        }
        break;
        case NAN_MSG_ID_SUBSCRIBE_SERVICE_CANCEL_RSP:
        {
            pNanSubscribeServiceCancelRspMsg pFwRsp = \
                (pNanSubscribeServiceCancelRspMsg)mNanVendorEvent;
            *id = (transaction_id)pFwRsp->fwHeader.transactionId;
            pRsp->status = (NanStatusType)pFwRsp->status;
            pRsp->value = pFwRsp->value;
            pRsp->response_type = NAN_RESPONSE_SUBSCRIBE_CANCEL;
            pRsp->body.subscribe_response.subscribe_id = \
                pFwRsp->fwHeader.handle;
            break;
        }
        case NAN_MSG_ID_TRANSMIT_FOLLOWUP_RSP:
        {
            pNanTransmitFollowupRspMsg pFwRsp = \
                (pNanTransmitFollowupRspMsg)mNanVendorEvent;
            *id = (transaction_id)pFwRsp->fwHeader.transactionId;
            pRsp->status = (NanStatusType)pFwRsp->status;
            pRsp->value = pFwRsp->value;
            pRsp->response_type = NAN_RESPONSE_TRANSMIT_FOLLOWUP;
            break;
        }
        case NAN_MSG_ID_STATS_RSP:
        {
            pNanStatsRspMsg pFwRsp = \
                (pNanStatsRspMsg)mNanVendorEvent;
            *id = (transaction_id)pFwRsp->fwHeader.transactionId;
            pRsp->status = (NanStatusType)pFwRsp->statsRspParams.status;
            pRsp->value = pFwRsp->statsRspParams.value;
            pRsp->response_type = NAN_RESPONSE_STATS;
            pRsp->body.stats_response.stats_type = \
                (NanStatsType)pFwRsp->statsRspParams.statsType;
            ALOGV("%s: stats_type:%d",__func__,
                  pRsp->body.stats_response.stats_type);
            u8 *pInputTlv = pFwRsp->ptlv;
            NanTlv outputTlv;
            memset(&outputTlv, 0, sizeof(outputTlv));
            u16 readLen = 0;
            int remainingLen = (mNanDataLen -  \
                (sizeof(NanMsgHeader) + sizeof(NanStatsRspParams)));
            if (remainingLen > 0) {
                readLen = NANTLV_ReadTlv(pInputTlv, &outputTlv);
                ALOGV("%s: Remaining Len:%d readLen:%d type:%d length:%d",
                      __func__, remainingLen, readLen, outputTlv.type,
                      outputTlv.length);
                if (outputTlv.length <= \
                    sizeof(pRsp->body.stats_response.data)) {
                    handleNanStatsResponse(pRsp->body.stats_response.stats_type,
                                           (char *)outputTlv.value,
                                           &pRsp->body.stats_response);
                }
            }
            else
                ALOGV("%s: No TLV's present",__func__);
            break;
        }
        case NAN_MSG_ID_ENABLE_RSP:
        {
            pNanEnableRspMsg pFwRsp = \
                (pNanEnableRspMsg)mNanVendorEvent;
            *id = (transaction_id)pFwRsp->fwHeader.transactionId;
            pRsp->status = (NanStatusType)pFwRsp->status;
            pRsp->value = pFwRsp->value;
            pRsp->response_type = NAN_RESPONSE_ENABLED;
            break;
        }
        case NAN_MSG_ID_DISABLE_RSP:
        {
            pNanDisableRspMsg pFwRsp = \
                (pNanDisableRspMsg)mNanVendorEvent;
            *id = (transaction_id)pFwRsp->fwHeader.transactionId;
            pRsp->status = (NanStatusType)pFwRsp->status;
            pRsp->value = 0;
            pRsp->response_type = NAN_RESPONSE_DISABLED;
            break;
        }
        case NAN_MSG_ID_TCA_RSP:
        {
            pNanTcaRspMsg pFwRsp = \
                (pNanTcaRspMsg)mNanVendorEvent;
            *id = (transaction_id)pFwRsp->fwHeader.transactionId;
            pRsp->status = (NanStatusType)pFwRsp->status;
            pRsp->value = pFwRsp->value;
            pRsp->response_type = NAN_RESPONSE_TCA;
            break;
        }
        case NAN_MSG_ID_BEACON_SDF_RSP:
        {
            pNanBeaconSdfPayloadRspMsg pFwRsp = \
                (pNanBeaconSdfPayloadRspMsg)mNanVendorEvent;
            *id = (transaction_id)pFwRsp->fwHeader.transactionId;
            pRsp->status = (NanStatusType)pFwRsp->status;
            pRsp->value = 0;
            pRsp->response_type = NAN_RESPONSE_BEACON_SDF_PAYLOAD;
            break;
        }
        case NAN_MSG_ID_CAPABILITIES_RSP:
        {
            pNanCapabilitiesRspMsg pFwRsp = \
                (pNanCapabilitiesRspMsg)mNanVendorEvent;
            *id = (transaction_id)pFwRsp->fwHeader.transactionId;
            pRsp->status = (NanStatusType)pFwRsp->status;
            pRsp->value = pFwRsp->value;
            pRsp->response_type = NAN_GET_CAPABILITIES;
            pRsp->body.nan_capabilities.max_concurrent_nan_clusters = \
                        pFwRsp->max_concurrent_nan_clusters;
            pRsp->body.nan_capabilities.max_publishes = \
                        pFwRsp->max_publishes;
            pRsp->body.nan_capabilities.max_subscribes = \
                        pFwRsp->max_subscribes;
            pRsp->body.nan_capabilities.max_service_name_len = \
                        pFwRsp->max_service_name_len;
            pRsp->body.nan_capabilities.max_match_filter_len = \
                        pFwRsp->max_match_filter_len;
            pRsp->body.nan_capabilities.max_total_match_filter_len = \
                        pFwRsp->max_total_match_filter_len;
            pRsp->body.nan_capabilities.max_service_specific_info_len = \
                        pFwRsp->max_service_specific_info_len;
            pRsp->body.nan_capabilities.max_vsa_data_len = \
                        pFwRsp->max_vsa_data_len;
            pRsp->body.nan_capabilities.max_mesh_data_len = \
                        pFwRsp->max_mesh_data_len;
            pRsp->body.nan_capabilities.max_ndi_interfaces = \
                       pFwRsp->max_ndi_interfaces;
            pRsp->body.nan_capabilities.max_ndp_sessions = \
                       pFwRsp->max_ndp_sessions;
            pRsp->body.nan_capabilities.max_app_info_len = \
                       pFwRsp->max_app_info_len;
            break;
        }
        default:
            return  -1;
    }
    return  0;
}

int NanCommand::handleNanResponse()
{
    //parse the data and call
    //the response callback handler with the populated
    //NanResponseMsg
    NanResponseMsg  rsp_data;
    int ret;
    transaction_id id;

    ALOGV("handleNanResponse called %p", this);
    memset(&rsp_data, 0, sizeof(rsp_data));
    //get the rsp_data
    ret = getNanResponse(&id, &rsp_data);

    ALOGI("handleNanResponse ret:%d status:%u value:%u response_type:%u",
          ret, rsp_data.status, rsp_data.value, rsp_data.response_type);
    if (ret == 0 && (rsp_data.response_type == NAN_RESPONSE_STATS) &&
        (mStaParam != NULL) &&
        (rsp_data.body.stats_response.stats_type == NAN_STATS_ID_DE_TIMING_SYNC)) {
        /*
           Fill the staParam with appropriate values and return from here.
           No need to call NotifyResponse as the request is for getting the
           STA response
        */
        NanSyncStats *pSyncStats = &rsp_data.body.stats_response.data.sync_stats;
        mStaParam->master_rank = pSyncStats->myRank;
        mStaParam->master_pref = (pSyncStats->myRank & 0xFF00000000000000) >> 56;
        mStaParam->random_factor = (pSyncStats->myRank & 0x00FF000000000000) >> 48;
        mStaParam->hop_count = pSyncStats->currAmHopCount;
        mStaParam->beacon_transmit_time = pSyncStats->currAmBTT;

        return ret;
    }
    //Call the NotifyResponse Handler
    if (ret == 0 && mHandler.NotifyResponse) {
        (*mHandler.NotifyResponse)(id, &rsp_data);
    }
    return ret;
}

void NanCommand::handleNanStatsResponse(NanStatsType stats_type,
                                       char *rspBuf,
                                       NanStatsResponse *pRsp)
{
    if (stats_type == NAN_STATS_ID_DE_PUBLISH) {
        NanPublishStats publish_stats;
        FwNanPublishStats *pPubStats = (FwNanPublishStats *)rspBuf;

        publish_stats.validPublishServiceReqMsgs =
                                    pPubStats->validPublishServiceReqMsgs;
        publish_stats.validPublishServiceRspMsgs =
                                    pPubStats->validPublishServiceRspMsgs;
        publish_stats.validPublishServiceCancelReqMsgs =
                                    pPubStats->validPublishServiceCancelReqMsgs;
        publish_stats.validPublishServiceCancelRspMsgs =
                                    pPubStats->validPublishServiceCancelRspMsgs;
        publish_stats.validPublishRepliedIndMsgs =
                                    pPubStats->validPublishRepliedIndMsgs;
        publish_stats.validPublishTerminatedIndMsgs =
                                    pPubStats->validPublishTerminatedIndMsgs;
        publish_stats.validActiveSubscribes = pPubStats->validActiveSubscribes;
        publish_stats.validMatches = pPubStats->validMatches;
        publish_stats.validFollowups = pPubStats->validFollowups;
        publish_stats.invalidPublishServiceReqMsgs =
                                    pPubStats->invalidPublishServiceReqMsgs;
        publish_stats.invalidPublishServiceCancelReqMsgs =
                                pPubStats->invalidPublishServiceCancelReqMsgs;
        publish_stats.invalidActiveSubscribes =
                                pPubStats->invalidActiveSubscribes;
        publish_stats.invalidMatches = pPubStats->invalidMatches;
        publish_stats.invalidFollowups = pPubStats->invalidFollowups;
        publish_stats.publishCount = pPubStats->publishCount;
        publish_stats.publishNewMatchCount = pPubStats->publishNewMatchCount;
        publish_stats.pubsubGlobalNewMatchCount =
                               pPubStats->pubsubGlobalNewMatchCount;
        memcpy(&pRsp->data, &publish_stats, sizeof(NanPublishStats));
    } else if (stats_type == NAN_STATS_ID_DE_SUBSCRIBE) {
        NanSubscribeStats sub_stats;
        FwNanSubscribeStats *pSubStats = (FwNanSubscribeStats *)rspBuf;

        sub_stats.validSubscribeServiceReqMsgs =
                                pSubStats->validSubscribeServiceReqMsgs;
        sub_stats.validSubscribeServiceRspMsgs =
                                pSubStats->validSubscribeServiceRspMsgs;
        sub_stats.validSubscribeServiceCancelReqMsgs =
                                pSubStats->validSubscribeServiceCancelReqMsgs;
        sub_stats.validSubscribeServiceCancelRspMsgs =
                                pSubStats->validSubscribeServiceCancelRspMsgs;
        sub_stats.validSubscribeTerminatedIndMsgs =
                                pSubStats->validSubscribeTerminatedIndMsgs;
        sub_stats.validSubscribeMatchIndMsgs =
                                pSubStats->validSubscribeMatchIndMsgs;
        sub_stats.validSubscribeUnmatchIndMsgs =
                                pSubStats->validSubscribeUnmatchIndMsgs;
        sub_stats.validSolicitedPublishes =
                                pSubStats->validSolicitedPublishes;
        sub_stats.validMatches = pSubStats->validMatches;
        sub_stats.validFollowups = pSubStats->validFollowups;
        sub_stats.invalidSubscribeServiceReqMsgs =
                            pSubStats->invalidSubscribeServiceReqMsgs;
        sub_stats.invalidSubscribeServiceCancelReqMsgs =
                            pSubStats->invalidSubscribeServiceCancelReqMsgs;
        sub_stats.invalidSubscribeFollowupReqMsgs =
                            pSubStats->invalidSubscribeFollowupReqMsgs;
        sub_stats.invalidSolicitedPublishes =
                            pSubStats->invalidSolicitedPublishes;
        sub_stats.invalidMatches = pSubStats->invalidMatches;
        sub_stats.invalidFollowups = pSubStats->invalidFollowups;
        sub_stats.subscribeCount = pSubStats->subscribeCount;
        sub_stats.bloomFilterIndex = pSubStats->bloomFilterIndex;
        sub_stats.subscribeNewMatchCount = pSubStats->subscribeNewMatchCount;
        sub_stats.pubsubGlobalNewMatchCount =
                                      pSubStats->pubsubGlobalNewMatchCount;
        memcpy(&pRsp->data, &sub_stats, sizeof(NanSubscribeStats));
    } else if (stats_type == NAN_STATS_ID_DE_DW) {
        NanDWStats dw_stats;
        FwNanMacStats *pMacStats = (FwNanMacStats *)rspBuf;

        dw_stats.validFrames = pMacStats->validFrames;
        dw_stats.validActionFrames = pMacStats->validActionFrames;
        dw_stats.validBeaconFrames = pMacStats->validBeaconFrames;
        dw_stats.ignoredActionFrames = pMacStats->ignoredActionFrames;
        dw_stats.invalidFrames = pMacStats->invalidFrames;
        dw_stats.invalidActionFrames = pMacStats->invalidActionFrames;
        dw_stats.invalidBeaconFrames = pMacStats->invalidBeaconFrames;
        dw_stats.invalidMacHeaders = pMacStats->invalidMacHeaders;
        dw_stats.invalidPafHeaders  = pMacStats->invalidPafHeaders;
        dw_stats.nonNanBeaconFrames = pMacStats->nonNanBeaconFrames;
        dw_stats.earlyActionFrames = pMacStats->earlyActionFrames;
        dw_stats.inDwActionFrames = pMacStats->inDwActionFrames;
        dw_stats.lateActionFrames = pMacStats->lateActionFrames;
        dw_stats.framesQueued =  pMacStats->framesQueued;
        dw_stats.totalTRSpUpdates = pMacStats->totalTRSpUpdates;
        dw_stats.completeByTRSp = pMacStats->completeByTRSp;
        dw_stats.completeByTp75DW = pMacStats->completeByTp75DW;
        dw_stats.completeByTendDW = pMacStats->completeByTendDW;
        dw_stats.lateActionFramesTx = pMacStats->lateActionFramesTx;
        memcpy(&pRsp->data, &dw_stats, sizeof(NanDWStats));
    } else if (stats_type == NAN_STATS_ID_DE_MAC) {
        NanMacStats mac_stats;
        FwNanMacStats *pMacStats = (FwNanMacStats *)rspBuf;

        mac_stats.validFrames = pMacStats->validFrames;
        mac_stats.validActionFrames = pMacStats->validActionFrames;
        mac_stats.validBeaconFrames = pMacStats->validBeaconFrames;
        mac_stats.ignoredActionFrames = pMacStats->ignoredActionFrames;
        mac_stats.invalidFrames = pMacStats->invalidFrames;
        mac_stats.invalidActionFrames = pMacStats->invalidActionFrames;
        mac_stats.invalidBeaconFrames = pMacStats->invalidBeaconFrames;
        mac_stats.invalidMacHeaders = pMacStats->invalidMacHeaders;
        mac_stats.invalidPafHeaders  = pMacStats->invalidPafHeaders;
        mac_stats.nonNanBeaconFrames = pMacStats->nonNanBeaconFrames;
        mac_stats.earlyActionFrames = pMacStats->earlyActionFrames;
        mac_stats.inDwActionFrames = pMacStats->inDwActionFrames;
        mac_stats.lateActionFrames = pMacStats->lateActionFrames;
        mac_stats.framesQueued =  pMacStats->framesQueued;
        mac_stats.totalTRSpUpdates = pMacStats->totalTRSpUpdates;
        mac_stats.completeByTRSp = pMacStats->completeByTRSp;
        mac_stats.completeByTp75DW = pMacStats->completeByTp75DW;
        mac_stats.completeByTendDW = pMacStats->completeByTendDW;
        mac_stats.lateActionFramesTx = pMacStats->lateActionFramesTx;
        mac_stats.twIncreases = pMacStats->twIncreases;
        mac_stats.twDecreases = pMacStats->twDecreases;
        mac_stats.twChanges = pMacStats->twChanges;
        mac_stats.twHighwater = pMacStats->twHighwater;
        mac_stats.bloomFilterIndex = pMacStats->bloomFilterIndex;
        memcpy(&pRsp->data, &mac_stats, sizeof(NanMacStats));
    } else if (stats_type == NAN_STATS_ID_DE_TIMING_SYNC) {
        NanSyncStats sync_stats;
        FwNanSyncStats *pSyncStats = (FwNanSyncStats *)rspBuf;

        sync_stats.currTsf = pSyncStats->currTsf;
        sync_stats.myRank = pSyncStats->myRank;
        sync_stats.currAmRank = pSyncStats->currAmRank;
        sync_stats.lastAmRank = pSyncStats->lastAmRank;
        sync_stats.currAmBTT = pSyncStats->currAmBTT;
        sync_stats.lastAmBTT = pSyncStats->lastAmBTT;
        sync_stats.currAmHopCount = pSyncStats->currAmHopCount;
        sync_stats.currRole = pSyncStats->currRole;
        sync_stats.currClusterId = pSyncStats->currClusterId;

        sync_stats.timeSpentInCurrRole = pSyncStats->timeSpentInCurrRole;
        sync_stats.totalTimeSpentAsMaster = pSyncStats->totalTimeSpentAsMaster;
        sync_stats.totalTimeSpentAsNonMasterSync =
                            pSyncStats->totalTimeSpentAsNonMasterSync;
        sync_stats.totalTimeSpentAsNonMasterNonSync =
                            pSyncStats->totalTimeSpentAsNonMasterNonSync;
        sync_stats.transitionsToAnchorMaster =
                            pSyncStats->transitionsToAnchorMaster;
        sync_stats.transitionsToMaster =
                            pSyncStats->transitionsToMaster;
        sync_stats.transitionsToNonMasterSync =
                            pSyncStats->transitionsToNonMasterSync;
        sync_stats.transitionsToNonMasterNonSync =
                            pSyncStats->transitionsToNonMasterNonSync;
        sync_stats.amrUpdateCount = pSyncStats->amrUpdateCount;
        sync_stats.amrUpdateRankChangedCount =
                            pSyncStats->amrUpdateRankChangedCount;
        sync_stats.amrUpdateBTTChangedCount =
                            pSyncStats->amrUpdateBTTChangedCount;
        sync_stats.amrUpdateHcChangedCount =
                            pSyncStats->amrUpdateHcChangedCount;
        sync_stats.amrUpdateNewDeviceCount =
                            pSyncStats->amrUpdateNewDeviceCount;
        sync_stats.amrExpireCount = pSyncStats->amrExpireCount;
        sync_stats.mergeCount = pSyncStats->mergeCount;
        sync_stats.beaconsAboveHcLimit = pSyncStats->beaconsAboveHcLimit;
        sync_stats.beaconsBelowRssiThresh = pSyncStats->beaconsBelowRssiThresh;
        sync_stats.beaconsIgnoredNoSpace = pSyncStats->beaconsIgnoredNoSpace;
        sync_stats.beaconsForOurCluster = pSyncStats->beaconsForOtherCluster;
        sync_stats.beaconsForOtherCluster = pSyncStats->beaconsForOtherCluster;
        sync_stats.beaconCancelRequests = pSyncStats->beaconCancelRequests;
        sync_stats.beaconCancelFailures = pSyncStats->beaconCancelFailures;
        sync_stats.beaconUpdateRequests = pSyncStats->beaconUpdateRequests;
        sync_stats.beaconUpdateFailures = pSyncStats->beaconUpdateFailures;
        sync_stats.syncBeaconTxAttempts = pSyncStats->syncBeaconTxAttempts;
        sync_stats.syncBeaconTxFailures = pSyncStats->syncBeaconTxFailures;
        sync_stats.discBeaconTxAttempts = pSyncStats->discBeaconTxAttempts;
        sync_stats.discBeaconTxFailures = pSyncStats->discBeaconTxFailures;
        sync_stats.amHopCountExpireCount = pSyncStats->amHopCountExpireCount;
        memcpy(&pRsp->data, &sync_stats, sizeof(NanSyncStats));
    } else if (stats_type == NAN_STATS_ID_DE) {
        NanDeStats de_stats;
        FwNanDeStats *pDeStats = (FwNanDeStats *)rspBuf;

        de_stats.validErrorRspMsgs = pDeStats->validErrorRspMsgs;
        de_stats.validTransmitFollowupReqMsgs =
                        pDeStats->validTransmitFollowupReqMsgs;
        de_stats.validTransmitFollowupRspMsgs =
                        pDeStats->validTransmitFollowupRspMsgs;
        de_stats.validFollowupIndMsgs =
                        pDeStats->validFollowupIndMsgs;
        de_stats.validConfigurationReqMsgs =
                        pDeStats->validConfigurationReqMsgs;
        de_stats.validConfigurationRspMsgs =
                        pDeStats->validConfigurationRspMsgs;
        de_stats.validStatsReqMsgs = pDeStats->validStatsReqMsgs;
        de_stats.validStatsRspMsgs = pDeStats->validStatsRspMsgs;
        de_stats.validEnableReqMsgs = pDeStats->validEnableReqMsgs;
        de_stats.validEnableRspMsgs = pDeStats->validEnableRspMsgs;
        de_stats.validDisableReqMsgs = pDeStats->validDisableReqMsgs;
        de_stats.validDisableRspMsgs = pDeStats->validDisableRspMsgs;
        de_stats.validDisableIndMsgs = pDeStats->validDisableIndMsgs;
        de_stats.validEventIndMsgs = pDeStats->validEventIndMsgs;
        de_stats.validTcaReqMsgs = pDeStats->validTcaReqMsgs;
        de_stats.validTcaRspMsgs = pDeStats->validTcaRspMsgs;
        de_stats.validTcaIndMsgs = pDeStats->validTcaIndMsgs;
        de_stats.invalidTransmitFollowupReqMsgs =
                            pDeStats->invalidTransmitFollowupReqMsgs;
        de_stats.invalidConfigurationReqMsgs =
                            pDeStats->invalidConfigurationReqMsgs;
        de_stats.invalidStatsReqMsgs = pDeStats->invalidStatsReqMsgs;
        de_stats.invalidEnableReqMsgs = pDeStats->invalidEnableReqMsgs;
        de_stats.invalidDisableReqMsgs = pDeStats->invalidDisableReqMsgs;
        de_stats.invalidTcaReqMsgs = pDeStats->invalidTcaReqMsgs;
        memcpy(&pRsp->data, &de_stats, sizeof(NanDeStats));
    } else {
        ALOGE("Unknown stats_type:%d\n", stats_type);
    }
}
