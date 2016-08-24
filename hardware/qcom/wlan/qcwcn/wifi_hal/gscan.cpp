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
#define LOG_TAG  "WifiHAL"
#include <utils/Log.h>
#include <time.h>
#include <errno.h>
#include <stdlib.h>

#include "common.h"
#include "cpp_bindings.h"
#include "gscancommand.h"
#include "gscan_event_handler.h"

#define GSCAN_EVENT_WAIT_TIME_SECONDS 4

/* Used to handle gscan command events from driver/firmware.*/
typedef struct gscan_event_handlers_s {
    GScanCommandEventHandler *gscanStartCmdEventHandler;
    GScanCommandEventHandler *gScanSetBssidHotlistCmdEventHandler;
    GScanCommandEventHandler *gScanSetSignificantChangeCmdEventHandler;
    GScanCommandEventHandler *gScanSetSsidHotlistCmdEventHandler;
    GScanCommandEventHandler *gScanSetPnoListCmdEventHandler;
    GScanCommandEventHandler *gScanPnoSetPasspointListCmdEventHandler;
} gscan_event_handlers;

wifi_error initializeGscanHandlers(hal_info *info)
{
    info->gscan_handlers = (gscan_event_handlers *)malloc(sizeof(gscan_event_handlers));
    if (info->gscan_handlers) {
        memset(info->gscan_handlers, 0, sizeof(gscan_event_handlers));
    }
    else {
        ALOGE("%s: Allocation of gscan event handlers failed",
              __FUNCTION__);
        return WIFI_ERROR_OUT_OF_MEMORY;
    }
    return WIFI_SUCCESS;
}

wifi_error cleanupGscanHandlers(hal_info *info)
{
    gscan_event_handlers* event_handlers;
    if (info && info->gscan_handlers) {
        event_handlers = (gscan_event_handlers*) info->gscan_handlers;
        if (event_handlers->gscanStartCmdEventHandler) {
            delete event_handlers->gscanStartCmdEventHandler;
        }
        if (event_handlers->gScanSetBssidHotlistCmdEventHandler) {
            delete event_handlers->gScanSetBssidHotlistCmdEventHandler;
        }
        if (event_handlers->gScanSetSignificantChangeCmdEventHandler) {
            delete event_handlers->gScanSetSignificantChangeCmdEventHandler;
        }
        if (event_handlers->gScanSetSsidHotlistCmdEventHandler) {
            delete event_handlers->gScanSetSsidHotlistCmdEventHandler;
        }
        if (event_handlers->gScanSetPnoListCmdEventHandler) {
            delete event_handlers->gScanSetPnoListCmdEventHandler;
        }
        if (event_handlers->gScanPnoSetPasspointListCmdEventHandler) {
            delete event_handlers->gScanPnoSetPasspointListCmdEventHandler;
        }
        memset(event_handlers, 0, sizeof(gscan_event_handlers));
        return WIFI_SUCCESS;
    }
    ALOGE ("%s: info or info->gscan_handlers NULL", __FUNCTION__);
    return WIFI_ERROR_UNKNOWN;
}

/* Implementation of the API functions exposed in gscan.h */
wifi_error wifi_get_valid_channels(wifi_interface_handle handle,
       int band, int max_channels, wifi_channel *channels, int *num_channels)
{
    int requestId, ret = 0, i=0;
    GScanCommand *gScanCommand;
    struct nlattr *nlData;
    interface_info *ifaceInfo = getIfaceInfo(handle);
    wifi_handle wifiHandle = getWifiHandle(handle);
    hal_info *info = getHalInfo(wifiHandle);
    lowi_cb_table_t *lowiWifiHalApi = NULL;

    /* Route GSCAN request through LOWI if supported */
    lowiWifiHalApi = getLowiCallbackTable(GSCAN_SUPPORTED);
    if (lowiWifiHalApi == NULL ||
        lowiWifiHalApi->get_valid_channels == NULL) {
        ALOGV("%s: Sending cmd directly to host", __FUNCTION__);
    } else {
        ret = lowiWifiHalApi->get_valid_channels(handle, band, max_channels,
                          channels, num_channels);
        ALOGV("%s: lowi get_valid_channels "
            "returned: %d. Exit.", __FUNCTION__, ret);
        return (wifi_error)ret;
    }

    /* No request id from caller, so generate one and pass it on to the driver.
     * Generate one randomly.
     */
    requestId = get_requestid();
    ALOGV("%s: RequestId:%d band:%d max_channels:%d", __FUNCTION__,
          requestId, band, max_channels);

    if (channels == NULL) {
        ALOGE("%s: NULL channels pointer provided. Exit.",
            __FUNCTION__);
        return WIFI_ERROR_INVALID_ARGS;
    }

    gScanCommand = new GScanCommand(
                            wifiHandle,
                            requestId,
                            OUI_QCA,
                            QCA_NL80211_VENDOR_SUBCMD_GSCAN_GET_VALID_CHANNELS);
    if (gScanCommand == NULL) {
        ALOGE("%s: Error GScanCommand NULL", __FUNCTION__);
        return WIFI_ERROR_UNKNOWN;
    }
    /* Create the NL message. */
    ret = gScanCommand->create();
    if (ret < 0)
        goto cleanup;

    /* Set the interface Id of the message. */
    ret = gScanCommand->set_iface_id(ifaceInfo->name);
    if (ret < 0)
        goto cleanup;

    /* Add the vendor specific attributes for the NL command. */
    nlData = gScanCommand->attr_start(NL80211_ATTR_VENDOR_DATA);
    if (!nlData)
        goto cleanup;

    if (gScanCommand->put_u32(
            QCA_WLAN_VENDOR_ATTR_GSCAN_SUBCMD_CONFIG_PARAM_REQUEST_ID,
            requestId) ||
        gScanCommand->put_u32(
        QCA_WLAN_VENDOR_ATTR_GSCAN_GET_VALID_CHANNELS_CONFIG_PARAM_WIFI_BAND,
            band) ||
        gScanCommand->put_u32(
        QCA_WLAN_VENDOR_ATTR_GSCAN_GET_VALID_CHANNELS_CONFIG_PARAM_MAX_CHANNELS,
            max_channels) )
    {
        goto cleanup;
    }
    gScanCommand->attr_end(nlData);
    /* Populate the input received from caller/framework. */
    gScanCommand->setMaxChannels(max_channels);
    gScanCommand->setChannels(channels);
    gScanCommand->setNumChannelsPtr(num_channels);

    /* Send the msg and wait for a response. */
    ret = gScanCommand->requestResponse();
    if (ret) {
        ALOGE("%s: Error %d happened. ", __FUNCTION__, ret);
    }

cleanup:
    delete gScanCommand;
    return (wifi_error)ret;
}

wifi_error wifi_get_gscan_capabilities(wifi_interface_handle handle,
                                 wifi_gscan_capabilities *capabilities)
{
    int requestId, ret = 0;
    GScanCommand *gScanCommand;
    struct nlattr *nlData;
    wifi_gscan_capabilities tCapabilities;
    interface_info *ifaceInfo = getIfaceInfo(handle);
    wifi_handle wifiHandle = getWifiHandle(handle);
    hal_info *info = getHalInfo(wifiHandle);
    lowi_cb_table_t *lowiWifiHalApi = NULL;

    if (!(info->supported_feature_set & WIFI_FEATURE_GSCAN)) {
        ALOGE("%s: GSCAN is not supported by driver",
            __FUNCTION__);
        return WIFI_ERROR_NOT_SUPPORTED;
    }

    /* Route GSCAN request through LOWI if supported */
    lowiWifiHalApi = getLowiCallbackTable(GSCAN_SUPPORTED);
    if (lowiWifiHalApi == NULL ||
        lowiWifiHalApi->get_gscan_capabilities == NULL) {
        ALOGV("%s: Sending cmd directly to host", __FUNCTION__);
    } else {
        ret = lowiWifiHalApi->get_gscan_capabilities(handle,
                                                     capabilities);
        ALOGV("%s: lowi get_gscan_capabilities "
            "returned: %d. Exit.", __FUNCTION__, ret);
        return (wifi_error)ret;
    }

    /* No request id from caller, so generate one and pass it on to the driver.
     * Generate it randomly.
     */
    requestId = get_requestid();

    if (capabilities == NULL) {
        ALOGE("%s: NULL capabilities pointer provided. Exit.",
            __FUNCTION__);
        return WIFI_ERROR_INVALID_ARGS;
    }

    gScanCommand = new GScanCommand(
                            wifiHandle,
                            requestId,
                            OUI_QCA,
                            QCA_NL80211_VENDOR_SUBCMD_GSCAN_GET_CAPABILITIES);
    if (gScanCommand == NULL) {
        ALOGE("%s: Error GScanCommand NULL", __FUNCTION__);
        return WIFI_ERROR_UNKNOWN;
    }

    /* Create the NL message. */
    ret = gScanCommand->create();
    if (ret < 0)
        goto cleanup;

    /* Set the interface Id of the message. */
    ret = gScanCommand->set_iface_id(ifaceInfo->name);
    if (ret < 0)
        goto cleanup;

    /* Add the vendor specific attributes for the NL command. */
    nlData = gScanCommand->attr_start(NL80211_ATTR_VENDOR_DATA);
    if (!nlData)
        goto cleanup;

    ret = gScanCommand->put_u32(
            QCA_WLAN_VENDOR_ATTR_GSCAN_SUBCMD_CONFIG_PARAM_REQUEST_ID,
            requestId);
    if (ret < 0)
        goto cleanup;

    gScanCommand->attr_end(nlData);
    ret = gScanCommand->allocRspParams(eGScanGetCapabilitiesRspParams);
    if (ret != 0) {
        ALOGE("%s: Failed to allocate memory fo response struct. Error:%d",
            __FUNCTION__, ret);
        goto cleanup;
    }

    ret = gScanCommand->requestResponse();
    if (ret != 0) {
        ALOGE("%s: requestResponse Error:%d",__FUNCTION__, ret);
        goto cleanup;
    }

    ret = gScanCommand->getGetCapabilitiesRspParams(capabilities);
    if (ret != 0) {
        ALOGE("%s: invalid capabilities received:%d",__FUNCTION__, ret);
        goto cleanup;
    }

cleanup:
    gScanCommand->freeRspParams(eGScanGetCapabilitiesRspParams);
    delete gScanCommand;
    return (wifi_error)ret;
}

wifi_error wifi_start_gscan(wifi_request_id id,
                            wifi_interface_handle iface,
                            wifi_scan_cmd_params params,
                            wifi_scan_result_handler handler)
{
    int ret = 0;
    u32 i, j;
    GScanCommand *gScanCommand;
    struct nlattr *nlData;
    interface_info *ifaceInfo = getIfaceInfo(iface);
    wifi_handle wifiHandle = getWifiHandle(iface);
    u32 num_scan_buckets, numChannelSpecs;
    wifi_scan_bucket_spec bucketSpec;
    struct nlattr *nlBuckectSpecList;
    bool previousGScanRunning = false;
    hal_info *info = getHalInfo(wifiHandle);
    lowi_cb_table_t *lowiWifiHalApi = NULL;
    gscan_event_handlers* event_handlers;
    GScanCommandEventHandler *gScanStartCmdEventHandler;

    event_handlers = (gscan_event_handlers*)info->gscan_handlers;
    gScanStartCmdEventHandler = event_handlers->gscanStartCmdEventHandler;

    if (!(info->supported_feature_set & WIFI_FEATURE_GSCAN)) {
        ALOGE("%s: GSCAN is not supported by driver",
            __FUNCTION__);
        return WIFI_ERROR_NOT_SUPPORTED;
    }

    /* Route GSCAN request through LOWI if supported */
    lowiWifiHalApi = getLowiCallbackTable(GSCAN_SUPPORTED);
    if (lowiWifiHalApi == NULL ||
        lowiWifiHalApi->start_gscan  == NULL) {
        ALOGV("%s: Sending cmd directly to host", __FUNCTION__);
    } else {
        ret = lowiWifiHalApi->start_gscan(id, iface, params, handler);
        ALOGV("%s: lowi start_gscan "
            "returned: %d. Exit.", __FUNCTION__, ret);
        return (wifi_error)ret;
    }

    ALOGV("%s: RequestId:%d ", __FUNCTION__, id);
    /* Wi-Fi HAL doesn't need to check if a similar request to start gscan was
     *  made earlier. If start_gscan() is called while another gscan is already
     *  running, the request will be sent down to driver and firmware. If new
     * request is successfully honored, then Wi-Fi HAL will use the new request
     * id for the gScanStartCmdEventHandler object.
     */
    gScanCommand = new GScanCommand(
                                wifiHandle,
                                id,
                                OUI_QCA,
                                QCA_NL80211_VENDOR_SUBCMD_GSCAN_START);
    if (gScanCommand == NULL) {
        ALOGE("%s: Error GScanCommand NULL", __FUNCTION__);
        return WIFI_ERROR_UNKNOWN;
    }

    /* Create the NL message. */
    ret = gScanCommand->create();
    if (ret < 0)
        goto cleanup;

    /* Set the interface Id of the message. */
    ret = gScanCommand->set_iface_id(ifaceInfo->name);
    if (ret < 0)
        goto cleanup;

    /* Add the vendor specific attributes for the NL command. */
    nlData = gScanCommand->attr_start(NL80211_ATTR_VENDOR_DATA);
    if (!nlData)
        goto cleanup;

    num_scan_buckets = (unsigned int)params.num_buckets > MAX_BUCKETS ?
                            MAX_BUCKETS : params.num_buckets;

    ALOGV("%s: Base Period:%d Max_ap_per_scan:%d "
          "Threshold_percent:%d Threshold_num_scans:%d "
          "num_buckets:%d", __FUNCTION__, params.base_period,
          params.max_ap_per_scan, params.report_threshold_percent,
          params.report_threshold_num_scans, num_scan_buckets);
    if (gScanCommand->put_u32(
            QCA_WLAN_VENDOR_ATTR_GSCAN_SUBCMD_CONFIG_PARAM_REQUEST_ID,
            id) ||
        gScanCommand->put_u32(
            QCA_WLAN_VENDOR_ATTR_GSCAN_SCAN_CMD_PARAMS_BASE_PERIOD,
            params.base_period) ||
        gScanCommand->put_u32(
            QCA_WLAN_VENDOR_ATTR_GSCAN_SCAN_CMD_PARAMS_MAX_AP_PER_SCAN,
            params.max_ap_per_scan) ||
        gScanCommand->put_u8(
            QCA_WLAN_VENDOR_ATTR_GSCAN_SCAN_CMD_PARAMS_REPORT_THRESHOLD_PERCENT,
            params.report_threshold_percent) ||
        gScanCommand->put_u8(
            QCA_WLAN_VENDOR_ATTR_GSCAN_SCAN_CMD_PARAMS_REPORT_THRESHOLD_NUM_SCANS,
            params.report_threshold_num_scans) ||
        gScanCommand->put_u8(
            QCA_WLAN_VENDOR_ATTR_GSCAN_SCAN_CMD_PARAMS_NUM_BUCKETS,
            num_scan_buckets))
    {
        goto cleanup;
    }

    nlBuckectSpecList =
        gScanCommand->attr_start(QCA_WLAN_VENDOR_ATTR_GSCAN_BUCKET_SPEC);
    /* Add NL attributes for scan bucket specs . */
    for (i = 0; i < num_scan_buckets; i++) {
        bucketSpec = params.buckets[i];
        numChannelSpecs = (unsigned int)bucketSpec.num_channels > MAX_CHANNELS ?
                                MAX_CHANNELS : bucketSpec.num_channels;

        ALOGV("%s: Index: %d Bucket Id:%d Band:%d Period:%d ReportEvent:%d "
              "numChannelSpecs:%d max_period:%d base:%d step_count:%d",
              __FUNCTION__, i, bucketSpec.bucket, bucketSpec.band,
              bucketSpec.period, bucketSpec.report_events,
              numChannelSpecs, bucketSpec.max_period,
              bucketSpec.base, bucketSpec.step_count);

        struct nlattr *nlBucketSpec = gScanCommand->attr_start(i);
        if (gScanCommand->put_u8(
                QCA_WLAN_VENDOR_ATTR_GSCAN_BUCKET_SPEC_INDEX,
                bucketSpec.bucket) ||
            gScanCommand->put_u8(
                QCA_WLAN_VENDOR_ATTR_GSCAN_BUCKET_SPEC_BAND,
                bucketSpec.band) ||
            gScanCommand->put_u32(
                QCA_WLAN_VENDOR_ATTR_GSCAN_BUCKET_SPEC_PERIOD,
                bucketSpec.period) ||
            gScanCommand->put_u8(
                QCA_WLAN_VENDOR_ATTR_GSCAN_BUCKET_SPEC_REPORT_EVENTS,
                bucketSpec.report_events) ||
            gScanCommand->put_u32(
                QCA_WLAN_VENDOR_ATTR_GSCAN_BUCKET_SPEC_NUM_CHANNEL_SPECS,
                numChannelSpecs) ||
            gScanCommand->put_u32(
                QCA_WLAN_VENDOR_ATTR_GSCAN_BUCKET_SPEC_MAX_PERIOD,
                bucketSpec.max_period) ||
            gScanCommand->put_u32(
                QCA_WLAN_VENDOR_ATTR_GSCAN_BUCKET_SPEC_BASE,
                bucketSpec.base) ||
            gScanCommand->put_u32(
                QCA_WLAN_VENDOR_ATTR_GSCAN_BUCKET_SPEC_STEP_COUNT,
                bucketSpec.step_count))
        {
            goto cleanup;
        }

        struct nlattr *nl_channelSpecList =
            gScanCommand->attr_start(QCA_WLAN_VENDOR_ATTR_GSCAN_CHANNEL_SPEC);

        /* Add NL attributes for scan channel specs . */
        for (j = 0; j < numChannelSpecs; j++) {
            struct nlattr *nl_channelSpec = gScanCommand->attr_start(j);
            wifi_scan_channel_spec channel_spec = bucketSpec.channels[j];

            ALOGV("%s: Channel Spec Index:%d Channel:%d Dwell Time:%d "
                  "passive:%d", __FUNCTION__, j, channel_spec.channel,
                  channel_spec.dwellTimeMs, channel_spec.passive);

            if ( gScanCommand->put_u32(
                    QCA_WLAN_VENDOR_ATTR_GSCAN_CHANNEL_SPEC_CHANNEL,
                    channel_spec.channel) ||
                gScanCommand->put_u32(
                    QCA_WLAN_VENDOR_ATTR_GSCAN_CHANNEL_SPEC_DWELL_TIME,
                    channel_spec.dwellTimeMs) ||
                gScanCommand->put_u8(
                    QCA_WLAN_VENDOR_ATTR_GSCAN_CHANNEL_SPEC_PASSIVE,
                    channel_spec.passive) )
            {
                goto cleanup;
            }

            gScanCommand->attr_end(nl_channelSpec);
        }
        gScanCommand->attr_end(nl_channelSpecList);
        gScanCommand->attr_end(nlBucketSpec);
    }
    gScanCommand->attr_end(nlBuckectSpecList);

    gScanCommand->attr_end(nlData);

    /* Set the callback handler functions for related events. */
    GScanCallbackHandler callbackHandler;
    memset(&callbackHandler, 0, sizeof(callbackHandler));
    callbackHandler.on_full_scan_result = handler.on_full_scan_result;
    callbackHandler.on_scan_event = handler.on_scan_event;

    /* Create an object to handle the related events from firmware/driver. */
    if (gScanStartCmdEventHandler == NULL) {
        gScanStartCmdEventHandler = new GScanCommandEventHandler(
                                    wifiHandle,
                                    id,
                                    OUI_QCA,
                                    QCA_NL80211_VENDOR_SUBCMD_GSCAN_START,
                                    callbackHandler);
        if (gScanStartCmdEventHandler == NULL) {
            ALOGE("%s: Error gScanStartCmdEventHandler NULL", __FUNCTION__);
            ret = WIFI_ERROR_UNKNOWN;
            goto cleanup;
        }
        event_handlers->gscanStartCmdEventHandler = gScanStartCmdEventHandler;
    } else {
        gScanStartCmdEventHandler->setCallbackHandler(callbackHandler);
    }

    ret = gScanCommand->requestResponse();
    if (ret != 0) {
        ALOGE("%s : requestResponse Error:%d", __FUNCTION__, ret);
        goto cleanup;
    }

    if (gScanStartCmdEventHandler != NULL) {
        gScanStartCmdEventHandler->set_request_id(id);
        gScanStartCmdEventHandler->enableEventHandling();
    }

cleanup:
    delete gScanCommand;
    /* Disable Event Handling if ret != 0 */
    if (ret && gScanStartCmdEventHandler) {
        ALOGI("%s: Error ret:%d, disable event handling",
            __FUNCTION__, ret);
        gScanStartCmdEventHandler->disableEventHandling();
    }
    return (wifi_error)ret;

}

wifi_error wifi_stop_gscan(wifi_request_id id,
                            wifi_interface_handle iface)
{
    int ret = 0;
    GScanCommand *gScanCommand;
    struct nlattr *nlData;
    lowi_cb_table_t *lowiWifiHalApi = NULL;

    interface_info *ifaceInfo = getIfaceInfo(iface);
    wifi_handle wifiHandle = getWifiHandle(iface);
    hal_info *info = getHalInfo(wifiHandle);
    gscan_event_handlers* event_handlers;
    GScanCommandEventHandler *gScanStartCmdEventHandler;

    event_handlers = (gscan_event_handlers*)info->gscan_handlers;
    gScanStartCmdEventHandler = event_handlers->gscanStartCmdEventHandler;

    if (!(info->supported_feature_set & WIFI_FEATURE_GSCAN)) {
        ALOGE("%s: GSCAN is not supported by driver",
            __FUNCTION__);
        return WIFI_ERROR_NOT_SUPPORTED;
    }

    /* Route GSCAN request through LOWI if supported */
    lowiWifiHalApi = getLowiCallbackTable(GSCAN_SUPPORTED);
    if (lowiWifiHalApi == NULL ||
        lowiWifiHalApi->stop_gscan == NULL) {
        ALOGV("%s: Sending cmd directly to host", __FUNCTION__);
    } else {
        ret = lowiWifiHalApi->stop_gscan(id, iface);
        ALOGV("%s: lowi stop_gscan "
            "returned: %d. Exit.", __FUNCTION__, ret);
        return (wifi_error)ret;
    }

    if (gScanStartCmdEventHandler == NULL ||
        gScanStartCmdEventHandler->isEventHandlingEnabled() == false) {
        ALOGE("%s: GSCAN isn't running or already stopped. "
            "Nothing to do. Exit", __FUNCTION__);
        return WIFI_ERROR_NOT_AVAILABLE;
    }

    gScanCommand = new GScanCommand(
                                wifiHandle,
                                id,
                                OUI_QCA,
                                QCA_NL80211_VENDOR_SUBCMD_GSCAN_STOP);
    if (gScanCommand == NULL) {
        ALOGE("%s: Error GScanCommand NULL", __FUNCTION__);
        return WIFI_ERROR_UNKNOWN;
    }

    /* Create the NL message. */
    ret = gScanCommand->create();
    if (ret < 0)
        goto cleanup;

    /* Set the interface Id of the message. */
    ret = gScanCommand->set_iface_id(ifaceInfo->name);
    if (ret < 0)
        goto cleanup;

    /* Add the vendor specific attributes for the NL command. */
    nlData = gScanCommand->attr_start(NL80211_ATTR_VENDOR_DATA);
    if (!nlData)
        goto cleanup;

    ret = gScanCommand->put_u32(
            QCA_WLAN_VENDOR_ATTR_GSCAN_SUBCMD_CONFIG_PARAM_REQUEST_ID,
            id);
    if (ret < 0)
        goto cleanup;

    gScanCommand->attr_end(nlData);

    ret = gScanCommand->requestResponse();
    if (ret != 0) {
        ALOGE("%s: requestResponse Error:%d",__FUNCTION__, ret);
    }

    /* Disable Event Handling. */
    if (gScanStartCmdEventHandler) {
        gScanStartCmdEventHandler->disableEventHandling();
    }

cleanup:
    delete gScanCommand;
    return (wifi_error)ret;
}

/* Set the GSCAN BSSID Hotlist. */
wifi_error wifi_set_bssid_hotlist(wifi_request_id id,
                                    wifi_interface_handle iface,
                                    wifi_bssid_hotlist_params params,
                                    wifi_hotlist_ap_found_handler handler)
{
    int i, numAp, ret = 0;
    GScanCommand *gScanCommand;
    struct nlattr *nlData, *nlApThresholdParamList;
    interface_info *ifaceInfo = getIfaceInfo(iface);
    wifi_handle wifiHandle = getWifiHandle(iface);
    bool previousGScanSetBssidRunning = false;
    hal_info *info = getHalInfo(wifiHandle);
    lowi_cb_table_t *lowiWifiHalApi = NULL;
    gscan_event_handlers* event_handlers;
    GScanCommandEventHandler *gScanSetBssidHotlistCmdEventHandler;

    event_handlers = (gscan_event_handlers*)info->gscan_handlers;
    gScanSetBssidHotlistCmdEventHandler =
        event_handlers->gScanSetBssidHotlistCmdEventHandler;

    if (!(info->supported_feature_set & WIFI_FEATURE_GSCAN)) {
        ALOGE("%s: GSCAN is not supported by driver",
            __FUNCTION__);
        return WIFI_ERROR_NOT_SUPPORTED;
    }

    /* Route request through LOWI if supported*/
    lowiWifiHalApi = getLowiCallbackTable(GSCAN_SUPPORTED);
    if (lowiWifiHalApi == NULL ||
        lowiWifiHalApi->set_bssid_hotlist == NULL) {
        ALOGV("%s: Sending cmd directly to host", __FUNCTION__);
    } else {
        ret = lowiWifiHalApi->set_bssid_hotlist(id, iface, params,handler);
        ALOGV("%s: lowi set_bssid_hotlist "
            "returned: %d. Exit.", __FUNCTION__, ret);
        return (wifi_error)ret;
    }

    /* Wi-Fi HAL doesn't need to check if a similar request to set bssid
     * hotlist was made earlier. If set_bssid_hotlist() is called while
     * another one is running, the request will be sent down to driver and
     * firmware. If the new request is successfully honored, then Wi-Fi HAL
     * will use the new request id for the gScanSetBssidHotlistCmdEventHandler
     * object.
     */

    gScanCommand =
        new GScanCommand(
                    wifiHandle,
                    id,
                    OUI_QCA,
                    QCA_NL80211_VENDOR_SUBCMD_GSCAN_SET_BSSID_HOTLIST);
    if (gScanCommand == NULL) {
        ALOGE("%s: Error GScanCommand NULL", __FUNCTION__);
        return WIFI_ERROR_UNKNOWN;
    }

    /* Create the NL message. */
    ret = gScanCommand->create();
    if (ret < 0)
        goto cleanup;

    /* Set the interface Id of the message. */
    ret = gScanCommand->set_iface_id(ifaceInfo->name);
    if (ret < 0)
        goto cleanup;

    /* Add the vendor specific attributes for the NL command. */
    nlData = gScanCommand->attr_start(NL80211_ATTR_VENDOR_DATA);
    if (!nlData)
        goto cleanup;

    numAp = (unsigned int)params.num_bssid > MAX_HOTLIST_APS ?
        MAX_HOTLIST_APS : params.num_bssid;
    if (gScanCommand->put_u32(
            QCA_WLAN_VENDOR_ATTR_GSCAN_SUBCMD_CONFIG_PARAM_REQUEST_ID,
            id) ||
        gScanCommand->put_u32(
            QCA_WLAN_VENDOR_ATTR_GSCAN_BSSID_HOTLIST_PARAMS_LOST_AP_SAMPLE_SIZE,
            params.lost_ap_sample_size) ||
        gScanCommand->put_u32(
            QCA_WLAN_VENDOR_ATTR_GSCAN_BSSID_HOTLIST_PARAMS_NUM_AP,
            numAp))
    {
        goto cleanup;
    }

    ALOGV("%s: lost_ap_sample_size:%d numAp:%d", __FUNCTION__,
          params.lost_ap_sample_size, numAp);
    /* Add the vendor specific attributes for the NL command. */
    nlApThresholdParamList =
        gScanCommand->attr_start(
                                QCA_WLAN_VENDOR_ATTR_GSCAN_AP_THRESHOLD_PARAM);
    if (!nlApThresholdParamList)
        goto cleanup;

    /* Add nested NL attributes for AP Threshold Param. */
    for (i = 0; i < numAp; i++) {
        ap_threshold_param apThreshold = params.ap[i];
        struct nlattr *nlApThresholdParam = gScanCommand->attr_start(i);
        if (!nlApThresholdParam)
            goto cleanup;
        if (gScanCommand->put_addr(
                QCA_WLAN_VENDOR_ATTR_GSCAN_AP_THRESHOLD_PARAM_BSSID,
                apThreshold.bssid) ||
            gScanCommand->put_s32(
                QCA_WLAN_VENDOR_ATTR_GSCAN_AP_THRESHOLD_PARAM_RSSI_LOW,
                apThreshold.low) ||
            gScanCommand->put_s32(
                QCA_WLAN_VENDOR_ATTR_GSCAN_AP_THRESHOLD_PARAM_RSSI_HIGH,
                apThreshold.high))
        {
            goto cleanup;
        }
        ALOGV("%s: Index:%d BssId: %hhx:%hhx:%hhx:%hhx:%hhx:%hhx "
              "Threshold low:%d high:%d", __FUNCTION__, i,
              apThreshold.bssid[0], apThreshold.bssid[1],
              apThreshold.bssid[2], apThreshold.bssid[3],
              apThreshold.bssid[4], apThreshold.bssid[5],
              apThreshold.low, apThreshold.high);
        gScanCommand->attr_end(nlApThresholdParam);
    }

    gScanCommand->attr_end(nlApThresholdParamList);

    gScanCommand->attr_end(nlData);

    GScanCallbackHandler callbackHandler;
    memset(&callbackHandler, 0, sizeof(callbackHandler));
    callbackHandler.on_hotlist_ap_found = handler.on_hotlist_ap_found;
    callbackHandler.on_hotlist_ap_lost = handler.on_hotlist_ap_lost;

    /* Create an object of the event handler class to take care of the
      * asychronous events on the north-bound.
      */
    if (gScanSetBssidHotlistCmdEventHandler == NULL) {
        gScanSetBssidHotlistCmdEventHandler = new GScanCommandEventHandler(
                            wifiHandle,
                            id,
                            OUI_QCA,
                            QCA_NL80211_VENDOR_SUBCMD_GSCAN_SET_BSSID_HOTLIST,
                            callbackHandler);
        if (gScanSetBssidHotlistCmdEventHandler == NULL) {
            ALOGE("%s: Error instantiating "
                "gScanSetBssidHotlistCmdEventHandler.", __FUNCTION__);
            ret = WIFI_ERROR_UNKNOWN;
            goto cleanup;
        }
        event_handlers->gScanSetBssidHotlistCmdEventHandler =
            gScanSetBssidHotlistCmdEventHandler;
    } else {
        gScanSetBssidHotlistCmdEventHandler->setCallbackHandler(callbackHandler);
    }

    ret = gScanCommand->requestResponse();
    if (ret != 0) {
        ALOGE("%s: requestResponse Error:%d",__FUNCTION__, ret);
        goto cleanup;
    }

    if (gScanSetBssidHotlistCmdEventHandler != NULL) {
        gScanSetBssidHotlistCmdEventHandler->set_request_id(id);
        gScanSetBssidHotlistCmdEventHandler->enableEventHandling();
    }

cleanup:
    delete gScanCommand;
    /* Disable Event Handling if ret != 0 */
    if (ret && gScanSetBssidHotlistCmdEventHandler) {
        ALOGI("%s: Error ret:%d, disable event handling",
            __FUNCTION__, ret);
        gScanSetBssidHotlistCmdEventHandler->disableEventHandling();
    }
    return (wifi_error)ret;
}

wifi_error wifi_reset_bssid_hotlist(wifi_request_id id,
                            wifi_interface_handle iface)
{
    int ret = 0;
    GScanCommand *gScanCommand;
    struct nlattr *nlData;
    interface_info *ifaceInfo = getIfaceInfo(iface);
    wifi_handle wifiHandle = getWifiHandle(iface);
    hal_info *info = getHalInfo(wifiHandle);
    lowi_cb_table_t *lowiWifiHalApi = NULL;
    gscan_event_handlers* event_handlers;
    GScanCommandEventHandler *gScanSetBssidHotlistCmdEventHandler;

    event_handlers = (gscan_event_handlers*)info->gscan_handlers;
    gScanSetBssidHotlistCmdEventHandler =
        event_handlers->gScanSetBssidHotlistCmdEventHandler;

    if (!(info->supported_feature_set & WIFI_FEATURE_GSCAN)) {
        ALOGE("%s: GSCAN is not supported by driver",
            __FUNCTION__);
        return WIFI_ERROR_NOT_SUPPORTED;
    }

    /* Route request through LOWI if supported*/
    lowiWifiHalApi = getLowiCallbackTable(GSCAN_SUPPORTED);
    if (lowiWifiHalApi == NULL ||
        lowiWifiHalApi->reset_bssid_hotlist == NULL) {
        ALOGV("%s: Sending cmd directly to host", __FUNCTION__);
    } else {
        ret = lowiWifiHalApi->reset_bssid_hotlist(id, iface);
        ALOGV("%s: lowi reset_bssid_hotlist "
            "returned: %d. Exit.", __FUNCTION__, ret);
        return (wifi_error)ret;
    }


    if (gScanSetBssidHotlistCmdEventHandler == NULL ||
        (gScanSetBssidHotlistCmdEventHandler->isEventHandlingEnabled() ==
         false)) {
        ALOGE("wifi_reset_bssid_hotlist: GSCAN bssid_hotlist isn't set. "
            "Nothing to do. Exit");
        return WIFI_ERROR_NOT_AVAILABLE;
    }

    gScanCommand = new GScanCommand(
                        wifiHandle,
                        id,
                        OUI_QCA,
                        QCA_NL80211_VENDOR_SUBCMD_GSCAN_RESET_BSSID_HOTLIST);

    if (gScanCommand == NULL) {
        ALOGE("%s: Error GScanCommand NULL", __FUNCTION__);
        return WIFI_ERROR_UNKNOWN;
    }

    /* Create the NL message. */
    ret = gScanCommand->create();
    if (ret < 0)
        goto cleanup;

    /* Set the interface Id of the message. */
    ret = gScanCommand->set_iface_id(ifaceInfo->name);
    if (ret < 0)
        goto cleanup;

    /* Add the vendor specific attributes for the NL command. */
    nlData = gScanCommand->attr_start(NL80211_ATTR_VENDOR_DATA);
    if (!nlData)
        goto cleanup;

    ret = gScanCommand->put_u32(
            QCA_WLAN_VENDOR_ATTR_GSCAN_SUBCMD_CONFIG_PARAM_REQUEST_ID, id);
    if (ret < 0)
        goto cleanup;

    gScanCommand->attr_end(nlData);

    ret = gScanCommand->requestResponse();
    if (ret != 0) {
        ALOGE("%s: requestResponse Error:%d",__FUNCTION__, ret);
    }

    /* Disable Event Handling. */
    if (gScanSetBssidHotlistCmdEventHandler) {
        gScanSetBssidHotlistCmdEventHandler->disableEventHandling();
    }

cleanup:
    delete gScanCommand;
    return (wifi_error)ret;
}

/* Set the GSCAN Significant AP Change list. */
wifi_error wifi_set_significant_change_handler(wifi_request_id id,
                                            wifi_interface_handle iface,
                                    wifi_significant_change_params params,
                                    wifi_significant_change_handler handler)
{
    int i, numAp, ret = 0;
    GScanCommand *gScanCommand;
    struct nlattr *nlData, *nlApThresholdParamList;
    interface_info *ifaceInfo = getIfaceInfo(iface);
    wifi_handle wifiHandle = getWifiHandle(iface);
    bool previousGScanSetSigChangeRunning = false;
    hal_info *info = getHalInfo(wifiHandle);
    lowi_cb_table_t *lowiWifiHalApi = NULL;
    gscan_event_handlers* event_handlers;
    GScanCommandEventHandler *gScanSetSignificantChangeCmdEventHandler;

    event_handlers = (gscan_event_handlers*)info->gscan_handlers;
    gScanSetSignificantChangeCmdEventHandler =
        event_handlers->gScanSetSignificantChangeCmdEventHandler;

    if (!(info->supported_feature_set & WIFI_FEATURE_GSCAN)) {
        ALOGE("%s: GSCAN is not supported by driver",
            __FUNCTION__);
        return WIFI_ERROR_NOT_SUPPORTED;
    }

    /* Route request through LOWI if supported*/
    lowiWifiHalApi = getLowiCallbackTable(GSCAN_SUPPORTED);
    if (lowiWifiHalApi == NULL ||
        lowiWifiHalApi->set_significant_change_handler == NULL) {
        ALOGV("%s: Sending cmd directly to host", __FUNCTION__);
    } else {
        ret = lowiWifiHalApi->set_significant_change_handler(id,
                                                             iface,
                                                             params,
                                                             handler);
        ALOGV("%s: lowi set_significant_change_handler "
            "returned: %d. Exit.", __FUNCTION__, ret);
        return (wifi_error)ret;
    }

    /* Wi-Fi HAL doesn't need to check if a similar request to set significant
     * change list was made earlier. If set_significant_change() is called while
     * another one is running, the request will be sent down to driver and
     * firmware. If the new request is successfully honored, then Wi-Fi HAL
     * will use the new request id for the gScanSetSignificantChangeCmdEventHandler
     * object.
     */

    gScanCommand = new GScanCommand(
                    wifiHandle,
                    id,
                    OUI_QCA,
                    QCA_NL80211_VENDOR_SUBCMD_GSCAN_SET_SIGNIFICANT_CHANGE);
    if (gScanCommand == NULL) {
        ALOGE("%s: Error GScanCommand NULL", __FUNCTION__);
        return WIFI_ERROR_UNKNOWN;
    }

    /* Create the NL message. */
    ret = gScanCommand->create();
    if (ret < 0)
        goto cleanup;

    /* Set the interface Id of the message. */
    ret = gScanCommand->set_iface_id(ifaceInfo->name);
    if (ret < 0)
        goto cleanup;

    /* Add the vendor specific attributes for the NL command. */
    nlData = gScanCommand->attr_start(NL80211_ATTR_VENDOR_DATA);
    if (!nlData)
        goto cleanup;

    numAp = (unsigned int)params.num_bssid > MAX_SIGNIFICANT_CHANGE_APS ?
        MAX_SIGNIFICANT_CHANGE_APS : params.num_bssid;

    if (gScanCommand->put_u32(
            QCA_WLAN_VENDOR_ATTR_GSCAN_SUBCMD_CONFIG_PARAM_REQUEST_ID,
            id) ||
        gScanCommand->put_u32(
        QCA_WLAN_VENDOR_ATTR_GSCAN_SIGNIFICANT_CHANGE_PARAMS_RSSI_SAMPLE_SIZE,
            params.rssi_sample_size) ||
        gScanCommand->put_u32(
        QCA_WLAN_VENDOR_ATTR_GSCAN_SIGNIFICANT_CHANGE_PARAMS_LOST_AP_SAMPLE_SIZE,
            params.lost_ap_sample_size) ||
        gScanCommand->put_u32(
            QCA_WLAN_VENDOR_ATTR_GSCAN_SIGNIFICANT_CHANGE_PARAMS_MIN_BREACHING,
            params.min_breaching) ||
        gScanCommand->put_u32(
            QCA_WLAN_VENDOR_ATTR_GSCAN_SIGNIFICANT_CHANGE_PARAMS_NUM_AP,
            numAp))
    {
        goto cleanup;
    }

    ALOGV("%s: Number of AP params:%d Rssi_sample_size:%d "
          "lost_ap_sample_size:%d min_breaching:%d", __FUNCTION__,
          numAp, params.rssi_sample_size, params.lost_ap_sample_size,
          params.min_breaching);

    /* Add the vendor specific attributes for the NL command. */
    nlApThresholdParamList =
        gScanCommand->attr_start(
                                QCA_WLAN_VENDOR_ATTR_GSCAN_AP_THRESHOLD_PARAM);
    if (!nlApThresholdParamList)
        goto cleanup;

    /* Add nested NL attributes for AP Threshold Param list. */
    for (i = 0; i < numAp; i++) {
        ap_threshold_param apThreshold = params.ap[i];
        struct nlattr *nlApThresholdParam = gScanCommand->attr_start(i);
        if (!nlApThresholdParam)
            goto cleanup;
        if ( gScanCommand->put_addr(
                QCA_WLAN_VENDOR_ATTR_GSCAN_AP_THRESHOLD_PARAM_BSSID,
                apThreshold.bssid) ||
            gScanCommand->put_s32(
                QCA_WLAN_VENDOR_ATTR_GSCAN_AP_THRESHOLD_PARAM_RSSI_LOW,
                apThreshold.low) ||
            gScanCommand->put_s32(
                QCA_WLAN_VENDOR_ATTR_GSCAN_AP_THRESHOLD_PARAM_RSSI_HIGH,
                apThreshold.high))
        {
            goto cleanup;
        }
        ALOGV("%s: ap[%d].bssid:%hhx:%hhx:%hhx:%hhx:%hhx:%hhx "
              "ap[%d].low:%d  ap[%d].high:%d", __FUNCTION__,
              i,
              apThreshold.bssid[0], apThreshold.bssid[1],
              apThreshold.bssid[2], apThreshold.bssid[3],
              apThreshold.bssid[4], apThreshold.bssid[5],
              i, apThreshold.low, i, apThreshold.high);
        gScanCommand->attr_end(nlApThresholdParam);
    }

    gScanCommand->attr_end(nlApThresholdParamList);

    gScanCommand->attr_end(nlData);

    GScanCallbackHandler callbackHandler;
    memset(&callbackHandler, 0, sizeof(callbackHandler));
    callbackHandler.on_significant_change = handler.on_significant_change;

    /* Create an object of the event handler class to take care of the
      * asychronous events on the north-bound.
      */
    if (gScanSetSignificantChangeCmdEventHandler == NULL) {
        gScanSetSignificantChangeCmdEventHandler =
            new GScanCommandEventHandler(
                     wifiHandle,
                     id,
                     OUI_QCA,
                     QCA_NL80211_VENDOR_SUBCMD_GSCAN_SET_SIGNIFICANT_CHANGE,
                     callbackHandler);
        if (gScanSetSignificantChangeCmdEventHandler == NULL) {
            ALOGE("%s: Error in instantiating, "
                "gScanSetSignificantChangeCmdEventHandler.",
                __FUNCTION__);
            ret = WIFI_ERROR_UNKNOWN;
            goto cleanup;
        }
        event_handlers->gScanSetSignificantChangeCmdEventHandler =
            gScanSetSignificantChangeCmdEventHandler;
    } else {
        gScanSetSignificantChangeCmdEventHandler->setCallbackHandler(callbackHandler);
    }

    ret = gScanCommand->requestResponse();
    if (ret != 0) {
        ALOGE("%s: requestResponse Error:%d",__FUNCTION__, ret);
        goto cleanup;
    }

    if (gScanSetSignificantChangeCmdEventHandler != NULL) {
        gScanSetSignificantChangeCmdEventHandler->set_request_id(id);
        gScanSetSignificantChangeCmdEventHandler->enableEventHandling();
    }

cleanup:
    /* Disable Event Handling if ret != 0 */
    if (ret && gScanSetSignificantChangeCmdEventHandler) {
        ALOGI("%s: Error ret:%d, disable event handling",
            __FUNCTION__, ret);
        gScanSetSignificantChangeCmdEventHandler->disableEventHandling();
    }
    delete gScanCommand;
    return (wifi_error)ret;
}

/* Clear the GSCAN Significant AP change list. */
wifi_error wifi_reset_significant_change_handler(wifi_request_id id,
                                            wifi_interface_handle iface)
{
    int ret = 0;
    GScanCommand *gScanCommand;
    struct nlattr *nlData;
    interface_info *ifaceInfo = getIfaceInfo(iface);
    wifi_handle wifiHandle = getWifiHandle(iface);
    hal_info *info = getHalInfo(wifiHandle);
    lowi_cb_table_t *lowiWifiHalApi = NULL;
    gscan_event_handlers* event_handlers;
    GScanCommandEventHandler *gScanSetSignificantChangeCmdEventHandler;

    event_handlers = (gscan_event_handlers*)info->gscan_handlers;
    gScanSetSignificantChangeCmdEventHandler =
        event_handlers->gScanSetSignificantChangeCmdEventHandler;

    if (!(info->supported_feature_set & WIFI_FEATURE_GSCAN)) {
        ALOGE("%s: GSCAN is not supported by driver",
            __FUNCTION__);
        return WIFI_ERROR_NOT_SUPPORTED;
    }

    /* Route request through LOWI if supported*/
    lowiWifiHalApi = getLowiCallbackTable(GSCAN_SUPPORTED);
    if (lowiWifiHalApi == NULL ||
        lowiWifiHalApi->reset_significant_change_handler == NULL) {
        ALOGV("%s: Sending cmd directly to host", __FUNCTION__);
    } else {
        ret = lowiWifiHalApi->reset_significant_change_handler(id, iface);
        ALOGV("%s: lowi reset_significant_change_handler "
            "returned: %d. Exit.", __FUNCTION__, ret);
        return (wifi_error)ret;
    }

    if (gScanSetSignificantChangeCmdEventHandler == NULL ||
        (gScanSetSignificantChangeCmdEventHandler->isEventHandlingEnabled() ==
        false)) {
        ALOGE("wifi_reset_significant_change_handler: GSCAN significant_change"
            " isn't set. Nothing to do. Exit");
        return WIFI_ERROR_NOT_AVAILABLE;
    }

    gScanCommand =
        new GScanCommand
                    (
                    wifiHandle,
                    id,
                    OUI_QCA,
                    QCA_NL80211_VENDOR_SUBCMD_GSCAN_RESET_SIGNIFICANT_CHANGE);
    if (gScanCommand == NULL) {
        ALOGE("%s: Error GScanCommand NULL", __FUNCTION__);
        return WIFI_ERROR_UNKNOWN;
    }

    /* Create the NL message. */
    ret = gScanCommand->create();
    if (ret < 0)
        goto cleanup;

    /* Set the interface Id of the message. */
    ret = gScanCommand->set_iface_id(ifaceInfo->name);
    if (ret < 0)
        goto cleanup;

    /* Add the vendor specific attributes for the NL command. */
    nlData = gScanCommand->attr_start(NL80211_ATTR_VENDOR_DATA);
    if (!nlData)
        goto cleanup;

    ret = gScanCommand->put_u32(
                    QCA_WLAN_VENDOR_ATTR_GSCAN_SUBCMD_CONFIG_PARAM_REQUEST_ID,
                    id);
    if (ret < 0)
        goto cleanup;

    gScanCommand->attr_end(nlData);

    ret = gScanCommand->requestResponse();
    if (ret != 0) {
        ALOGE("%s: requestResponse Error:%d",__FUNCTION__, ret);
    }

    /* Disable Event Handling. */
    if (gScanSetSignificantChangeCmdEventHandler) {
        gScanSetSignificantChangeCmdEventHandler->disableEventHandling();
    }

cleanup:
    delete gScanCommand;
    return (wifi_error)ret;
}

/* Get the GSCAN cached scan results. */
wifi_error wifi_get_cached_gscan_results(wifi_interface_handle iface,
                                            byte flush, int max,
                                            wifi_cached_scan_results *results,
                                            int *num)
{
    int requestId, ret = 0, retRequestRsp = 0;
    wifi_cached_scan_results *result = results;
    u32 j = 0;
    int i = 0;
    u8 moreData = 0;
    u16 waitTime = GSCAN_EVENT_WAIT_TIME_SECONDS;
    GScanCommand *gScanCommand;
    struct nlattr *nlData;
    wifi_cached_scan_results *cached_results;
    lowi_cb_table_t *lowiWifiHalApi = NULL;

    interface_info *ifaceInfo = getIfaceInfo(iface);
    wifi_handle wifiHandle = getWifiHandle(iface);
    hal_info *info = getHalInfo(wifiHandle);

    if (!(info->supported_feature_set & WIFI_FEATURE_GSCAN)) {
        ALOGE("%s: GSCAN is not supported by driver",
            __FUNCTION__);
        return WIFI_ERROR_NOT_SUPPORTED;
    }

    /* Route GSCAN request through LOWI if supported */
    lowiWifiHalApi = getLowiCallbackTable(GSCAN_SUPPORTED);
    if (lowiWifiHalApi == NULL ||
        lowiWifiHalApi->get_cached_gscan_results == NULL) {
        ALOGV("%s: Sending cmd directly to host", __FUNCTION__);
    } else {
        ret = lowiWifiHalApi->get_cached_gscan_results(iface,
                                                       flush,
                                                       max,
                                                       results,
                                                       num);
        ALOGV("%s: lowi get_cached_gscan_results"
            "returned: %d. Exit.", __FUNCTION__, ret);
        return (wifi_error)ret;
    }

    /* No request id from caller, so generate one and pass it on to the driver. */
    /* Generate it randomly */
    requestId = get_requestid();

    if (results == NULL || num == NULL) {
        ALOGE("%s: NULL pointer provided. Exit.",
            __FUNCTION__);
        return WIFI_ERROR_INVALID_ARGS;
    }

    gScanCommand = new GScanCommand(
                        wifiHandle,
                        requestId,
                        OUI_QCA,
                        QCA_NL80211_VENDOR_SUBCMD_GSCAN_GET_CACHED_RESULTS);
    if (gScanCommand == NULL) {
        ALOGE("%s: Error GScanCommand NULL", __FUNCTION__);
        return WIFI_ERROR_UNKNOWN;
    }

    ret = gScanCommand->allocRspParams(eGScanGetCachedResultsRspParams);
    if (ret != 0) {
        ALOGE("%s: Failed to allocate memory for response struct. Error:%d",
            __FUNCTION__, ret);
        goto cleanup;
    }

    ret = gScanCommand->allocCachedResultsTemp(max, results);
    if (ret != 0) {
        ALOGE("%s: Failed to allocate memory for temp gscan cached list. "
            "Error:%d", __FUNCTION__, ret);
        goto cleanup;
    }

    /* Clear the destination cached results list before copying results. */
    memset(results, 0, max * sizeof(wifi_cached_scan_results));

    /* Create the NL message. */
    ret = gScanCommand->create();
    if (ret < 0)
        goto cleanup;

    /* Set the interface Id of the message. */
    ret = gScanCommand->set_iface_id(ifaceInfo->name);
    if (ret < 0)
        goto cleanup;

    /* Add the vendor specific attributes for the NL command. */
    nlData = gScanCommand->attr_start(NL80211_ATTR_VENDOR_DATA);
    if (!nlData)
        goto cleanup;

    if (ret < 0)
        goto cleanup;

    if (gScanCommand->put_u32(
         QCA_WLAN_VENDOR_ATTR_GSCAN_SUBCMD_CONFIG_PARAM_REQUEST_ID,
            requestId) ||
        gScanCommand->put_u8(
         QCA_WLAN_VENDOR_ATTR_GSCAN_GET_CACHED_SCAN_RESULTS_CONFIG_PARAM_FLUSH,
            flush) ||
        gScanCommand->put_u32(
         QCA_WLAN_VENDOR_ATTR_GSCAN_GET_CACHED_SCAN_RESULTS_CONFIG_PARAM_MAX,
            max))
    {
        goto cleanup;
    }

    ALOGV("%s: flush:%d max:%d", __FUNCTION__, flush, max);
    gScanCommand->attr_end(nlData);

    retRequestRsp = gScanCommand->requestResponse();
    if (retRequestRsp != 0) {
        ALOGE("%s: requestResponse Error:%d",
            __FUNCTION__, retRequestRsp);
        if (retRequestRsp != -ETIMEDOUT) {
            /* Proceed to cleanup & return no results */
            goto cleanup;
        }
    }

    /* No more data, copy the parsed results into the caller's results array */
    ret = gScanCommand->copyCachedScanResults(num, results);
    ALOGV("%s: max: %d, num:%d", __FUNCTION__, max, *num);

    if (!ret) {
        /* If requestResponse returned a TIMEOUT */
        if (retRequestRsp == -ETIMEDOUT) {
            if (*num > 0) {
                /* Mark scan results as incomplete for the last scan_id */
                results[(*num)-1].flags = WIFI_SCAN_FLAG_INTERRUPTED;
                ALOGV("%s: Timeout happened. Mark scan results as incomplete "
                    "for scan_id:%d", __FUNCTION__, results[(*num)-1].scan_id);
                ret = WIFI_SUCCESS;
            } else
                ret = WIFI_ERROR_TIMED_OUT;
        }
    }
cleanup:
    gScanCommand->freeRspParams(eGScanGetCachedResultsRspParams);
    delete gScanCommand;
    return (wifi_error)ret;
}

/* Random MAC OUI for PNO */
wifi_error wifi_set_scanning_mac_oui(wifi_interface_handle handle, oui scan_oui)
{
    int ret = 0;
    struct nlattr *nlData;
    WifiVendorCommand *vCommand = NULL;
    interface_info *iinfo = getIfaceInfo(handle);
    wifi_handle wifiHandle = getWifiHandle(handle);

    vCommand = new WifiVendorCommand(wifiHandle, 0,
            OUI_QCA,
            QCA_NL80211_VENDOR_SUBCMD_SCANNING_MAC_OUI);
    if (vCommand == NULL) {
        ALOGE("%s: Error vCommand NULL", __FUNCTION__);
        return WIFI_ERROR_OUT_OF_MEMORY;
    }

    /* create the message */
    ret = vCommand->create();
    if (ret < 0)
        goto cleanup;

    ret = vCommand->set_iface_id(iinfo->name);
    if (ret < 0)
        goto cleanup;

    /* Add the vendor specific attributes for the NL command. */
    nlData = vCommand->attr_start(NL80211_ATTR_VENDOR_DATA);
    if (!nlData)
        goto cleanup;

    ALOGV("%s: MAC_OUI - %02x:%02x:%02x", __FUNCTION__,
          scan_oui[0], scan_oui[1], scan_oui[2]);

    /* Add the fixed part of the mac_oui to the nl command */
    ret = vCommand->put_bytes(
            QCA_WLAN_VENDOR_ATTR_SET_SCANNING_MAC_OUI,
            (char *)scan_oui,
            WIFI_SCANNING_MAC_OUI_LENGTH);
    if (ret < 0)
        goto cleanup;

    vCommand->attr_end(nlData);

    ret = vCommand->requestResponse();
    if (ret != 0) {
        ALOGE("%s: requestResponse Error:%d",__FUNCTION__, ret);
        goto cleanup;
    }

cleanup:
    delete vCommand;
    return (wifi_error)ret;
}


GScanCommand::GScanCommand(wifi_handle handle, int id, u32 vendor_id,
                                  u32 subcmd)
        : WifiVendorCommand(handle, id, vendor_id, subcmd)
{
    /* Initialize the member data variables here */
    mGetCapabilitiesRspParams = NULL;
    mGetCachedResultsRspParams = NULL;
    mChannels = NULL;
    mMaxChannels = 0;
    mNumChannelsPtr = NULL;

    mRequestId = id;
    memset(&mHandler, 0,sizeof(mHandler));
}

GScanCommand::~GScanCommand()
{
    unregisterVendorHandler(mVendor_id, mSubcmd);
}


/* This function implements creation of Vendor command */
int GScanCommand::create() {
    int ret = mMsg.create(NL80211_CMD_VENDOR, 0, 0);
    if (ret < 0) {
        return ret;
    }

    /* Insert the oui in the msg */
    ret = mMsg.put_u32(NL80211_ATTR_VENDOR_ID, mVendor_id);
    if (ret < 0)
        goto out;
    /* Insert the subcmd in the msg */
    ret = mMsg.put_u32(NL80211_ATTR_VENDOR_SUBCMD, mSubcmd);
    if (ret < 0)
        goto out;

     ALOGV("%s: mVendor_id = %d, Subcmd = %d.",
        __FUNCTION__, mVendor_id, mSubcmd);
out:
    return ret;
}

/* Callback handlers registered for nl message send */
static int error_handler_gscan(struct sockaddr_nl *nla, struct nlmsgerr *err,
                                   void *arg)
{
    struct sockaddr_nl *tmp;
    int *ret = (int *)arg;
    tmp = nla;
    *ret = err->error;
    ALOGE("%s: Error code:%d (%s)", __FUNCTION__, *ret, strerror(-(*ret)));
    return NL_STOP;
}

/* Callback handlers registered for nl message send */
static int ack_handler_gscan(struct nl_msg *msg, void *arg)
{
    int *ret = (int *)arg;
    struct nl_msg * a;

    ALOGE("%s: called", __FUNCTION__);
    a = msg;
    *ret = 0;
    return NL_STOP;
}

/* Callback handlers registered for nl message send */
static int finish_handler_gscan(struct nl_msg *msg, void *arg)
{
  int *ret = (int *)arg;
  struct nl_msg * a;

  ALOGE("%s: called", __FUNCTION__);
  a = msg;
  *ret = 0;
  return NL_SKIP;
}

int GScanCommand::requestResponse()
{
    return WifiCommand::requestResponse(mMsg);
}

int GScanCommand::handleResponse(WifiEvent &reply) {
    u32 status;
    int i = 0;
    int ret = WIFI_SUCCESS;
    u32 val;

    WifiVendorCommand::handleResponse(reply);

    struct nlattr *tbVendor[
        QCA_WLAN_VENDOR_ATTR_GSCAN_RESULTS_MAX + 1];
    nla_parse(tbVendor, QCA_WLAN_VENDOR_ATTR_GSCAN_RESULTS_MAX,
                (struct nlattr *)mVendorData,mDataLen, NULL);

    switch(mSubcmd)
    {
        case QCA_NL80211_VENDOR_SUBCMD_GSCAN_GET_VALID_CHANNELS:
        {
            if (!tbVendor[QCA_WLAN_VENDOR_ATTR_GSCAN_RESULTS_NUM_CHANNELS]) {
                ALOGE("%s: QCA_WLAN_VENDOR_ATTR_GSCAN_RESULTS_NUM_CHANNELS"
                    " not found", __FUNCTION__);
                ret = WIFI_ERROR_INVALID_ARGS;
                break;
            }
            val = nla_get_u32(tbVendor[
                QCA_WLAN_VENDOR_ATTR_GSCAN_RESULTS_NUM_CHANNELS]);

            val = val > (unsigned int)mMaxChannels ?
                    (unsigned int)mMaxChannels : val;
            *mNumChannelsPtr = val;

            /* Extract the list of channels. */
            if (*mNumChannelsPtr > 0 ) {
                if (!tbVendor[QCA_WLAN_VENDOR_ATTR_GSCAN_RESULTS_CHANNELS]) {
                    ALOGE("%s: QCA_WLAN_VENDOR_ATTR_GSCAN_RESULTS_CHANNELS"
                        " not found", __FUNCTION__);
                    ret = WIFI_ERROR_INVALID_ARGS;
                    break;
                }
                nla_memcpy(mChannels,
                    tbVendor[
                    QCA_WLAN_VENDOR_ATTR_GSCAN_RESULTS_CHANNELS],
                    sizeof(wifi_channel) * (*mNumChannelsPtr));
            }
            char buf[100];
            size_t len = 0;
            for (i = 0; i < *mNumChannelsPtr && len < sizeof(buf); i++) {
                 len +=  snprintf(buf + len, sizeof(buf)-len, "%u ",
                                  *(mChannels + i));
            }
            ALOGV("%s: Num Channels %d: List of valid channels are: %s",
                  __FUNCTION__, *mNumChannelsPtr, buf);

        }
        break;
        case QCA_NL80211_VENDOR_SUBCMD_GSCAN_GET_CAPABILITIES:
        {
            ret = gscan_parse_capabilities(tbVendor);
            if (ret) {
                break;
            }

            if (mGetCapabilitiesRspParams) {
                wifi_gscan_capabilities capa =
                    mGetCapabilitiesRspParams->capabilities;
                ALOGV("%s: max_ap_cache_per_scan:%d\n"
                        "max_bssid_history_entries:%d\n"
                        "max_hotlist_bssids:%d\n"
                        "max_hotlist_ssids:%d\n"
                        "max_rssi_sample_size:%d\n"
                        "max_scan_buckets:%d\n"
                        "max_scan_cache_size:%d\n"
                        "max_scan_reporting_threshold:%d\n"
                        "max_significant_wifi_change_aps:%d\n"
                        "max_number_epno_networks:%d\n"
                        "max_number_epno_networks_by_ssid:%d\n"
                        "max_number_of_white_listed_ssid:%d.",
                        __FUNCTION__, capa.max_ap_cache_per_scan,
                        capa.max_bssid_history_entries,
                        capa.max_hotlist_bssids,
                        capa.max_hotlist_ssids,
                        capa.max_rssi_sample_size,
                        capa.max_scan_buckets,
                        capa.max_scan_cache_size,
                        capa.max_scan_reporting_threshold,
                        capa.max_significant_wifi_change_aps,
                        capa.max_number_epno_networks,
                        capa.max_number_epno_networks_by_ssid,
                        capa.max_number_of_white_listed_ssid);
            }
        }
        break;
        case QCA_NL80211_VENDOR_SUBCMD_GSCAN_GET_CACHED_RESULTS:
        {
            wifi_request_id id;
            u32 numResults = 0;
            u32 startingIndex;
            int firstScanIdInPatch = -1;

            if (!tbVendor[
                QCA_WLAN_VENDOR_ATTR_GSCAN_RESULTS_REQUEST_ID]) {
                ALOGE("%s: GSCAN_RESULTS_REQUEST_ID not"
                    "found", __FUNCTION__);
                ret = WIFI_ERROR_INVALID_ARGS;
                break;
            }
            id = nla_get_u32(
                    tbVendor[QCA_WLAN_VENDOR_ATTR_GSCAN_RESULTS_REQUEST_ID]
                    );
            /* If this is not for us, just ignore it. */
            if (id != mRequestId) {
                ALOGV("%s: Event has Req. ID:%d <> ours:%d",
                    __FUNCTION__, id, mRequestId);
                break;
            }
            if (!tbVendor[
                QCA_WLAN_VENDOR_ATTR_GSCAN_RESULTS_NUM_RESULTS_AVAILABLE]) {
                ALOGE("%s: GSCAN_RESULTS_NUM_RESULTS_AVAILABLE not"
                    "found", __FUNCTION__);
                ret = WIFI_ERROR_INVALID_ARGS;
                break;
            }
            /* Read num of cached scan results in this data chunk. Note that
             * this value doesn't represent the number of unique gscan scan Ids
             * since the first scan id in this new chunk could be similar to
             * the last scan id in the previous chunk.
             */
            numResults = nla_get_u32(tbVendor[
                QCA_WLAN_VENDOR_ATTR_GSCAN_RESULTS_NUM_RESULTS_AVAILABLE]);
            ALOGV("%s: num Cached results in this fragment:%d",
                       __FUNCTION__, numResults);

            if (!mGetCachedResultsRspParams) {
                ALOGE("%s: mGetCachedResultsRspParams is NULL, exit.",
                    __FUNCTION__);
                ret = WIFI_ERROR_INVALID_ARGS;
                break;
            }

            /* To support fragmentation from firmware, monitor the
             * MORE_DATA flag and cache results until MORE_DATA = 0.
             */
            if (!tbVendor[
                QCA_WLAN_VENDOR_ATTR_GSCAN_RESULTS_SCAN_RESULT_MORE_DATA]) {
                ALOGE("%s: GSCAN_RESULTS_NUM_RESULTS_MORE_DATA "
                    "not found", __FUNCTION__);
                ret = WIFI_ERROR_INVALID_ARGS;
                break;
            } else {
                mGetCachedResultsRspParams->more_data = nla_get_u8(
                    tbVendor[
                QCA_WLAN_VENDOR_ATTR_GSCAN_RESULTS_SCAN_RESULT_MORE_DATA]);
            }

            /* No data in this chunk so skip this chunk */
            if (numResults == 0) {
                return NL_SKIP;
            }

            if (!tbVendor[QCA_WLAN_VENDOR_ATTR_GSCAN_CACHED_RESULTS_SCAN_ID]) {
                ALOGE("GSCAN_CACHED_RESULTS_SCAN_ID not found");
                ret = WIFI_ERROR_INVALID_ARGS;
                break;
            }

            /* Get the first Scan-Id in this chuck of cached results. */
            firstScanIdInPatch = nla_get_u32(tbVendor[
                    QCA_WLAN_VENDOR_ATTR_GSCAN_CACHED_RESULTS_SCAN_ID]);

            ALOGV("More data: %d, firstScanIdInPatch: %d, lastProcessedScanId: %d",
                mGetCachedResultsRspParams->more_data, firstScanIdInPatch,
                mGetCachedResultsRspParams->lastProcessedScanId);

            if (numResults) {
                if (firstScanIdInPatch !=
                    mGetCachedResultsRspParams->lastProcessedScanId) {
                    /* New result scan Id block, update the starting index. */
                    mGetCachedResultsRspParams->cachedResultsStartingIndex++;
                }

                ret = gscan_get_cached_results(
                                    mGetCachedResultsRspParams->cached_results,
                                    tbVendor);
                /* If a parsing error occurred, exit and proceed for cleanup. */
                if (ret)
                    break;
            }
        }
        break;
        default:
            /* Error case should not happen print log */
            ALOGE("%s: Wrong GScan subcmd response received %d",
                __FUNCTION__, mSubcmd);
    }

    /* A parsing error occurred, do the cleanup of gscan result lists. */
    if (ret) {
        switch(mSubcmd)
        {
            case QCA_NL80211_VENDOR_SUBCMD_GSCAN_GET_CACHED_RESULTS:
            {
                ALOGE("%s: Parsing error, free CachedResultsRspParams",
                    __FUNCTION__);
                freeRspParams(eGScanGetCachedResultsRspParams);
            }
            break;
            case QCA_NL80211_VENDOR_SUBCMD_GSCAN_GET_CAPABILITIES:
            {
                ALOGE("%s: Parsing error, free CapabilitiesRspParams",
                    __FUNCTION__);
                freeRspParams(eGScanGetCapabilitiesRspParams);
            }
            break;
            default:
                ALOGE("%s: Wrong GScan subcmd received %d", __FUNCTION__, mSubcmd);
        }
    }
    return NL_SKIP;
}

/* Parses and extracts gscan capabilities results. */
int GScanCommand::gscan_parse_capabilities(struct nlattr **tbVendor)
{
    if (!mGetCapabilitiesRspParams){
        ALOGE("%s: mGetCapabilitiesRspParams ptr is NULL. Exit.",
            __FUNCTION__);
        return WIFI_ERROR_INVALID_ARGS;
    }

    if (!tbVendor[
    QCA_WLAN_VENDOR_ATTR_GSCAN_RESULTS_CAPABILITIES_MAX_SCAN_CACHE_SIZE
            ]) {
        ALOGE("%s: QCA_WLAN_VENDOR_ATTR_GSCAN_RESULTS_"
            "CAPABILITIES_MAX_SCAN_CACHE_SIZE not found", __FUNCTION__);
        return WIFI_ERROR_INVALID_ARGS;
    }
    mGetCapabilitiesRspParams->capabilities.max_scan_cache_size =
        nla_get_u32(tbVendor[
        QCA_WLAN_VENDOR_ATTR_GSCAN_RESULTS_CAPABILITIES_MAX_SCAN_CACHE_SIZE]);

    if (!tbVendor[
    QCA_WLAN_VENDOR_ATTR_GSCAN_RESULTS_CAPABILITIES_MAX_SCAN_BUCKETS
            ]) {
        ALOGE("%s: QCA_WLAN_VENDOR_ATTR_GSCAN_RESULTS_CAPABILITIES_MAX"
            "_SCAN_BUCKETS not found", __FUNCTION__);
        return WIFI_ERROR_INVALID_ARGS;
    }
    mGetCapabilitiesRspParams->capabilities.max_scan_buckets =
        nla_get_u32(tbVendor[
        QCA_WLAN_VENDOR_ATTR_GSCAN_RESULTS_CAPABILITIES_MAX_SCAN_BUCKETS]
                        );

    if (!tbVendor[
    QCA_WLAN_VENDOR_ATTR_GSCAN_RESULTS_CAPABILITIES_MAX_AP_CACHE_PER_SCAN
            ]) {
        ALOGE("%s: QCA_WLAN_VENDOR_ATTR_GSCAN_RESULTS_CAPABILITIES_MAX"
            "_AP_CACHE_PER_SCAN not found", __FUNCTION__);
        return WIFI_ERROR_INVALID_ARGS;
    }
    mGetCapabilitiesRspParams->capabilities.max_ap_cache_per_scan =
            nla_get_u32(tbVendor[
        QCA_WLAN_VENDOR_ATTR_GSCAN_RESULTS_CAPABILITIES_MAX_AP_CACHE_PER_SCAN]);

    if (!tbVendor[
    QCA_WLAN_VENDOR_ATTR_GSCAN_RESULTS_CAPABILITIES_MAX_RSSI_SAMPLE_SIZE
            ]) {
        ALOGE("%s: QCA_WLAN_VENDOR_ATTR_GSCAN_RESULTS_CAPABILITIES_MAX"
            "_RSSI_SAMPLE_SIZE not found", __FUNCTION__);
        return WIFI_ERROR_INVALID_ARGS;
    }
    mGetCapabilitiesRspParams->capabilities.max_rssi_sample_size =
        nla_get_u32(tbVendor[
        QCA_WLAN_VENDOR_ATTR_GSCAN_RESULTS_CAPABILITIES_MAX_RSSI_SAMPLE_SIZE]);

    if (!tbVendor[
    QCA_WLAN_VENDOR_ATTR_GSCAN_RESULTS_CAPABILITIES_MAX_SCAN_REPORTING_THRESHOLD
            ]) {
        ALOGE("%s: QCA_WLAN_VENDOR_ATTR_GSCAN_RESULTS_CAPABILITIES_"
            "MAX_SCAN_REPORTING_THRESHOLD not found", __FUNCTION__);
        return WIFI_ERROR_INVALID_ARGS;
    }
    mGetCapabilitiesRspParams->capabilities.max_scan_reporting_threshold =
            nla_get_u32(tbVendor[
    QCA_WLAN_VENDOR_ATTR_GSCAN_RESULTS_CAPABILITIES_MAX_SCAN_REPORTING_THRESHOLD
    ]);

    if (!tbVendor[
    QCA_WLAN_VENDOR_ATTR_GSCAN_RESULTS_CAPABILITIES_MAX_HOTLIST_BSSIDS
            ]) {
        ALOGE("%s: QCA_WLAN_VENDOR_ATTR_GSCAN_RESULTS_CAPABILITIES_"
            "MAX_HOTLIST_BSSIDS not found", __FUNCTION__);
        return WIFI_ERROR_INVALID_ARGS;
    }
    mGetCapabilitiesRspParams->capabilities.max_hotlist_bssids =
            nla_get_u32(tbVendor[
            QCA_WLAN_VENDOR_ATTR_GSCAN_RESULTS_CAPABILITIES_MAX_HOTLIST_BSSIDS]);

    if (!tbVendor[
    QCA_WLAN_VENDOR_ATTR_GSCAN_RESULTS_CAPABILITIES_MAX_SIGNIFICANT_WIFI_CHANGE_APS
            ]) {
        ALOGE("%s: QCA_WLAN_VENDOR_ATTR_GSCAN_RESULTS_CAPABILITIES_MAX"
            "_SIGNIFICANT_WIFI_CHANGE_APS not found", __FUNCTION__);
        return WIFI_ERROR_INVALID_ARGS;
    }
    mGetCapabilitiesRspParams->capabilities.max_significant_wifi_change_aps =
            nla_get_u32(tbVendor[
    QCA_WLAN_VENDOR_ATTR_GSCAN_RESULTS_CAPABILITIES_MAX_SIGNIFICANT_WIFI_CHANGE_APS]);

    if (!tbVendor[
    QCA_WLAN_VENDOR_ATTR_GSCAN_RESULTS_CAPABILITIES_MAX_BSSID_HISTORY_ENTRIES
            ]) {
        ALOGE("%s: QCA_WLAN_VENDOR_ATTR_GSCAN_RESULTS_CAPABILITIES_MAX"
            "_BSSID_HISTORY_ENTRIES not found", __FUNCTION__);
        return WIFI_ERROR_INVALID_ARGS;
    }
    mGetCapabilitiesRspParams->capabilities.max_bssid_history_entries =
            nla_get_u32(tbVendor[
    QCA_WLAN_VENDOR_ATTR_GSCAN_RESULTS_CAPABILITIES_MAX_BSSID_HISTORY_ENTRIES
    ]);

    if (!tbVendor[
    QCA_WLAN_VENDOR_ATTR_GSCAN_RESULTS_CAPABILITIES_MAX_HOTLIST_SSIDS
            ]) {
        ALOGE("%s: QCA_WLAN_VENDOR_ATTR_GSCAN_RESULTS_CAPABILITIES"
            "_MAX_HOTLIST_SSIDS not found. Set to 0.", __FUNCTION__);
        mGetCapabilitiesRspParams->capabilities.max_hotlist_ssids = 0;
    } else {
        mGetCapabilitiesRspParams->capabilities.max_hotlist_ssids =
                nla_get_u32(tbVendor[
        QCA_WLAN_VENDOR_ATTR_GSCAN_RESULTS_CAPABILITIES_MAX_HOTLIST_SSIDS
        ]);
    }

    if (!tbVendor[
    QCA_WLAN_VENDOR_ATTR_GSCAN_RESULTS_CAPABILITIES_MAX_NUM_EPNO_NETS
            ]) {
        ALOGE("%s: QCA_WLAN_VENDOR_ATTR_GSCAN_RESULTS_CAPABILITIES_MAX"
            "_NUM_EPNO_NETS not found. Set to 0.", __FUNCTION__);
        mGetCapabilitiesRspParams->capabilities.\
            max_number_epno_networks = 0;
    } else {
        mGetCapabilitiesRspParams->capabilities.max_number_epno_networks
            = nla_get_u32(tbVendor[
        QCA_WLAN_VENDOR_ATTR_GSCAN_RESULTS_CAPABILITIES_MAX_NUM_EPNO_NETS
        ]);
    }

    if (!tbVendor[
    QCA_WLAN_VENDOR_ATTR_GSCAN_RESULTS_CAPABILITIES_MAX_NUM_EPNO_NETS_BY_SSID
            ]) {
        ALOGE("%s: QCA_WLAN_VENDOR_ATTR_GSCAN_RESULTS_CAPABILITIES_MAX"
            "_NUM_EPNO_NETS_BY_SSID not found. Set to 0.", __FUNCTION__);
        mGetCapabilitiesRspParams->capabilities.\
            max_number_epno_networks_by_ssid = 0;
    } else {
        mGetCapabilitiesRspParams->capabilities.max_number_epno_networks_by_ssid
            = nla_get_u32(tbVendor[
        QCA_WLAN_VENDOR_ATTR_GSCAN_RESULTS_CAPABILITIES_MAX_NUM_EPNO_NETS_BY_SSID
        ]);
    }

    if (!tbVendor[
       QCA_WLAN_VENDOR_ATTR_GSCAN_RESULTS_CAPABILITIES_MAX_NUM_WHITELISTED_SSID
            ]) {
        ALOGE("%s: QCA_WLAN_VENDOR_ATTR_GSCAN_RESULTS_CAPABILITIES_MAX"
            "_NUM_WHITELISTED_SSID not found. Set to 0.", __FUNCTION__);
        mGetCapabilitiesRspParams->capabilities.\
            max_number_of_white_listed_ssid = 0;
    } else {
        mGetCapabilitiesRspParams->capabilities.max_number_of_white_listed_ssid
            = nla_get_u32(tbVendor[
        QCA_WLAN_VENDOR_ATTR_GSCAN_RESULTS_CAPABILITIES_MAX_NUM_WHITELISTED_SSID
        ]);
    }
    return WIFI_SUCCESS;
}

/* Called to parse and extract cached results. */
int GScanCommand:: gscan_get_cached_results(
                                      wifi_cached_scan_results *cached_results,
                                      struct nlattr **tb_vendor)
{
    u32 j = 0;
    struct nlattr *scanResultsInfo, *wifiScanResultsInfo;
    int rem = 0, remResults = 0;
    u32 len = 0, numScanResults = 0;
    u32 i = mGetCachedResultsRspParams->cachedResultsStartingIndex;
    ALOGV("%s: starting counter: %d", __FUNCTION__, i);

    for (scanResultsInfo = (struct nlattr *) nla_data(tb_vendor[
               QCA_WLAN_VENDOR_ATTR_GSCAN_CACHED_RESULTS_LIST]),
               rem = nla_len(tb_vendor[
               QCA_WLAN_VENDOR_ATTR_GSCAN_CACHED_RESULTS_LIST]);
           nla_ok(scanResultsInfo, rem) && i < mGetCachedResultsRspParams->max;
           scanResultsInfo = nla_next(scanResultsInfo, &(rem)))
       {
           struct nlattr *tb2[QCA_WLAN_VENDOR_ATTR_GSCAN_RESULTS_MAX + 1];
           nla_parse(tb2, QCA_WLAN_VENDOR_ATTR_GSCAN_RESULTS_MAX,
           (struct nlattr *) nla_data(scanResultsInfo),
                   nla_len(scanResultsInfo), NULL);

           if (!
               tb2[
                   QCA_WLAN_VENDOR_ATTR_GSCAN_CACHED_RESULTS_SCAN_ID
                   ])
           {
               ALOGE("%s: GSCAN_CACHED_RESULTS_SCAN_ID"
                   " not found", __FUNCTION__);
               return WIFI_ERROR_INVALID_ARGS;
           }
           cached_results[i].scan_id =
               nla_get_u32(
               tb2[
                   QCA_WLAN_VENDOR_ATTR_GSCAN_CACHED_RESULTS_SCAN_ID
                   ]);

           if (!
               tb2[
                   QCA_WLAN_VENDOR_ATTR_GSCAN_CACHED_RESULTS_FLAGS
                   ])
           {
               ALOGE("%s: GSCAN_CACHED_RESULTS_FLAGS "
                   "not found", __FUNCTION__);
               return WIFI_ERROR_INVALID_ARGS;
           }
           cached_results[i].flags =
               nla_get_u32(
               tb2[QCA_WLAN_VENDOR_ATTR_GSCAN_CACHED_RESULTS_FLAGS]);

           if (!tb2[QCA_WLAN_VENDOR_ATTR_GSCAN_RESULTS_BUCKETS_SCANNED])
           {
               ALOGI("%s: GSCAN_RESULTS_BUCKETS_SCANNED"
                   "not found", __FUNCTION__);
           } else {
               cached_results[i].buckets_scanned = nla_get_u32(
                       tb2[QCA_WLAN_VENDOR_ATTR_GSCAN_RESULTS_BUCKETS_SCANNED]);
           }

           if (!
               tb2[
                   QCA_WLAN_VENDOR_ATTR_GSCAN_RESULTS_NUM_RESULTS_AVAILABLE
                   ])
           {
               ALOGE("%s: RESULTS_NUM_RESULTS_AVAILABLE "
                   "not found", __FUNCTION__);
               return WIFI_ERROR_INVALID_ARGS;
           }
           numScanResults =
               nla_get_u32(
               tb2[QCA_WLAN_VENDOR_ATTR_GSCAN_RESULTS_NUM_RESULTS_AVAILABLE]);

           if (mGetCachedResultsRspParams->lastProcessedScanId !=
                                        cached_results[i].scan_id) {
               j = 0; /* reset wifi_scan_result counter */
               cached_results[i].num_results = 0;
               ALOGV("parsing: *lastProcessedScanId [%d] !="
                     " cached_results[%d].scan_id:%d, j:%d "
                     "numScanResults: %d",
                     mGetCachedResultsRspParams->lastProcessedScanId, i,
                     cached_results[i].scan_id, j, numScanResults);
               mGetCachedResultsRspParams->lastProcessedScanId =
                   cached_results[i].scan_id;
               mGetCachedResultsRspParams->wifiScanResultsStartingIndex = 0;
               /* Increment the number of cached scan results received */
               mGetCachedResultsRspParams->num_cached_results++;
           } else {
               j = mGetCachedResultsRspParams->wifiScanResultsStartingIndex;
               ALOGV("parsing: *lastProcessedScanId [%d] == "
                     "cached_results[%d].scan_id:%d, j:%d "
                     "numScanResults:%d",
                     mGetCachedResultsRspParams->lastProcessedScanId, i,
                     cached_results[i].scan_id, j, numScanResults);
           }

           ALOGV("%s: scan_id %d ", __FUNCTION__,
            cached_results[i].scan_id);
           ALOGV("%s: flags  %u ", __FUNCTION__,
            cached_results[i].flags);

           for (wifiScanResultsInfo = (struct nlattr *) nla_data(tb2[
                QCA_WLAN_VENDOR_ATTR_GSCAN_RESULTS_LIST]),
                remResults = nla_len(tb2[
                QCA_WLAN_VENDOR_ATTR_GSCAN_RESULTS_LIST]);
                nla_ok(wifiScanResultsInfo, remResults);
                wifiScanResultsInfo = nla_next(wifiScanResultsInfo, &(remResults)))
           {
                struct nlattr *tb3[QCA_WLAN_VENDOR_ATTR_GSCAN_RESULTS_MAX + 1];
                nla_parse(tb3, QCA_WLAN_VENDOR_ATTR_GSCAN_RESULTS_MAX,
                        (struct nlattr *) nla_data(wifiScanResultsInfo),
                        nla_len(wifiScanResultsInfo), NULL);
                if (j < MAX_AP_CACHE_PER_SCAN) {
                    if (!
                        tb3[
                           QCA_WLAN_VENDOR_ATTR_GSCAN_RESULTS_SCAN_RESULT_TIME_STAMP
                           ])
                    {
                        ALOGE("%s: "
                            "RESULTS_SCAN_RESULT_TIME_STAMP not found",
                            __FUNCTION__);
                        return WIFI_ERROR_INVALID_ARGS;
                    }
                    cached_results[i].results[j].ts =
                        nla_get_u64(
                        tb3[
                            QCA_WLAN_VENDOR_ATTR_GSCAN_RESULTS_SCAN_RESULT_TIME_STAMP
                            ]);
                    if (!
                        tb3[
                            QCA_WLAN_VENDOR_ATTR_GSCAN_RESULTS_SCAN_RESULT_SSID
                            ])
                    {
                        ALOGE("%s: "
                            "RESULTS_SCAN_RESULT_SSID not found",
                            __FUNCTION__);
                        return WIFI_ERROR_INVALID_ARGS;
                    }
                    len = nla_len(tb3[
                            QCA_WLAN_VENDOR_ATTR_GSCAN_RESULTS_SCAN_RESULT_SSID]);
                    len =
                        sizeof(cached_results[i].results[j].ssid) <= len ?
                        sizeof(cached_results[i].results[j].ssid) : len;
                    memcpy((void *)&cached_results[i].results[j].ssid,
                        nla_data(
                        tb3[
                        QCA_WLAN_VENDOR_ATTR_GSCAN_RESULTS_SCAN_RESULT_SSID]),
                        len);
                    if (!
                        tb3[
                            QCA_WLAN_VENDOR_ATTR_GSCAN_RESULTS_SCAN_RESULT_BSSID
                            ])
                    {
                        ALOGE("%s: "
                            "RESULTS_SCAN_RESULT_BSSID not found",
                            __FUNCTION__);
                        return WIFI_ERROR_INVALID_ARGS;
                    }
                    len = nla_len(
                        tb3[
                        QCA_WLAN_VENDOR_ATTR_GSCAN_RESULTS_SCAN_RESULT_BSSID]);
                    len =
                        sizeof(cached_results[i].results[j].bssid) <= len ?
                        sizeof(cached_results[i].results[j].bssid) : len;
                    memcpy(&cached_results[i].results[j].bssid,
                        nla_data(
                        tb3[
                        QCA_WLAN_VENDOR_ATTR_GSCAN_RESULTS_SCAN_RESULT_BSSID]),
                        len);
                    if (!
                        tb3[
                            QCA_WLAN_VENDOR_ATTR_GSCAN_RESULTS_SCAN_RESULT_CHANNEL
                            ])
                    {
                        ALOGE("%s: "
                            "RESULTS_SCAN_RESULT_CHANNEL not found",
                            __FUNCTION__);
                        return WIFI_ERROR_INVALID_ARGS;
                    }
                    cached_results[i].results[j].channel =
                        nla_get_u32(
                        tb3[QCA_WLAN_VENDOR_ATTR_GSCAN_RESULTS_SCAN_RESULT_CHANNEL]);
                    if (!
                        tb3[
                            QCA_WLAN_VENDOR_ATTR_GSCAN_RESULTS_SCAN_RESULT_RSSI
                            ])
                    {
                        ALOGE("%s: "
                            "RESULTS_SCAN_RESULT_RSSI not found",
                            __FUNCTION__);
                        return WIFI_ERROR_INVALID_ARGS;
                    }
                    cached_results[i].results[j].rssi =
                        get_s32(
                        tb3[QCA_WLAN_VENDOR_ATTR_GSCAN_RESULTS_SCAN_RESULT_RSSI]);
                    if (!
                        tb3[
                            QCA_WLAN_VENDOR_ATTR_GSCAN_RESULTS_SCAN_RESULT_RTT
                            ])
                    {
                        ALOGE("%s: "
                            "RESULTS_SCAN_RESULT_RTT not found",
                            __FUNCTION__);
                        return WIFI_ERROR_INVALID_ARGS;
                    }
                    cached_results[i].results[j].rtt =
                        nla_get_u32(
                        tb3[QCA_WLAN_VENDOR_ATTR_GSCAN_RESULTS_SCAN_RESULT_RTT]);
                    if (!
                        tb3[
                            QCA_WLAN_VENDOR_ATTR_GSCAN_RESULTS_SCAN_RESULT_RTT_SD
                        ])
                    {
                        ALOGE("%s: "
                            "RESULTS_SCAN_RESULT_RTT_SD not found",
                            __FUNCTION__);
                        return WIFI_ERROR_INVALID_ARGS;
                    }
                    cached_results[i].results[j].rtt_sd =
                        nla_get_u32(
                        tb3[QCA_WLAN_VENDOR_ATTR_GSCAN_RESULTS_SCAN_RESULT_RTT_SD]);
#ifdef QC_HAL_DEBUG
                    /* Enable these prints for debugging if needed. */
                    ALOGD("%s: ts  %" PRId64, __FUNCTION__,
                        cached_results[i].results[j].ts);
                    ALOGD("%s: SSID  %s ", __FUNCTION__,
                        cached_results[i].results[j].ssid);
                    ALOGD("%s: BSSID: %02x:%02x:%02x:%02x:%02x:%02x \n",
                        __FUNCTION__, cached_results[i].results[j].bssid[0],
                        cached_results[i].results[j].bssid[1],
                        cached_results[i].results[j].bssid[2],
                        cached_results[i].results[j].bssid[3],
                        cached_results[i].results[j].bssid[4],
                        cached_results[i].results[j].bssid[5]);
                    ALOGD("%s: channel %d ", __FUNCTION__,
                        cached_results[i].results[j].channel);
                    ALOGD("%s: rssi  %d ", __FUNCTION__,
                        cached_results[i].results[j].rssi);
                    ALOGD("%s: rtt  %" PRId64, __FUNCTION__,
                        cached_results[i].results[j].rtt);
                    ALOGD("%s: rtt_sd  %" PRId64, __FUNCTION__,
                        cached_results[i].results[j].rtt_sd);
#endif
                    /* Increment loop index for next record */
                    j++;
                    /* For this scan id, update the wifiScanResultsStartingIndex
                    * and number of cached results parsed so far.
                    */
                    mGetCachedResultsRspParams->wifiScanResultsStartingIndex = j;
                    cached_results[i].num_results++;
                } else {
                    /* We already parsed and stored up to max wifi_scan_results
                     * specified by the caller. Now, continue to loop over NL
                     * entries in order to properly update NL parsing pointer
                     * so it points to the next scan_id results.
                     */
                    ALOGD("%s: loop index:%d > max num"
                        " of wifi_scan_results:%d for gscan cached results"
                        " bucket:%d. Dummy loop", __FUNCTION__,
                        j, MAX_AP_CACHE_PER_SCAN, i);
                }
           }
           ALOGV("%s: cached_results[%d].num_results: %d ", __FUNCTION__,
            i, cached_results[i].num_results);
           /* Increment loop index for next cached scan result record */
           i++;
       }
       /* Increment starting index of filling cached results received */
       if (mGetCachedResultsRspParams->num_cached_results)
           mGetCachedResultsRspParams->cachedResultsStartingIndex =
               mGetCachedResultsRspParams->num_cached_results - 1;
    return WIFI_SUCCESS;
}

/* Set the GSCAN BSSID Hotlist. */
wifi_error wifi_set_epno_list(wifi_request_id id,
                                wifi_interface_handle iface,
                                const wifi_epno_params *epno_params,
                                wifi_epno_handler handler)
{
    int i, ret = 0, num_networks;
    GScanCommand *gScanCommand;
    struct nlattr *nlData, *nlPnoParamList;
    interface_info *ifaceInfo = getIfaceInfo(iface);
    wifi_handle wifiHandle = getWifiHandle(iface);
    bool previousGScanSetEpnoListRunning = false;
    hal_info *info = getHalInfo(wifiHandle);
    gscan_event_handlers* event_handlers;
    GScanCommandEventHandler *gScanSetPnoListCmdEventHandler;

    event_handlers = (gscan_event_handlers*)info->gscan_handlers;
    gScanSetPnoListCmdEventHandler =
        event_handlers->gScanSetPnoListCmdEventHandler;

    if (!(info->supported_feature_set & WIFI_FEATURE_HAL_EPNO)) {
        ALOGE("%s: Enhanced PNO is not supported by the driver",
            __FUNCTION__);
        return WIFI_ERROR_NOT_SUPPORTED;
    }

    /* Wi-Fi HAL doesn't need to check if a similar request to set ePNO
     * list was made earlier. If wifi_set_epno_list() is called while
     * another one is running, the request will be sent down to driver and
     * firmware. If the new request is successfully honored, then Wi-Fi HAL
     * will use the new request id for the gScanSetPnoListCmdEventHandler
     * object.
     */

    gScanCommand =
        new GScanCommand(
                    wifiHandle,
                    id,
                    OUI_QCA,
                    QCA_NL80211_VENDOR_SUBCMD_PNO_SET_LIST);
    if (gScanCommand == NULL) {
        ALOGE("%s: Error GScanCommand NULL", __FUNCTION__);
        return WIFI_ERROR_UNKNOWN;
    }

    /* Create the NL message. */
    ret = gScanCommand->create();
    if (ret < 0) {
        ALOGE("%s: Failed to create the NL msg. Error:%d", __FUNCTION__, ret);
        goto cleanup;
    }

    /* Set the interface Id of the message. */
    ret = gScanCommand->set_iface_id(ifaceInfo->name);
    if (ret < 0) {
        ALOGE("%s: Failed to set iface id. Error:%d", __FUNCTION__, ret);
        goto cleanup;
    }

    /* Add the vendor specific attributes for the NL command. */
    nlData = gScanCommand->attr_start(NL80211_ATTR_VENDOR_DATA);
    if (!nlData) {
        ALOGE("%s: Failed to add attribute NL80211_ATTR_VENDOR_DATA. Error:%d",
            __FUNCTION__, ret);
        goto cleanup;
    }

    num_networks = (unsigned int)epno_params->num_networks > MAX_EPNO_NETWORKS ?
                   MAX_EPNO_NETWORKS : epno_params->num_networks;
    if (gScanCommand->put_u32(
            QCA_WLAN_VENDOR_ATTR_GSCAN_SUBCMD_CONFIG_PARAM_REQUEST_ID,
            id) ||
        gScanCommand->put_u32(
            QCA_WLAN_VENDOR_ATTR_EPNO_MIN5GHZ_RSSI,
            epno_params->min5GHz_rssi) ||
        gScanCommand->put_u32(
            QCA_WLAN_VENDOR_ATTR_EPNO_MIN24GHZ_RSSI,
            epno_params->min24GHz_rssi) ||
        gScanCommand->put_u32(
            QCA_WLAN_VENDOR_ATTR_EPNO_INITIAL_SCORE_MAX,
            epno_params->initial_score_max) ||
        gScanCommand->put_u32(
            QCA_WLAN_VENDOR_ATTR_EPNO_CURRENT_CONNECTION_BONUS,
            epno_params->current_connection_bonus) ||
        gScanCommand->put_u32(
            QCA_WLAN_VENDOR_ATTR_EPNO_SAME_NETWORK_BONUS,
            epno_params->same_network_bonus) ||
        gScanCommand->put_u32(
            QCA_WLAN_VENDOR_ATTR_EPNO_SECURE_BONUS,
            epno_params->secure_bonus) ||
        gScanCommand->put_u32(
            QCA_WLAN_VENDOR_ATTR_EPNO_BAND5GHZ_BONUS,
            epno_params->band5GHz_bonus) ||
        gScanCommand->put_u32(
            QCA_WLAN_VENDOR_ATTR_PNO_SET_LIST_PARAM_NUM_NETWORKS,
            num_networks))
    {
        ALOGE("%s: Failed to add vendor atributes. Error:%d", __FUNCTION__, ret);
        goto cleanup;
    }

    /* Add the vendor specific attributes for the NL command. */
    nlPnoParamList =
        gScanCommand->attr_start(
                QCA_WLAN_VENDOR_ATTR_PNO_SET_LIST_PARAM_EPNO_NETWORKS_LIST);
    if (!nlPnoParamList) {
        ALOGE("%s: Failed to add attr. PNO_SET_LIST_PARAM_EPNO_NETWORKS_LIST. "
            "Error:%d", __FUNCTION__, ret);
        goto cleanup;
    }

    /* Add nested NL attributes for ePno List. */
    for (i = 0; i < num_networks; i++) {
        wifi_epno_network pnoNetwork = epno_params->networks[i];
        struct nlattr *nlPnoNetwork = gScanCommand->attr_start(i);
        if (!nlPnoNetwork) {
            ALOGE("%s: Failed attr_start for nlPnoNetwork. Error:%d",
                __FUNCTION__, ret);
            goto cleanup;
        }
        if (gScanCommand->put_string(
                QCA_WLAN_VENDOR_ATTR_PNO_SET_LIST_PARAM_EPNO_NETWORK_SSID,
                pnoNetwork.ssid) ||
            gScanCommand->put_u8(
                QCA_WLAN_VENDOR_ATTR_PNO_SET_LIST_PARAM_EPNO_NETWORK_FLAGS,
                pnoNetwork.flags) ||
            gScanCommand->put_u8(
                QCA_WLAN_VENDOR_ATTR_PNO_SET_LIST_PARAM_EPNO_NETWORK_AUTH_BIT,
                pnoNetwork.auth_bit_field))
        {
            ALOGE("%s: Failed to add PNO_SET_LIST_PARAM_EPNO_NETWORK_*. "
                "Error:%d", __FUNCTION__, ret);
            goto cleanup;
        }
        gScanCommand->attr_end(nlPnoNetwork);
    }

    gScanCommand->attr_end(nlPnoParamList);

    gScanCommand->attr_end(nlData);

    GScanCallbackHandler callbackHandler;
    memset(&callbackHandler, 0, sizeof(callbackHandler));
    callbackHandler.on_pno_network_found = handler.on_network_found;

    /* Create an object of the event handler class to take care of the
      * asychronous events on the north-bound.
      */
    if (gScanSetPnoListCmdEventHandler == NULL) {
        gScanSetPnoListCmdEventHandler = new GScanCommandEventHandler(
                            wifiHandle,
                            id,
                            OUI_QCA,
                            QCA_NL80211_VENDOR_SUBCMD_PNO_SET_LIST,
                            callbackHandler);
        if (gScanSetPnoListCmdEventHandler == NULL) {
            ALOGE("%s: Error instantiating "
                "gScanSetPnoListCmdEventHandler.", __FUNCTION__);
            ret = WIFI_ERROR_UNKNOWN;
            goto cleanup;
        }
        event_handlers->gScanSetPnoListCmdEventHandler =
            gScanSetPnoListCmdEventHandler;
    } else {
        gScanSetPnoListCmdEventHandler->setCallbackHandler(callbackHandler);
    }

    ret = gScanCommand->requestResponse();
    if (ret != 0) {
        ALOGE("%s: requestResponse Error:%d",__FUNCTION__, ret);
        goto cleanup;
    }

    if (gScanSetPnoListCmdEventHandler != NULL) {
        gScanSetPnoListCmdEventHandler->set_request_id(id);
        gScanSetPnoListCmdEventHandler->enableEventHandling();
    }

cleanup:
    delete gScanCommand;
    /* Disable Event Handling if ret != 0 */
    if (ret && gScanSetPnoListCmdEventHandler) {
        ALOGI("%s: Error ret:%d, disable event handling",
            __FUNCTION__, ret);
        gScanSetPnoListCmdEventHandler->disableEventHandling();
    }
    return (wifi_error)ret;
}

/* Reset the ePNO list - no ePNO networks should be matched after this */
wifi_error wifi_reset_epno_list(wifi_request_id id, wifi_interface_handle iface)
{
    int ret = 0;
    GScanCommand *gScanCommand;
    struct nlattr *nlData;
    interface_info *ifaceInfo = getIfaceInfo(iface);
    wifi_handle wifiHandle = getWifiHandle(iface);
    hal_info *info = getHalInfo(wifiHandle);

    if (!(info->supported_feature_set & WIFI_FEATURE_HAL_EPNO)) {
        ALOGE("%s: Enhanced PNO is not supported by the driver",
            __FUNCTION__);
        return WIFI_ERROR_NOT_SUPPORTED;
    }

    gScanCommand = new GScanCommand(wifiHandle,
                                    id,
                                    OUI_QCA,
                                    QCA_NL80211_VENDOR_SUBCMD_PNO_SET_LIST);
    if (gScanCommand == NULL) {
        ALOGE("%s: Error GScanCommand NULL", __FUNCTION__);
        return WIFI_ERROR_UNKNOWN;
    }

    /* Create the NL message. */
    ret = gScanCommand->create();
    if (ret < 0) {
        ALOGE("%s: Failed to create the NL msg. Error:%d", __FUNCTION__, ret);
        goto cleanup;
    }

    /* Set the interface Id of the message. */
    ret = gScanCommand->set_iface_id(ifaceInfo->name);
    if (ret < 0) {
        ALOGE("%s: Failed to set iface id. Error:%d", __FUNCTION__, ret);
        goto cleanup;
    }

    /* Add the vendor specific attributes for the NL command. */
    nlData = gScanCommand->attr_start(NL80211_ATTR_VENDOR_DATA);
    if (!nlData) {
        ALOGE("%s: Failed to add attribute NL80211_ATTR_VENDOR_DATA. Error:%d",
            __FUNCTION__, ret);
        goto cleanup;
    }

    if (gScanCommand->put_u32(
            QCA_WLAN_VENDOR_ATTR_GSCAN_SUBCMD_CONFIG_PARAM_REQUEST_ID,
            id) ||
        gScanCommand->put_u32(
            QCA_WLAN_VENDOR_ATTR_PNO_SET_LIST_PARAM_NUM_NETWORKS,
            EPNO_NO_NETWORKS))
    {
        ALOGE("%s: Failed to add vendor atributes Error:%d", __FUNCTION__, ret);
        goto cleanup;
    }

    gScanCommand->attr_end(nlData);

    ret = gScanCommand->requestResponse();
    if (ret != 0) {
        ALOGE("%s: requestResponse Error:%d",__FUNCTION__, ret);
    }

cleanup:
    delete gScanCommand;
    return (wifi_error)ret;
}

/* Set the ePNO Passpoint List. */
wifi_error wifi_set_passpoint_list(wifi_request_id id,
                                   wifi_interface_handle iface, int num,
                                   wifi_passpoint_network *networks,
                                   wifi_passpoint_event_handler handler)
{
    int i, numAp, ret = 0;
    GScanCommand *gScanCommand;
    struct nlattr *nlData, *nlPasspointNetworksParamList;
    interface_info *ifaceInfo = getIfaceInfo(iface);
    wifi_handle wifiHandle = getWifiHandle(iface);
    bool previousGScanPnoSetPasspointListRunning = false;
    hal_info *info = getHalInfo(wifiHandle);
    gscan_event_handlers* event_handlers;
    GScanCommandEventHandler *gScanPnoSetPasspointListCmdEventHandler;

    event_handlers = (gscan_event_handlers*)info->gscan_handlers;
    gScanPnoSetPasspointListCmdEventHandler =
        event_handlers->gScanPnoSetPasspointListCmdEventHandler;

    if (!(info->supported_feature_set & WIFI_FEATURE_HAL_EPNO)) {
        ALOGE("%s: Enhanced PNO is not supported by the driver",
            __FUNCTION__);
        return WIFI_ERROR_NOT_SUPPORTED;
    }

    /* Wi-Fi HAL doesn't need to check if a similar request to set ePNO
     * passpoint list was made earlier. If wifi_set_passpoint_list() is called
     * while another one is running, the request will be sent down to driver and
     * firmware. If the new request is successfully honored, then Wi-Fi HAL
     * will use the new request id for the
     * gScanPnoSetPasspointListCmdEventHandler object.
     */
    gScanCommand =
        new GScanCommand(
                    wifiHandle,
                    id,
                    OUI_QCA,
                    QCA_NL80211_VENDOR_SUBCMD_PNO_SET_PASSPOINT_LIST);
    if (gScanCommand == NULL) {
        ALOGE("%s: Error GScanCommand NULL", __FUNCTION__);
        return WIFI_ERROR_UNKNOWN;
    }

    /* Create the NL message. */
    ret = gScanCommand->create();
    if (ret < 0) {
        ALOGE("%s: Failed to create the NL msg. Error:%d", __FUNCTION__, ret);
        goto cleanup;
    }

    /* Set the interface Id of the message. */
    ret = gScanCommand->set_iface_id(ifaceInfo->name);
    if (ret < 0) {
        ALOGE("%s: Failed to set iface id. Error:%d", __FUNCTION__, ret);
        goto cleanup;
    }

    /* Add the vendor specific attributes for the NL command. */
    nlData = gScanCommand->attr_start(NL80211_ATTR_VENDOR_DATA);
    if (!nlData) {
        ALOGE("%s: Failed to add attribute NL80211_ATTR_VENDOR_DATA. Error:%d",
            __FUNCTION__, ret);
        goto cleanup;
    }

    if (gScanCommand->put_u32(
            QCA_WLAN_VENDOR_ATTR_GSCAN_SUBCMD_CONFIG_PARAM_REQUEST_ID,
            id) ||
        gScanCommand->put_u32(
            QCA_WLAN_VENDOR_ATTR_PNO_PASSPOINT_LIST_PARAM_NUM,
            num))
    {
        ALOGE("%s: Failed to add vendor atributes. Error:%d", __FUNCTION__, ret);
        goto cleanup;
    }

    /* Add the vendor specific attributes for the NL command. */
    nlPasspointNetworksParamList =
        gScanCommand->attr_start(
            QCA_WLAN_VENDOR_ATTR_PNO_PASSPOINT_LIST_PARAM_NETWORK_ARRAY);
    if (!nlPasspointNetworksParamList) {
        ALOGE("%s: Failed attr_start for PASSPOINT_LIST_PARAM_NETWORK_ARRAY. "
            "Error:%d", __FUNCTION__, ret);
        goto cleanup;
    }

    /* Add nested NL attributes for Passpoint List param. */
    for (i = 0; i < num; i++) {
        wifi_passpoint_network passpointNetwork = networks[i];
        struct nlattr *nlPasspointNetworkParam = gScanCommand->attr_start(i);
        if (!nlPasspointNetworkParam) {
            ALOGE("%s: Failed attr_start for nlPasspointNetworkParam. "
                "Error:%d", __FUNCTION__, ret);
            goto cleanup;
        }
        if (gScanCommand->put_u32(
                QCA_WLAN_VENDOR_ATTR_PNO_PASSPOINT_NETWORK_PARAM_ID,
                passpointNetwork.id) ||
            gScanCommand->put_string(
                QCA_WLAN_VENDOR_ATTR_PNO_PASSPOINT_NETWORK_PARAM_REALM,
                passpointNetwork.realm) ||
            gScanCommand->put_bytes(
         QCA_WLAN_VENDOR_ATTR_PNO_PASSPOINT_NETWORK_PARAM_ROAM_CNSRTM_ID,
                (char*)passpointNetwork.roamingConsortiumIds,
                16 * sizeof(int64_t)) ||
            gScanCommand->put_bytes(
            QCA_WLAN_VENDOR_ATTR_PNO_PASSPOINT_NETWORK_PARAM_ROAM_PLMN,
                (char*)passpointNetwork.plmn, 3 * sizeof(u8)))
        {
            ALOGE("%s: Failed to add PNO_PASSPOINT_NETWORK_PARAM_ROAM_* attr. "
                "Error:%d", __FUNCTION__, ret);
            goto cleanup;
        }
        gScanCommand->attr_end(nlPasspointNetworkParam);
    }

    gScanCommand->attr_end(nlPasspointNetworksParamList);

    gScanCommand->attr_end(nlData);

    GScanCallbackHandler callbackHandler;
    memset(&callbackHandler, 0, sizeof(callbackHandler));
    callbackHandler.on_passpoint_network_found =
                        handler.on_passpoint_network_found;

    /* Create an object of the event handler class to take care of the
      * asychronous events on the north-bound.
      */
    if (gScanPnoSetPasspointListCmdEventHandler == NULL) {
        gScanPnoSetPasspointListCmdEventHandler = new GScanCommandEventHandler(
                        wifiHandle,
                        id,
                        OUI_QCA,
                        QCA_NL80211_VENDOR_SUBCMD_PNO_SET_PASSPOINT_LIST,
                        callbackHandler);
        if (gScanPnoSetPasspointListCmdEventHandler == NULL) {
            ALOGE("%s: Error instantiating "
                "gScanPnoSetPasspointListCmdEventHandler.", __FUNCTION__);
            ret = WIFI_ERROR_UNKNOWN;
            goto cleanup;
        }
        event_handlers->gScanPnoSetPasspointListCmdEventHandler =
            gScanPnoSetPasspointListCmdEventHandler;
    } else {
        gScanPnoSetPasspointListCmdEventHandler->setCallbackHandler(callbackHandler);
    }

    ret = gScanCommand->requestResponse();
    if (ret != 0) {
        ALOGE("%s: requestResponse Error:%d",__FUNCTION__, ret);
        goto cleanup;
    }

    if (gScanPnoSetPasspointListCmdEventHandler != NULL) {
        gScanPnoSetPasspointListCmdEventHandler->set_request_id(id);
        gScanPnoSetPasspointListCmdEventHandler->enableEventHandling();
    }

cleanup:
    delete gScanCommand;
    /* Disable Event Handling if ret != 0 */
    if (ret && gScanPnoSetPasspointListCmdEventHandler) {
        ALOGI("%s: Error ret:%d, disable event handling",
            __FUNCTION__, ret);
        gScanPnoSetPasspointListCmdEventHandler->disableEventHandling();
    }
    return (wifi_error)ret;
}

wifi_error wifi_reset_passpoint_list(wifi_request_id id,
                            wifi_interface_handle iface)
{
    int ret = 0;
    GScanCommand *gScanCommand;
    struct nlattr *nlData;
    interface_info *ifaceInfo = getIfaceInfo(iface);
    wifi_handle wifiHandle = getWifiHandle(iface);
    hal_info *info = getHalInfo(wifiHandle);
    gscan_event_handlers* event_handlers;
    GScanCommandEventHandler *gScanPnoSetPasspointListCmdEventHandler;

    event_handlers = (gscan_event_handlers*)info->gscan_handlers;
    gScanPnoSetPasspointListCmdEventHandler =
        event_handlers->gScanPnoSetPasspointListCmdEventHandler;

    if (!(info->supported_feature_set & WIFI_FEATURE_HAL_EPNO)) {
        ALOGE("%s: Enhanced PNO is not supported by the driver",
            __FUNCTION__);
        return WIFI_ERROR_NOT_SUPPORTED;
    }

    if (gScanPnoSetPasspointListCmdEventHandler == NULL ||
        (gScanPnoSetPasspointListCmdEventHandler->isEventHandlingEnabled() ==
         false)) {
        ALOGE("wifi_reset_passpoint_list: ePNO passpoint_list isn't set. "
            "Nothing to do. Exit.");
        return WIFI_ERROR_NOT_AVAILABLE;
    }

    gScanCommand = new GScanCommand(
                    wifiHandle,
                    id,
                    OUI_QCA,
                    QCA_NL80211_VENDOR_SUBCMD_PNO_RESET_PASSPOINT_LIST);

    if (gScanCommand == NULL) {
        ALOGE("%s: Error GScanCommand NULL", __FUNCTION__);
        return WIFI_ERROR_UNKNOWN;
    }

    /* Create the NL message. */
    ret = gScanCommand->create();
    if (ret < 0) {
        ALOGE("%s: Failed to create the NL msg. Error:%d", __FUNCTION__, ret);
        goto cleanup;
    }

    /* Set the interface Id of the message. */
    ret = gScanCommand->set_iface_id(ifaceInfo->name);
    if (ret < 0) {
        ALOGE("%s: Failed to set iface id. Error:%d", __FUNCTION__, ret);
        goto cleanup;
    }

    /* Add the vendor specific attributes for the NL command. */
    nlData = gScanCommand->attr_start(NL80211_ATTR_VENDOR_DATA);
    if (!nlData) {
        ALOGE("%s: Failed to add attribute NL80211_ATTR_VENDOR_DATA. Error:%d",
            __FUNCTION__, ret);
        goto cleanup;
    }

    ret = gScanCommand->put_u32(
            QCA_WLAN_VENDOR_ATTR_GSCAN_SUBCMD_CONFIG_PARAM_REQUEST_ID, id);
    if (ret < 0) {
        ALOGE("%s: Failed to add vendor data attributes. Error:%d",
            __FUNCTION__, ret);
        goto cleanup;
    }

    gScanCommand->attr_end(nlData);

    ret = gScanCommand->requestResponse();
    if (ret != 0) {
        ALOGE("%s: requestResponse Error:%d",__FUNCTION__, ret);
    }

    /* Disable Event Handling. */
    if (gScanPnoSetPasspointListCmdEventHandler) {
        gScanPnoSetPasspointListCmdEventHandler->disableEventHandling();
    }

cleanup:
    delete gScanCommand;
    return (wifi_error)ret;
}

int GScanCommand::allocCachedResultsTemp(int max,
                                     wifi_cached_scan_results *cached_results)
{
    wifi_cached_scan_results *tempCachedResults = NULL;

    /* Alloc memory for "max" number of cached results. */
    mGetCachedResultsRspParams->cached_results =
        (wifi_cached_scan_results*)
        malloc(max * sizeof(wifi_cached_scan_results));
    if (!mGetCachedResultsRspParams->cached_results) {
        ALOGE("%s: Failed to allocate memory for "
              "mGetCachedResultsRspParams->cached_results.",
              __FUNCTION__);
        return WIFI_ERROR_OUT_OF_MEMORY;
    }
    memset(mGetCachedResultsRspParams->cached_results, 0,
           max * sizeof(wifi_cached_scan_results));

    mGetCachedResultsRspParams->max = max;

    return WIFI_SUCCESS;
}

/*
 * Allocates memory for the subCmd response struct and initializes status = -1
 */
int GScanCommand::allocRspParams(eGScanRspRarams cmd)
{
    int ret = 0;
    switch(cmd)
    {
        case eGScanGetCapabilitiesRspParams:
            mGetCapabilitiesRspParams = (GScanGetCapabilitiesRspParams *)
                malloc(sizeof(GScanGetCapabilitiesRspParams));
            if (!mGetCapabilitiesRspParams)
                ret = -1;
            else  {
                memset(&mGetCapabilitiesRspParams->capabilities, 0,
                    sizeof(wifi_gscan_capabilities));
            }
        break;
        case eGScanGetCachedResultsRspParams:
            mGetCachedResultsRspParams = (GScanGetCachedResultsRspParams *)
                malloc(sizeof(GScanGetCachedResultsRspParams));
            if (!mGetCachedResultsRspParams)
                ret = -1;
            else {
                mGetCachedResultsRspParams->num_cached_results = 0;
                mGetCachedResultsRspParams->more_data = false;
                mGetCachedResultsRspParams->cachedResultsStartingIndex = -1;
                mGetCachedResultsRspParams->lastProcessedScanId = -1;
                mGetCachedResultsRspParams->wifiScanResultsStartingIndex = -1;
                mGetCachedResultsRspParams->max = 0;
                mGetCachedResultsRspParams->cached_results = NULL;
            }
        break;
        default:
            ALOGD("%s: Wrong request for alloc.", __FUNCTION__);
            ret = -1;
    }
    return ret;
}

void GScanCommand::freeRspParams(eGScanRspRarams cmd)
{
    u32 i = 0;
    wifi_cached_scan_results *cached_results = NULL;

    switch(cmd)
    {
        case eGScanGetCapabilitiesRspParams:
            if (mGetCapabilitiesRspParams) {
                free(mGetCapabilitiesRspParams);
                mGetCapabilitiesRspParams = NULL;
            }
        break;
        case eGScanGetCachedResultsRspParams:
            if (mGetCachedResultsRspParams) {
                if (mGetCachedResultsRspParams->cached_results) {
                    free(mGetCachedResultsRspParams->cached_results);
                    mGetCachedResultsRspParams->cached_results = NULL;
                }
                free(mGetCachedResultsRspParams);
                mGetCachedResultsRspParams = NULL;
            }
        break;
        default:
            ALOGD("%s: Wrong request for free.", __FUNCTION__);
    }
}

wifi_error GScanCommand::copyCachedScanResults(
                                      int *numResults,
                                      wifi_cached_scan_results *cached_results)
{
    wifi_error ret = WIFI_SUCCESS;
    int i;
    wifi_cached_scan_results *cachedResultRsp;

    if (mGetCachedResultsRspParams && cached_results)
    {
        /* Populate the number of parsed cached results. */
        *numResults = mGetCachedResultsRspParams->num_cached_results;

        for (i = 0; i < *numResults; i++) {
            cachedResultRsp = &mGetCachedResultsRspParams->cached_results[i];
            cached_results[i].scan_id = cachedResultRsp->scan_id;
            cached_results[i].flags = cachedResultRsp->flags;
            cached_results[i].num_results = cachedResultRsp->num_results;
            cached_results[i].buckets_scanned = cachedResultRsp->buckets_scanned;

            if (!cached_results[i].num_results) {
                ALOGI("Error: cached_results[%d].num_results=0", i);
                continue;
            }

            ALOGV("copyCachedScanResults: "
                "cached_results[%d].num_results : %d",
                i, cached_results[i].num_results);

            memcpy(cached_results[i].results,
                cachedResultRsp->results,
                cached_results[i].num_results * sizeof(wifi_scan_result));
        }
    } else {
        ALOGE("%s: mGetCachedResultsRspParams is NULL", __FUNCTION__);
        *numResults = 0;
        ret = WIFI_ERROR_INVALID_ARGS;
    }
    return ret;
}

wifi_error GScanCommand::getGetCapabilitiesRspParams(
                                        wifi_gscan_capabilities *capabilities)
{
    if (mGetCapabilitiesRspParams && capabilities)
    {
        if (mGetCapabilitiesRspParams->capabilities.max_scan_buckets == 0) {
            ALOGE("%s: max_scan_buckets is 0", __FUNCTION__);
            return WIFI_ERROR_NOT_AVAILABLE;
        }
        memcpy(capabilities,
            &mGetCapabilitiesRspParams->capabilities,
            sizeof(wifi_gscan_capabilities));
    } else {
        ALOGE("%s: mGetCapabilitiesRspParams is NULL", __FUNCTION__);
        return WIFI_ERROR_NOT_AVAILABLE;
    }

    return WIFI_SUCCESS;
}

void GScanCommand::setMaxChannels(int max_channels) {
    mMaxChannels = max_channels;
}

void GScanCommand::setChannels(int *channels) {
    mChannels = channels;
}

void GScanCommand::setNumChannelsPtr(int *num_channels) {
    mNumChannelsPtr = num_channels;
}

wifi_error wifi_set_bssid_blacklist(wifi_request_id id,
                                    wifi_interface_handle iface,
                                    wifi_bssid_params params)
{
    int ret = 0, i;
    GScanCommand *roamCommand;
    struct nlattr *nlData, *nlBssids;
    interface_info *ifaceInfo = getIfaceInfo(iface);
    wifi_handle wifiHandle = getWifiHandle(iface);
    hal_info *info = getHalInfo(wifiHandle);

    if (!(info->supported_feature_set & WIFI_FEATURE_GSCAN)) {
        ALOGE("%s: GSCAN is not supported by driver",
            __FUNCTION__);
        return WIFI_ERROR_NOT_SUPPORTED;
    }

    for (i = 0; i < params.num_bssid; i++) {
        ALOGV("BSSID: %d : %02x:%02x:%02x:%02x:%02x:%02x", i,
                params.bssids[i][0], params.bssids[i][1],
                params.bssids[i][2], params.bssids[i][3],
                params.bssids[i][4], params.bssids[i][5]);
    }

    roamCommand =
         new GScanCommand(wifiHandle,
                          id,
                          OUI_QCA,
                          QCA_NL80211_VENDOR_SUBCMD_ROAM);
    if (roamCommand == NULL) {
        ALOGE("%s: Error roamCommand NULL", __FUNCTION__);
        return WIFI_ERROR_UNKNOWN;
    }

    /* Create the NL message. */
    ret = roamCommand->create();
    if (ret < 0)
        goto cleanup;

    /* Set the interface Id of the message. */
    ret = roamCommand->set_iface_id(ifaceInfo->name);
    if (ret < 0)
        goto cleanup;

    /* Add the vendor specific attributes for the NL command. */
    nlData = roamCommand->attr_start(NL80211_ATTR_VENDOR_DATA);
    if (!nlData)
        goto cleanup;

    if (roamCommand->put_u32(QCA_WLAN_VENDOR_ATTR_ROAMING_SUBCMD,
            QCA_WLAN_VENDOR_ATTR_ROAM_SUBCMD_SET_BLACKLIST_BSSID) ||
        roamCommand->put_u32(
            QCA_WLAN_VENDOR_ATTR_ROAMING_REQ_ID,
            id) ||
        roamCommand->put_u32(
            QCA_WLAN_VENDOR_ATTR_ROAMING_PARAM_SET_BSSID_PARAMS_NUM_BSSID,
            params.num_bssid)) {
        goto cleanup;
    }

    nlBssids = roamCommand->attr_start(
            QCA_WLAN_VENDOR_ATTR_ROAMING_PARAM_SET_BSSID_PARAMS);
    for (i = 0; i < params.num_bssid; i++) {
        struct nlattr *nl_ssid = roamCommand->attr_start(i);

        if (roamCommand->put_addr(
                QCA_WLAN_VENDOR_ATTR_ROAMING_PARAM_SET_BSSID_PARAMS_BSSID,
                (u8 *)params.bssids[i])) {
            goto cleanup;
        }

        roamCommand->attr_end(nl_ssid);
    }
    roamCommand->attr_end(nlBssids);

    roamCommand->attr_end(nlData);

    ret = roamCommand->requestResponse();
    if (ret != 0) {
        ALOGE("wifi_set_bssid_blacklist(): requestResponse Error:%d", ret);
    }

cleanup:
    delete roamCommand;
    return (wifi_error)ret;

}
