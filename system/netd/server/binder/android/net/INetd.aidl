/**
 * Copyright (c) 2016, The Android Open Source Project
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

package android.net;

import android.net.UidRange;

/** {@hide} */
interface INetd {
    /**
     * Returns true if the service is responding.
     */
    boolean isAlive();

    /**
     * Replaces the contents of the specified UID-based firewall chain.
     *
     * The chain may be a whitelist chain or a blacklist chain. A blacklist chain contains DROP
     * rules for the specified UIDs and a RETURN rule at the end. A whitelist chain contains RETURN
     * rules for the system UID range (0 to {@code UID_APP} - 1), RETURN rules for for the specified
     * UIDs, and a DROP rule at the end. The chain will be created if it does not exist.
     *
     * @param chainName The name of the chain to replace.
     * @param isWhitelist Whether this is a whitelist or blacklist chain.
     * @param uids The list of UIDs to allow/deny.
     * @return true if the chain was successfully replaced, false otherwise.
     */
    boolean firewallReplaceUidChain(String chainName, boolean isWhitelist, in int[] uids);

    /**
     * Enables or disables data saver mode on costly network interfaces.
     *
     * - When disabled, all packets to/from apps in the penalty box chain are rejected on costly
     *   interfaces. Traffic to/from other apps or on other network interfaces is allowed.
     * - When enabled, only apps that are in the happy box chain and not in the penalty box chain
     *   are allowed network connectivity on costly interfaces. All other packets on these
     *   interfaces are rejected. The happy box chain always contains all system UIDs; to disallow
     *   traffic from system UIDs, place them in the penalty box chain.
     *
     * By default, data saver mode is disabled. This command has no effect but might still return an
     * error) if {@code enable} is the same as the current value.
     *
     * @param enable whether to enable or disable data saver mode.
     * @return true if the if the operation was successful, false otherwise.
     */
    boolean bandwidthEnableDataSaver(boolean enable);

    /**
     * Adds or removes one rule for each supplied UID range to prohibit all network activity outside
     * of secure VPN.
     *
     * When a UID is covered by one of these rules, traffic sent through any socket that is not
     * protected or explicitly overriden by the system will be rejected. The kernel will respond
     * with an ICMP prohibit message.
     *
     * Initially, there are no such rules. Any rules that are added will only last until the next
     * restart of netd or the device.
     *
     * @param add {@code true} if the specified UID ranges should be denied access to any network
     *        which is not secure VPN by adding rules, {@code false} to remove existing rules.
     * @param uidRanges a set of non-overlapping, contiguous ranges of UIDs to which to apply or
     *        remove this restriction.
     *        <p> Added rules should not overlap with existing rules. Likewise, removed rules should
     *        each correspond to an existing rule.
     *
     * @throws ServiceSpecificException in case of failure, with an error code corresponding to the
     *         unix errno.
     */
    void networkRejectNonSecureVpn(boolean add, in UidRange[] uidRanges);

    /**
     * Administratively closes sockets belonging to the specified UIDs.
     */
    void socketDestroy(in UidRange[] uidRanges, in int[] exemptUids);

    // Array indices for resolver parameters.
    const int RESOLVER_PARAMS_SAMPLE_VALIDITY = 0;
    const int RESOLVER_PARAMS_SUCCESS_THRESHOLD = 1;
    const int RESOLVER_PARAMS_MIN_SAMPLES = 2;
    const int RESOLVER_PARAMS_MAX_SAMPLES = 3;
    const int RESOLVER_PARAMS_COUNT = 4;

    /**
     * Sets the name servers, search domains and resolver params for the given network. Flushes the
     * cache as needed (i.e. when the servers or the number of samples to store changes).
     *
     * @param netId the network ID of the network for which information should be configured.
     * @param servers the DNS servers to configure for the network.
     * @param domains the search domains to configure.
     * @param params the params to set. This array contains RESOLVER_PARAMS_COUNT integers that
     *   encode the contents of Bionic's __res_params struct, i.e. sample_validity is stored at
     *   position RESOLVER_PARAMS_SAMPLE_VALIDITY, etc.
     * @throws ServiceSpecificException in case of failure, with an error code corresponding to the
     *         unix errno.
     */
    void setResolverConfiguration(int netId, in @utf8InCpp String[] servers,
            in @utf8InCpp String[] domains, in int[] params);

    // Array indices for resolver stats.
    const int RESOLVER_STATS_SUCCESSES = 0;
    const int RESOLVER_STATS_ERRORS = 1;
    const int RESOLVER_STATS_TIMEOUTS = 2;
    const int RESOLVER_STATS_INTERNAL_ERRORS = 3;
    const int RESOLVER_STATS_RTT_AVG = 4;
    const int RESOLVER_STATS_LAST_SAMPLE_TIME = 5;
    const int RESOLVER_STATS_USABLE = 6;
    const int RESOLVER_STATS_COUNT = 7;

    /**
     * Retrieves the name servers, search domains and resolver stats associated with the given
     * network ID.
     *
     * @param netId the network ID of the network for which information should be retrieved.
     * @param servers the DNS servers that are currently configured for the network.
     * @param domains the search domains currently configured.
     * @param params the resolver parameters configured, i.e. the contents of __res_params in order.
     * @param stats the stats for each server in the order specified by RESOLVER_STATS_XXX
     *         constants, serialized as an int array. The contents of this array are the number of
     *         <ul>
     *           <li> successes,
     *           <li> errors,
     *           <li> timeouts,
     *           <li> internal errors,
     *           <li> the RTT average,
     *           <li> the time of the last recorded sample,
     *           <li> and an integer indicating whether the server is usable (1) or broken (0).
     *         </ul>
     *         in this order. For example, the timeout counter for server N is stored at position
     *         RESOLVER_STATS_COUNT*N + RESOLVER_STATS_TIMEOUTS
     * @throws ServiceSpecificException in case of failure, with an error code corresponding to the
     *         unix errno.
     */
    void getResolverInfo(int netId, out @utf8InCpp String[] servers,
            out @utf8InCpp String[] domains, out int[] params, out int[] stats);
}
