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

#ifndef __WIFI_HAL_NAN_COMMAND_H__
#define __WIFI_HAL_NAN_COMMAND_H__

#include "common.h"
#include "cpp_bindings.h"
#include "wifi_hal.h"

class NanCommand : public WifiVendorCommand
{
private:
    NanCallbackHandler mHandler;
    char *mNanVendorEvent;
    u32 mNanDataLen;
    NanStaParameter *mStaParam;

    //Function to check the initial few bytes of data to
    //determine whether NanResponse or NanEvent
    int isNanResponse();
    //Function which unparses the data and calls the NotifyResponse
    int handleNanResponse();
    //Function which will parse the mVendorData and gets
    // the rsp_data appropriately.
    int getNanResponse(transaction_id *id, NanResponseMsg *pRsp);
    //Function which will return the Nan Indication type based on
    //the initial few bytes of mVendorData
    NanIndicationType getIndicationType();
    //Function which calls the necessaryIndication callback
    //based on the indication type
    int handleNanIndication();
    //Various Functions to get the appropriate indications
    int getNanPublishTerminated(NanPublishTerminatedInd *event);
    int getNanMatch(NanMatchInd *event);
    int getNanMatchExpired(NanMatchExpiredInd *event);
    int getNanSubscribeTerminated(NanSubscribeTerminatedInd *event);
    int getNanFollowup(NanFollowupInd *event);
    int getNanDiscEngEvent(NanDiscEngEventInd *event);
    int getNanDisabled(NanDisabledInd *event);
    int getNanTca(NanTCAInd *event);
    int getNanBeaconSdfPayload(NanBeaconSdfPayloadInd *event);
    //Internal cleanup function
    void cleanup();

    static NanCommand *mNanCommandInstance;

    // Other private helper functions
    int calcNanTransmitPostDiscoverySize(
        const NanTransmitPostDiscovery *pPostDiscovery);
    void fillNanSocialChannelParamVal(
        const NanSocialChannelScanParams *pScanParams,
        u32* pChannelParamArr);
    u32 getNanTransmitPostConnectivityCapabilityVal(
        const NanTransmitPostConnectivityCapability *pCapab);
    void fillNanTransmitPostDiscoveryVal(
        const NanTransmitPostDiscovery *pTxDisc,
        u8 *pOutValue);
    int calcNanFurtherAvailabilityMapSize(
        const NanFurtherAvailabilityMap *pFam);
    void fillNanFurtherAvailabilityMapVal(
        const NanFurtherAvailabilityMap *pFam,
        u8 *pOutValue);

    void getNanReceivePostConnectivityCapabilityVal(
        const u8* pInValue,
        NanReceivePostConnectivityCapability *pRxCapab);
    int getNanReceivePostDiscoveryVal(const u8 *pInValue,
                                      u32 length,
                                      NanReceivePostDiscovery *pRxDisc);
    int getNanFurtherAvailabilityMap(const u8 *pInValue,
                                     u32 length,
                                     u8* num_chans,
                                     NanFurtherAvailabilityChannel *pFac);
    void handleNanStatsResponse(NanStatsType stats_type,
                                char* rspBuf,
                                NanStatsResponse *pRsp);

public:
    NanCommand(wifi_handle handle, int id, u32 vendor_id, u32 subcmd);
    static NanCommand* instance(wifi_handle handle);
    virtual ~NanCommand();

    // This function implements creation of NAN specific Request
    // based on  the request type
    virtual int create();
    virtual int requestEvent();
    virtual int handleResponse(WifiEvent &reply);
    virtual int handleEvent(WifiEvent &event);
    int setCallbackHandler(NanCallbackHandler nHandler);


    //Functions to fill the vendor data appropriately
    int putNanEnable(transaction_id id, const NanEnableRequest *pReq);
    int putNanDisable(transaction_id id);
    int putNanPublish(transaction_id id, const NanPublishRequest *pReq);
    int putNanPublishCancel(transaction_id id, const NanPublishCancelRequest *pReq);
    int putNanSubscribe(transaction_id id, const NanSubscribeRequest *pReq);
    int putNanSubscribeCancel(transaction_id id, const NanSubscribeCancelRequest *pReq);
    int putNanTransmitFollowup(transaction_id id, const NanTransmitFollowupRequest *pReq);
    int putNanStats(transaction_id id, const NanStatsRequest *pReq);
    int putNanConfig(transaction_id id, const NanConfigRequest *pReq);
    int putNanTCA(transaction_id id, const NanTCARequest *pReq);
    int putNanBeaconSdfPayload(transaction_id id, const NanBeaconSdfPayloadRequest *pReq);
    int getNanStaParameter(wifi_interface_handle iface, NanStaParameter *pRsp);
    int putNanCapabilities(transaction_id id);
};
#endif /* __WIFI_HAL_NAN_COMMAND_H__ */

