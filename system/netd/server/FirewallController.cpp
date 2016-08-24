/*
 * Copyright (C) 2012 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define LOG_TAG "FirewallController"
#define LOG_NDEBUG 0

#include <android-base/stringprintf.h>
#include <cutils/log.h>

#include "NetdConstants.h"
#include "FirewallController.h"

using android::base::StringAppendF;

auto FirewallController::execIptables = ::execIptables;
auto FirewallController::execIptablesSilently = ::execIptables;
auto FirewallController::execIptablesRestore = ::execIptablesRestore;

const char* FirewallController::TABLE = "filter";

const char* FirewallController::LOCAL_INPUT = "fw_INPUT";
const char* FirewallController::LOCAL_OUTPUT = "fw_OUTPUT";
const char* FirewallController::LOCAL_FORWARD = "fw_FORWARD";

const char* FirewallController::LOCAL_DOZABLE = "fw_dozable";
const char* FirewallController::LOCAL_STANDBY = "fw_standby";
const char* FirewallController::LOCAL_POWERSAVE = "fw_powersave";

// ICMPv6 types that are required for any form of IPv6 connectivity to work. Note that because the
// fw_dozable chain is called from both INPUT and OUTPUT, this includes both packets that we need
// to be able to send (e.g., RS, NS), and packets that we need to receive (e.g., RA, NA).
const char* FirewallController::ICMPV6_TYPES[] = {
    "packet-too-big",
    "router-solicitation",
    "router-advertisement",
    "neighbour-solicitation",
    "neighbour-advertisement",
    "redirect",
};

FirewallController::FirewallController(void) {
    // If no rules are set, it's in BLACKLIST mode
    mFirewallType = BLACKLIST;
}

int FirewallController::setupIptablesHooks(void) {
    int res = 0;
    // child chains are created but not attached, they will be attached explicitly.
    FirewallType firewallType = getFirewallType(DOZABLE);
    res |= createChain(LOCAL_DOZABLE, LOCAL_INPUT, firewallType);

    firewallType = getFirewallType(STANDBY);
    res |= createChain(LOCAL_STANDBY, LOCAL_INPUT, firewallType);

    firewallType = getFirewallType(POWERSAVE);
    res |= createChain(LOCAL_POWERSAVE, LOCAL_INPUT, firewallType);

    return res;
}

int FirewallController::enableFirewall(FirewallType ftype) {
    int res = 0;
    if (mFirewallType != ftype) {
        // flush any existing rules
        disableFirewall();

        if (ftype == WHITELIST) {
            // create default rule to drop all traffic
            res |= execIptables(V4V6, "-A", LOCAL_INPUT, "-j", "DROP", NULL);
            res |= execIptables(V4V6, "-A", LOCAL_OUTPUT, "-j", "REJECT", NULL);
            res |= execIptables(V4V6, "-A", LOCAL_FORWARD, "-j", "REJECT", NULL);
        }

        // Set this after calling disableFirewall(), since it defaults to WHITELIST there
        mFirewallType = ftype;
    }
    return res;
}

int FirewallController::disableFirewall(void) {
    int res = 0;

    mFirewallType = WHITELIST;

    // flush any existing rules
    res |= execIptables(V4V6, "-F", LOCAL_INPUT, NULL);
    res |= execIptables(V4V6, "-F", LOCAL_OUTPUT, NULL);
    res |= execIptables(V4V6, "-F", LOCAL_FORWARD, NULL);

    return res;
}

int FirewallController::enableChildChains(ChildChain chain, bool enable) {
    int res = 0;
    const char* name;
    switch(chain) {
        case DOZABLE:
            name = LOCAL_DOZABLE;
            break;
        case STANDBY:
            name = LOCAL_STANDBY;
            break;
        case POWERSAVE:
            name = LOCAL_POWERSAVE;
            break;
        default:
            return res;
    }

    if (enable) {
        res |= attachChain(name, LOCAL_INPUT);
        res |= attachChain(name, LOCAL_OUTPUT);
    } else {
        res |= detachChain(name, LOCAL_INPUT);
        res |= detachChain(name, LOCAL_OUTPUT);
    }
    return res;
}

int FirewallController::isFirewallEnabled(void) {
    // TODO: verify that rules are still in place near top
    return -1;
}

int FirewallController::setInterfaceRule(const char* iface, FirewallRule rule) {
    if (mFirewallType == BLACKLIST) {
        // Unsupported in BLACKLIST mode
        return -1;
    }

    if (!isIfaceName(iface)) {
        errno = ENOENT;
        return -1;
    }

    const char* op;
    if (rule == ALLOW) {
        op = "-I";
    } else {
        op = "-D";
    }

    int res = 0;
    res |= execIptables(V4V6, op, LOCAL_INPUT, "-i", iface, "-j", "RETURN", NULL);
    res |= execIptables(V4V6, op, LOCAL_OUTPUT, "-o", iface, "-j", "RETURN", NULL);
    return res;
}

int FirewallController::setEgressSourceRule(const char* addr, FirewallRule rule) {
    if (mFirewallType == BLACKLIST) {
        // Unsupported in BLACKLIST mode
        return -1;
    }

    IptablesTarget target = V4;
    if (strchr(addr, ':')) {
        target = V6;
    }

    const char* op;
    if (rule == ALLOW) {
        op = "-I";
    } else {
        op = "-D";
    }

    int res = 0;
    res |= execIptables(target, op, LOCAL_INPUT, "-d", addr, "-j", "RETURN", NULL);
    res |= execIptables(target, op, LOCAL_OUTPUT, "-s", addr, "-j", "RETURN", NULL);
    return res;
}

int FirewallController::setEgressDestRule(const char* addr, int protocol, int port,
        FirewallRule rule) {
    if (mFirewallType == BLACKLIST) {
        // Unsupported in BLACKLIST mode
        return -1;
    }

    IptablesTarget target = V4;
    if (strchr(addr, ':')) {
        target = V6;
    }

    char protocolStr[16];
    sprintf(protocolStr, "%d", protocol);

    char portStr[16];
    sprintf(portStr, "%d", port);

    const char* op;
    if (rule == ALLOW) {
        op = "-I";
    } else {
        op = "-D";
    }

    int res = 0;
    res |= execIptables(target, op, LOCAL_INPUT, "-s", addr, "-p", protocolStr,
            "--sport", portStr, "-j", "RETURN", NULL);
    res |= execIptables(target, op, LOCAL_OUTPUT, "-d", addr, "-p", protocolStr,
            "--dport", portStr, "-j", "RETURN", NULL);
    return res;
}

FirewallType FirewallController::getFirewallType(ChildChain chain) {
    switch(chain) {
        case DOZABLE:
            return WHITELIST;
        case STANDBY:
            return BLACKLIST;
        case POWERSAVE:
            return WHITELIST;
        case NONE:
            return mFirewallType;
        default:
            return BLACKLIST;
    }
}

int FirewallController::setUidRule(ChildChain chain, int uid, FirewallRule rule) {
    char uidStr[16];
    sprintf(uidStr, "%d", uid);

    const char* op;
    const char* target;
    FirewallType firewallType = getFirewallType(chain);
    if (firewallType == WHITELIST) {
        target = "RETURN";
        // When adding, insert RETURN rules at the front, before the catch-all DROP at the end.
        op = (rule == ALLOW)? "-I" : "-D";
    } else { // BLACKLIST mode
        target = "DROP";
        // When adding, append DROP rules at the end, after the RETURN rule that matches TCP RSTs.
        op = (rule == DENY)? "-A" : "-D";
    }

    int res = 0;
    switch(chain) {
        case DOZABLE:
            res |= execIptables(V4V6, op, LOCAL_DOZABLE, "-m", "owner", "--uid-owner",
                    uidStr, "-j", target, NULL);
            break;
        case STANDBY:
            res |= execIptables(V4V6, op, LOCAL_STANDBY, "-m", "owner", "--uid-owner",
                    uidStr, "-j", target, NULL);
            break;
        case POWERSAVE:
            res |= execIptables(V4V6, op, LOCAL_POWERSAVE, "-m", "owner", "--uid-owner",
                    uidStr, "-j", target, NULL);
            break;
        case NONE:
            res |= execIptables(V4V6, op, LOCAL_INPUT, "-m", "owner", "--uid-owner", uidStr,
                    "-j", target, NULL);
            res |= execIptables(V4V6, op, LOCAL_OUTPUT, "-m", "owner", "--uid-owner", uidStr,
                    "-j", target, NULL);
            break;
        default:
            ALOGW("Unknown child chain: %d", chain);
            break;
    }
    return res;
}

int FirewallController::attachChain(const char* childChain, const char* parentChain) {
    return execIptables(V4V6, "-t", TABLE, "-A", parentChain, "-j", childChain, NULL);
}

int FirewallController::detachChain(const char* childChain, const char* parentChain) {
    return execIptables(V4V6, "-t", TABLE, "-D", parentChain, "-j", childChain, NULL);
}

int FirewallController::createChain(const char* childChain,
        const char* parentChain, FirewallType type) {
    execIptablesSilently(V4V6, "-t", TABLE, "-D", parentChain, "-j", childChain, NULL);
    std::vector<int32_t> uids;
    return replaceUidChain(childChain, type == WHITELIST, uids);
}

std::string FirewallController::makeUidRules(IptablesTarget target, const char *name,
        bool isWhitelist, const std::vector<int32_t>& uids) {
    std::string commands;
    StringAppendF(&commands, "*filter\n:%s -\n", name);

    // Allow TCP RSTs so we can cleanly close TCP connections of apps that no longer have network
    // access. Both incoming and outgoing RSTs are allowed.
    StringAppendF(&commands, "-A %s -p tcp --tcp-flags RST RST -j RETURN\n", name);

    if (isWhitelist) {
        // Allow ICMPv6 packets necessary to make IPv6 connectivity work. http://b/23158230 .
        if (target == V6) {
            for (size_t i = 0; i < ARRAY_SIZE(ICMPV6_TYPES); i++) {
                StringAppendF(&commands, "-A %s -p icmpv6 --icmpv6-type %s -j RETURN\n",
                       name, ICMPV6_TYPES[i]);
            }
        }

        // Always whitelist system UIDs.
        StringAppendF(&commands,
                "-A %s -m owner --uid-owner %d-%d -j RETURN\n", name, 0, MAX_SYSTEM_UID);
    }

    // Whitelist or blacklist the specified UIDs.
    const char *action = isWhitelist ? "RETURN" : "DROP";
    for (auto uid : uids) {
        StringAppendF(&commands, "-A %s -m owner --uid-owner %d -j %s\n", name, uid, action);
    }

    // If it's a whitelist chain, add a default DROP at the end. This is not necessary for a
    // blacklist chain, because all user-defined chains implicitly RETURN at the end.
    if (isWhitelist) {
        StringAppendF(&commands, "-A %s -j DROP\n", name);
    }

    StringAppendF(&commands, "COMMIT\n\x04");  // EOT.

    return commands;
}

int FirewallController::replaceUidChain(
        const char *name, bool isWhitelist, const std::vector<int32_t>& uids) {
   std::string commands4 = makeUidRules(V4, name, isWhitelist, uids);
   std::string commands6 = makeUidRules(V6, name, isWhitelist, uids);
   return execIptablesRestore(V4, commands4.c_str()) | execIptablesRestore(V6, commands6.c_str());
}
