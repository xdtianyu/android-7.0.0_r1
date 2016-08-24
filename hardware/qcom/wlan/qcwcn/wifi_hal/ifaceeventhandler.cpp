/* Copyright (c) 2014, The Linux Foundation. All rights reserved.
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
#include <time.h>

#include "ifaceeventhandler.h"

/* Used to handle NL command events from driver/firmware. */
IfaceEventHandlerCommand *mwifiEventHandler = NULL;

/* Set the interface event monitor handler*/
wifi_error wifi_set_iface_event_handler(wifi_request_id id,
                                        wifi_interface_handle iface,
                                        wifi_event_handler eh)
{
    int i, numAp, ret = 0;
    interface_info *ifaceInfo = getIfaceInfo(iface);
    wifi_handle wifiHandle = getWifiHandle(iface);

    /* Check if a similar request to set iface event handler was made earlier.
     * Right now we don't differentiate between the case where (i) the new
     * Request Id is different from the current one vs (ii) both new and
     * Request Ids are the same.
     */
    if (mwifiEventHandler)
    {
        if (id == mwifiEventHandler->get_request_id()) {
            ALOGE("%s: Iface Event Handler Set for request Id %d is still"
                "running. Exit", __func__, id);
            return WIFI_ERROR_TOO_MANY_REQUESTS;
        } else {
            ALOGE("%s: Iface Event Handler Set for a different Request "
                "Id:%d is requested. Not supported. Exit", __func__, id);
            return WIFI_ERROR_NOT_SUPPORTED;
        }
    }

    mwifiEventHandler = new IfaceEventHandlerCommand(
                    wifiHandle,
                    id,
                    NL80211_CMD_REG_CHANGE);
    if (mwifiEventHandler == NULL) {
        ALOGE("%s: Error mwifiEventHandler NULL", __func__);
        return WIFI_ERROR_UNKNOWN;
    }
    mwifiEventHandler->setCallbackHandler(eh);

cleanup:
    return (wifi_error)ret;
}

/* Reset monitoring for the NL event*/
wifi_error wifi_reset_iface_event_handler(wifi_request_id id,
                                          wifi_interface_handle iface)
{
    int ret = 0;

    if (mwifiEventHandler)
    {
        if (id == mwifiEventHandler->get_request_id()) {
            ALOGV("Delete Object mwifiEventHandler for id = %d", id);
            delete mwifiEventHandler;
            mwifiEventHandler = NULL;
        } else {
            ALOGE("%s: Iface Event Handler Set for a different Request "
                "Id:%d is requested. Not supported. Exit", __func__, id);
            return WIFI_ERROR_NOT_SUPPORTED;
        }
    } else {
        ALOGV("Object mwifiEventHandler for id = %d already Deleted", id);
    }

cleanup:
    return (wifi_error)ret;
}

/* This function will be the main handler for the registered incoming
 * (from driver) Commads. Calls the appropriate callback handler after
 * parsing the vendor data.
 */
int IfaceEventHandlerCommand::handleEvent(WifiEvent &event)
{
    int ret = WIFI_SUCCESS;

    wifiEventHandler::handleEvent(event);

    switch(mSubcmd)
    {
        case NL80211_CMD_REG_CHANGE:
        {
            char code[2];
            memset(&code[0], 0, 2);
            if(tb[NL80211_ATTR_REG_ALPHA2])
            {
                memcpy(&code[0], (char *) nla_data(tb[NL80211_ATTR_REG_ALPHA2]), 2);
            } else {
                ALOGE("%s: NL80211_ATTR_REG_ALPHA2 not found", __func__);
            }
            ALOGV("Country : %c%c", code[0], code[1]);
            if(mHandler.on_country_code_changed)
            {
                mHandler.on_country_code_changed(code);
            }
        }
        break;
        default:
            ALOGV("NL Event : %d Not supported", mSubcmd);
    }

    return NL_SKIP;
}

IfaceEventHandlerCommand::IfaceEventHandlerCommand(wifi_handle handle, int id, u32 subcmd)
        : wifiEventHandler(handle, id, subcmd)
{
    ALOGV("wifiEventHandler %p constructed", this);
    registerHandler(mSubcmd);
    memset(&mHandler, 0, sizeof(wifi_event_handler));
    mEventData = NULL;
    mDataLen = 0;
}

IfaceEventHandlerCommand::~IfaceEventHandlerCommand()
{
    ALOGV("IfaceEventHandlerCommand %p destructor", this);
    unregisterHandler(mSubcmd);
}

void IfaceEventHandlerCommand::setCallbackHandler(wifi_event_handler nHandler)
{
    mHandler = nHandler;
}

int wifiEventHandler::get_request_id()
{
    return mRequestId;
}

int IfaceEventHandlerCommand::get_request_id()
{
    return wifiEventHandler::get_request_id();
}

wifiEventHandler::wifiEventHandler(wifi_handle handle, int id, u32 subcmd)
        : WifiCommand(handle, id)
{
    mRequestId = id;
    mSubcmd = subcmd;
    registerHandler(mSubcmd);
    ALOGV("wifiEventHandler %p constructed", this);
}

wifiEventHandler::~wifiEventHandler()
{
    ALOGV("wifiEventHandler %p destructor", this);
    unregisterHandler(mSubcmd);
}

int wifiEventHandler::handleEvent(WifiEvent &event)
{
    struct genlmsghdr *gnlh = event.header();
    mSubcmd = gnlh->cmd;
    nla_parse(tb, NL80211_ATTR_MAX, genlmsg_attrdata(gnlh, 0),
            genlmsg_attrlen(gnlh, 0), NULL);
    ALOGV("Got NL Event : %d from the Driver.", gnlh->cmd);

    return NL_SKIP;
}

WifihalGeneric::WifihalGeneric(wifi_handle handle, int id, u32 vendor_id,
                                  u32 subcmd)
        : WifiVendorCommand(handle, id, vendor_id, subcmd)
{
    /* Initialize the member data variables here */
    mSet = 0;
    mSetSizeMax = 0;
    mSetSizePtr = NULL;
    mConcurrencySet = 0;
    filterVersion = 0;
    filterLength = 0;
    firmware_bus_max_size = 0;
}

WifihalGeneric::~WifihalGeneric()
{
}

int WifihalGeneric::requestResponse()
{
    return WifiCommand::requestResponse(mMsg);
}

int WifihalGeneric::handleResponse(WifiEvent &reply)
{
    ALOGV("Got a Wi-Fi HAL module message from Driver");
    int i = 0;
    u32 status;
    WifiVendorCommand::handleResponse(reply);

    // Parse the vendordata and get the attribute
    switch(mSubcmd)
    {
        case QCA_NL80211_VENDOR_SUBCMD_GET_SUPPORTED_FEATURES:
            {
                struct nlattr *tb_vendor[QCA_WLAN_VENDOR_ATTR_FEATURE_SET_MAX + 1];
                nla_parse(tb_vendor, QCA_WLAN_VENDOR_ATTR_FEATURE_SET_MAX,
                        (struct nlattr *)mVendorData,
                        mDataLen, NULL);

                if (!tb_vendor[QCA_WLAN_VENDOR_ATTR_FEATURE_SET])
                {
                    ALOGE("%s: QCA_WLAN_VENDOR_ATTR_FEATURE_SET not found", __func__);
                    return WIFI_ERROR_INVALID_ARGS;
                }
                mSet = nla_get_u32(tb_vendor[QCA_WLAN_VENDOR_ATTR_FEATURE_SET]);
                ALOGV("Supported feature set : %x", mSet);

                break;
            }
        case QCA_NL80211_VENDOR_SUBCMD_GET_CONCURRENCY_MATRIX:
            {
                struct nlattr *tb_vendor[
                    QCA_WLAN_VENDOR_ATTR_GET_CONCURRENCY_MATRIX_MAX + 1];
                nla_parse(tb_vendor,
                    QCA_WLAN_VENDOR_ATTR_GET_CONCURRENCY_MATRIX_MAX,
                    (struct nlattr *)mVendorData,mDataLen, NULL);

                if (tb_vendor[
                    QCA_WLAN_VENDOR_ATTR_GET_CONCURRENCY_MATRIX_RESULTS_SET_SIZE]) {
                    u32 val;
                    val = nla_get_u32(
                        tb_vendor[
                    QCA_WLAN_VENDOR_ATTR_GET_CONCURRENCY_MATRIX_RESULTS_SET_SIZE]);

                    ALOGV("%s: Num of concurrency combinations: %d",
                        __func__, val);
                    val = val > (unsigned int)mSetSizeMax ?
                          (unsigned int)mSetSizeMax : val;
                    *mSetSizePtr = val;

                    /* Extract the list of channels. */
                    if (*mSetSizePtr > 0 &&
                        tb_vendor[
                        QCA_WLAN_VENDOR_ATTR_GET_CONCURRENCY_MATRIX_RESULTS_SET]) {
                        nla_memcpy(mConcurrencySet,
                            tb_vendor[
                        QCA_WLAN_VENDOR_ATTR_GET_CONCURRENCY_MATRIX_RESULTS_SET],
                            sizeof(feature_set) * (*mSetSizePtr));
                    }

                    ALOGV("%s: Get concurrency matrix response received.",
                        __func__);
                    ALOGV("%s: Num of concurrency combinations : %d",
                        __func__, *mSetSizePtr);
                    ALOGV("%s: List of valid concurrency combinations is: ",
                        __func__);
                    for(i = 0; i < *mSetSizePtr; i++)
                    {
                        ALOGV("%x", *(mConcurrencySet + i));
                    }
                }
            }
            break;
        case QCA_NL80211_VENDOR_SUBCMD_PACKET_FILTER:
            {
                struct nlattr *tb_vendor[
                        QCA_WLAN_VENDOR_ATTR_PACKET_FILTER_MAX + 1];
                nla_parse(tb_vendor, QCA_WLAN_VENDOR_ATTR_PACKET_FILTER_MAX,
                        (struct nlattr *)mVendorData,
                        mDataLen, NULL);

                if (!tb_vendor[QCA_WLAN_VENDOR_ATTR_PACKET_FILTER_VERSION])
                {
                    ALOGE("%s: QCA_WLAN_VENDOR_ATTR_PACKET_FILTER_VERSION"
                          " not found", __FUNCTION__);
                    return WIFI_ERROR_INVALID_ARGS;
                }
                filterVersion = nla_get_u32(
                       tb_vendor[QCA_WLAN_VENDOR_ATTR_PACKET_FILTER_VERSION]);
                ALOGV("Current version : %u", filterVersion);

                if (!tb_vendor[QCA_WLAN_VENDOR_ATTR_PACKET_FILTER_TOTAL_LENGTH])
                {
                    ALOGE("%s: QCA_WLAN_VENDOR_ATTR_PACKET_FILTER_TOTAL_LENGTH"
                          " not found", __FUNCTION__);
                    return WIFI_ERROR_INVALID_ARGS;
                }
                filterLength = nla_get_u32(
                    tb_vendor[QCA_WLAN_VENDOR_ATTR_PACKET_FILTER_TOTAL_LENGTH]);
                ALOGV("Max filter length Supported : %u", filterLength);

            }
            break;
        case QCA_NL80211_VENDOR_SUBCMD_GET_BUS_SIZE:
            {
                struct nlattr *tb_vendor[
                        QCA_WLAN_VENDOR_ATTR_DRV_INFO_MAX + 1];
                nla_parse(tb_vendor, QCA_WLAN_VENDOR_ATTR_DRV_INFO_MAX,
                          (struct nlattr *)mVendorData, mDataLen, NULL);

                if (!tb_vendor[QCA_WLAN_VENDOR_ATTR_DRV_INFO_BUS_SIZE])
                {
                    ALOGE("%s: QCA_WLAN_VENDOR_ATTR_DRV_INFO_BUS_SIZE"
                          " not found", __FUNCTION__);
                    return WIFI_ERROR_INVALID_ARGS;
                }
                firmware_bus_max_size = nla_get_u32(
                       tb_vendor[QCA_WLAN_VENDOR_ATTR_DRV_INFO_BUS_SIZE]);
                ALOGV("Max BUS size Supported: %d", firmware_bus_max_size);
            }
            break;
        default :
            ALOGE("%s: Wrong Wi-Fi HAL event received %d", __func__, mSubcmd);
    }
    return NL_SKIP;
}

void WifihalGeneric::getResponseparams(feature_set *pset)
{
    *pset = mSet;
}

void WifihalGeneric::setMaxSetSize(int set_size_max) {
    mSetSizeMax = set_size_max;
}

void WifihalGeneric::setConcurrencySet(feature_set set[]) {
    mConcurrencySet = set;
}

void WifihalGeneric::setSizePtr(int *set_size) {
    mSetSizePtr = set_size;
}

int WifihalGeneric::getFilterVersion() {
    return filterVersion;
}

int WifihalGeneric::getFilterLength() {
    return filterLength;
}

int WifihalGeneric::getBusSize() {
    return firmware_bus_max_size;
}
