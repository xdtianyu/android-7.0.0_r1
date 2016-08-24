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

#ifndef __VENDOR_DEFINITIONS_H__
#define __VENDOR_DEFINITIONS_H__

#define WIFI_SCANNING_MAC_OUI_LENGTH 3
#define WIFI_MAC_ADDR_LENGTH 6

/*Internal to Android HAL component */
/* subcommands for link layer statistics start here */
#ifndef QCA_NL80211_VENDOR_SUBCMD_LL_STATS_SET
#define QCA_NL80211_VENDOR_SUBCMD_LL_STATS_SET 14
#define QCA_NL80211_VENDOR_SUBCMD_LL_STATS_GET 15
#define QCA_NL80211_VENDOR_SUBCMD_LL_STATS_CLR 16
/* subcommands for gscan start here */
#define QCA_NL80211_VENDOR_SUBCMD_GSCAN_START 20
#define QCA_NL80211_VENDOR_SUBCMD_GSCAN_STOP 21
#define QCA_NL80211_VENDOR_SUBCMD_GSCAN_GET_VALID_CHANNELS 22
#define QCA_NL80211_VENDOR_SUBCMD_GSCAN_GET_CAPABILITIES 23
#define QCA_NL80211_VENDOR_SUBCMD_GSCAN_GET_CACHED_RESULTS 24
/* Used when report_threshold is reached in scan cache. */
#define QCA_NL80211_VENDOR_SUBCMD_GSCAN_SCAN_RESULTS_AVAILABLE 25
/* Used to report scan results when each probe rsp. is received,
 * if report_events enabled in wifi_scan_cmd_params.
 */
#define QCA_NL80211_VENDOR_SUBCMD_GSCAN_FULL_SCAN_RESULT 26
/* Indicates progress of scanning state-machine. */
#define QCA_NL80211_VENDOR_SUBCMD_GSCAN_SCAN_EVENT 27
/* Indicates BSSID Hotlist. */
#define QCA_NL80211_VENDOR_SUBCMD_GSCAN_HOTLIST_AP_FOUND 28
#define QCA_NL80211_VENDOR_SUBCMD_GSCAN_SET_BSSID_HOTLIST 29
#define QCA_NL80211_VENDOR_SUBCMD_GSCAN_RESET_BSSID_HOTLIST 30
#define QCA_NL80211_VENDOR_SUBCMD_GSCAN_SIGNIFICANT_CHANGE 31
#define QCA_NL80211_VENDOR_SUBCMD_GSCAN_SET_SIGNIFICANT_CHANGE 32
#define QCA_NL80211_VENDOR_SUBCMD_GSCAN_RESET_SIGNIFICANT_CHANGE 33
/* TDLS Commands. */
#define QCA_NL80211_VENDOR_SUBCMD_TDLS_ENABLE 34
#define QCA_NL80211_VENDOR_SUBCMD_TDLS_DISABLE 35
#define QCA_NL80211_VENDOR_SUBCMD_TDLS_GET_STATUS 36
#define QCA_NL80211_VENDOR_SUBCMD_TDLS_STATE 37
/* Supported features. */
#define QCA_NL80211_VENDOR_SUBCMD_GET_SUPPORTED_FEATURES 38
/* set scanning_mac_oui */
#define QCA_NL80211_VENDOR_SUBCMD_SCANNING_MAC_OUI 39
/* Set nodfs_flag */
#define QCA_NL80211_VENDOR_SUBCMD_NO_DFS_FLAG 40
/* Indicates BSSID Hotlist AP lost. */
#define QCA_NL80211_VENDOR_SUBCMD_GSCAN_HOTLIST_AP_LOST 41
/* Get Concurrency Matrix */
#define QCA_NL80211_VENDOR_SUBCMD_GET_CONCURRENCY_MATRIX 42
/* Get Wifi Specific Info */
#define QCA_NL80211_VENDOR_SUBCMD_GET_WIFI_INFO 61
/* Start Wifi Logger */
#define QCA_NL80211_VENDOR_SUBCMD_WIFI_LOGGER_START 62
/* Start Wifi Memory Dump */
#define QCA_NL80211_VENDOR_SUBCMD_WIFI_LOGGER_MEMORY_DUMP 63
/* Roaming */
#define QCA_NL80211_VENDOR_SUBCMD_ROAM 64

/* APIs corresponding to the sub commands 65-68 are deprecated.
 * These sub commands are reserved and not supposed to be used
 * for any other purpose
 */
/* PNO */
#define QCA_NL80211_VENDOR_SUBCMD_PNO_SET_LIST 69
#define QCA_NL80211_VENDOR_SUBCMD_PNO_SET_PASSPOINT_LIST 70
#define QCA_NL80211_VENDOR_SUBCMD_PNO_RESET_PASSPOINT_LIST 71
#define QCA_NL80211_VENDOR_SUBCMD_PNO_NETWORK_FOUND 72
#define QCA_NL80211_VENDOR_SUBCMD_PNO_PASSPOINT_NETWORK_FOUND 73
/* Wi-Fi Configuration subcommands */
#define QCA_NL80211_VENDOR_SUBCMD_SET_WIFI_CONFIGURATION 74
#define QCA_NL80211_VENDOR_SUBCMD_GET_WIFI_CONFIGURATION 75
/* WiFiLogger Support feature set */
#define QCA_NL80211_VENDOR_SUBCMD_GET_LOGGER_FEATURE_SET 76
/* WiFiLogger Get Ring Data */
#define QCA_NL80211_VENDOR_SUBCMD_GET_RING_DATA 77
/* Get tdls capabilities */
#define QCA_NL80211_VENDOR_SUBCMD_TDLS_GET_CAPABILITIES 78
/* offloaded packets*/
#define QCA_NL80211_VENDOR_SUBCMD_OFFLOADED_PACKETS 79
/* RSSI monitoring*/
#define QCA_NL80211_VENDOR_SUBCMD_MONITOR_RSSI 80
/* Nan Data Path */
#define QCA_NL80211_VENDOR_SUBCMD_NDP 81
/* Neighbour Discovery offload */
#define QCA_NL80211_VENDOR_SUBCMD_ND_OFFLOAD 82
/* Set packet filter for BPF*/
#define QCA_NL80211_VENDOR_SUBCMD_PACKET_FILTER 83
/* Get Driver-firmware interface maximum supported size*/
#define QCA_NL80211_VENDOR_SUBCMD_GET_BUS_SIZE 84
/* Get wake reason stats */
#define QCA_NL80211_VENDOR_SUBCMD_GET_WAKE_REASON_STATS 85
#endif

enum qca_wlan_vendor_attr_ll_stats_set
{
    QCA_WLAN_VENDOR_ATTR_LL_STATS_SET_INVALID = 0,
    /* Unsigned 32-bit value */
    QCA_WLAN_VENDOR_ATTR_LL_STATS_SET_CONFIG_MPDU_SIZE_THRESHOLD = 1,
    QCA_WLAN_VENDOR_ATTR_LL_STATS_SET_CONFIG_AGGRESSIVE_STATS_GATHERING,
    /* keep last */
    QCA_WLAN_VENDOR_ATTR_LL_STATS_SET_AFTER_LAST,
    QCA_WLAN_VENDOR_ATTR_LL_STATS_SET_MAX =
        QCA_WLAN_VENDOR_ATTR_LL_STATS_SET_AFTER_LAST - 1,
};

enum qca_wlan_vendor_attr_ll_stats_clr
{
    QCA_WLAN_VENDOR_ATTR_LL_STATS_CLR_INVALID = 0,
    /* Unsigned 32bit bitmap  for clearing statistics
     * All radio statistics                     0x00000001
     * cca_busy_time (within radio statistics)  0x00000002
     * All channel stats (within radio statistics) 0x00000004
     * All scan statistics (within radio statistics) 0x00000008
     * All interface statistics                     0x00000010
     * All tx rate statistics (within interface statistics) 0x00000020
     * All ac statistics (with in interface statistics) 0x00000040
     * All contention (min, max, avg) statistics (within ac statisctics)
     * 0x00000080.
     */
    QCA_WLAN_VENDOR_ATTR_LL_STATS_CLR_CONFIG_REQ_MASK,
    /* Unsigned 8bit value : Request to stop statistics collection */
    QCA_WLAN_VENDOR_ATTR_LL_STATS_CLR_CONFIG_STOP_REQ,

    /* Unsigned 32bit bitmap : Response from the driver
     * for the cleared statistics
     */
    QCA_WLAN_VENDOR_ATTR_LL_STATS_CLR_CONFIG_RSP_MASK,
    /* Unsigned 8bit value: Response from driver/firmware
     * for the stop request
     */
    QCA_WLAN_VENDOR_ATTR_LL_STATS_CLR_CONFIG_STOP_RSP,
    /* keep last */
    QCA_WLAN_VENDOR_ATTR_LL_STATS_CLR_AFTER_LAST,
    QCA_WLAN_VENDOR_ATTR_LL_STATS_CLR_MAX =
        QCA_WLAN_VENDOR_ATTR_LL_STATS_CLR_AFTER_LAST - 1,
};

enum qca_wlan_vendor_attr_ll_stats_get
{
    QCA_WLAN_VENDOR_ATTR_LL_STATS_GET_INVALID = 0,
    /* Unsigned 32bit value provided by the caller issuing the GET stats
     * command. When reporting the stats results, the driver uses the same
     * value to indicate which GET request the results correspond to.
     */
    QCA_WLAN_VENDOR_ATTR_LL_STATS_GET_CONFIG_REQ_ID,
    /* Unsigned 32bit value - bit mask to identify what statistics are
     * requested for retrieval.
     * Radio Statistics 0x00000001
     * Interface Statistics 0x00000020
     * All Peer Statistics 0x00000040
     * Peer Statistics     0x00000080
     */
    QCA_WLAN_VENDOR_ATTR_LL_STATS_GET_CONFIG_REQ_MASK,
    /* keep last */
    QCA_WLAN_VENDOR_ATTR_LL_STATS_GET_AFTER_LAST,
    QCA_WLAN_VENDOR_ATTR_LL_STATS_GET_MAX =
        QCA_WLAN_VENDOR_ATTR_LL_STATS_GET_AFTER_LAST - 1,
};

enum qca_wlan_vendor_attr_ll_stats_results
{
    QCA_WLAN_VENDOR_ATTR_LL_STATS_INVALID = 0,
    /* Unsigned 32bit value. Used by the driver; must match the request id
     * provided with the QCA_NL80211_VENDOR_SUBCMD_LL_STATS_GET command.
     */
    QCA_WLAN_VENDOR_ATTR_LL_STATS_RESULTS_REQ_ID,

    /* Unsigned 32bit value */
    QCA_WLAN_VENDOR_ATTR_LL_STATS_IFACE_BEACON_RX,
    /* Unsigned 32bit value */
    QCA_WLAN_VENDOR_ATTR_LL_STATS_IFACE_MGMT_RX,
    /* Unsigned 32bit value */
    QCA_WLAN_VENDOR_ATTR_LL_STATS_IFACE_MGMT_ACTION_RX,
    /* Unsigned 32bit value */
    QCA_WLAN_VENDOR_ATTR_LL_STATS_IFACE_MGMT_ACTION_TX,
    /* Signed 32bit value */
    QCA_WLAN_VENDOR_ATTR_LL_STATS_IFACE_RSSI_MGMT,
    /* Signed 32bit value */
    QCA_WLAN_VENDOR_ATTR_LL_STATS_IFACE_RSSI_DATA,
    /* Signed 32bit value */
    QCA_WLAN_VENDOR_ATTR_LL_STATS_IFACE_RSSI_ACK,

    /* Attributes of type QCA_WLAN_VENDOR_ATTR_LL_STATS_IFACE_INFO_* are
     * nested within the interface stats.
     */

    /* Interface mode, e.g., STA, SOFTAP, IBSS, etc.
     * Type = enum wifi_interface_mode.
     */
    QCA_WLAN_VENDOR_ATTR_LL_STATS_IFACE_INFO_MODE,
    /* Interface MAC address. An array of 6 Unsigned int8 */
    QCA_WLAN_VENDOR_ATTR_LL_STATS_IFACE_INFO_MAC_ADDR,
    /* Type = enum wifi_connection_state, e.g., DISCONNECTED,
     * AUTHENTICATING, etc. valid for STA, CLI only.
     */
    QCA_WLAN_VENDOR_ATTR_LL_STATS_IFACE_INFO_STATE,
    /* Type = enum wifi_roam_state. Roaming state, e.g., IDLE or ACTIVE
     */
    QCA_WLAN_VENDOR_ATTR_LL_STATS_IFACE_INFO_ROAMING,
    /* Unsigned 32bit value. WIFI_CAPABILITY_XXX */
    QCA_WLAN_VENDOR_ATTR_LL_STATS_IFACE_INFO_CAPABILITIES,
    /* NULL terminated SSID. An array of 33 Unsigned 8bit values */
    QCA_WLAN_VENDOR_ATTR_LL_STATS_IFACE_INFO_SSID,
    /* BSSID. An array of 6 Unsigned 8bit values */
    QCA_WLAN_VENDOR_ATTR_LL_STATS_IFACE_INFO_BSSID,
    /* Country string advertised by AP. An array of 3 Unsigned 8bit
     * values.
     */
    QCA_WLAN_VENDOR_ATTR_LL_STATS_IFACE_INFO_AP_COUNTRY_STR,
    /* Country string for this association. An array of 3 Unsigned 8bit
     * values.
     */
    QCA_WLAN_VENDOR_ATTR_LL_STATS_IFACE_INFO_COUNTRY_STR,

    /* Attributes of type QCA_WLAN_VENDOR_ATTR_LL_STATS_WMM_AC_* could
     * be nested within the interface stats.
     */

    /* Type = enum wifi_traffic_ac, e.g., V0, VI, BE and BK */
    QCA_WLAN_VENDOR_ATTR_LL_STATS_WMM_AC_AC,
    /* Unsigned int 32 value corresponding to respective AC */
    QCA_WLAN_VENDOR_ATTR_LL_STATS_WMM_AC_TX_MPDU,
    /* Unsigned int 32 value corresponding to respective AC */
    QCA_WLAN_VENDOR_ATTR_LL_STATS_WMM_AC_RX_MPDU,
    /* Unsigned int 32 value corresponding to respective AC */
    QCA_WLAN_VENDOR_ATTR_LL_STATS_WMM_AC_TX_MCAST,
    /* Unsigned int 32 value corresponding to respective AC */
    QCA_WLAN_VENDOR_ATTR_LL_STATS_WMM_AC_RX_MCAST,
    /* Unsigned int 32 value corresponding to respective AC */
    QCA_WLAN_VENDOR_ATTR_LL_STATS_WMM_AC_RX_AMPDU,
    /* Unsigned int 32 value corresponding to respective AC */
    QCA_WLAN_VENDOR_ATTR_LL_STATS_WMM_AC_TX_AMPDU,
    /* Unsigned int 32 value corresponding to respective AC */
    QCA_WLAN_VENDOR_ATTR_LL_STATS_WMM_AC_MPDU_LOST,
    /* Unsigned int 32 value corresponding to respective AC */
    QCA_WLAN_VENDOR_ATTR_LL_STATS_WMM_AC_RETRIES,
    /* Unsigned int 32 value corresponding to respective AC  */
    QCA_WLAN_VENDOR_ATTR_LL_STATS_WMM_AC_RETRIES_SHORT,
    /* Unsigned int 32 values corresponding to respective AC */
    QCA_WLAN_VENDOR_ATTR_LL_STATS_WMM_AC_RETRIES_LONG,
    /* Unsigned int 32 values corresponding to respective AC */
    QCA_WLAN_VENDOR_ATTR_LL_STATS_WMM_AC_CONTENTION_TIME_MIN,
    /* Unsigned int 32 values corresponding to respective AC */
    QCA_WLAN_VENDOR_ATTR_LL_STATS_WMM_AC_CONTENTION_TIME_MAX,
    /* Unsigned int 32 values corresponding to respective AC */
    QCA_WLAN_VENDOR_ATTR_LL_STATS_WMM_AC_CONTENTION_TIME_AVG,
    /* Unsigned int 32 values corresponding to respective AC */
    QCA_WLAN_VENDOR_ATTR_LL_STATS_WMM_AC_CONTENTION_NUM_SAMPLES,
    /* Unsigned 32bit value. Number of peers */
    QCA_WLAN_VENDOR_ATTR_LL_STATS_IFACE_NUM_PEERS,

    /* Attributes of type QCA_WLAN_VENDOR_ATTR_LL_STATS_PEER_INFO_* are
     * nested within the interface stats.
     */

    /* Type = enum wifi_peer_type. Peer type, e.g., STA, AP, P2P GO etc. */
    QCA_WLAN_VENDOR_ATTR_LL_STATS_PEER_INFO_TYPE,
    /* MAC addr corresponding to respective peer. An array of 6 Unsigned
     * 8bit values.
     */
    QCA_WLAN_VENDOR_ATTR_LL_STATS_PEER_INFO_MAC_ADDRESS,
    /* Unsigned int 32bit value representing capabilities corresponding
     * to respective peer.
     */
    QCA_WLAN_VENDOR_ATTR_LL_STATS_PEER_INFO_CAPABILITIES,
    /* Unsigned 32bit value. Number of rates */
    QCA_WLAN_VENDOR_ATTR_LL_STATS_PEER_INFO_NUM_RATES,

    /* Attributes of type QCA_WLAN_VENDOR_ATTR_LL_STATS_RATE_*
     * are nested within the rate stat.
     */

    /* Wi-Fi Rate - separate attributes defined for individual fields */

    /* Unsigned int 8bit value; 0: OFDM, 1:CCK, 2:HT 3:VHT 4..7 reserved */
    QCA_WLAN_VENDOR_ATTR_LL_STATS_RATE_PREAMBLE,
    /* Unsigned int 8bit value; 0:1x1, 1:2x2, 3:3x3, 4:4x4 */
    QCA_WLAN_VENDOR_ATTR_LL_STATS_RATE_NSS,
    /* Unsigned int 8bit value; 0:20MHz, 1:40Mhz, 2:80Mhz, 3:160Mhz */
    QCA_WLAN_VENDOR_ATTR_LL_STATS_RATE_BW,
    /* Unsigned int 8bit value; OFDM/CCK rate code would be as per IEEE Std
     * in the units of 0.5mbps HT/VHT it would be mcs index */
    QCA_WLAN_VENDOR_ATTR_LL_STATS_RATE_MCS_INDEX,

    /* Unsigned 32bit value. Bit rate in units of 100Kbps */
    QCA_WLAN_VENDOR_ATTR_LL_STATS_RATE_BIT_RATE,


    /* Attributes of type QCA_WLAN_VENDOR_ATTR_LL_STATS_RATE_STAT_* could be
     * nested within the peer info stats.
     */

    /* Unsigned int 32bit value. Number of successfully transmitted data pkts,
     * i.e., with ACK received corresponding to the respective rate.
     */
    QCA_WLAN_VENDOR_ATTR_LL_STATS_RATE_TX_MPDU,
    /* Unsigned int 32bit value. Number of received data pkts corresponding
     *  to the respective rate.
     */
    QCA_WLAN_VENDOR_ATTR_LL_STATS_RATE_RX_MPDU,
    /* Unsigned int 32bit value. Number of data pkts losses, i.e., no ACK
     * received corresponding to *the respective rate.
     */
    QCA_WLAN_VENDOR_ATTR_LL_STATS_RATE_MPDU_LOST,
    /* Unsigned int 32bit value. Total number of data pkt retries for the
     * respective rate.
     */
    QCA_WLAN_VENDOR_ATTR_LL_STATS_RATE_RETRIES,
    /* Unsigned int 32bit value. Total number of short data pkt retries
     * for the respective rate.
     */
    QCA_WLAN_VENDOR_ATTR_LL_STATS_RATE_RETRIES_SHORT,
    /* Unsigned int 32bit value. Total number of long data pkt retries
     * for the respective rate.
     */
    QCA_WLAN_VENDOR_ATTR_LL_STATS_RATE_RETRIES_LONG,


    QCA_WLAN_VENDOR_ATTR_LL_STATS_RADIO_ID,
    /* Unsigned 32bit value. Total number of msecs the radio is awake
     * accruing over time.
     */
    QCA_WLAN_VENDOR_ATTR_LL_STATS_RADIO_ON_TIME,
    /* Unsigned 32bit value. Total number of msecs the radio is transmitting
     * accruing over time.
     */
    QCA_WLAN_VENDOR_ATTR_LL_STATS_RADIO_TX_TIME,
    /* Unsigned 32bit value. Total number of msecs the radio is in active
     * receive accruing over time.
     */
    QCA_WLAN_VENDOR_ATTR_LL_STATS_RADIO_RX_TIME,
    /* Unsigned 32bit value. Total number of msecs the radio is awake due
     * to all scan accruing over time.
     */
    QCA_WLAN_VENDOR_ATTR_LL_STATS_RADIO_ON_TIME_SCAN,
    /* Unsigned 32bit value. Total number of msecs the radio is awake due
     * to NAN accruing over time.
     */
    QCA_WLAN_VENDOR_ATTR_LL_STATS_RADIO_ON_TIME_NBD,
    /* Unsigned 32bit value. Total number of msecs the radio is awake due
     * to GSCAN accruing over time.
     */
    QCA_WLAN_VENDOR_ATTR_LL_STATS_RADIO_ON_TIME_GSCAN,
    /* Unsigned 32bit value. Total number of msecs the radio is awake due
     * to roam scan accruing over time.
     */
    QCA_WLAN_VENDOR_ATTR_LL_STATS_RADIO_ON_TIME_ROAM_SCAN,
    /* Unsigned 32bit value. Total number of msecs the radio is awake due
     * to PNO scan accruing over time.
     */
    QCA_WLAN_VENDOR_ATTR_LL_STATS_RADIO_ON_TIME_PNO_SCAN,
    /* Unsigned 32bit value. Total number of msecs the radio is awake due
     * to HS2.0 scans and GAS *exchange accruing over time.
     */
    QCA_WLAN_VENDOR_ATTR_LL_STATS_RADIO_ON_TIME_HS20,
    /* Unsigned 32bit value. Number of channels. */
    QCA_WLAN_VENDOR_ATTR_LL_STATS_RADIO_NUM_CHANNELS,

    /* Attributes of type QCA_WLAN_VENDOR_ATTR_LL_STATS_CHANNEL_INFO_* could
     * be nested within the channel stats.
     */

    /* Type = enum wifi_channel_width. Channel width, e.g., 20, 40, 80 */
    QCA_WLAN_VENDOR_ATTR_LL_STATS_CHANNEL_INFO_WIDTH,
    /* Unsigned 32bit value. Primary 20MHz channel. */
    QCA_WLAN_VENDOR_ATTR_LL_STATS_CHANNEL_INFO_CENTER_FREQ,
    /* Unsigned 32bit value. Center frequency (MHz) first segment. */
    QCA_WLAN_VENDOR_ATTR_LL_STATS_CHANNEL_INFO_CENTER_FREQ0,
    /* Unsigned 32bit value. Center frequency (MHz) second segment. */
    QCA_WLAN_VENDOR_ATTR_LL_STATS_CHANNEL_INFO_CENTER_FREQ1,

    /* Attributes of type QCA_WLAN_VENDOR_ATTR_LL_STATS_CHANNEL_* could be
     * nested within the radio stats.
     */

    /* Unsigned int 32bit value representing total number of msecs the radio
     * is awake on that *channel accruing over time, corresponding to the
     * respective channel.
     */
    QCA_WLAN_VENDOR_ATTR_LL_STATS_CHANNEL_ON_TIME,
    /* Unsigned int 32bit value representing total number of msecs the CCA
     * register is busy accruing  *over time corresponding to the respective
     * channel.
     */
    QCA_WLAN_VENDOR_ATTR_LL_STATS_CHANNEL_CCA_BUSY_TIME,

    QCA_WLAN_VENDOR_ATTR_LL_STATS_NUM_RADIOS,

    /* Signifies the nested list of channel attributes
     * QCA_WLAN_VENDOR_ATTR_LL_STATS_CHANNEL_INFO_*
     */
    QCA_WLAN_VENDOR_ATTR_LL_STATS_CH_INFO,

    /* Signifies the nested list of peer info attributes
     * QCA_WLAN_VENDOR_ATTR_LL_STATS_PEER_INFO_*
     */
    QCA_WLAN_VENDOR_ATTR_LL_STATS_PEER_INFO,

    /* Signifies the nested list of rate info attributes
     * QCA_WLAN_VENDOR_ATTR_LL_STATS_RATE_*
     */
    QCA_WLAN_VENDOR_ATTR_LL_STATS_PEER_INFO_RATE_INFO,

    /* Signifies the nested list of wmm info attributes
     * QCA_WLAN_VENDOR_ATTR_LL_STATS_WMM_AC_*
     */
    QCA_WLAN_VENDOR_ATTR_LL_STATS_WMM_INFO,

    /* Unsigned 8bit value. Used by the driver; if set to 1, it indicates that
     * more stats, e.g., peers or radio, are to follow in the next
     * QCA_NL80211_VENDOR_SUBCMD_LL_STATS_*_RESULTS event.
     * Otherwise, it is set to 0.
     */
    QCA_WLAN_VENDOR_ATTR_LL_STATS_RESULTS_MORE_DATA,

    /* Unsigned 64bit value */
    QCA_WLAN_VENDOR_ATTR_LL_STATS_IFACE_AVERAGE_TSF_OFFSET,

    /* Unsigned 32bit value */
    QCA_WLAN_VENDOR_ATTR_LL_STATS_IFACE_LEAKY_AP_DETECTED,

    /* Unsigned 32bit value */
    QCA_WLAN_VENDOR_ATTR_LL_STATS_IFACE_LEAKY_AP_AVG_NUM_FRAMES_LEAKED,

    /* Unsigned 32bit value */
    QCA_WLAN_VENDOR_ATTR_LL_STATS_IFACE_LEAKY_AP_GUARD_TIME,

    /* Unsigned 32bit value */
    QCA_WLAN_VENDOR_ATTR_LL_STATS_TYPE,

    /* Unsigned 32bit value */
    QCA_WLAN_VENDOR_ATTR_LL_STATS_RADIO_NUM_TX_LEVELS,

    /* Number of msecs the radio spent in transmitting for each power level */
    QCA_WLAN_VENDOR_ATTR_LL_STATS_RADIO_TX_TIME_PER_LEVEL,

    /* keep last */
    QCA_WLAN_VENDOR_ATTR_LL_STATS_AFTER_LAST,
    QCA_WLAN_VENDOR_ATTR_LL_STATS_MAX = QCA_WLAN_VENDOR_ATTR_LL_STATS_AFTER_LAST - 1,
};

enum qca_wlan_vendor_attr_ll_stats_type
{
    QCA_NL80211_VENDOR_SUBCMD_LL_STATS_TYPE_INVALID = 0,
    QCA_NL80211_VENDOR_SUBCMD_LL_STATS_TYPE_RADIO,
    QCA_NL80211_VENDOR_SUBCMD_LL_STATS_TYPE_IFACE,
    QCA_NL80211_VENDOR_SUBCMD_LL_STATS_TYPE_PEERS
};

enum qca_wlan_vendor_attr_gscan_config_params
{
    QCA_WLAN_VENDOR_ATTR_GSCAN_SUBCMD_CONFIG_PARAM_INVALID = 0,

    /* Unsigned 32-bit value; Middleware provides it to the driver. Middle ware
     * either gets it from caller, e.g., framework, or generates one if
     * framework doesn't provide it.
     */
    QCA_WLAN_VENDOR_ATTR_GSCAN_SUBCMD_CONFIG_PARAM_REQUEST_ID,

    /* NL attributes for data used by
     * QCA_NL80211_VENDOR_SUBCMD_GSCAN_GET_VALID_CHANNELS sub command.
     */
    /* Unsigned 32-bit value */
    QCA_WLAN_VENDOR_ATTR_GSCAN_GET_VALID_CHANNELS_CONFIG_PARAM_WIFI_BAND,
    /* Unsigned 32-bit value */
    QCA_WLAN_VENDOR_ATTR_GSCAN_GET_VALID_CHANNELS_CONFIG_PARAM_MAX_CHANNELS,

    /* NL attributes for input params used by
     * QCA_NL80211_VENDOR_SUBCMD_GSCAN_START sub command.
     */

    /* Unsigned 32-bit value; channel frequency */
    QCA_WLAN_VENDOR_ATTR_GSCAN_CHANNEL_SPEC_CHANNEL,
    /* Unsigned 32-bit value; dwell time in ms. */
    QCA_WLAN_VENDOR_ATTR_GSCAN_CHANNEL_SPEC_DWELL_TIME,
    /* Unsigned 8-bit value; 0: active; 1: passive; N/A for DFS */
    QCA_WLAN_VENDOR_ATTR_GSCAN_CHANNEL_SPEC_PASSIVE,
    /* Unsigned 8-bit value; channel class */
    QCA_WLAN_VENDOR_ATTR_GSCAN_CHANNEL_SPEC_CLASS,

    /* Unsigned 8-bit value; bucket index, 0 based */
    QCA_WLAN_VENDOR_ATTR_GSCAN_BUCKET_SPEC_INDEX,
    /* Unsigned 8-bit value; band. */
    QCA_WLAN_VENDOR_ATTR_GSCAN_BUCKET_SPEC_BAND,
    /* Unsigned 32-bit value; desired period, in ms. */
    QCA_WLAN_VENDOR_ATTR_GSCAN_BUCKET_SPEC_PERIOD,
    /* Unsigned 8-bit value; report events semantics. */
    QCA_WLAN_VENDOR_ATTR_GSCAN_BUCKET_SPEC_REPORT_EVENTS,
    /* Unsigned 32-bit value. Followed by a nested array of GSCAN_CHANNEL_SPEC_*
     * attributes.
     */
    QCA_WLAN_VENDOR_ATTR_GSCAN_BUCKET_SPEC_NUM_CHANNEL_SPECS,

    /* Array of QCA_WLAN_VENDOR_ATTR_GSCAN_CHANNEL_SPEC_* attributes.
     * Array size: QCA_WLAN_VENDOR_ATTR_GSCAN_BUCKET_SPEC_NUM_CHANNEL_SPECS
     */
    QCA_WLAN_VENDOR_ATTR_GSCAN_CHANNEL_SPEC,

    /* Unsigned 32-bit value; base timer period in ms. */
    QCA_WLAN_VENDOR_ATTR_GSCAN_SCAN_CMD_PARAMS_BASE_PERIOD,
    /* Unsigned 32-bit value; number of APs to store in each scan in the
     * BSSID/RSSI history buffer (keep the highest RSSI APs).
     */
    QCA_WLAN_VENDOR_ATTR_GSCAN_SCAN_CMD_PARAMS_MAX_AP_PER_SCAN,
    /* Unsigned 8-bit value; in %, when scan buffer is this much full, wake up
     * AP.
     */
    QCA_WLAN_VENDOR_ATTR_GSCAN_SCAN_CMD_PARAMS_REPORT_THRESHOLD_PERCENT,

    /* Unsigned 8-bit value; number of scan bucket specs; followed by a nested
     * array of_GSCAN_BUCKET_SPEC_* attributes and values. The size of the
     * array is determined by NUM_BUCKETS.
     */
    QCA_WLAN_VENDOR_ATTR_GSCAN_SCAN_CMD_PARAMS_NUM_BUCKETS,

    /* Array of QCA_WLAN_VENDOR_ATTR_GSCAN_BUCKET_SPEC_* attributes.
     * Array size: QCA_WLAN_VENDOR_ATTR_GSCAN_SCAN_CMD_PARAMS_NUM_BUCKETS
     */
    QCA_WLAN_VENDOR_ATTR_GSCAN_BUCKET_SPEC,

    /* Unsigned 8-bit value */
    QCA_WLAN_VENDOR_ATTR_GSCAN_GET_CACHED_SCAN_RESULTS_CONFIG_PARAM_FLUSH,
    /* Unsigned 32-bit value; maximum number of results to be returned. */
    QCA_WLAN_VENDOR_ATTR_GSCAN_GET_CACHED_SCAN_RESULTS_CONFIG_PARAM_MAX,

    /* An array of 6 x Unsigned 8-bit value */
    QCA_WLAN_VENDOR_ATTR_GSCAN_AP_THRESHOLD_PARAM_BSSID,
    /* Signed 32-bit value */
    QCA_WLAN_VENDOR_ATTR_GSCAN_AP_THRESHOLD_PARAM_RSSI_LOW,
    /* Signed 32-bit value */
    QCA_WLAN_VENDOR_ATTR_GSCAN_AP_THRESHOLD_PARAM_RSSI_HIGH,
    /* Unsigned 32-bit value */
    QCA_WLAN_VENDOR_ATTR_GSCAN_AP_THRESHOLD_PARAM_CHANNEL,


    /* Number of hotlist APs as unsigned 32-bit value, followed by a nested array of
     * AP_THRESHOLD_PARAM attributes and values. The size of the array is
     * determined by NUM_AP.
     */
    QCA_WLAN_VENDOR_ATTR_GSCAN_BSSID_HOTLIST_PARAMS_NUM_AP,

    /* Array of QCA_WLAN_VENDOR_ATTR_GSCAN_AP_THRESHOLD_PARAM_* attributes.
     * Array size: QCA_WLAN_VENDOR_ATTR_GSCAN_BUCKET_SPEC_NUM_CHANNEL_SPECS
     */
    QCA_WLAN_VENDOR_ATTR_GSCAN_AP_THRESHOLD_PARAM,

    /* Unsigned 32bit value; number of samples for averaging RSSI. */
    QCA_WLAN_VENDOR_ATTR_GSCAN_SIGNIFICANT_CHANGE_PARAMS_RSSI_SAMPLE_SIZE,
    /* Unsigned 32bit value; number of samples to confirm AP loss. */
    QCA_WLAN_VENDOR_ATTR_GSCAN_SIGNIFICANT_CHANGE_PARAMS_LOST_AP_SAMPLE_SIZE,
    /* Unsigned 32bit value; number of APs breaching threshold. */
    QCA_WLAN_VENDOR_ATTR_GSCAN_SIGNIFICANT_CHANGE_PARAMS_MIN_BREACHING,
    /* Unsigned 32bit value; number of APs. Followed by an array of
     * AP_THRESHOLD_PARAM attributes. Size of the array is NUM_AP.
     */
    QCA_WLAN_VENDOR_ATTR_GSCAN_SIGNIFICANT_CHANGE_PARAMS_NUM_AP,
    /* Unsigned 32bit value; number of samples to confirm AP loss. */
    QCA_WLAN_VENDOR_ATTR_GSCAN_BSSID_HOTLIST_PARAMS_LOST_AP_SAMPLE_SIZE,
    /* Unsigned 32-bit value. If max_period is non zero or different than
     * period, then this bucket is an exponential backoff bucket.
     */
    QCA_WLAN_VENDOR_ATTR_GSCAN_BUCKET_SPEC_MAX_PERIOD,
    /* Unsigned 32-bit value. */
    QCA_WLAN_VENDOR_ATTR_GSCAN_BUCKET_SPEC_BASE,
    /* Unsigned 32-bit value. For exponential back off bucket, number of scans
     * to perform for a given period.
     */
    QCA_WLAN_VENDOR_ATTR_GSCAN_BUCKET_SPEC_STEP_COUNT,
    /* Unsigned 8-bit value; in number of scans, wake up AP after these
     * many scans.
     */
    QCA_WLAN_VENDOR_ATTR_GSCAN_SCAN_CMD_PARAMS_REPORT_THRESHOLD_NUM_SCANS,

    /* NL attributes for data used by
     * QCA_NL80211_VENDOR_SUBCMD_GSCAN_SET_SSID_HOTLIST sub command.
     */
    /* Unsigned 32bit value; number of samples to confirm SSID loss. */
    QCA_WLAN_VENDOR_ATTR_GSCAN_SSID_HOTLIST_PARAMS_LOST_SSID_SAMPLE_SIZE,
    /* Number of hotlist SSIDs as unsigned 32-bit value, followed by a nested
     * array of SSID_THRESHOLD_PARAM_* attributes and values. The size of the
     * array is determined by NUM_SSID.
     */
    QCA_WLAN_VENDOR_ATTR_GSCAN_SSID_HOTLIST_PARAMS_NUM_SSID,
    /* Array of QCA_WLAN_VENDOR_ATTR_GSCAN_SSID_THRESHOLD_PARAM_* attributes.
     * Array size: QCA_WLAN_VENDOR_ATTR_GSCAN_SSID_HOTLIST_PARAMS_NUM_SSID
     */
    QCA_WLAN_VENDOR_ATTR_GSCAN_SSID_THRESHOLD_PARAM,

    /* An array of 33 x Unsigned 8-bit value; NULL terminated SSID */
    QCA_WLAN_VENDOR_ATTR_GSCAN_SSID_THRESHOLD_PARAM_SSID,
    /* Unsigned 8-bit value */
    QCA_WLAN_VENDOR_ATTR_GSCAN_SSID_THRESHOLD_PARAM_BAND,
    /* Signed 32-bit value */
    QCA_WLAN_VENDOR_ATTR_GSCAN_SSID_THRESHOLD_PARAM_RSSI_LOW,
    /* Signed 32-bit value */
    QCA_WLAN_VENDOR_ATTR_GSCAN_SSID_THRESHOLD_PARAM_RSSI_HIGH,
    /* Unsigned 32-bit value; a bitmask w/additional gscan config flag. */
    QCA_WLAN_VENDOR_ATTR_GSCAN_CONFIGURATION_FLAGS,

    /* keep last */
    QCA_WLAN_VENDOR_ATTR_GSCAN_SUBCMD_CONFIG_PARAM_AFTER_LAST,
    QCA_WLAN_VENDOR_ATTR_GSCAN_SUBCMD_CONFIG_PARAM_MAX =
        QCA_WLAN_VENDOR_ATTR_GSCAN_SUBCMD_CONFIG_PARAM_AFTER_LAST - 1,
};

enum qca_wlan_vendor_attr_gscan_results
{
    QCA_WLAN_VENDOR_ATTR_GSCAN_RESULTS_INVALID = 0,

    /* Unsigned 32-bit value; must match the request Id supplied by Wi-Fi HAL
     * in the corresponding subcmd NL msg
     */
    QCA_WLAN_VENDOR_ATTR_GSCAN_RESULTS_REQUEST_ID,

    /* Unsigned 32-bit value; used to indicate the status response from
     * firmware/driver for the vendor sub-command.
     */
    QCA_WLAN_VENDOR_ATTR_GSCAN_STATUS,

    /* GSCAN Valid Channels attributes */
    /* Unsigned 32bit value; followed by a nested array of CHANNELS.
     */
    QCA_WLAN_VENDOR_ATTR_GSCAN_RESULTS_NUM_CHANNELS,
    /* An array of NUM_CHANNELS x Unsigned 32bit value integers representing
     * channel numbers
     */
    QCA_WLAN_VENDOR_ATTR_GSCAN_RESULTS_CHANNELS,

    /* GSCAN Capabilities attributes */
    /* Unsigned 32bit value */
    QCA_WLAN_VENDOR_ATTR_GSCAN_RESULTS_CAPABILITIES_MAX_SCAN_CACHE_SIZE,
    /* Unsigned 32bit value */
    QCA_WLAN_VENDOR_ATTR_GSCAN_RESULTS_CAPABILITIES_MAX_SCAN_BUCKETS,
    /* Unsigned 32bit value */
    QCA_WLAN_VENDOR_ATTR_GSCAN_RESULTS_CAPABILITIES_MAX_AP_CACHE_PER_SCAN,
    /* Unsigned 32bit value */
    QCA_WLAN_VENDOR_ATTR_GSCAN_RESULTS_CAPABILITIES_MAX_RSSI_SAMPLE_SIZE,
    /* Signed 32bit value */
    QCA_WLAN_VENDOR_ATTR_GSCAN_RESULTS_CAPABILITIES_MAX_SCAN_REPORTING_THRESHOLD,
    /* Unsigned 32bit value */
    QCA_WLAN_VENDOR_ATTR_GSCAN_RESULTS_CAPABILITIES_MAX_HOTLIST_BSSIDS,
    /* Unsigned 32bit value */
    QCA_WLAN_VENDOR_ATTR_GSCAN_RESULTS_CAPABILITIES_MAX_SIGNIFICANT_WIFI_CHANGE_APS,
    /* Unsigned 32bit value */
    QCA_WLAN_VENDOR_ATTR_GSCAN_RESULTS_CAPABILITIES_MAX_BSSID_HISTORY_ENTRIES,

    /* GSCAN Attributes used with
     * QCA_NL80211_VENDOR_SUBCMD_GSCAN_SCAN_RESULTS_AVAILABLE sub-command.
     */

    /* Unsigned 32-bit value */
    QCA_WLAN_VENDOR_ATTR_GSCAN_RESULTS_NUM_RESULTS_AVAILABLE,

    /* GSCAN attributes used with
     * QCA_NL80211_VENDOR_SUBCMD_GSCAN_FULL_SCAN_RESULT sub-command.
     */

    /* An array of NUM_RESULTS_AVAILABLE x
     * QCA_WLAN_VENDOR_ATTR_GSCAN_RESULTS_SCAN_RESULT_*
     */
    QCA_WLAN_VENDOR_ATTR_GSCAN_RESULTS_LIST,

    /* Unsigned 64-bit value; age of sample at the time of retrieval */
    QCA_WLAN_VENDOR_ATTR_GSCAN_RESULTS_SCAN_RESULT_TIME_STAMP,
    /* 33 x unsiged 8-bit value; NULL terminated SSID */
    QCA_WLAN_VENDOR_ATTR_GSCAN_RESULTS_SCAN_RESULT_SSID,
    /* An array of 6 x Unsigned 8-bit value */
    QCA_WLAN_VENDOR_ATTR_GSCAN_RESULTS_SCAN_RESULT_BSSID,
    /* Unsigned 32-bit value; channel frequency in MHz */
    QCA_WLAN_VENDOR_ATTR_GSCAN_RESULTS_SCAN_RESULT_CHANNEL,
    /* Signed 32-bit value */
    QCA_WLAN_VENDOR_ATTR_GSCAN_RESULTS_SCAN_RESULT_RSSI,
    /* Unsigned 32-bit value */
    QCA_WLAN_VENDOR_ATTR_GSCAN_RESULTS_SCAN_RESULT_RTT,
    /* Unsigned 32-bit value */
    QCA_WLAN_VENDOR_ATTR_GSCAN_RESULTS_SCAN_RESULT_RTT_SD,
    /* Unsigned 16-bit value */
    QCA_WLAN_VENDOR_ATTR_GSCAN_RESULTS_SCAN_RESULT_BEACON_PERIOD,
    /* Unsigned 16-bit value */
    QCA_WLAN_VENDOR_ATTR_GSCAN_RESULTS_SCAN_RESULT_CAPABILITY,
    /* Unsigned 32-bit value; size of the IE DATA blob */
    QCA_WLAN_VENDOR_ATTR_GSCAN_RESULTS_SCAN_RESULT_IE_LENGTH,
    /* An array of IE_LENGTH x Unsigned 8-bit value; blob of all the
     * information elements found in the beacon; this data should be a
     * packed list of wifi_information_element objects, one after the
     * other.
     */
    QCA_WLAN_VENDOR_ATTR_GSCAN_RESULTS_SCAN_RESULT_IE_DATA,

    /* Unsigned 8-bit value; set by driver to indicate more scan results are
     * available.
     */
    QCA_WLAN_VENDOR_ATTR_GSCAN_RESULTS_SCAN_RESULT_MORE_DATA,

    /* GSCAN attributes for
     * QCA_NL80211_VENDOR_SUBCMD_GSCAN_SCAN_EVENT sub-command.
     */
    /* Unsigned 8-bit value */
    QCA_WLAN_VENDOR_ATTR_GSCAN_RESULTS_SCAN_EVENT_TYPE,
    /* Unsigned 32-bit value */
    QCA_WLAN_VENDOR_ATTR_GSCAN_RESULTS_SCAN_EVENT_STATUS,

    /* GSCAN attributes for
     * QCA_NL80211_VENDOR_SUBCMD_GSCAN_HOTLIST_AP_FOUND sub-command.
     */
    /* Use attr QCA_WLAN_VENDOR_ATTR_GSCAN_RESULTS_NUM_RESULTS_AVAILABLE
     * to indicate number of results.
     * Also, use QCA_WLAN_VENDOR_ATTR_GSCAN_RESULTS_LIST to indicate the list
     * of results.
     */

    /* GSCAN attributes for
     * QCA_NL80211_VENDOR_SUBCMD_GSCAN_SIGNIFICANT_CHANGE sub-command.
     */
    /* An array of 6 x Unsigned 8-bit value */
    QCA_WLAN_VENDOR_ATTR_GSCAN_RESULTS_SIGNIFICANT_CHANGE_RESULT_BSSID,
    /* Unsigned 32-bit value */
    QCA_WLAN_VENDOR_ATTR_GSCAN_RESULTS_SIGNIFICANT_CHANGE_RESULT_CHANNEL,
    /* Unsigned 32-bit value.
     */
    QCA_WLAN_VENDOR_ATTR_GSCAN_RESULTS_SIGNIFICANT_CHANGE_RESULT_NUM_RSSI,
    /* A nested array of signed 32-bit RSSI values. Size of the array is determined by
     * (NUM_RSSI of SIGNIFICANT_CHANGE_RESULT_NUM_RSSI.
     */
    QCA_WLAN_VENDOR_ATTR_GSCAN_RESULTS_SIGNIFICANT_CHANGE_RESULT_RSSI_LIST,

    /* GSCAN attributes used with
     * QCA_NL80211_VENDOR_SUBCMD_GSCAN_GET_CACHED_RESULTS sub-command.
     */
    /* Use attr QCA_WLAN_VENDOR_ATTR_GSCAN_RESULTS_NUM_RESULTS_AVAILABLE
     * to indicate number of gscan cached results returned.
     * Also, use QCA_WLAN_VENDOR_ATTR_GSCAN_CACHED_RESULTS_LIST to indicate
     *  the list of gscan cached results.
     */

    /* An array of NUM_RESULTS_AVAILABLE x
     * QCA_NL80211_VENDOR_ATTR_GSCAN_CACHED_RESULTS_*
     */
    QCA_WLAN_VENDOR_ATTR_GSCAN_CACHED_RESULTS_LIST,
    /* Unsigned 32-bit value; a unique identifier for the scan unit. */
    QCA_WLAN_VENDOR_ATTR_GSCAN_CACHED_RESULTS_SCAN_ID,
    /* Unsigned 32-bit value; a bitmask w/additional information about scan. */
    QCA_WLAN_VENDOR_ATTR_GSCAN_CACHED_RESULTS_FLAGS,
    /* Use attr QCA_WLAN_VENDOR_ATTR_GSCAN_RESULTS_NUM_RESULTS_AVAILABLE
     * to indicate number of wifi scan results/bssids retrieved by the scan.
     * Also, use QCA_WLAN_VENDOR_ATTR_GSCAN_RESULTS_LIST to indicate the list
     * of wifi scan results returned for each cached result block.
     */

    /* GSCAN attributes for
     * QCA_NL80211_VENDOR_SUBCMD_PNO_NETWORK_FOUND sub-command.
     */
    /* Use QCA_WLAN_VENDOR_ATTR_GSCAN_RESULTS_NUM_RESULTS_AVAILABLE for number
     * of results.
     * Use QCA_WLAN_VENDOR_ATTR_GSCAN_RESULTS_LIST to indicate the nested
     * list of wifi scan results returned for each wifi_passpoint_match_result block.
     * Array size: QCA_WLAN_VENDOR_ATTR_GSCAN_RESULTS_NUM_RESULTS_AVAILABLE.
     */

    /* GSCAN attributes for
     * QCA_NL80211_VENDOR_SUBCMD_PNO_PASSPOINT_NETWORK_FOUND sub-command.
     */
    /* Unsigned 32-bit value */
    QCA_WLAN_VENDOR_ATTR_GSCAN_PNO_RESULTS_PASSPOINT_NETWORK_FOUND_NUM_MATCHES,
    /* A nested array of
     * QCA_WLAN_VENDOR_ATTR_GSCAN_PNO_RESULTS_PASSPOINT_MATCH_*
     * attributes. Array size =
     * *_ATTR_GSCAN_PNO_RESULTS_PASSPOINT_NETWORK_FOUND_NUM_MATCHES.
     */
    QCA_WLAN_VENDOR_ATTR_GSCAN_PNO_RESULTS_PASSPOINT_MATCH_RESULT_LIST,

    /* Unsigned 32-bit value; network block id for the matched network */
    QCA_WLAN_VENDOR_ATTR_GSCAN_PNO_RESULTS_PASSPOINT_MATCH_ID,
    /* Use QCA_WLAN_VENDOR_ATTR_GSCAN_RESULTS_LIST to indicate the nested
     * list of wifi scan results returned for each wifi_passpoint_match_result block.
     */
    /* Unsigned 32-bit value */
    QCA_WLAN_VENDOR_ATTR_GSCAN_PNO_RESULTS_PASSPOINT_MATCH_ANQP_LEN,
    /* An array size of PASSPOINT_MATCH_ANQP_LEN of unsigned 8-bit values;
     * ANQP data in the information_element format.
     */
    QCA_WLAN_VENDOR_ATTR_GSCAN_PNO_RESULTS_PASSPOINT_MATCH_ANQP,

    /* Unsigned 32bit value; a GSCAN Capabilities attribute. */
    QCA_WLAN_VENDOR_ATTR_GSCAN_RESULTS_CAPABILITIES_MAX_HOTLIST_SSIDS,
    /* Unsigned 32bit value; a GSCAN Capabilities attribute. */
    QCA_WLAN_VENDOR_ATTR_GSCAN_RESULTS_CAPABILITIES_MAX_NUM_EPNO_NETS,
    /* Unsigned 32bit value; a GSCAN Capabilities attribute. */
    QCA_WLAN_VENDOR_ATTR_GSCAN_RESULTS_CAPABILITIES_MAX_NUM_EPNO_NETS_BY_SSID,
    /* Unsigned 32bit value; a GSCAN Capabilities attribute. */
    QCA_WLAN_VENDOR_ATTR_GSCAN_RESULTS_CAPABILITIES_MAX_NUM_WHITELISTED_SSID,

    QCA_WLAN_VENDOR_ATTR_GSCAN_RESULTS_BUCKETS_SCANNED,
    /* keep last */
    QCA_WLAN_VENDOR_ATTR_GSCAN_RESULTS_AFTER_LAST,
    QCA_WLAN_VENDOR_ATTR_GSCAN_RESULTS_MAX =
        QCA_WLAN_VENDOR_ATTR_GSCAN_RESULTS_AFTER_LAST - 1,
};

enum qca_wlan_vendor_attr_tdls_enable
{
    QCA_WLAN_VENDOR_ATTR_TDLS_ENABLE_INVALID = 0,
    /* An array of 6 x Unsigned 8-bit value */
    QCA_WLAN_VENDOR_ATTR_TDLS_ENABLE_MAC_ADDR,
    QCA_WLAN_VENDOR_ATTR_TDLS_ENABLE_CHANNEL,
    QCA_WLAN_VENDOR_ATTR_TDLS_ENABLE_GLOBAL_OPERATING_CLASS,
    QCA_WLAN_VENDOR_ATTR_TDLS_ENABLE_MAX_LATENCY_MS,
    QCA_WLAN_VENDOR_ATTR_TDLS_ENABLE_MIN_BANDWIDTH_KBPS,
    /* keep last */
    QCA_WLAN_VENDOR_ATTR_TDLS_ENABLE_AFTER_LAST,
    QCA_WLAN_VENDOR_ATTR_TDLS_ENABLE_MAX =
        QCA_WLAN_VENDOR_ATTR_TDLS_ENABLE_AFTER_LAST - 1,
};

enum qca_wlan_vendor_attr_tdls_disable
{
    QCA_WLAN_VENDOR_ATTR_TDLS_DISABLE_INVALID = 0,
    /* An array of 6 x Unsigned 8-bit value */
    QCA_WLAN_VENDOR_ATTR_TDLS_DISABLE_MAC_ADDR,
    /* keep last */
    QCA_WLAN_VENDOR_ATTR_TDLS_DISABLE_AFTER_LAST,
    QCA_WLAN_VENDOR_ATTR_TDLS_DISABLE_MAX =
        QCA_WLAN_VENDOR_ATTR_TDLS_DISABLE_AFTER_LAST - 1,
};

enum qca_wlan_vendor_attr_tdls_get_status
{
    QCA_WLAN_VENDOR_ATTR_TDLS_GET_STATUS_INVALID = 0,
    /* An array of 6 x Unsigned 8-bit value */
    QCA_WLAN_VENDOR_ATTR_TDLS_GET_STATUS_MAC_ADDR,
    QCA_WLAN_VENDOR_ATTR_TDLS_GET_STATUS_STATE,
    QCA_WLAN_VENDOR_ATTR_TDLS_GET_STATUS_REASON,
    QCA_WLAN_VENDOR_ATTR_TDLS_GET_STATUS_CHANNEL,
    QCA_WLAN_VENDOR_ATTR_TDLS_GET_STATUS_GLOBAL_OPERATING_CLASS,
    /* keep last */
    QCA_WLAN_VENDOR_ATTR_TDLS_GET_STATUS_AFTER_LAST,
    QCA_WLAN_VENDOR_ATTR_TDLS_GET_STATUS_MAX =
        QCA_WLAN_VENDOR_ATTR_TDLS_GET_STATUS_AFTER_LAST - 1,
};

enum qca_wlan_vendor_attr_tdls_state
{
    QCA_WLAN_VENDOR_ATTR_TDLS_STATE_INVALID = 0,
    /* An array of 6 x Unsigned 8-bit value */
    QCA_WLAN_VENDOR_ATTR_TDLS_MAC_ADDR,
    QCA_WLAN_VENDOR_ATTR_TDLS_STATE,
    QCA_WLAN_VENDOR_ATTR_TDLS_REASON,
    QCA_WLAN_VENDOR_ATTR_TDLS_CHANNEL,
    QCA_WLAN_VENDOR_ATTR_TDLS_GLOBAL_OPERATING_CLASS,
    /* keep last */
    QCA_WLAN_VENDOR_ATTR_TDLS_STATE_AFTER_LAST,
    QCA_WLAN_VENDOR_ATTR_TDLS_STATE_MAX =
        QCA_WLAN_VENDOR_ATTR_TDLS_STATE_AFTER_LAST - 1,
};

enum qca_wlan_vendor_attr_tdls_get_capabilities
{
    QCA_WLAN_VENDOR_ATTR_TDLS_GET_CAPS_INVALID = 0,

    QCA_WLAN_VENDOR_ATTR_TDLS_GET_CAPS_MAX_CONC_SESSIONS,
    QCA_WLAN_VENDOR_ATTR_TDLS_GET_CAPS_FEATURES_SUPPORTED,

    /* keep last */
    QCA_WLAN_VENDOR_ATTR_TDLS_GET_CAPS_AFTER_LAST,
    QCA_WLAN_VENDOR_ATTR_TDLS_GET_CAPS_MAX =
        QCA_WLAN_VENDOR_ATTR_TDLS_GET_CAPS_AFTER_LAST - 1,
};

enum qca_wlan_vendor_attr_get_supported_features
{
    QCA_WLAN_VENDOR_ATTR_FEATURE_SET_INVALID = 0,
    /* Unsigned 32-bit value */
    QCA_WLAN_VENDOR_ATTR_FEATURE_SET = 1,
    /* keep last */
    QCA_WLAN_VENDOR_ATTR_FEATURE_SET_AFTER_LAST,
    QCA_WLAN_VENDOR_ATTR_FEATURE_SET_MAX =
        QCA_WLAN_VENDOR_ATTR_FEATURE_SET_AFTER_LAST - 1,
};

enum qca_wlan_vendor_attr_set_scanning_mac_oui
{
    QCA_WLAN_VENDOR_ATTR_SET_SCANNING_MAC_OUI_INVALID = 0,
    /* An array of 3 x Unsigned 8-bit value */
    QCA_WLAN_VENDOR_ATTR_SET_SCANNING_MAC_OUI = 1,
    /* keep last */
    QCA_WLAN_VENDOR_ATTR_SET_SCANNING_MAC_OUI_AFTER_LAST,
    QCA_WLAN_VENDOR_ATTR_SET_SCANNING_MAC_OUI_MAX =
        QCA_WLAN_VENDOR_ATTR_SET_SCANNING_MAC_OUI_AFTER_LAST - 1,
};

enum qca_wlan_vendor_attr_set_no_dfs_flag
{
    QCA_WLAN_VENDOR_ATTR_SET_NO_DFS_FLAG_INVALID = 0,
    /* Unsigned 32-bit value */
    QCA_WLAN_VENDOR_ATTR_SET_NO_DFS_FLAG = 1,
    /* keep last */
    QCA_WLAN_VENDOR_ATTR_SET_NO_DFS_FLAG_AFTER_LAST,
    QCA_WLAN_VENDOR_ATTR_SET_NO_DFS_FLAG_MAX =
        QCA_WLAN_VENDOR_ATTR_SET_NO_DFS_FLAG_AFTER_LAST - 1,
};

/* NL attributes for data used by
 * QCA_NL80211_VENDOR_SUBCMD_GET_CONCURRENCY_MATRIX sub command.
 */
enum qca_wlan_vendor_attr_get_concurrency_matrix
{
    QCA_WLAN_VENDOR_ATTR_GET_CONCURRENCY_MATRIX_INVALID = 0,
    /* Unsigned 32-bit value */
    QCA_WLAN_VENDOR_ATTR_GET_CONCURRENCY_MATRIX_CONFIG_PARAM_SET_SIZE_MAX = 1,
    /* Unsigned 32-bit value */
    QCA_WLAN_VENDOR_ATTR_GET_CONCURRENCY_MATRIX_RESULTS_SET_SIZE = 2,
    /* An array of SET_SIZE x Unsigned 32bit values representing
     * concurrency combinations.
     */
    QCA_WLAN_VENDOR_ATTR_GET_CONCURRENCY_MATRIX_RESULTS_SET = 3,
    /* keep last */
    QCA_WLAN_VENDOR_ATTR_GET_CONCURRENCY_MATRIX_AFTER_LAST,
    QCA_WLAN_VENDOR_ATTR_GET_CONCURRENCY_MATRIX_MAX =
        QCA_WLAN_VENDOR_ATTR_GET_CONCURRENCY_MATRIX_AFTER_LAST - 1,
};

/* NL attributes for data used by
 * QCA_NL80211_VENDOR_SUBCMD_SET|GET_WIFI_CONFIGURATION sub commands.
 */
enum qca_wlan_vendor_attr_wifi_config {
    QCA_WLAN_VENDOR_ATTR_WIFI_CONFIG_INVALID = 0,
    /* Unsigned 32-bit value */
    QCA_WLAN_VENDOR_ATTR_WIFI_CONFIG_DYNAMIC_DTIM = 1,
    /* Unsigned 32-bit value */
    QCA_WLAN_VENDOR_ATTR_WIFI_CONFIG_STATS_AVG_FACTOR = 2,
    /* Unsigned 32-bit value */
    QCA_WLAN_VENDOR_ATTR_WIFI_CONFIG_GUARD_TIME = 3,
    /* Unsigned 32-bit value */
    QCA_WLAN_VENDOR_ATTR_WIFI_CONFIG_FINE_TIME_MEASUREMENT = 4,
    /* Unsigned 32-bit value */
    QCA_WLAN_VENDOR_ATTR_WIFI_CONFIG_TX_RATE = 5,
    /* Unsigned 32-bit value */
    QCA_WLAN_VENDOR_ATTR_WIFI_CONFIG_PENALIZE_AFTER_NCONS_BEACON_MISS = 6,
    /* keep last */
    QCA_WLAN_VENDOR_ATTR_WIFI_CONFIG_AFTER_LAST,
    QCA_WLAN_VENDOR_ATTR_WIFI_CONFIG_MAX =
        QCA_WLAN_VENDOR_ATTR_WIFI_CONFIG_AFTER_LAST - 1,
};

enum qca_wlan_epno_type
{
    QCA_WLAN_EPNO,
    QCA_WLAN_PNO
};

#define EPNO_NO_NETWORKS 0

enum qca_wlan_vendor_attr_pno_config_params {
    QCA_WLAN_VENDOR_ATTR_PNO_INVALID = 0,
    /* NL attributes for data used by
     * QCA_NL80211_VENDOR_SUBCMD_PNO_SET_PASSPOINT_LIST sub command.
     */
    /* Unsigned 32-bit value */
    QCA_WLAN_VENDOR_ATTR_PNO_PASSPOINT_LIST_PARAM_NUM = 1,
    /* Array of nested QCA_WLAN_VENDOR_ATTR_PNO_PASSPOINT_NETWORK_PARAM_*
     * attributes. Array size =
     * QCA_WLAN_VENDOR_ATTR_PNO_PASSPOINT_LIST_PARAM_NUM.
     */
    QCA_WLAN_VENDOR_ATTR_PNO_PASSPOINT_LIST_PARAM_NETWORK_ARRAY = 2,

    /* Unsigned 32-bit value */
    QCA_WLAN_VENDOR_ATTR_PNO_PASSPOINT_NETWORK_PARAM_ID = 3,
    /* An array of 256 x Unsigned 8-bit value; NULL terminated UTF8 encoded
     * realm, 0 if unspecified.
     */
    QCA_WLAN_VENDOR_ATTR_PNO_PASSPOINT_NETWORK_PARAM_REALM = 4,
    /* An array of 16 x Unsigned 32-bit value; roaming consortium ids to match,
     * 0 if unspecified.
     */
    QCA_WLAN_VENDOR_ATTR_PNO_PASSPOINT_NETWORK_PARAM_ROAM_CNSRTM_ID = 5,
    /* An array of 6 x Unsigned 8-bit value; mcc/mnc combination, 0s if
    *  unspecified.
    */
    QCA_WLAN_VENDOR_ATTR_PNO_PASSPOINT_NETWORK_PARAM_ROAM_PLMN = 6,


    /* NL attributes for data used by
     * QCA_NL80211_VENDOR_SUBCMD_PNO_SET_LIST sub command.
     */
    /* Unsigned 32-bit value */
    QCA_WLAN_VENDOR_ATTR_PNO_SET_LIST_PARAM_NUM_NETWORKS = 7,
    /* Array of nested
     * QCA_WLAN_VENDOR_ATTR_PNO_SET_LIST_PARAM_EPNO_NETWORK_*
     * attributes. Array size =
     *            QCA_WLAN_VENDOR_ATTR_PNO_SET_LIST_PARAM_NUM_NETWORKS.
     */
    QCA_WLAN_VENDOR_ATTR_PNO_SET_LIST_PARAM_EPNO_NETWORKS_LIST = 8,
    /* An array of 33 x Unsigned 8-bit value; NULL terminated SSID */
    QCA_WLAN_VENDOR_ATTR_PNO_SET_LIST_PARAM_EPNO_NETWORK_SSID = 9,
    /* Signed 8-bit value; threshold for considering this SSID as found,
     * required granularity for this threshold is 4dBm to 8dBm
     */
    QCA_WLAN_VENDOR_ATTR_PNO_SET_LIST_PARAM_EPNO_NETWORK_RSSI_THRESHOLD = 10,
    /* Unsigned 8-bit value; WIFI_PNO_FLAG_XXX */
    QCA_WLAN_VENDOR_ATTR_PNO_SET_LIST_PARAM_EPNO_NETWORK_FLAGS = 11,
    /* Unsigned 8-bit value; auth bit field for matching WPA IE */
    QCA_WLAN_VENDOR_ATTR_PNO_SET_LIST_PARAM_EPNO_NETWORK_AUTH_BIT = 12,
    /* Unsigned 8-bit to indicate ePNO type;
     * It takes values from qca_wlan_epno_type
     */
    QCA_WLAN_VENDOR_ATTR_PNO_SET_LIST_PARAM_EPNO_TYPE = 13,

    /* Nested attribute to send the channel list */
    QCA_WLAN_VENDOR_ATTR_PNO_SET_LIST_PARAM_EPNO_CHANNEL_LIST = 14,

    /* Unsigned 32-bit value; indicates the Interval between PNO scan
     * cycles in msec
     */
    QCA_WLAN_VENDOR_ATTR_PNO_SET_LIST_PARAM_EPNO_SCAN_INTERVAL = 15,
    QCA_WLAN_VENDOR_ATTR_EPNO_MIN5GHZ_RSSI = 16,
    QCA_WLAN_VENDOR_ATTR_EPNO_MIN24GHZ_RSSI = 17,
    QCA_WLAN_VENDOR_ATTR_EPNO_INITIAL_SCORE_MAX = 18,
    QCA_WLAN_VENDOR_ATTR_EPNO_CURRENT_CONNECTION_BONUS = 19,
    QCA_WLAN_VENDOR_ATTR_EPNO_SAME_NETWORK_BONUS = 20,
    QCA_WLAN_VENDOR_ATTR_EPNO_SECURE_BONUS = 21,
    QCA_WLAN_VENDOR_ATTR_EPNO_BAND5GHZ_BONUS = 22,

    /* keep last */
    QCA_WLAN_VENDOR_ATTR_PNO_AFTER_LAST,
    QCA_WLAN_VENDOR_ATTR_PNO_MAX =
        QCA_WLAN_VENDOR_ATTR_PNO_AFTER_LAST - 1,
};

enum qca_wlan_vendor_attr_roaming_config_params {
    QCA_WLAN_VENDOR_ATTR_ROAMING_PARAM_INVALID = 0,

    QCA_WLAN_VENDOR_ATTR_ROAMING_SUBCMD = 1,
    QCA_WLAN_VENDOR_ATTR_ROAMING_REQ_ID = 2,

    /* Attributes for wifi_set_ssid_white_list */
    QCA_WLAN_VENDOR_ATTR_ROAMING_PARAM_WHITE_LIST_SSID_NUM_NETWORKS = 3,
    QCA_WLAN_VENDOR_ATTR_ROAMING_PARAM_WHITE_LIST_SSID_LIST = 4,
    QCA_WLAN_VENDOR_ATTR_ROAMING_PARAM_WHITE_LIST_SSID = 5,

    /* Attributes for set_roam_params */
    QCA_WLAN_VENDOR_ATTR_ROAMING_PARAM_A_BAND_BOOST_THRESHOLD = 6,
    QCA_WLAN_VENDOR_ATTR_ROAMING_PARAM_A_BAND_PENALTY_THRESHOLD = 7,
    QCA_WLAN_VENDOR_ATTR_ROAMING_PARAM_A_BAND_BOOST_FACTOR = 8,
    QCA_WLAN_VENDOR_ATTR_ROAMING_PARAM_A_BAND_PENALTY_FACTOR = 9,
    QCA_WLAN_VENDOR_ATTR_ROAMING_PARAM_A_BAND_MAX_BOOST = 10,
    QCA_WLAN_VENDOR_ATTR_ROAMING_PARAM_LAZY_ROAM_HISTERESYS = 11,
    QCA_WLAN_VENDOR_ATTR_ROAMING_PARAM_ALERT_ROAM_RSSI_TRIGGER = 12,

    /* Attribute for set_lazy_roam*/
    QCA_WLAN_VENDOR_ATTR_ROAMING_PARAM_SET_LAZY_ROAM_ENABLE = 13,

    /* Attribute for set_lazy_roam with preferences*/
    QCA_WLAN_VENDOR_ATTR_ROAMING_PARAM_SET_BSSID_PREFS = 14,
    QCA_WLAN_VENDOR_ATTR_ROAMING_PARAM_SET_LAZY_ROAM_NUM_BSSID = 15,
    QCA_WLAN_VENDOR_ATTR_ROAMING_PARAM_SET_LAZY_ROAM_BSSID = 16,
    QCA_WLAN_VENDOR_ATTR_ROAMING_PARAM_SET_LAZY_ROAM_RSSI_MODIFIER = 17,

    /* Attribute for set_ blacklist bssid params */
    QCA_WLAN_VENDOR_ATTR_ROAMING_PARAM_SET_BSSID_PARAMS = 18,
    QCA_WLAN_VENDOR_ATTR_ROAMING_PARAM_SET_BSSID_PARAMS_NUM_BSSID = 19,
    QCA_WLAN_VENDOR_ATTR_ROAMING_PARAM_SET_BSSID_PARAMS_BSSID = 20,

    /* keep last */
    QCA_WLAN_VENDOR_ATTR_ROAMING_PARAM_AFTER_LAST,
    QCA_WLAN_VENDOR_ATTR_ROAMING_PARAM_MAX =
        QCA_WLAN_VENDOR_ATTR_ROAMING_PARAM_AFTER_LAST - 1,
};

/*
 * QCA_NL80211_VENDOR_SUBCMD_ROAM sub commands.
 */
enum qca_wlan_vendor_attr_roam_subcmd
{
   QCA_WLAN_VENDOR_ATTR_ROAM_SUBCMD_INVALID = 0,
   QCA_WLAN_VENDOR_ATTR_ROAM_SUBCMD_SSID_WHITE_LIST = 1,
   QCA_WLAN_VENDOR_ATTR_ROAM_SUBCMD_SET_GSCAN_ROAM_PARAMS = 2,
   QCA_WLAN_VENDOR_ATTR_ROAM_SUBCMD_SET_LAZY_ROAM = 3,
   QCA_WLAN_VENDOR_ATTR_ROAM_SUBCMD_SET_BSSID_PREFS = 4,
   QCA_WLAN_VENDOR_ATTR_ROAM_SUBCMD_SET_BSSID_PARAMS = 5,
   QCA_WLAN_VENDOR_ATTR_ROAM_SUBCMD_SET_BLACKLIST_BSSID = 6,

   /* KEEP LAST */
   QCA_WLAN_VENDOR_ATTR_ROAM_SUBCMD_AFTER_LAST,
   QCA_WLAN_VENDOR_ATTR_ROAM_SUBCMD_MAX =
        QCA_WLAN_VENDOR_ATTR_ROAM_SUBCMD_AFTER_LAST - 1,
};

/* NL attributes for data used by
 * QCA_NL80211_VENDOR_SUBCMD_GET_WIFI_INFO sub command.
 */
enum qca_wlan_vendor_attr_get_wifi_info {
    QCA_WLAN_VENDOR_ATTR_WIFI_INFO_GET_INVALID = 0,
    QCA_WLAN_VENDOR_ATTR_WIFI_INFO_DRIVER_VERSION     = 1,
    QCA_WLAN_VENDOR_ATTR_WIFI_INFO_FIRMWARE_VERSION   = 2,
    /* keep last */
    QCA_WLAN_VENDOR_ATTR_WIFI_INFO_GET_AFTER_LAST,
    QCA_WLAN_VENDOR_ATTR_WIFI_INFO_GET_MAX  =
           QCA_WLAN_VENDOR_ATTR_WIFI_INFO_GET_AFTER_LAST - 1,
};
/* NL attributes for data used by
 * QCA_NL80211_VENDOR_SUBCMD_WIFI_LOGGER_START sub command.
 */
enum qca_wlan_vendor_attr_wifi_logger_start {
    QCA_WLAN_VENDOR_ATTR_WIFI_LOGGER_START_INVALID = 0,
    QCA_WLAN_VENDOR_ATTR_WIFI_LOGGER_RING_ID = 1,
    QCA_WLAN_VENDOR_ATTR_WIFI_LOGGER_VERBOSE_LEVEL = 2,
    QCA_WLAN_VENDOR_ATTR_WIFI_LOGGER_FLAGS = 3,
    /* keep last */
    QCA_WLAN_VENDOR_ATTR_WIFI_LOGGER_START_AFTER_LAST,
    QCA_WLAN_VENDOR_ATTR_WIFI_LOGGER_START_GET_MAX  =
        QCA_WLAN_VENDOR_ATTR_WIFI_LOGGER_START_AFTER_LAST - 1,
};

enum qca_wlan_vendor_attr_logger_results
{
    QCA_WLAN_VENDOR_ATTR_LOGGER_RESULTS_INVALID = 0,

    /* Unsigned 32-bit value; must match the request Id supplied by Wi-Fi HAL
     * in the corresponding subcmd NL msg
     */
    QCA_WLAN_VENDOR_ATTR_LOGGER_RESULTS_REQUEST_ID,

    /* Unsigned 32-bit value; used to indicate the size of memory
       dump to be allocated.
     */
    QCA_WLAN_VENDOR_ATTR_LOGGER_RESULTS_MEMDUMP_SIZE,

    /* keep last */
    QCA_WLAN_VENDOR_ATTR_LOGGER_RESULTS_AFTER_LAST,
    QCA_WLAN_VENDOR_ATTR_LOGGER_RESULTS_MAX =
        QCA_WLAN_VENDOR_ATTR_LOGGER_RESULTS_AFTER_LAST - 1,

};

enum qca_wlan_offloaded_packets_sending_control
{
    QCA_WLAN_OFFLOADED_PACKETS_SENDING_CONTROL_INVALID = 0,
    QCA_WLAN_OFFLOADED_PACKETS_SENDING_START,
    QCA_WLAN_OFFLOADED_PACKETS_SENDING_STOP
};

enum qca_wlan_vendor_attr_offloaded_packets
{
    QCA_WLAN_VENDOR_ATTR_OFFLOADED_PACKETS_INVALID = 0,

    /* Takes valid value from the enum
     * qca_wlan_offloaded_packets_sending_control
     */
    QCA_WLAN_VENDOR_ATTR_OFFLOADED_PACKETS_SENDING_CONTROL,

    QCA_WLAN_VENDOR_ATTR_OFFLOADED_PACKETS_REQUEST_ID,

    /* Packet in hex format */
    QCA_WLAN_VENDOR_ATTR_OFFLOADED_PACKETS_IP_PACKET,
    QCA_WLAN_VENDOR_ATTR_OFFLOADED_PACKETS_SRC_MAC_ADDR,
    QCA_WLAN_VENDOR_ATTR_OFFLOADED_PACKETS_DST_MAC_ADDR,
    QCA_WLAN_VENDOR_ATTR_OFFLOADED_PACKETS_PERIOD,

    /* keep last */
    QCA_WLAN_VENDOR_ATTR_OFFLOADED_PACKETS_AFTER_LAST,
    QCA_WLAN_VENDOR_ATTR_OFFLOADED_PACKETS_MAX =
        QCA_WLAN_VENDOR_ATTR_OFFLOADED_PACKETS_AFTER_LAST - 1,

};

enum qca_wlan_rssi_monitoring_control
{
    QCA_WLAN_RSSI_MONITORING_CONTROL_INVALID = 0,
    QCA_WLAN_RSSI_MONITORING_START,
    QCA_WLAN_RSSI_MONITORING_STOP,
};

enum qca_wlan_vendor_attr_rssi_monitoring
{
    QCA_WLAN_VENDOR_ATTR_RSSI_MONITORING_INVALID = 0,

    /* Takes valid value from the enum
     * qca_wlan_rssi_monitoring_control
     */
    QCA_WLAN_VENDOR_ATTR_RSSI_MONITORING_CONTROL,

    QCA_WLAN_VENDOR_ATTR_RSSI_MONITORING_REQUEST_ID,

    QCA_WLAN_VENDOR_ATTR_RSSI_MONITORING_MAX_RSSI,
    QCA_WLAN_VENDOR_ATTR_RSSI_MONITORING_MIN_RSSI,

    /* attributes to be used/received in callback */
    QCA_WLAN_VENDOR_ATTR_RSSI_MONITORING_CUR_BSSID,
    QCA_WLAN_VENDOR_ATTR_RSSI_MONITORING_CUR_RSSI,

    /* keep last */
    QCA_WLAN_VENDOR_ATTR_RSSI_MONITORING_AFTER_LAST,
    QCA_WLAN_VENDOR_ATTR_RSSI_MONITORING_MAX =
        QCA_WLAN_VENDOR_ATTR_RSSI_MONITORING_AFTER_LAST - 1,

};

enum qca_wlan_vendor_attr_ndp_params
{
    QCA_WLAN_VENDOR_ATTR_NDP_PARAM_INVALID = 0,
    /* enum of sub commands */
    QCA_WLAN_VENDOR_ATTR_NDP_SUBCMD,
    /* Unsigned 16-bit value */
    QCA_WLAN_VENDOR_ATTR_NDP_TRANSACTION_ID,
    /* NL attributes for data used NDP SUB cmds */
    /* Unsigned 16-bit value indicating a service info */
    QCA_WLAN_VENDOR_ATTR_NDP_SERVICE_INSTANCE_ID,
    /* Unsigned 32-bit value; channel frequency */
    QCA_WLAN_VENDOR_ATTR_NDP_CHANNEL_SPEC_CHANNEL,
    /* Interface Discovery MAC address. An array of 6 Unsigned int8 */
    QCA_WLAN_VENDOR_ATTR_NDP_PEER_DISCOVERY_MAC_ADDR,
    /* Interface name on which NDP is being created */
    QCA_WLAN_VENDOR_ATTR_NDP_IFACE_STR,
    /* Unsigned 32-bit value for security */
    QCA_WLAN_VENDOR_ATTR_NDP_CONFIG_SECURITY,
    /* Unsigned 32-bit value for Qos */
    QCA_WLAN_VENDOR_ATTR_NDP_CONFIG_QOS,
    /* Unsigned 16-bit value for app info length */
    QCA_WLAN_VENDOR_ATTR_NDP_APP_INFO_LEN,
    /* Array of u8: len = QCA_WLAN_VENDOR_ATTR_NAN_DP_APP_INFO_LEN */
    QCA_WLAN_VENDOR_ATTR_NDP_APP_INFO,
    /* Unsigned 32-bit value for NDP instance Id */
    QCA_WLAN_VENDOR_ATTR_NDP_INSTANCE_ID,
    /* Unsigned 32-bit value for schedule update response code accept/reject */
    QCA_WLAN_VENDOR_ATTR_NDP_SCHEDULE_RESPONSE_CODE,
    /* Unsigned 32-bit value for schedule status success/fail */
    QCA_WLAN_VENDOR_ATTR_NDP_SCHEDULE_STATUS_CODE,
    /* NDI MAC address. An array of 6 Unsigned int8 */
    QCA_WLAN_VENDOR_ATTR_NDP_NDI_MAC_ADDR,

   /* KEEP LAST */
   QCA_WLAN_VENDOR_ATTR_NDP_AFTER_LAST,
   QCA_WLAN_VENDOR_ATTR_NDP_MAX =
        QCA_WLAN_VENDOR_ATTR_NDP_AFTER_LAST - 1,
};

enum qca_wlan_vendor_attr_ndp_cfg_security
{
   /* Security info will be added when proposed in the specification */
   QCA_WLAN_VENDOR_ATTR_NDP_SECURITY_TYPE = 1,

};

enum qca_wlan_vendor_attr_ndp_qos
{
   /* Qos info will be added when proposed in the specification */
   QCA_WLAN_VENDOR_ATTR_NDP_QOS_CONFIG = 1,

};

/*
 * QCA_NL80211_VENDOR_SUBCMD_NDP sub commands.
 */
enum qca_wlan_vendor_attr_ndp_sub_cmd_value
{
   QCA_WLAN_VENDOR_ATTR_NDP_INTERFACE_CREATE = 1,
   QCA_WLAN_VENDOR_ATTR_NDP_INTERFACE_DELETE = 2,
   QCA_WLAN_VENDOR_ATTR_NDP_INITIATOR_REQUEST = 3,
   QCA_WLAN_VENDOR_ATTR_NDP_INITIATOR_RESPONSE = 4,
   QCA_WLAN_VENDOR_ATTR_NDP_RESPONDER_REQUEST = 5,
   QCA_WLAN_VENDOR_ATTR_NDP_RESPONDER_RESPONSE = 6,
   QCA_WLAN_VENDOR_ATTR_NDP_END_REQUEST = 7,
   QCA_WLAN_VENDOR_ATTR_NDP_END_RESPONSE = 8,
   QCA_WLAN_VENDOR_ATTR_NDP_SCHEDULE_UPDATE_REQUEST = 9,
   QCA_WLAN_VENDOR_ATTR_NDP_SCHEDULE_UPDATE_RESPONSE = 10,
   QCA_WLAN_VENDOR_ATTR_NDP_DATA_REQUEST_IND = 11,
   QCA_WLAN_VENDOR_ATTR_NDP_CONFIRM_IND = 12,
   QCA_WLAN_VENDOR_ATTR_NDP_SCHEDULE_UPDATE_IND = 13,
   QCA_WLAN_VENDOR_ATTR_NDP_END_IND = 14
};

#define PACKET_FILTER_ID 0

enum packet_filter_sub_cmd
{
    QCA_WLAN_SET_PACKET_FILTER = 1,
    QCA_WLAN_GET_PACKET_FILTER_SIZE = 2,
};

enum qca_wlan_vendor_attr_packet_filter
{
    QCA_WLAN_VENDOR_ATTR_PACKET_FILTER_INVALID = 0,

    QCA_WLAN_VENDOR_ATTR_PACKET_FILTER_SUB_CMD,
    QCA_WLAN_VENDOR_ATTR_PACKET_FILTER_VERSION,
    QCA_WLAN_VENDOR_ATTR_PACKET_FILTER_ID,
    QCA_WLAN_VENDOR_ATTR_PACKET_FILTER_TOTAL_LENGTH,
    QCA_WLAN_VENDOR_ATTR_PACKET_FILTER_CURRENT_OFFSET,
    QCA_WLAN_VENDOR_ATTR_PACKET_FILTER_PROGRAM,

    /* keep last */
    QCA_WLAN_VENDOR_ATTR_PACKET_FILTER_AFTER_LAST,
    QCA_WLAN_VENDOR_ATTR_PACKET_FILTER_MAX =
        QCA_WLAN_VENDOR_ATTR_PACKET_FILTER_AFTER_LAST - 1,

};

enum qca_wlan_vendor_attr_nd_offload
{
    QCA_WLAN_VENDOR_ATTR_ND_OFFLOAD_INVALID = 0,

    QCA_WLAN_VENDOR_ATTR_ND_OFFLOAD_FLAG,

    /* keep last */
    QCA_WLAN_VENDOR_ATTR_ND_OFFLOAD_AFTER_LAST,
    QCA_WLAN_VENDOR_ATTR_ND_OFFLOAD_MAX =
        QCA_WLAN_VENDOR_ATTR_ND_OFFLOAD_AFTER_LAST - 1,

};

enum qca_wlan_vendor_drv_info
{
    QCA_WLAN_VENDOR_ATTR_DRV_INFO_INVALID = 0,

    QCA_WLAN_VENDOR_ATTR_DRV_INFO_BUS_SIZE,

    /* keep last */
    QCA_WLAN_VENDOR_ATTR_DRV_INFO_AFTER_LAST,
    QCA_WLAN_VENDOR_ATTR_DRV_INFO_MAX =
        QCA_WLAN_VENDOR_ATTR_DRV_INFO_AFTER_LAST - 1,

};

/* NL attributes for data used by
 * QCA_NL80211_VENDOR_SUBCMD_GET_WAKE_REASON_STATS  sub command.
 */
enum qca_wlan_vendor_attr_wake_stats
{
    QCA_WLAN_VENDOR_ATTR_WAKE_STATS_INVALID = 0,
    QCA_WLAN_VENDOR_ATTR_WAKE_STATS_TOTAL_CMD_EVENT_WAKE,
    QCA_WLAN_VENDOR_ATTR_WAKE_STATS_CMD_EVENT_WAKE_CNT_PTR,
    QCA_WLAN_VENDOR_ATTR_WAKE_STATS_CMD_EVENT_WAKE_CNT_SZ,
    QCA_WLAN_VENDOR_ATTR_WAKE_STATS_TOTAL_DRIVER_FW_LOCAL_WAKE,
    QCA_WLAN_VENDOR_ATTR_WAKE_STATS_DRIVER_FW_LOCAL_WAKE_CNT_PTR,
    QCA_WLAN_VENDOR_ATTR_WAKE_STATS_DRIVER_FW_LOCAL_WAKE_CNT_SZ,
    QCA_WLAN_VENDOR_ATTR_WAKE_STATS_TOTAL_RX_DATA_WAKE,
    QCA_WLAN_VENDOR_ATTR_WAKE_STATS_RX_UNICAST_CNT,
    QCA_WLAN_VENDOR_ATTR_WAKE_STATS_RX_MULTICAST_CNT,
    QCA_WLAN_VENDOR_ATTR_WAKE_STATS_RX_BROADCAST_CNT,
    QCA_WLAN_VENDOR_ATTR_WAKE_STATS_ICMP_PKT,
    QCA_WLAN_VENDOR_ATTR_WAKE_STATS_ICMP6_PKT,
    QCA_WLAN_VENDOR_ATTR_WAKE_STATS_ICMP6_RA,
    QCA_WLAN_VENDOR_ATTR_WAKE_STATS_ICMP6_NA,
    QCA_WLAN_VENDOR_ATTR_WAKE_STATS_ICMP6_NS,
    QCA_WLAN_VENDOR_ATTR_WAKE_STATS_ICMP4_RX_MULTICAST_CNT,
    QCA_WLAN_VENDOR_ATTR_WAKE_STATS_ICMP6_RX_MULTICAST_CNT,
    QCA_WLAN_VENDOR_ATTR_WAKE_STATS_OTHER_RX_MULTICAST_CNT,
    /* keep last */
    QCA_WLAN_VENDOR_ATTR_WAKE_STATS_AFTER_LAST,
    QCA_WLAN_VENDOR_ATTR_WAKE_STATS_MAX =
        QCA_WLAN_VENDOR_ATTR_WAKE_STATS_AFTER_LAST - 1,
};
#endif
