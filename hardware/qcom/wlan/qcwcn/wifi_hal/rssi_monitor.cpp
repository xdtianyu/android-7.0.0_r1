/* Copyright (c) 2015, The Linux Foundation. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *  * Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *  * Neither the name of The Linux Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 * BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 * OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 * IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "sync.h"

#define LOG_TAG  "WifiHAL"

#include <utils/Log.h>

#include "wifi_hal.h"
#include "common.h"
#include "cpp_bindings.h"
#include "rssi_monitor.h"
#include "qca-vendor.h"
#include "vendor_definitions.h"

//Singleton Static Instance
RSSIMonitorCommand* RSSIMonitorCommand::mRSSIMonitorCommandInstance = NULL;

RSSIMonitorCommand::RSSIMonitorCommand(wifi_handle handle, int id,
                                       u32 vendor_id, u32 subcmd)
        : WifiVendorCommand(handle, id, vendor_id, subcmd)
{
    mRSSIMonitorCommandInstance = NULL;
    memset(&mHandler, 0, sizeof(mHandler));
}

RSSIMonitorCommand::~RSSIMonitorCommand()
{
    mRSSIMonitorCommandInstance = NULL;
}

void RSSIMonitorCommand::setReqId(wifi_request_id reqid)
{
    mId = reqid;
}

RSSIMonitorCommand* RSSIMonitorCommand::instance(wifi_handle handle,
                                                 wifi_request_id id)
{
    if (handle == NULL) {
        ALOGE("Interface Handle is invalid");
        return NULL;
    }
    if (mRSSIMonitorCommandInstance == NULL) {
        mRSSIMonitorCommandInstance = new RSSIMonitorCommand(handle, id,
                OUI_QCA,
                QCA_NL80211_VENDOR_SUBCMD_MONITOR_RSSI);
        return mRSSIMonitorCommandInstance;
    }
    else
    {
        if (handle != getWifiHandle(mRSSIMonitorCommandInstance->mInfo))
        {
            /* upper layer must have cleaned up the handle and reinitialized,
               so we need to update the same */
            ALOGV("Handle different, update the handle");
            mRSSIMonitorCommandInstance->mInfo = (hal_info *)handle;
        }
        mRSSIMonitorCommandInstance->setReqId(id);
    }
    return mRSSIMonitorCommandInstance;
}

/* This function will be the main handler for incoming event.
 * Call the appropriate callback handler after parsing the vendor data.
 */
int RSSIMonitorCommand::handleEvent(WifiEvent &event)
{
    int ret = WIFI_SUCCESS;
    WifiVendorCommand::handleEvent(event);

    /* Parse the vendordata and get the attribute */
    switch(mSubcmd)
    {
        case QCA_NL80211_VENDOR_SUBCMD_MONITOR_RSSI:
        {
            mac_addr addr;
            s8 rssi;
            wifi_request_id reqId;
            struct nlattr *tb_vendor[QCA_WLAN_VENDOR_ATTR_RSSI_MONITORING_MAX
                                     + 1];
            nla_parse(tb_vendor, QCA_WLAN_VENDOR_ATTR_RSSI_MONITORING_MAX,
                    (struct nlattr *)mVendorData,
                    mDataLen, NULL);

            memset(addr, 0, sizeof(mac_addr));

            if (!tb_vendor[
                QCA_WLAN_VENDOR_ATTR_RSSI_MONITORING_REQUEST_ID])
            {
                ALOGE("%s: ATTR_RSSI_MONITORING_REQUEST_ID not found. Exit.",
                    __FUNCTION__);
                ret = WIFI_ERROR_INVALID_ARGS;
                break;
            }
            reqId = nla_get_u32(
                    tb_vendor[QCA_WLAN_VENDOR_ATTR_RSSI_MONITORING_REQUEST_ID]
                    );
            /* If event has a different request_id, ignore that and use the
             *  request_id value which we're maintaining.
             */
            if (reqId != id()) {
                ALOGV("%s: Event has Req. ID:%d <> Ours:%d, continue...",
                    __FUNCTION__, reqId, id());
                reqId = id();
            }
            ret = get_mac_addr(tb_vendor,
                    QCA_WLAN_VENDOR_ATTR_RSSI_MONITORING_CUR_BSSID,
                    addr);
            if (ret != WIFI_SUCCESS) {
                return ret;
            }
            ALOGV(MAC_ADDR_STR, MAC_ADDR_ARRAY(addr));

            if (!tb_vendor[QCA_WLAN_VENDOR_ATTR_RSSI_MONITORING_CUR_RSSI])
            {
                ALOGE("%s: QCA_WLAN_VENDOR_ATTR_RSSI_MONITORING_CUR_RSSI"
                      " not found", __FUNCTION__);
                return WIFI_ERROR_INVALID_ARGS;
            }
            rssi = get_s8(tb_vendor[
                        QCA_WLAN_VENDOR_ATTR_RSSI_MONITORING_CUR_RSSI]);
            ALOGV("Current RSSI : %d ", rssi);

            if (mHandler.on_rssi_threshold_breached)
                (*mHandler.on_rssi_threshold_breached)(reqId, addr, rssi);
            else
                ALOGE("RSSI Monitoring: No Callback registered: ");
        }
        break;

        default:
            /* Error case should not happen print log */
            ALOGE("%s: Wrong subcmd received %d", __FUNCTION__, mSubcmd);
    }

    return NL_SKIP;
}

int RSSIMonitorCommand::setCallbackHandler(wifi_rssi_event_handler nHandler,
                                           u32 event)
{
    int ret;
    mHandler = nHandler;
    ret = registerVendorHandler(mVendor_id, event);
    if (ret != 0) {
        /* Error case should not happen print log */
        ALOGE("%s: Unable to register Vendor Handler Vendor Id=0x%x subcmd=%u",
              __FUNCTION__, mVendor_id, mSubcmd);
    }
    return ret;
}

wifi_error RSSIMonitorCommand::unregisterHandler(u32 subCmd)
{
    unregisterVendorHandler(mVendor_id, subCmd);
    return WIFI_SUCCESS;
}

wifi_error wifi_start_rssi_monitoring(wifi_request_id id,
                                      wifi_interface_handle iface,
                                      s8 max_rssi,
                                      s8 min_rssi,
                                      wifi_rssi_event_handler eh)
{
    int ret = WIFI_SUCCESS;
    struct nlattr *nlData;
    WifiVendorCommand *vCommand = NULL;
    wifi_handle wifiHandle = getWifiHandle(iface);
    RSSIMonitorCommand *rssiCommand;

    ret = initialize_vendor_cmd(iface, id,
                                QCA_NL80211_VENDOR_SUBCMD_MONITOR_RSSI,
                                &vCommand);
    if (ret != WIFI_SUCCESS) {
        ALOGE("%s: Initialization failed", __FUNCTION__);
        return (wifi_error)ret;
    }

    ALOGV("%s: Max RSSI:%d Min RSSI:%d", __FUNCTION__,
          max_rssi, min_rssi);
    /* Add the vendor specific attributes for the NL command. */
    nlData = vCommand->attr_start(NL80211_ATTR_VENDOR_DATA);
    if (!nlData)
        goto cleanup;

    if (vCommand->put_u32(
            QCA_WLAN_VENDOR_ATTR_RSSI_MONITORING_CONTROL,
            QCA_WLAN_RSSI_MONITORING_START) ||
        vCommand->put_u32(
            QCA_WLAN_VENDOR_ATTR_RSSI_MONITORING_REQUEST_ID,
            id) ||
        vCommand->put_s8(
            QCA_WLAN_VENDOR_ATTR_RSSI_MONITORING_MAX_RSSI,
            max_rssi) ||
        vCommand->put_s8(
            QCA_WLAN_VENDOR_ATTR_RSSI_MONITORING_MIN_RSSI,
            min_rssi))
    {
        goto cleanup;
    }

    vCommand->attr_end(nlData);

    rssiCommand = RSSIMonitorCommand::instance(wifiHandle, id);
    if (rssiCommand == NULL) {
        ALOGE("%s: Error rssiCommand NULL", __FUNCTION__);
        return WIFI_ERROR_OUT_OF_MEMORY;
    }

    ret = rssiCommand->setCallbackHandler(eh,
            QCA_NL80211_VENDOR_SUBCMD_MONITOR_RSSI);
    if (ret < 0)
        goto cleanup;

    ret = vCommand->requestResponse();
    if (ret < 0)
        goto cleanup;

cleanup:
    delete vCommand;
    return (wifi_error)ret;
}

wifi_error wifi_stop_rssi_monitoring(wifi_request_id id,
                                     wifi_interface_handle iface)
{
    int ret = WIFI_SUCCESS;
    struct nlattr *nlData;
    WifiVendorCommand *vCommand = NULL;
    wifi_handle wifiHandle = getWifiHandle(iface);
    RSSIMonitorCommand *rssiCommand;

    ret = initialize_vendor_cmd(iface, id,
                                QCA_NL80211_VENDOR_SUBCMD_MONITOR_RSSI,
                                &vCommand);
    if (ret != WIFI_SUCCESS) {
        ALOGE("%s: Initialization failed", __FUNCTION__);
        return (wifi_error)ret;
    }

    /* Add the vendor specific attributes for the NL command. */
    nlData = vCommand->attr_start(NL80211_ATTR_VENDOR_DATA);
    if (!nlData)
        goto cleanup;

    if (vCommand->put_u32(
            QCA_WLAN_VENDOR_ATTR_RSSI_MONITORING_CONTROL,
            QCA_WLAN_RSSI_MONITORING_STOP) ||
        vCommand->put_u32(
            QCA_WLAN_VENDOR_ATTR_RSSI_MONITORING_REQUEST_ID,
            id))
    {
        goto cleanup;
    }

    vCommand->attr_end(nlData);

    ret = vCommand->requestResponse();
    if (ret < 0)
        goto cleanup;

    rssiCommand = RSSIMonitorCommand::instance(wifiHandle, id);
    if (rssiCommand == NULL) {
        ALOGE("%s: Error rssiCommand NULL", __FUNCTION__);
        ret = WIFI_ERROR_OUT_OF_MEMORY;
        goto cleanup;
    }

    ret = rssiCommand->unregisterHandler(
                                        QCA_NL80211_VENDOR_SUBCMD_MONITOR_RSSI);
    if (ret != WIFI_SUCCESS)
        goto cleanup;

    delete rssiCommand;

cleanup:
    delete vCommand;
    return (wifi_error)ret;
}
