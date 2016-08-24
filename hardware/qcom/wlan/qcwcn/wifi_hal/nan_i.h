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

#ifndef __NAN_I_H__
#define __NAN_I_H__

#include "common.h"
#include "cpp_bindings.h"
#include "wifi_hal.h"

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

#ifndef PACKED
#define PACKED  __attribute__((packed))
#endif
#define BIT_NONE            0x00
#define BIT_0               0x01
#define BIT_1               0x02
#define BIT_2               0x04
#define BIT_3               0x08
#define BIT_4               0x10
#define BIT_5               0x20
#define BIT_6               0x40
#define BIT_7               0x80
#define BIT_8               0x0100
#define BIT_9               0x0200
#define BIT_10              0x0400
#define BIT_11              0x0800
#define BIT_12              0x1000
#define BIT_13              0x2000
#define BIT_14              0x4000
#define BIT_15              0x8000
#define BIT_16              0x010000
#define BIT_17              0x020000
#define BIT_18              0x040000
#define BIT_19              0x080000
#define BIT_20              0x100000
#define BIT_21              0x200000
#define BIT_22              0x400000
#define BIT_23              0x800000
#define BIT_24              0x01000000
#define BIT_25              0x02000000
#define BIT_26              0x04000000
#define BIT_27              0x08000000
#define BIT_28              0x10000000
#define BIT_29              0x20000000
#define BIT_30              0x40000000
#define BIT_31              0x80000000
typedef u8 SirMacAddr[NAN_MAC_ADDR_LEN];
/*---------------------------------------------------------------------------
* WLAN NAN CONSTANTS
*--------------------------------------------------------------------------*/

typedef enum
{
    NAN_MSG_ID_ERROR_RSP                    = 0,
    NAN_MSG_ID_CONFIGURATION_REQ            = 1,
    NAN_MSG_ID_CONFIGURATION_RSP            = 2,
    NAN_MSG_ID_PUBLISH_SERVICE_REQ          = 3,
    NAN_MSG_ID_PUBLISH_SERVICE_RSP          = 4,
    NAN_MSG_ID_PUBLISH_SERVICE_CANCEL_REQ   = 5,
    NAN_MSG_ID_PUBLISH_SERVICE_CANCEL_RSP   = 6,
    NAN_MSG_ID_PUBLISH_REPLIED_IND          = 7,
    NAN_MSG_ID_PUBLISH_TERMINATED_IND       = 8,
    NAN_MSG_ID_SUBSCRIBE_SERVICE_REQ        = 9,
    NAN_MSG_ID_SUBSCRIBE_SERVICE_RSP        = 10,
    NAN_MSG_ID_SUBSCRIBE_SERVICE_CANCEL_REQ = 11,
    NAN_MSG_ID_SUBSCRIBE_SERVICE_CANCEL_RSP = 12,
    NAN_MSG_ID_MATCH_IND                    = 13,
    NAN_MSG_ID_MATCH_EXPIRED_IND            = 14,
    NAN_MSG_ID_SUBSCRIBE_TERMINATED_IND     = 15,
    NAN_MSG_ID_DE_EVENT_IND                 = 16,
    NAN_MSG_ID_TRANSMIT_FOLLOWUP_REQ        = 17,
    NAN_MSG_ID_TRANSMIT_FOLLOWUP_RSP        = 18,
    NAN_MSG_ID_FOLLOWUP_IND                 = 19,
    NAN_MSG_ID_STATS_REQ                    = 20,
    NAN_MSG_ID_STATS_RSP                    = 21,
    NAN_MSG_ID_ENABLE_REQ                   = 22,
    NAN_MSG_ID_ENABLE_RSP                   = 23,
    NAN_MSG_ID_DISABLE_REQ                  = 24,
    NAN_MSG_ID_DISABLE_RSP                  = 25,
    NAN_MSG_ID_DISABLE_IND                  = 26,
    NAN_MSG_ID_TCA_REQ                      = 27,
    NAN_MSG_ID_TCA_RSP                      = 28,
    NAN_MSG_ID_TCA_IND                      = 29,
    NAN_MSG_ID_BEACON_SDF_REQ               = 30,
    NAN_MSG_ID_BEACON_SDF_RSP               = 31,
    NAN_MSG_ID_BEACON_SDF_IND               = 32,
    NAN_MSG_ID_CAPABILITIES_REQ             = 33,
    NAN_MSG_ID_CAPABILITIES_RSP             = 34
} NanMsgId;

/*
  Various TLV Type ID sent as part of NAN Stats Response
  or NAN TCA Indication
*/
typedef enum
{
    NAN_TLV_TYPE_FIRST = 0,

    /* Service Discovery Frame types */
    NAN_TLV_TYPE_SDF_FIRST = NAN_TLV_TYPE_FIRST,
    NAN_TLV_TYPE_SERVICE_NAME = NAN_TLV_TYPE_SDF_FIRST,
    NAN_TLV_TYPE_SDF_MATCH_FILTER,
    NAN_TLV_TYPE_TX_MATCH_FILTER,
    NAN_TLV_TYPE_RX_MATCH_FILTER,
    NAN_TLV_TYPE_SERVICE_SPECIFIC_INFO,
    NAN_TLV_TYPE_EXT_SERVICE_SPECIFIC_INFO =5,
    NAN_TLV_TYPE_VENDOR_SPECIFIC_ATTRIBUTE_TRANSMIT = 6,
    NAN_TLV_TYPE_VENDOR_SPECIFIC_ATTRIBUTE_RECEIVE = 7,
    NAN_TLV_TYPE_POST_NAN_CONNECTIVITY_CAPABILITIES_RECEIVE = 8,
    NAN_TLV_TYPE_POST_NAN_DISCOVERY_ATTRIBUTE_RECEIVE = 9,
    NAN_TLV_TYPE_BEACON_SDF_PAYLOAD_RECEIVE = 10,
    NAN_TLV_TYPE_SDF_LAST = 4095,

    /* Configuration types */
    NAN_TLV_TYPE_CONFIG_FIRST = 4096,
    NAN_TLV_TYPE_24G_SUPPORT = NAN_TLV_TYPE_CONFIG_FIRST,
    NAN_TLV_TYPE_24G_BEACON,
    NAN_TLV_TYPE_24G_SDF,
    NAN_TLV_TYPE_24G_RSSI_CLOSE,
    NAN_TLV_TYPE_24G_RSSI_MIDDLE = 4100,
    NAN_TLV_TYPE_24G_RSSI_CLOSE_PROXIMITY,
    NAN_TLV_TYPE_5G_SUPPORT,
    NAN_TLV_TYPE_5G_BEACON,
    NAN_TLV_TYPE_5G_SDF,
    NAN_TLV_TYPE_5G_RSSI_CLOSE,
    NAN_TLV_TYPE_5G_RSSI_MIDDLE,
    NAN_TLV_TYPE_5G_RSSI_CLOSE_PROXIMITY,
    NAN_TLV_TYPE_SID_BEACON,
    NAN_TLV_TYPE_HOP_COUNT_LIMIT,
    NAN_TLV_TYPE_MASTER_PREFERENCE = 4110,
    NAN_TLV_TYPE_CLUSTER_ID_LOW,
    NAN_TLV_TYPE_CLUSTER_ID_HIGH,
    NAN_TLV_TYPE_RSSI_AVERAGING_WINDOW_SIZE,
    NAN_TLV_TYPE_CLUSTER_OUI_NETWORK_ID,
    NAN_TLV_TYPE_SOURCE_MAC_ADDRESS,
    NAN_TLV_TYPE_CLUSTER_ATTRIBUTE_IN_SDF,
    NAN_TLV_TYPE_SOCIAL_CHANNEL_SCAN_PARAMS,
    NAN_TLV_TYPE_DEBUGGING_FLAGS,
    NAN_TLV_TYPE_POST_NAN_CONNECTIVITY_CAPABILITIES_TRANSMIT,
    NAN_TLV_TYPE_POST_NAN_DISCOVERY_ATTRIBUTE_TRANSMIT = 4120,
    NAN_TLV_TYPE_FURTHER_AVAILABILITY_MAP,
    NAN_TLV_TYPE_HOP_COUNT_FORCE,
    NAN_TLV_TYPE_RANDOM_FACTOR_FORCE,
    NAN_TLV_TYPE_RANDOM_UPDATE_TIME = 4124,
    NAN_TLV_TYPE_EARLY_WAKEUP,
    NAN_TLV_TYPE_PERIODIC_SCAN_INTERVAL,
    NAN_TLV_TYPE_DW_INTERVAL = 4128,
    NAN_TLV_TYPE_DB_INTERVAL,
    NAN_TLV_TYPE_FURTHER_AVAILABILITY,
    NAN_TLV_TYPE_24G_CHANNEL,
    NAN_TLV_TYPE_5G_CHANNEL,
    NAN_TLV_TYPE_CONFIG_LAST = 8191,

    /* Attributes types */
    NAN_TLV_TYPE_ATTRS_FIRST = 8192,
    NAN_TLV_TYPE_AVAILABILITY_INTERVALS_MAP = NAN_TLV_TYPE_ATTRS_FIRST,
    NAN_TLV_TYPE_WLAN_MESH_ID,
    NAN_TLV_TYPE_MAC_ADDRESS,
    NAN_TLV_TYPE_RECEIVED_RSSI_VALUE,
    NAN_TLV_TYPE_CLUSTER_ATTRIBUTE,
    NAN_TLV_TYPE_WLAN_INFRA_SSID,
    NAN_TLV_TYPE_ATTRS_LAST = 12287,

    /* Events Type */
    NAN_TLV_TYPE_EVENTS_FIRST = 12288,
    NAN_TLV_TYPE_EVENT_SELF_STATION_MAC_ADDRESS = NAN_TLV_TYPE_EVENTS_FIRST,
    NAN_TLV_TYPE_EVENT_STARTED_CLUSTER,
    NAN_TLV_TYPE_EVENT_JOINED_CLUSTER,
    NAN_TLV_TYPE_EVENT_CLUSTER_SCAN_RESULTS,
    NAN_TLV_TYPE_FAW_MEM_AVAIL,
    NAN_TLV_TYPE_EVENTS_LAST = 16383,

    /* TCA types */
    NAN_TLV_TYPE_TCA_FIRST = 16384,
    NAN_TLV_TYPE_CLUSTER_SIZE_REQ = NAN_TLV_TYPE_TCA_FIRST,
    NAN_TLV_TYPE_CLUSTER_SIZE_RSP,
    NAN_TLV_TYPE_TCA_LAST = 32767,

    /* Statistics types */
    NAN_TLV_TYPE_STATS_FIRST = 32768,
    NAN_TLV_TYPE_DE_PUBLISH_STATS = NAN_TLV_TYPE_STATS_FIRST,
    NAN_TLV_TYPE_DE_SUBSCRIBE_STATS,
    NAN_TLV_TYPE_DE_MAC_STATS,
    NAN_TLV_TYPE_DE_TIMING_SYNC_STATS,
    NAN_TLV_TYPE_DE_DW_STATS,
    NAN_TLV_TYPE_DE_STATS,
    NAN_TLV_TYPE_STATS_LAST = 36863,

    NAN_TLV_TYPE_LAST = 65535
} NanTlvType;

/* 8-byte control message header used by NAN*/
typedef struct PACKED
{
   u16 msgVersion:4;
   u16 msgId:12;
   u16 msgLen;
   u16 handle;
   u16 transactionId;
} NanMsgHeader, *pNanMsgHeader;

/* Enumeration for Version */
typedef enum
{
   NAN_MSG_VERSION1 = 1,
}NanMsgVersion;

typedef struct PACKED
{
    u16 type;
    u16 length;
    u8* value;
} NanTlv, *pNanTlv;

#define SIZEOF_TLV_HDR (sizeof(NanTlv::type) + sizeof(NanTlv::length))
/* NAN TLV Groups and Types */
typedef enum
{
    NAN_TLV_GROUP_FIRST = 0,
    NAN_TLV_GROUP_SDF = NAN_TLV_GROUP_FIRST,
    NAN_TLV_GROUP_CONFIG,
    NAN_TLV_GROUP_STATS,
    NAN_TLV_GROUP_ATTRS,
    NAN_TLV_NUM_GROUPS,
    NAN_TLV_GROUP_LAST = NAN_TLV_NUM_GROUPS
} NanTlvGroup;

/* NAN Miscellaneous Constants */
#define NAN_TTL_INFINITE            0
#define NAN_REPLY_COUNT_INFINITE    0

/* NAN Confguration 5G Channel Access Bit */
#define NAN_5G_CHANNEL_ACCESS_UNSUPPORTED   0
#define NAN_5G_CHANNEL_ACCESS_SUPPORTED     1

/* NAN Configuration Service IDs Enclosure Bit */
#define NAN_SIDS_NOT_ENCLOSED_IN_BEACONS    0
#define NAN_SIBS_ENCLOSED_IN_BEACONS        1

/* NAN Configuration Priority */
#define NAN_CFG_PRIORITY_SERVICE_DISCOVERY  0
#define NAN_CFG_PRIORITY_DATA_CONNECTION    1

/* NAN Configuration 5G Channel Usage */
#define NAN_5G_CHANNEL_USAGE_SYNC_AND_DISCOVERY 0
#define NAN_5G_CHANNEL_USAGE_DISCOVERY_ONLY     1

/* NAN Configuration TX_Beacon Content */
#define NAN_TX_BEACON_CONTENT_OLD_AM_INFO       0
#define NAN_TX_BEACON_CONTENT_UPDATED_AM_INFO   1

/* NAN Configuration Miscellaneous Constants */
#define NAN_MAC_INTERFACE_PERIODICITY_MIN   30
#define NAN_MAC_INTERFACE_PERIODICITY_MAX   255

#define NAN_DW_RANDOM_TIME_MIN  120
#define NAN_DW_RANDOM_TIME_MAX  240

#define NAN_INITIAL_SCAN_MIN_IDEAL_PERIOD   200
#define NAN_INITIAL_SCAN_MAX_IDEAL_PERIOD   300

#define NAN_ONGOING_SCAN_MIN_PERIOD 10
#define NAN_ONGOING_SCAN_MAX_PERIOD 30

#define NAN_HOP_COUNT_LIMIT 5

#define NAN_WINDOW_DW   0
#define NAN_WINDOW_FAW  1

/* NAN Error Rsp */
typedef struct PACKED
{
    NanMsgHeader fwHeader;
    u16 status;
    u16 value;
} NanErrorRspMsg, *pNanErrorRspMsg;

//* NAN Publish Service Req */
typedef struct PACKED
{
    u16 ttl;
    u16 period;
    u32 reserved:1;
    u32 publishType:2;
    u32 txType:1;
    u32 rssiThresholdFlag:1;
    u32 ota_flag:1;
    u32 matchAlg:2;
    u32 count:8;
    u32 connmap:8;
    u32 pubTerminatedIndDisableFlag:1;
    u32 pubMatchExpiredIndDisableFlag:1;
    u32 followupRxIndDisableFlag:1;
    u32 reserved2:5;
    /*
     * Excludes TLVs
     *
     * Required: Service Name,
     * Optional: Tx Match Filter, Rx Match Filter, Service Specific Info,
     */
} NanPublishServiceReqParams, *pNanPublishServiceReqParams;

typedef struct PACKED
{
    NanMsgHeader fwHeader;
    NanPublishServiceReqParams publishServiceReqParams;
    u8 ptlv[];
} NanPublishServiceReqMsg, *pNanPublishServiceReqMsg;

/* NAN Publish Service Rsp */
typedef struct PACKED
{
    NanMsgHeader fwHeader;
    /* status of the request */
    u16 status;
    u16 value;
} NanPublishServiceRspMsg, *pNanPublishServiceRspMsg;

/* NAN Publish Service Cancel Req */
typedef struct PACKED
{
    NanMsgHeader fwHeader;
} NanPublishServiceCancelReqMsg, *pNanPublishServiceCancelReqMsg;

/* NAN Publish Service Cancel Rsp */
typedef struct PACKED
{
    NanMsgHeader fwHeader;
    /* status of the request */
    u16 status;
    u16 value;
} NanPublishServiceCancelRspMsg, *pNanPublishServiceCancelRspMsg;

/* NAN Publish Terminated Ind */
typedef struct PACKED
{
    NanMsgHeader fwHeader;
    /* reason for the termination */
    u16 reason;
    u16 reserved;
} NanPublishTerminatedIndMsg, *pNanPublishTerminatedIndMsg;

/* NAN Subscribe Service Req */
typedef struct PACKED
{
    u16 ttl;
    u16 period;
    u32 subscribeType:1;
    u32 srfAttr:1;
    u32 srfInclude:1;
    u32 srfSend:1;
    u32 ssiRequired:1;
    u32 matchAlg:2;
    u32 xbit:1;
    u32 count:8;
    u32 rssiThresholdFlag:1;
    u32 ota_flag:1;
    u32 subTerminatedIndDisableFlag:1;
    u32 subMatchExpiredIndDisableFlag:1;
    u32 followupRxIndDisableFlag:1;
    u32 reserved:3;
    u32 connmap:8;
    /*
     * Excludes TLVs
     *
     * Required: Service Name
     * Optional: Rx Match Filter, Tx Match Filter, Service Specific Info,
     */
} NanSubscribeServiceReqParams, *pNanSubscribeServiceReqParams;

typedef struct PACKED
{
    NanMsgHeader fwHeader;
    NanSubscribeServiceReqParams subscribeServiceReqParams;
    u8 ptlv[];
} NanSubscribeServiceReqMsg, *pNanSubscribeServiceReqMsg;

/* NAN Subscribe Service Rsp */
typedef struct PACKED
{
    NanMsgHeader fwHeader;
    /* status of the request */
    u16 status;
    u16 value;
} NanSubscribeServiceRspMsg, *pNanSubscribeServiceRspMsg;

/* NAN Subscribe Service Cancel Req */
typedef struct PACKED
{
    NanMsgHeader fwHeader;
} NanSubscribeServiceCancelReqMsg, *pNanSubscribeServiceCancelReqMsg;

/* NAN Subscribe Service Cancel Rsp */
typedef struct PACKED
{
    NanMsgHeader fwHeader;
    /* status of the request */
    u16 status;
    u16 value;
} NanSubscribeServiceCancelRspMsg, *pNanSubscribeServiceCancelRspMsg;

/* NAN Subscribe Match Ind */
typedef struct PACKED
{
    u32 matchHandle;
    u32 matchOccuredFlag:1;
    u32 outOfResourceFlag:1;
    u32 reserved:30;
} NanMatchIndParams;

typedef struct PACKED
{
    NanMsgHeader fwHeader;
    NanMatchIndParams matchIndParams;
    u8 ptlv[];
} NanMatchIndMsg, *pNanMatchIndMsg;

/* NAN Subscribe Unmatch Ind */
typedef struct PACKED
{
    u32 matchHandle;
} NanmatchExpiredIndParams;

typedef struct PACKED
{
    NanMsgHeader fwHeader;
    NanmatchExpiredIndParams matchExpiredIndParams;
} NanMatchExpiredIndMsg, *pNanMatchExpiredIndMsg;

/* NAN Subscribe Terminated Ind */
typedef struct PACKED
{
    NanMsgHeader fwHeader;
    /* reason for the termination */
    u16 reason;
    u16 reserved;
} NanSubscribeTerminatedIndMsg, *pNanSubscribeTerminatedIndMsg;

/* Event Ind */
typedef struct PACKED
{
    u32 eventId:8;
    u32 reserved:24;
} NanEventIndParams;

typedef struct PACKED
{
    NanMsgHeader fwHeader;
    u8 ptlv[];
} NanEventIndMsg, *pNanEventIndMsg;

/* NAN Transmit Followup Req */
typedef struct PACKED
{
    u32 matchHandle;
    u32 priority:4;
    u32 window:1;
    u32 followupTxRspDisableFlag:1;
    u32 reserved:26;
    /*
     * Excludes TLVs
     *
     * Required: Service Specific Info or Extended Service Specific Info
     */
} NanTransmitFollowupReqParams;

typedef struct PACKED
{
    NanMsgHeader fwHeader;
    NanTransmitFollowupReqParams transmitFollowupReqParams;
    u8 ptlv[];
} NanTransmitFollowupReqMsg, *pNanTransmitFollowupReqMsg;

/* NAN Transmit Followup Rsp */
typedef struct PACKED
{
    NanMsgHeader fwHeader;
    /* status of the request */
    u16 status;
    u16 value;
} NanTransmitFollowupRspMsg, *pNanTransmitFollowupRspMsg;

/* NAN Publish Followup Ind */
typedef struct PACKED
{
    u32 matchHandle;
    u32 window:1;
    u32 reserved:31;
    /*
     * Excludes TLVs
     *
     * Required: Service Specific Info or Extended Service Specific Info
     */
} NanFollowupIndParams;

typedef struct PACKED
{
    NanMsgHeader fwHeader;
    NanFollowupIndParams followupIndParams;
    u8 ptlv[];
} NanFollowupIndMsg, *pNanFollowupIndMsg;

/* NAN Statistics Req */
typedef struct PACKED
{
    u32 statsType:8;
    u32 clear:1;
    u32 reserved:23;
} NanStatsReqParams, *pNanStatsReqParams;

typedef struct PACKED
{
    NanMsgHeader fwHeader;
    NanStatsReqParams statsReqParams;
} NanStatsReqMsg, *pNanStatsReqMsg;

/* NAN Statistics Rsp */
typedef struct PACKED
{
    /* status of the request */
    u16 status;
    u16 value;
    u8 statsType;
    u8 reserved;
} NanStatsRspParams, *pNanStatsRspParams;

typedef struct PACKED
{
    NanMsgHeader fwHeader;
    NanStatsRspParams statsRspParams;
    u8 ptlv[];
} NanStatsRspMsg, *pNanStatsRspMsg;

typedef struct PACKED
{
    u8 count:7;
    u8 s:1;
} NanSidAttr, *pSidAttr;


/* NAN Configuration Req */
typedef struct PACKED
{
    NanMsgHeader fwHeader;
    /*
     * TLVs:
     *
     * Required: None.
     * Optional: SID, Random Time, Master Preference, WLAN Intra Attr,
     *           P2P Operation Attr, WLAN IBSS Attr, WLAN Mesh Attr
     */
    u8 ptlv[];
} NanConfigurationReqMsg, *pNanConfigurationReqMsg;

/*
 * Because the Configuration Req message has TLVs in it use the macro below
 * for the size argument to buffer allocation functions (vs. sizeof(msg)).
 */
#define NAN_MAX_CONFIGURATION_REQ_SIZE                       \
    (                                                        \
        sizeof(NanMsgHeader)                             +   \
        SIZEOF_TLV_HDR + sizeof(u8)  /* SID Beacon    */ +   \
        SIZEOF_TLV_HDR + sizeof(u8)  /* Random Time   */ +   \
        SIZEOF_TLV_HDR + sizeof(u8)  /* Master Pref   */     \
    )

/* NAN Configuration Rsp */
typedef struct PACKED
{
    NanMsgHeader fwHeader;
    /* status of the request */
    u16 status;
    u16 value;
} NanConfigurationRspMsg, *pNanConfigurationRspMsg;

/*
 * Because the Enable Req message has TLVs in it use the macro below for
 * the size argument to buffer allocation functions (vs. sizeof(msg)).
 */
#define NAN_MAX_ENABLE_REQ_SIZE                                 \
    (                                                           \
        sizeof(NanMsgHeader)                                +   \
        SIZEOF_TLV_HDR + sizeof(u16) /* Cluster Low   */    +   \
        SIZEOF_TLV_HDR + sizeof(u16) /* Cluster High  */    +   \
        SIZEOF_TLV_HDR + sizeof(u8)  /* Master Pref   */        \
    )

/* NAN Enable Req */
typedef struct PACKED
{
    NanMsgHeader fwHeader;
    /*
     * TLVs:
     *
     * Required: Cluster Low, Cluster High, Master Preference,
     * Optional: 5G Support, SID, 5G Sync Disc, RSSI Close, RSSI Medium,
     *           Hop Count Limit, Random Time, Master Preference,
     *           WLAN Intra Attr, P2P Operation Attr, WLAN IBSS Attr,
     *           WLAN Mesh Attr
     */
    u8 ptlv[];
} NanEnableReqMsg, *pNanEnableReqMsg;

/* NAN Enable Rsp */
typedef struct PACKED
{
    NanMsgHeader fwHeader;
    /* status of the request */
    u16 status;
    u16 value;
} NanEnableRspMsg, *pNanEnableRspMsg;

/* NAN Disable Req */
typedef struct PACKED
{
    NanMsgHeader fwHeader;
} NanDisableReqMsg, *pNanDisableReqMsg;

/* NAN Disable Rsp */
typedef struct PACKED
{
    NanMsgHeader fwHeader;
    /* status of the request */
    u16 status;
    u16 reserved;
} NanDisableRspMsg, *pNanDisableRspMsg;

/* NAN Disable Ind */
typedef struct PACKED
{
    NanMsgHeader fwHeader;
    /* reason for the termination */
    u16 reason;
    u16 reserved;
} NanDisableIndMsg, *pNanDisableIndMsg;

typedef struct PACKED
{
    NanMsgHeader fwHeader;
    u8 ptlv[];
} NanTcaReqMsg, *pNanTcaReqMsg;

/* NAN TCA Rsp */
typedef struct PACKED
{
    NanMsgHeader   fwHeader;
    /* status of the request */
    u16 status;
    u16 value;
} NanTcaRspMsg, *pNanTcaRspMsg;

typedef struct PACKED
{
    NanMsgHeader fwHeader;
    /*
     * TLVs:
     *
     * Optional: Cluster size.
     */
    u8 ptlv[];
} NanTcaIndMsg, *pNanTcaIndMsg;

/*
 * Because the TCA Ind message has TLVs in it use the macro below for the
 * size argument to buffer allocation functions (vs. sizeof(msg)).
 */
#define NAN_MAX_TCA_IND_SIZE                                 \
    (                                                        \
        sizeof(NanMsgHeader)                             +   \
        sizeof(NanTcaIndParams)                          +   \
        SIZEOF_TLV_HDR + sizeof(u16) /* Cluster Size */      \
    )

/* Function Declarations */
u8* addTlv(u16 type, u16 length, const u8* value, u8* pOutTlv);
u16 NANTLV_ReadTlv(u8 *pInTlv, pNanTlv pOutTlv);
u16 NANTLV_WriteTlv(pNanTlv pInTlv, u8 *pOutTlv);

/* NAN Beacon Sdf Payload Req */
typedef struct PACKED
{
    NanMsgHeader fwHeader;
    /*
     * TLVs:
     *
     * Optional: Vendor specific attribute
     */
    u8 ptlv[];
} NanBeaconSdfPayloadReqMsg, *pNanBeaconSdfPayloadReqMsg;

/* NAN Beacon Sdf Payload Rsp */
typedef struct PACKED
{
    NanMsgHeader   fwHeader;
    /* status of the request */
    u16 status;
    u16 reserved;
} NanBeaconSdfPayloadRspMsg, *pNanBeaconSdfPayloadRspMsg;

/* NAN Beacon Sdf Payload Ind */
typedef struct PACKED
{
    NanMsgHeader fwHeader;
    /*
     * TLVs:
     *
     * Required: Mac address
     * Optional: Vendor specific attribute, sdf payload
     * receive
     */
    u8 ptlv[];
} NanBeaconSdfPayloadIndMsg, *pNanBeaconSdfPayloadIndMsg;

typedef struct PACKED
{
    u8 availIntDuration:2;
    u8 mapId:4;
    u8 reserved:2;
} NanApiEntryCtrl;

/*
 * Valid Operating Classes were derived from IEEE Std. 802.11-2012 Annex E
 * Table E-4 Global Operating Classe and, filtered by channel, are: 81, 83,
 * 84, 103, 114, 115, 116, 124, 125.
 */
typedef struct PACKED
{
    NanApiEntryCtrl entryCtrl;
    u8 opClass;
    u8 channel;
    u8 availIntBitmap[4];
} NanFurtherAvailabilityChan, *pNanFurtherAvailabilityChan;

typedef struct PACKED
{
    u8 numChan;
    u8 pFaChan[];
} NanFurtherAvailabilityMapAttrTlv, *pNanFurtherAvailabilityMapAttrTlv;

/* Publish statistics. */
typedef struct PACKED
{
    u32 validPublishServiceReqMsgs;
    u32 validPublishServiceRspMsgs;
    u32 validPublishServiceCancelReqMsgs;
    u32 validPublishServiceCancelRspMsgs;
    u32 validPublishRepliedIndMsgs;
    u32 validPublishTerminatedIndMsgs;
    u32 validActiveSubscribes;
    u32 validMatches;
    u32 validFollowups;
    u32 invalidPublishServiceReqMsgs;
    u32 invalidPublishServiceCancelReqMsgs;
    u32 invalidActiveSubscribes;
    u32 invalidMatches;
    u32 invalidFollowups;
    u32 publishCount;
    u32 publishNewMatchCount;
    u32 pubsubGlobalNewMatchCount;
} FwNanPublishStats, *pFwNanPublishStats;

/* Subscribe statistics. */
typedef struct PACKED
{
    u32 validSubscribeServiceReqMsgs;
    u32 validSubscribeServiceRspMsgs;
    u32 validSubscribeServiceCancelReqMsgs;
    u32 validSubscribeServiceCancelRspMsgs;
    u32 validSubscribeTerminatedIndMsgs;
    u32 validSubscribeMatchIndMsgs;
    u32 validSubscribeUnmatchIndMsgs;
    u32 validSolicitedPublishes;
    u32 validMatches;
    u32 validFollowups;
    u32 invalidSubscribeServiceReqMsgs;
    u32 invalidSubscribeServiceCancelReqMsgs;
    u32 invalidSubscribeFollowupReqMsgs;
    u32 invalidSolicitedPublishes;
    u32 invalidMatches;
    u32 invalidFollowups;
    u32 subscribeCount;
    u32 bloomFilterIndex;
    u32 subscribeNewMatchCount;
    u32 pubsubGlobalNewMatchCount;
} FwNanSubscribeStats, *pFwNanSubscribeStats;

/* NAN MAC Statistics. Used for MAC and DW statistics. */
typedef struct PACKED
{
    /* RX stats */
    u32 validFrames;
    u32 validActionFrames;
    u32 validBeaconFrames;
    u32 ignoredActionFrames;
    u32 ignoredBeaconFrames;
    u32 invalidFrames;
    u32 invalidActionFrames;
    u32 invalidBeaconFrames;
    u32 invalidMacHeaders;
    u32 invalidPafHeaders;
    u32 nonNanBeaconFrames;

    u32 earlyActionFrames;
    u32 inDwActionFrames;
    u32 lateActionFrames;

    /* TX stats */
    u32 framesQueued;
    u32 totalTRSpUpdates;
    u32 completeByTRSp;
    u32 completeByTp75DW;
    u32 completeByTendDW;
    u32 lateActionFramesTx;

    /* Misc stats - ignored for DW. */
    u32 twIncreases;
    u32 twDecreases;
    u32 twChanges;
    u32 twHighwater;
    u32 bloomFilterIndex;
} FwNanMacStats, *pFwNanMacStats;

/* NAN Sync and DW Statistics*/
typedef struct PACKED
{
    u64 currTsf;
    u64 myRank;
    u64 currAmRank;
    u64 lastAmRank;
    u32 currAmBTT;
    u32 lastAmBTT;
    u8  currAmHopCount;
    u8  currRole;
    u16 currClusterId;
    u32 reserved1;

    u64 timeSpentInCurrRole;
    u64 totalTimeSpentAsMaster;
    u64 totalTimeSpentAsNonMasterSync;
    u64 totalTimeSpentAsNonMasterNonSync;
    u32 transitionsToAnchorMaster;
    u32 transitionsToMaster;
    u32 transitionsToNonMasterSync;
    u32 transitionsToNonMasterNonSync;
    u32 amrUpdateCount;
    u32 amrUpdateRankChangedCount;
    u32 amrUpdateBTTChangedCount;
    u32 amrUpdateHcChangedCount;
    u32 amrUpdateNewDeviceCount;
    u32 amrExpireCount;
    u32 mergeCount;
    u32 beaconsAboveHcLimit;
    u32 beaconsBelowRssiThresh;
    u32 beaconsIgnoredNoSpace;
    u32 beaconsForOurCluster;
    u32 beaconsForOtherCluster;
    u32 beaconCancelRequests;
    u32 beaconCancelFailures;
    u32 beaconUpdateRequests;
    u32 beaconUpdateFailures;
    u32 syncBeaconTxAttempts;
    u32 syncBeaconTxFailures;
    u32 discBeaconTxAttempts;
    u32 discBeaconTxFailures;
    u32 amHopCountExpireCount;
} FwNanSyncStats, *pFwNanSyncStats;

/* NAN Misc DE Statistics */
typedef struct PACKED
{
    u32 validErrorRspMsgs;
    u32 validTransmitFollowupReqMsgs;
    u32 validTransmitFollowupRspMsgs;
    u32 validFollowupIndMsgs;
    u32 validConfigurationReqMsgs;
    u32 validConfigurationRspMsgs;
    u32 validStatsReqMsgs;
    u32 validStatsRspMsgs;
    u32 validEnableReqMsgs;
    u32 validEnableRspMsgs;
    u32 validDisableReqMsgs;
    u32 validDisableRspMsgs;
    u32 validDisableIndMsgs;
    u32 validEventIndMsgs;
    u32 validTcaReqMsgs;
    u32 validTcaRspMsgs;
    u32 validTcaIndMsgs;
    u32 invalidTransmitFollowupReqMsgs;
    u32 invalidConfigurationReqMsgs;
    u32 invalidStatsReqMsgs;
    u32 invalidEnableReqMsgs;
    u32 invalidDisableReqMsgs;
    u32 invalidTcaReqMsgs;
} FwNanDeStats, *pFwNanDeStats;

/*
  Definition of various NanIndication(events)
*/
typedef enum {
    NAN_INDICATION_PUBLISH_TERMINATED      =1,
    NAN_INDICATION_MATCH                   =2,
    NAN_INDICATION_MATCH_EXPIRED           =3,
    NAN_INDICATION_SUBSCRIBE_TERMINATED    =4,
    NAN_INDICATION_DE_EVENT                =5,
    NAN_INDICATION_FOLLOWUP                =6,
    NAN_INDICATION_DISABLED                =7,
    NAN_INDICATION_TCA                     =8,
    NAN_INDICATION_BEACON_SDF_PAYLOAD      =9,
    NAN_INDICATION_UNKNOWN                 =0xFFFF
} NanIndicationType;

typedef struct {
  /* NAN master rank being advertised by DE */
  u64 master_rank;
  /* NAN master preference being advertised by DE */
  u8 master_pref;
  /* random value being advertised by DE */
  u8 random_factor;
  /* hop_count from anchor master */
  u8 hop_count;
  u32 beacon_transmit_time;
} NanStaParameter;

/* NAN Capabilities Req */
typedef struct PACKED
{
    NanMsgHeader fwHeader;
} NanCapabilitiesReqMsg, *pNanCapabilitiesReqMsg;

/* NAN Capabilities Rsp */
typedef struct PACKED
{
    NanMsgHeader fwHeader;
    /* status of the request */
    u32 status;
    u32 value;
    u32 max_concurrent_nan_clusters;
    u32 max_publishes;
    u32 max_subscribes;
    u32 max_service_name_len;
    u32 max_match_filter_len;
    u32 max_total_match_filter_len;
    u32 max_service_specific_info_len;
    u32 max_vsa_data_len;
    u32 max_mesh_data_len;
    u32 max_ndi_interfaces;
    u32 max_ndp_sessions;
    u32 max_app_info_len;
} NanCapabilitiesRspMsg, *pNanCapabilitiesRspMsg;

/*
    Function to get the sta_parameter expected by Sigma
    as per CAPI spec.
*/
wifi_error nan_get_sta_parameter(wifi_request_id id,
                                 wifi_interface_handle iface,
                                 NanStaParameter* msg);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __NAN_I_H__ */

