/*
 * Copyright (C) 2008 The Android Open Source Project
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

// #define LOG_NDEBUG 0

#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <dirent.h>
#include <errno.h>
#include <string.h>
#include <linux/if.h>
#include <resolv_netid.h>
#include <resolv_params.h>

#define __STDC_FORMAT_MACROS 1
#include <inttypes.h>

#define LOG_TAG "CommandListener"

#include <cutils/log.h>
#include <netutils/ifc.h>
#include <sysutils/SocketClient.h>

#include "Controllers.h"
#include "CommandListener.h"
#include "ResponseCode.h"
#include "BandwidthController.h"
#include "IdletimerController.h"
#include "oem_iptables_hook.h"
#include "NetdConstants.h"
#include "FirewallController.h"
#include "RouteController.h"
#include "UidRanges.h"

#include <string>
#include <vector>

using android::net::gCtls;

namespace {

const unsigned NUM_OEM_IDS = NetworkController::MAX_OEM_ID - NetworkController::MIN_OEM_ID + 1;

Permission stringToPermission(const char* arg) {
    if (!strcmp(arg, "NETWORK")) {
        return PERMISSION_NETWORK;
    }
    if (!strcmp(arg, "SYSTEM")) {
        return PERMISSION_SYSTEM;
    }
    return PERMISSION_NONE;
}

unsigned stringToNetId(const char* arg) {
    if (!strcmp(arg, "local")) {
        return NetworkController::LOCAL_NET_ID;
    }
    // OEM NetIds are "oem1", "oem2", .., "oem50".
    if (!strncmp(arg, "oem", 3)) {
        unsigned n = strtoul(arg + 3, NULL, 0);
        if (1 <= n && n <= NUM_OEM_IDS) {
            return NetworkController::MIN_OEM_ID + n;
        }
        return NETID_UNSET;
    }
    // strtoul() returns 0 on errors, which is fine because 0 is an invalid netId.
    return strtoul(arg, NULL, 0);
}

class LockingFrameworkCommand : public FrameworkCommand {
public:
    LockingFrameworkCommand(FrameworkCommand *wrappedCmd, android::RWLock& lock) :
            FrameworkCommand(wrappedCmd->getCommand()),
            mWrappedCmd(wrappedCmd),
            mLock(lock) {}

    int runCommand(SocketClient *c, int argc, char **argv) {
        android::RWLock::AutoWLock lock(mLock);
        return mWrappedCmd->runCommand(c, argc, argv);
    }

private:
    FrameworkCommand *mWrappedCmd;
    android::RWLock& mLock;
};


}  // namespace

/**
 * List of module chains to be created, along with explicit ordering. ORDERING
 * IS CRITICAL, AND SHOULD BE TRIPLE-CHECKED WITH EACH CHANGE.
 */
static const char* FILTER_INPUT[] = {
        // Bandwidth should always be early in input chain, to make sure we
        // correctly count incoming traffic against data plan.
        BandwidthController::LOCAL_INPUT,
        FirewallController::LOCAL_INPUT,
        NULL,
};

static const char* FILTER_FORWARD[] = {
        OEM_IPTABLES_FILTER_FORWARD,
        FirewallController::LOCAL_FORWARD,
        BandwidthController::LOCAL_FORWARD,
        NatController::LOCAL_FORWARD,
        NULL,
};

static const char* FILTER_OUTPUT[] = {
        OEM_IPTABLES_FILTER_OUTPUT,
        FirewallController::LOCAL_OUTPUT,
        StrictController::LOCAL_OUTPUT,
        BandwidthController::LOCAL_OUTPUT,
        NULL,
};

static const char* RAW_PREROUTING[] = {
        BandwidthController::LOCAL_RAW_PREROUTING,
        IdletimerController::LOCAL_RAW_PREROUTING,
        NULL,
};

static const char* MANGLE_POSTROUTING[] = {
        BandwidthController::LOCAL_MANGLE_POSTROUTING,
        IdletimerController::LOCAL_MANGLE_POSTROUTING,
        NULL,
};

static const char* MANGLE_FORWARD[] = {
        NatController::LOCAL_MANGLE_FORWARD,
        NULL,
};

static const char* NAT_PREROUTING[] = {
        OEM_IPTABLES_NAT_PREROUTING,
        NULL,
};

static const char* NAT_POSTROUTING[] = {
        NatController::LOCAL_NAT_POSTROUTING,
        NULL,
};

static void createChildChains(IptablesTarget target, const char* table, const char* parentChain,
        const char** childChains) {
    const char** childChain = childChains;
    do {
        // Order is important:
        // -D to delete any pre-existing jump rule (removes references
        //    that would prevent -X from working)
        // -F to flush any existing chain
        // -X to delete any existing chain
        // -N to create the chain
        // -A to append the chain to parent

        execIptablesSilently(target, "-t", table, "-D", parentChain, "-j", *childChain, NULL);
        execIptablesSilently(target, "-t", table, "-F", *childChain, NULL);
        execIptablesSilently(target, "-t", table, "-X", *childChain, NULL);
        execIptables(target, "-t", table, "-N", *childChain, NULL);
        execIptables(target, "-t", table, "-A", parentChain, "-j", *childChain, NULL);
    } while (*(++childChain) != NULL);
}

void CommandListener::registerLockingCmd(FrameworkCommand *cmd, android::RWLock& lock) {
    registerCmd(new LockingFrameworkCommand(cmd, lock));
}

CommandListener::CommandListener() :
                 FrameworkListener("netd", true) {
    registerLockingCmd(new InterfaceCmd());
    registerLockingCmd(new IpFwdCmd());
    registerLockingCmd(new TetherCmd());
    registerLockingCmd(new NatCmd());
    registerLockingCmd(new ListTtysCmd());
    registerLockingCmd(new PppdCmd());
    registerLockingCmd(new SoftapCmd());
    registerLockingCmd(new BandwidthControlCmd(), gCtls->bandwidthCtrl.lock);
    registerLockingCmd(new IdletimerControlCmd());
    registerLockingCmd(new ResolverCmd());
    registerLockingCmd(new FirewallCmd(), gCtls->firewallCtrl.lock);
    registerLockingCmd(new ClatdCmd());
    registerLockingCmd(new NetworkCommand());
    registerLockingCmd(new StrictCmd());

    /*
     * This is the only time we touch top-level chains in iptables; controllers
     * should only mutate rules inside of their children chains, as created by
     * the constants above.
     *
     * Modules should never ACCEPT packets (except in well-justified cases);
     * they should instead defer to any remaining modules using RETURN, or
     * otherwise DROP/REJECT.
     */

    // Create chains for children modules
    createChildChains(V4V6, "filter", "INPUT", FILTER_INPUT);
    createChildChains(V4V6, "filter", "FORWARD", FILTER_FORWARD);
    createChildChains(V4V6, "filter", "OUTPUT", FILTER_OUTPUT);
    createChildChains(V4V6, "raw", "PREROUTING", RAW_PREROUTING);
    createChildChains(V4V6, "mangle", "POSTROUTING", MANGLE_POSTROUTING);
    createChildChains(V4, "mangle", "FORWARD", MANGLE_FORWARD);
    createChildChains(V4, "nat", "PREROUTING", NAT_PREROUTING);
    createChildChains(V4, "nat", "POSTROUTING", NAT_POSTROUTING);

    // Let each module setup their child chains
    setupOemIptablesHook();

    /* When enabled, DROPs all packets except those matching rules. */
    gCtls->firewallCtrl.setupIptablesHooks();

    /* Does DROPs in FORWARD by default */
    gCtls->natCtrl.setupIptablesHooks();
    /*
     * Does REJECT in INPUT, OUTPUT. Does counting also.
     * No DROP/REJECT allowed later in netfilter-flow hook order.
     */
    gCtls->bandwidthCtrl.setupIptablesHooks();
    /*
     * Counts in nat: PREROUTING, POSTROUTING.
     * No DROP/REJECT allowed later in netfilter-flow hook order.
     */
    gCtls->idletimerCtrl.setupIptablesHooks();

    gCtls->bandwidthCtrl.enableBandwidthControl(false);

    if (int ret = RouteController::Init(NetworkController::LOCAL_NET_ID)) {
        ALOGE("failed to initialize RouteController (%s)", strerror(-ret));
    }
}

CommandListener::InterfaceCmd::InterfaceCmd() :
                 NetdCommand("interface") {
}

int CommandListener::InterfaceCmd::runCommand(SocketClient *cli,
                                                      int argc, char **argv) {
    if (argc < 2) {
        cli->sendMsg(ResponseCode::CommandSyntaxError, "Missing argument", false);
        return 0;
    }

    if (!strcmp(argv[1], "list")) {
        DIR *d;
        struct dirent *de;

        if (!(d = opendir("/sys/class/net"))) {
            cli->sendMsg(ResponseCode::OperationFailed, "Failed to open sysfs dir", true);
            return 0;
        }

        while((de = readdir(d))) {
            if (de->d_name[0] == '.')
                continue;
            cli->sendMsg(ResponseCode::InterfaceListResult, de->d_name, false);
        }
        closedir(d);
        cli->sendMsg(ResponseCode::CommandOkay, "Interface list completed", false);
        return 0;
    } else {
        /*
         * These commands take a minimum of 3 arguments
         */
        if (argc < 3) {
            cli->sendMsg(ResponseCode::CommandSyntaxError, "Missing argument", false);
            return 0;
        }

        if (!strcmp(argv[1], "getcfg")) {
            struct in_addr addr;
            int prefixLength;
            unsigned char hwaddr[6];
            unsigned flags = 0;

            ifc_init();
            memset(hwaddr, 0, sizeof(hwaddr));

            if (ifc_get_info(argv[2], &addr.s_addr, &prefixLength, &flags)) {
                cli->sendMsg(ResponseCode::OperationFailed, "Interface not found", true);
                ifc_close();
                return 0;
            }

            if (ifc_get_hwaddr(argv[2], (void *) hwaddr)) {
                ALOGW("Failed to retrieve HW addr for %s (%s)", argv[2], strerror(errno));
            }

            char *addr_s = strdup(inet_ntoa(addr));
            const char *updown, *brdcst, *loopbk, *ppp, *running, *multi;

            updown =  (flags & IFF_UP)           ? "up" : "down";
            brdcst =  (flags & IFF_BROADCAST)    ? " broadcast" : "";
            loopbk =  (flags & IFF_LOOPBACK)     ? " loopback" : "";
            ppp =     (flags & IFF_POINTOPOINT)  ? " point-to-point" : "";
            running = (flags & IFF_RUNNING)      ? " running" : "";
            multi =   (flags & IFF_MULTICAST)    ? " multicast" : "";

            char *flag_s;

            asprintf(&flag_s, "%s%s%s%s%s%s", updown, brdcst, loopbk, ppp, running, multi);

            char *msg = NULL;
            asprintf(&msg, "%.2x:%.2x:%.2x:%.2x:%.2x:%.2x %s %d %s",
                     hwaddr[0], hwaddr[1], hwaddr[2], hwaddr[3], hwaddr[4], hwaddr[5],
                     addr_s, prefixLength, flag_s);

            cli->sendMsg(ResponseCode::InterfaceGetCfgResult, msg, false);

            free(addr_s);
            free(flag_s);
            free(msg);

            ifc_close();
            return 0;
        } else if (!strcmp(argv[1], "setcfg")) {
            // arglist: iface [addr prefixLength] flags
            if (argc < 4) {
                cli->sendMsg(ResponseCode::CommandSyntaxError, "Missing argument", false);
                return 0;
            }
            ALOGD("Setting iface cfg");

            struct in_addr addr;
            int index = 5;

            ifc_init();

            if (!inet_aton(argv[3], &addr)) {
                // Handle flags only case
                index = 3;
            } else {
                if (ifc_set_addr(argv[2], 0)) {
                    cli->sendMsg(ResponseCode::OperationFailed, "Failed to clear address", true);
                    ifc_close();
                    return 0;
                }
                if (addr.s_addr != 0) {
                    if (ifc_add_address(argv[2], argv[3], atoi(argv[4]))) {
                        cli->sendMsg(ResponseCode::OperationFailed, "Failed to set address", true);
                        ifc_close();
                        return 0;
                    }
                }
            }

            /* Process flags */
            for (int i = index; i < argc; i++) {
                char *flag = argv[i];
                if (!strcmp(flag, "up")) {
                    ALOGD("Trying to bring up %s", argv[2]);
                    if (ifc_up(argv[2])) {
                        ALOGE("Error upping interface");
                        cli->sendMsg(ResponseCode::OperationFailed, "Failed to up interface", true);
                        ifc_close();
                        return 0;
                    }
                } else if (!strcmp(flag, "down")) {
                    ALOGD("Trying to bring down %s", argv[2]);
                    if (ifc_down(argv[2])) {
                        ALOGE("Error downing interface");
                        cli->sendMsg(ResponseCode::OperationFailed, "Failed to down interface", true);
                        ifc_close();
                        return 0;
                    }
                } else if (!strcmp(flag, "broadcast")) {
                    // currently ignored
                } else if (!strcmp(flag, "multicast")) {
                    // currently ignored
                } else if (!strcmp(flag, "running")) {
                    // currently ignored
                } else if (!strcmp(flag, "loopback")) {
                    // currently ignored
                } else if (!strcmp(flag, "point-to-point")) {
                    // currently ignored
                } else {
                    cli->sendMsg(ResponseCode::CommandParameterError, "Flag unsupported", false);
                    ifc_close();
                    return 0;
                }
            }

            cli->sendMsg(ResponseCode::CommandOkay, "Interface configuration set", false);
            ifc_close();
            return 0;
        } else if (!strcmp(argv[1], "clearaddrs")) {
            // arglist: iface
            ALOGD("Clearing all IP addresses on %s", argv[2]);

            ifc_clear_addresses(argv[2]);

            cli->sendMsg(ResponseCode::CommandOkay, "Interface IP addresses cleared", false);
            return 0;
        } else if (!strcmp(argv[1], "ipv6privacyextensions")) {
            if (argc != 4) {
                cli->sendMsg(ResponseCode::CommandSyntaxError,
                        "Usage: interface ipv6privacyextensions <interface> <enable|disable>",
                        false);
                return 0;
            }
            int enable = !strncmp(argv[3], "enable", 7);
            if (gCtls->interfaceCtrl.setIPv6PrivacyExtensions(argv[2], enable) == 0) {
                cli->sendMsg(ResponseCode::CommandOkay, "IPv6 privacy extensions changed", false);
            } else {
                cli->sendMsg(ResponseCode::OperationFailed,
                        "Failed to set ipv6 privacy extensions", true);
            }
            return 0;
        } else if (!strcmp(argv[1], "ipv6")) {
            if (argc != 4) {
                cli->sendMsg(ResponseCode::CommandSyntaxError,
                        "Usage: interface ipv6 <interface> <enable|disable>",
                        false);
                return 0;
            }

            int enable = !strncmp(argv[3], "enable", 7);
            if (gCtls->interfaceCtrl.setEnableIPv6(argv[2], enable) == 0) {
                cli->sendMsg(ResponseCode::CommandOkay, "IPv6 state changed", false);
            } else {
                cli->sendMsg(ResponseCode::OperationFailed,
                        "Failed to change IPv6 state", true);
            }
            return 0;
        } else if (!strcmp(argv[1], "ipv6ndoffload")) {
            if (argc != 4) {
                cli->sendMsg(ResponseCode::CommandSyntaxError,
                        "Usage: interface ipv6ndoffload <interface> <enable|disable>",
                        false);
                return 0;
            }
            int enable = !strncmp(argv[3], "enable", 7);
            if (gCtls->interfaceCtrl.setIPv6NdOffload(argv[2], enable) == 0) {
                cli->sendMsg(ResponseCode::CommandOkay, "IPv6 ND offload changed", false);
            } else {
                cli->sendMsg(ResponseCode::OperationFailed,
                        "Failed to change IPv6 ND offload state", true);
            }
            return 0;
        } else if (!strcmp(argv[1], "setmtu")) {
            if (argc != 4) {
                cli->sendMsg(ResponseCode::CommandSyntaxError,
                        "Usage: interface setmtu <interface> <val>", false);
                return 0;
            }
            if (gCtls->interfaceCtrl.setMtu(argv[2], argv[3]) == 0) {
                cli->sendMsg(ResponseCode::CommandOkay, "MTU changed", false);
            } else {
                cli->sendMsg(ResponseCode::OperationFailed,
                        "Failed to set MTU", true);
            }
            return 0;
        } else {
            cli->sendMsg(ResponseCode::CommandSyntaxError, "Unknown interface cmd", false);
            return 0;
        }
    }
    return 0;
}


CommandListener::ListTtysCmd::ListTtysCmd() :
                 NetdCommand("list_ttys") {
}

int CommandListener::ListTtysCmd::runCommand(SocketClient *cli,
                                             int /* argc */, char ** /* argv */) {
    TtyCollection *tlist = gCtls->pppCtrl.getTtyList();
    TtyCollection::iterator it;

    for (it = tlist->begin(); it != tlist->end(); ++it) {
        cli->sendMsg(ResponseCode::TtyListResult, *it, false);
    }

    cli->sendMsg(ResponseCode::CommandOkay, "Ttys listed.", false);
    return 0;
}

CommandListener::IpFwdCmd::IpFwdCmd() :
                 NetdCommand("ipfwd") {
}

int CommandListener::IpFwdCmd::runCommand(SocketClient *cli, int argc, char **argv) {
    bool matched = false;
    bool success;

    if (argc == 2) {
        //   0     1
        // ipfwd status
        if (!strcmp(argv[1], "status")) {
            char *tmp = NULL;

            asprintf(&tmp, "Forwarding %s",
                     ((gCtls->tetherCtrl.forwardingRequestCount() > 0) ? "enabled" : "disabled"));
            cli->sendMsg(ResponseCode::IpFwdStatusResult, tmp, false);
            free(tmp);
            return 0;
        }
    } else if (argc == 3) {
        //  0      1         2
        // ipfwd enable  <requester>
        // ipfwd disable <requester>
        if (!strcmp(argv[1], "enable")) {
            matched = true;
            success = gCtls->tetherCtrl.enableForwarding(argv[2]);
        } else if (!strcmp(argv[1], "disable")) {
            matched = true;
            success = gCtls->tetherCtrl.disableForwarding(argv[2]);
        }
    } else if (argc == 4) {
        //  0      1      2     3
        // ipfwd  add   wlan0 dummy0
        // ipfwd remove wlan0 dummy0
        int ret = 0;
        if (!strcmp(argv[1], "add")) {
            matched = true;
            ret = RouteController::enableTethering(argv[2], argv[3]);
        } else if (!strcmp(argv[1], "remove")) {
            matched = true;
            ret = RouteController::disableTethering(argv[2], argv[3]);
        }
        success = (ret == 0);
        errno = -ret;
    }

    if (!matched) {
        cli->sendMsg(ResponseCode::CommandSyntaxError, "Unknown ipfwd cmd", false);
        return 0;
    }

    if (success) {
        cli->sendMsg(ResponseCode::CommandOkay, "ipfwd operation succeeded", false);
    } else {
        cli->sendMsg(ResponseCode::OperationFailed, "ipfwd operation failed", true);
    }
    return 0;
}

CommandListener::TetherCmd::TetherCmd() :
                 NetdCommand("tether") {
}

int CommandListener::TetherCmd::runCommand(SocketClient *cli,
                                                      int argc, char **argv) {
    int rc = 0;

    if (argc < 2) {
        cli->sendMsg(ResponseCode::CommandSyntaxError, "Missing argument", false);
        return 0;
    }

    if (!strcmp(argv[1], "stop")) {
        rc = gCtls->tetherCtrl.stopTethering();
    } else if (!strcmp(argv[1], "status")) {
        char *tmp = NULL;

        asprintf(&tmp, "Tethering services %s",
                 (gCtls->tetherCtrl.isTetheringStarted() ? "started" : "stopped"));
        cli->sendMsg(ResponseCode::TetherStatusResult, tmp, false);
        free(tmp);
        return 0;
    } else if (argc == 3) {
        if (!strcmp(argv[1], "interface") && !strcmp(argv[2], "list")) {
            InterfaceCollection *ilist = gCtls->tetherCtrl.getTetheredInterfaceList();
            InterfaceCollection::iterator it;
            for (it = ilist->begin(); it != ilist->end(); ++it) {
                cli->sendMsg(ResponseCode::TetherInterfaceListResult, *it, false);
            }
        } else if (!strcmp(argv[1], "dns") && !strcmp(argv[2], "list")) {
            char netIdStr[UINT32_STRLEN];
            snprintf(netIdStr, sizeof(netIdStr), "%u", gCtls->tetherCtrl.getDnsNetId());
            cli->sendMsg(ResponseCode::TetherDnsFwdNetIdResult, netIdStr, false);

            for (const auto &fwdr : *(gCtls->tetherCtrl.getDnsForwarders())) {
                cli->sendMsg(ResponseCode::TetherDnsFwdTgtListResult, fwdr.c_str(), false);
            }
        }
    } else {
        /*
         * These commands take a minimum of 4 arguments
         */
        if (argc < 4) {
            cli->sendMsg(ResponseCode::CommandSyntaxError, "Missing argument", false);
            return 0;
        }

        if (!strcmp(argv[1], "start")) {
            if (argc % 2 == 1) {
                cli->sendMsg(ResponseCode::CommandSyntaxError, "Bad number of arguments", false);
                return 0;
            }

            const int num_addrs = argc - 2;
            // TODO: consider moving this validation into TetherController.
            struct in_addr tmp_addr;
            for (int arg_index = 2; arg_index < argc; arg_index++) {
                if (!inet_aton(argv[arg_index], &tmp_addr)) {
                    cli->sendMsg(ResponseCode::CommandParameterError, "Invalid address", false);
                    return 0;
                }
            }

            rc = gCtls->tetherCtrl.startTethering(num_addrs, &(argv[2]));
        } else if (!strcmp(argv[1], "interface")) {
            if (!strcmp(argv[2], "add")) {
                rc = gCtls->tetherCtrl.tetherInterface(argv[3]);
            } else if (!strcmp(argv[2], "remove")) {
                rc = gCtls->tetherCtrl.untetherInterface(argv[3]);
            /* else if (!strcmp(argv[2], "list")) handled above */
            } else {
                cli->sendMsg(ResponseCode::CommandParameterError,
                             "Unknown tether interface operation", false);
                return 0;
            }
        } else if (!strcmp(argv[1], "dns")) {
            if (!strcmp(argv[2], "set")) {
                if (argc < 5) {
                    cli->sendMsg(ResponseCode::CommandSyntaxError, "Missing argument", false);
                    return 0;
                }
                unsigned netId = stringToNetId(argv[3]);
                rc = gCtls->tetherCtrl.setDnsForwarders(netId, &argv[4], argc - 4);
            /* else if (!strcmp(argv[2], "list")) handled above */
            } else {
                cli->sendMsg(ResponseCode::CommandParameterError,
                             "Unknown tether interface operation", false);
                return 0;
            }
        } else {
            cli->sendMsg(ResponseCode::CommandSyntaxError, "Unknown tether cmd", false);
            return 0;
        }
    }

    if (!rc) {
        cli->sendMsg(ResponseCode::CommandOkay, "Tether operation succeeded", false);
    } else {
        cli->sendMsg(ResponseCode::OperationFailed, "Tether operation failed", true);
    }

    return 0;
}

CommandListener::NatCmd::NatCmd() :
                 NetdCommand("nat") {
}

int CommandListener::NatCmd::runCommand(SocketClient *cli,
                                                      int argc, char **argv) {
    int rc = 0;

    if (argc < 5) {
        cli->sendMsg(ResponseCode::CommandSyntaxError, "Missing argument", false);
        return 0;
    }

    //  0     1       2        3
    // nat  enable intiface extiface
    // nat disable intiface extiface
    if (!strcmp(argv[1], "enable") && argc >= 4) {
        rc = gCtls->natCtrl.enableNat(argv[2], argv[3]);
        if(!rc) {
            /* Ignore ifaces for now. */
            rc = gCtls->bandwidthCtrl.setGlobalAlertInForwardChain();
        }
    } else if (!strcmp(argv[1], "disable") && argc >= 4) {
        /* Ignore ifaces for now. */
        rc = gCtls->bandwidthCtrl.removeGlobalAlertInForwardChain();
        rc |= gCtls->natCtrl.disableNat(argv[2], argv[3]);
    } else {
        cli->sendMsg(ResponseCode::CommandSyntaxError, "Unknown nat cmd", false);
        return 0;
    }

    if (!rc) {
        cli->sendMsg(ResponseCode::CommandOkay, "Nat operation succeeded", false);
    } else {
        cli->sendMsg(ResponseCode::OperationFailed, "Nat operation failed", true);
    }

    return 0;
}

CommandListener::PppdCmd::PppdCmd() :
                 NetdCommand("pppd") {
}

int CommandListener::PppdCmd::runCommand(SocketClient *cli,
                                                      int argc, char **argv) {
    int rc = 0;

    if (argc < 3) {
        cli->sendMsg(ResponseCode::CommandSyntaxError, "Missing argument", false);
        return 0;
    }

    if (!strcmp(argv[1], "attach")) {
        struct in_addr l, r, dns1, dns2;

        memset(&dns1, 0, sizeof(struct in_addr));
        memset(&dns2, 0, sizeof(struct in_addr));

        if (!inet_aton(argv[3], &l)) {
            cli->sendMsg(ResponseCode::CommandParameterError, "Invalid local address", false);
            return 0;
        }
        if (!inet_aton(argv[4], &r)) {
            cli->sendMsg(ResponseCode::CommandParameterError, "Invalid remote address", false);
            return 0;
        }
        if ((argc > 3) && (!inet_aton(argv[5], &dns1))) {
            cli->sendMsg(ResponseCode::CommandParameterError, "Invalid dns1 address", false);
            return 0;
        }
        if ((argc > 4) && (!inet_aton(argv[6], &dns2))) {
            cli->sendMsg(ResponseCode::CommandParameterError, "Invalid dns2 address", false);
            return 0;
        }
        rc = gCtls->pppCtrl.attachPppd(argv[2], l, r, dns1, dns2);
    } else if (!strcmp(argv[1], "detach")) {
        rc = gCtls->pppCtrl.detachPppd(argv[2]);
    } else {
        cli->sendMsg(ResponseCode::CommandSyntaxError, "Unknown pppd cmd", false);
        return 0;
    }

    if (!rc) {
        cli->sendMsg(ResponseCode::CommandOkay, "Pppd operation succeeded", false);
    } else {
        cli->sendMsg(ResponseCode::OperationFailed, "Pppd operation failed", true);
    }

    return 0;
}

CommandListener::SoftapCmd::SoftapCmd() :
                 NetdCommand("softap") {
}

int CommandListener::SoftapCmd::runCommand(SocketClient *cli,
                                        int argc, char **argv) {
    int rc = ResponseCode::SoftapStatusResult;
    char *retbuf = NULL;

    if (gCtls == nullptr) {
      cli->sendMsg(ResponseCode::ServiceStartFailed, "SoftAP is not available", false);
      return -1;
    }
    if (argc < 2) {
        cli->sendMsg(ResponseCode::CommandSyntaxError,
                     "Missing argument in a SoftAP command", false);
        return 0;
    }

    if (!strcmp(argv[1], "startap")) {
        rc = gCtls->softapCtrl.startSoftap();
    } else if (!strcmp(argv[1], "stopap")) {
        rc = gCtls->softapCtrl.stopSoftap();
    } else if (!strcmp(argv[1], "fwreload")) {
        rc = gCtls->softapCtrl.fwReloadSoftap(argc, argv);
    } else if (!strcmp(argv[1], "status")) {
        asprintf(&retbuf, "Softap service %s running",
                 (gCtls->softapCtrl.isSoftapStarted() ? "is" : "is not"));
        cli->sendMsg(rc, retbuf, false);
        free(retbuf);
        return 0;
    } else if (!strcmp(argv[1], "set")) {
        rc = gCtls->softapCtrl.setSoftap(argc, argv);
    } else {
        cli->sendMsg(ResponseCode::CommandSyntaxError, "Unrecognized SoftAP command", false);
        return 0;
    }

    if (rc >= 400 && rc < 600)
      cli->sendMsg(rc, "SoftAP command has failed", false);
    else
      cli->sendMsg(rc, "Ok", false);

    return 0;
}

CommandListener::ResolverCmd::ResolverCmd() :
        NetdCommand("resolver") {
}

int CommandListener::ResolverCmd::runCommand(SocketClient *cli, int argc, char **margv) {
    int rc = 0;
    const char **argv = const_cast<const char **>(margv);

    if (argc < 3) {
        cli->sendMsg(ResponseCode::CommandSyntaxError, "Resolver missing arguments", false);
        return 0;
    }

    unsigned netId = stringToNetId(argv[2]);
    // TODO: Consider making NetworkController.isValidNetwork() public
    // and making that check here.

    if (!strcmp(argv[1], "setnetdns")) {
        if (!parseAndExecuteSetNetDns(netId, argc, argv)) {
            cli->sendMsg(ResponseCode::CommandSyntaxError,
                    "Wrong number of or invalid arguments to resolver setnetdns", false);
            return 0;
        }
    } else if (!strcmp(argv[1], "clearnetdns")) { // "resolver clearnetdns <netId>"
        if (argc == 3) {
            rc = gCtls->resolverCtrl.clearDnsServers(netId);
        } else {
            cli->sendMsg(ResponseCode::CommandSyntaxError,
                    "Wrong number of arguments to resolver clearnetdns", false);
            return 0;
        }
    } else {
        cli->sendMsg(ResponseCode::CommandSyntaxError,"Resolver unknown command", false);
        return 0;
    }

    if (!rc) {
        cli->sendMsg(ResponseCode::CommandOkay, "Resolver command succeeded", false);
    } else {
        cli->sendMsg(ResponseCode::OperationFailed, "Resolver command failed", true);
    }

    return 0;
}

bool CommandListener::ResolverCmd::parseAndExecuteSetNetDns(int netId, int argc,
        const char** argv) {
    // "resolver setnetdns <netId> <domains> <dns1> [<dns2> ...] [--params <params>]"
    // TODO: This code has to be replaced by a Binder call ASAP
    if (argc < 5) {
        return false;
    }
    int end = argc;
    __res_params params;
    const __res_params* paramsPtr = nullptr;
    if (end > 6 && !strcmp(argv[end - 2], "--params")) {
        const char* paramsStr = argv[end - 1];
        end -= 2;
        if (sscanf(paramsStr, "%hu %hhu %hhu %hhu", &params.sample_validity,
                &params.success_threshold, &params.min_samples, &params.max_samples) != 4) {
            return false;
        }
        paramsPtr = &params;
    }
    return gCtls->resolverCtrl.setDnsServers(netId, argv[3], &argv[4], end - 4, paramsPtr) == 0;
}

CommandListener::BandwidthControlCmd::BandwidthControlCmd() :
    NetdCommand("bandwidth") {
}

void CommandListener::BandwidthControlCmd::sendGenericSyntaxError(SocketClient *cli, const char *usageMsg) {
    char *msg;
    asprintf(&msg, "Usage: bandwidth %s", usageMsg);
    cli->sendMsg(ResponseCode::CommandSyntaxError, msg, false);
    free(msg);
}

void CommandListener::BandwidthControlCmd::sendGenericOkFail(SocketClient *cli, int cond) {
    if (!cond) {
        cli->sendMsg(ResponseCode::CommandOkay, "Bandwidth command succeeeded", false);
    } else {
        cli->sendMsg(ResponseCode::OperationFailed, "Bandwidth command failed", false);
    }
}

void CommandListener::BandwidthControlCmd::sendGenericOpFailed(SocketClient *cli, const char *errMsg) {
    cli->sendMsg(ResponseCode::OperationFailed, errMsg, false);
}

int CommandListener::BandwidthControlCmd::runCommand(SocketClient *cli, int argc, char **argv) {
    if (argc < 2) {
        sendGenericSyntaxError(cli, "<cmds> <args...>");
        return 0;
    }

    ALOGV("bwctrlcmd: argc=%d %s %s ...", argc, argv[0], argv[1]);

    if (!strcmp(argv[1], "enable")) {
        int rc = gCtls->bandwidthCtrl.enableBandwidthControl(true);
        sendGenericOkFail(cli, rc);
        return 0;

    }
    if (!strcmp(argv[1], "disable")) {
        int rc = gCtls->bandwidthCtrl.disableBandwidthControl();
        sendGenericOkFail(cli, rc);
        return 0;

    }
    if (!strcmp(argv[1], "removequota") || !strcmp(argv[1], "rq")) {
        if (argc != 3) {
            sendGenericSyntaxError(cli, "removequota <interface>");
            return 0;
        }
        int rc = gCtls->bandwidthCtrl.removeInterfaceSharedQuota(argv[2]);
        sendGenericOkFail(cli, rc);
        return 0;

    }
    if (!strcmp(argv[1], "getquota") || !strcmp(argv[1], "gq")) {
        int64_t bytes;
        if (argc != 2) {
            sendGenericSyntaxError(cli, "getquota");
            return 0;
        }
        int rc = gCtls->bandwidthCtrl.getInterfaceSharedQuota(&bytes);
        if (rc) {
            sendGenericOpFailed(cli, "Failed to get quota");
            return 0;
        }

        char *msg;
        asprintf(&msg, "%" PRId64, bytes);
        cli->sendMsg(ResponseCode::QuotaCounterResult, msg, false);
        free(msg);
        return 0;

    }
    if (!strcmp(argv[1], "getiquota") || !strcmp(argv[1], "giq")) {
        int64_t bytes;
        if (argc != 3) {
            sendGenericSyntaxError(cli, "getiquota <iface>");
            return 0;
        }

        int rc = gCtls->bandwidthCtrl.getInterfaceQuota(argv[2], &bytes);
        if (rc) {
            sendGenericOpFailed(cli, "Failed to get quota");
            return 0;
        }
        char *msg;
        asprintf(&msg, "%" PRId64, bytes);
        cli->sendMsg(ResponseCode::QuotaCounterResult, msg, false);
        free(msg);
        return 0;

    }
    if (!strcmp(argv[1], "setquota") || !strcmp(argv[1], "sq")) {
        if (argc != 4) {
            sendGenericSyntaxError(cli, "setquota <interface> <bytes>");
            return 0;
        }
        int rc = gCtls->bandwidthCtrl.setInterfaceSharedQuota(argv[2], atoll(argv[3]));
        sendGenericOkFail(cli, rc);
        return 0;
    }
    if (!strcmp(argv[1], "setquotas") || !strcmp(argv[1], "sqs")) {
        int rc;
        if (argc < 4) {
            sendGenericSyntaxError(cli, "setquotas <bytes> <interface> ...");
            return 0;
        }

        for (int q = 3; argc >= 4; q++, argc--) {
            rc = gCtls->bandwidthCtrl.setInterfaceSharedQuota(argv[q], atoll(argv[2]));
            if (rc) {
                char *msg;
                asprintf(&msg, "bandwidth setquotas %s %s failed", argv[2], argv[q]);
                cli->sendMsg(ResponseCode::OperationFailed,
                             msg, false);
                free(msg);
                return 0;
            }
        }
        sendGenericOkFail(cli, rc);
        return 0;

    }
    if (!strcmp(argv[1], "removequotas") || !strcmp(argv[1], "rqs")) {
        int rc;
        if (argc < 3) {
            sendGenericSyntaxError(cli, "removequotas <interface> ...");
            return 0;
        }

        for (int q = 2; argc >= 3; q++, argc--) {
            rc = gCtls->bandwidthCtrl.removeInterfaceSharedQuota(argv[q]);
            if (rc) {
                char *msg;
                asprintf(&msg, "bandwidth removequotas %s failed", argv[q]);
                cli->sendMsg(ResponseCode::OperationFailed,
                             msg, false);
                free(msg);
                return 0;
            }
        }
        sendGenericOkFail(cli, rc);
        return 0;

    }
    if (!strcmp(argv[1], "removeiquota") || !strcmp(argv[1], "riq")) {
        if (argc != 3) {
            sendGenericSyntaxError(cli, "removeiquota <interface>");
            return 0;
        }
        int rc = gCtls->bandwidthCtrl.removeInterfaceQuota(argv[2]);
        sendGenericOkFail(cli, rc);
        return 0;

    }
    if (!strcmp(argv[1], "setiquota") || !strcmp(argv[1], "siq")) {
        if (argc != 4) {
            sendGenericSyntaxError(cli, "setiquota <interface> <bytes>");
            return 0;
        }
        int rc = gCtls->bandwidthCtrl.setInterfaceQuota(argv[2], atoll(argv[3]));
        sendGenericOkFail(cli, rc);
        return 0;

    }
    if (!strcmp(argv[1], "addnaughtyapps") || !strcmp(argv[1], "ana")) {
        if (argc < 3) {
            sendGenericSyntaxError(cli, "addnaughtyapps <appUid> ...");
            return 0;
        }
        int rc = gCtls->bandwidthCtrl.addNaughtyApps(argc - 2, argv + 2);
        sendGenericOkFail(cli, rc);
        return 0;


    }
    if (!strcmp(argv[1], "removenaughtyapps") || !strcmp(argv[1], "rna")) {
        if (argc < 3) {
            sendGenericSyntaxError(cli, "removenaughtyapps <appUid> ...");
            return 0;
        }
        int rc = gCtls->bandwidthCtrl.removeNaughtyApps(argc - 2, argv + 2);
        sendGenericOkFail(cli, rc);
        return 0;
    }
    if (!strcmp(argv[1], "addniceapps") || !strcmp(argv[1], "aha")) {
        if (argc < 3) {
            sendGenericSyntaxError(cli, "addniceapps <appUid> ...");
            return 0;
        }
        int rc = gCtls->bandwidthCtrl.addNiceApps(argc - 2, argv + 2);
        sendGenericOkFail(cli, rc);
        return 0;
    }
    if (!strcmp(argv[1], "removeniceapps") || !strcmp(argv[1], "rha")) {
        if (argc < 3) {
            sendGenericSyntaxError(cli, "removeniceapps <appUid> ...");
            return 0;
        }
        int rc = gCtls->bandwidthCtrl.removeNiceApps(argc - 2, argv + 2);
        sendGenericOkFail(cli, rc);
        return 0;
    }
    if (!strcmp(argv[1], "setglobalalert") || !strcmp(argv[1], "sga")) {
        if (argc != 3) {
            sendGenericSyntaxError(cli, "setglobalalert <bytes>");
            return 0;
        }
        int rc = gCtls->bandwidthCtrl.setGlobalAlert(atoll(argv[2]));
        sendGenericOkFail(cli, rc);
        return 0;
    }
    if (!strcmp(argv[1], "debugsettetherglobalalert") || !strcmp(argv[1], "dstga")) {
        if (argc != 4) {
            sendGenericSyntaxError(cli, "debugsettetherglobalalert <interface0> <interface1>");
            return 0;
        }
        /* We ignore the interfaces for now. */
        int rc = gCtls->bandwidthCtrl.setGlobalAlertInForwardChain();
        sendGenericOkFail(cli, rc);
        return 0;

    }
    if (!strcmp(argv[1], "removeglobalalert") || !strcmp(argv[1], "rga")) {
        if (argc != 2) {
            sendGenericSyntaxError(cli, "removeglobalalert");
            return 0;
        }
        int rc = gCtls->bandwidthCtrl.removeGlobalAlert();
        sendGenericOkFail(cli, rc);
        return 0;

    }
    if (!strcmp(argv[1], "debugremovetetherglobalalert") || !strcmp(argv[1], "drtga")) {
        if (argc != 4) {
            sendGenericSyntaxError(cli, "debugremovetetherglobalalert <interface0> <interface1>");
            return 0;
        }
        /* We ignore the interfaces for now. */
        int rc = gCtls->bandwidthCtrl.removeGlobalAlertInForwardChain();
        sendGenericOkFail(cli, rc);
        return 0;

    }
    if (!strcmp(argv[1], "setsharedalert") || !strcmp(argv[1], "ssa")) {
        if (argc != 3) {
            sendGenericSyntaxError(cli, "setsharedalert <bytes>");
            return 0;
        }
        int rc = gCtls->bandwidthCtrl.setSharedAlert(atoll(argv[2]));
        sendGenericOkFail(cli, rc);
        return 0;

    }
    if (!strcmp(argv[1], "removesharedalert") || !strcmp(argv[1], "rsa")) {
        if (argc != 2) {
            sendGenericSyntaxError(cli, "removesharedalert");
            return 0;
        }
        int rc = gCtls->bandwidthCtrl.removeSharedAlert();
        sendGenericOkFail(cli, rc);
        return 0;

    }
    if (!strcmp(argv[1], "setinterfacealert") || !strcmp(argv[1], "sia")) {
        if (argc != 4) {
            sendGenericSyntaxError(cli, "setinterfacealert <interface> <bytes>");
            return 0;
        }
        int rc = gCtls->bandwidthCtrl.setInterfaceAlert(argv[2], atoll(argv[3]));
        sendGenericOkFail(cli, rc);
        return 0;

    }
    if (!strcmp(argv[1], "removeinterfacealert") || !strcmp(argv[1], "ria")) {
        if (argc != 3) {
            sendGenericSyntaxError(cli, "removeinterfacealert <interface>");
            return 0;
        }
        int rc = gCtls->bandwidthCtrl.removeInterfaceAlert(argv[2]);
        sendGenericOkFail(cli, rc);
        return 0;

    }
    if (!strcmp(argv[1], "gettetherstats") || !strcmp(argv[1], "gts")) {
        BandwidthController::TetherStats tetherStats;
        std::string extraProcessingInfo = "";
        if (argc < 2 || argc > 4) {
            sendGenericSyntaxError(cli, "gettetherstats [<intInterface> <extInterface>]");
            return 0;
        }
        tetherStats.intIface = argc > 2 ? argv[2] : "";
        tetherStats.extIface = argc > 3 ? argv[3] : "";
        // No filtering requested and there are no interface pairs to lookup.
        if (argc <= 2 && gCtls->natCtrl.ifacePairList.empty()) {
            cli->sendMsg(ResponseCode::CommandOkay, "Tethering stats list completed", false);
            return 0;
        }
        int rc = gCtls->bandwidthCtrl.getTetherStats(cli, tetherStats, extraProcessingInfo);
        if (rc) {
                extraProcessingInfo.insert(0, "Failed to get tethering stats.\n");
                sendGenericOpFailed(cli, extraProcessingInfo.c_str());
                return 0;
        }
        return 0;

    }

    cli->sendMsg(ResponseCode::CommandSyntaxError, "Unknown bandwidth cmd", false);
    return 0;
}

CommandListener::IdletimerControlCmd::IdletimerControlCmd() :
    NetdCommand("idletimer") {
}

int CommandListener::IdletimerControlCmd::runCommand(SocketClient *cli, int argc, char **argv) {
  // TODO(ashish): Change the error statements
    if (argc < 2) {
        cli->sendMsg(ResponseCode::CommandSyntaxError, "Missing argument", false);
        return 0;
    }

    ALOGV("idletimerctrlcmd: argc=%d %s %s ...", argc, argv[0], argv[1]);

    if (!strcmp(argv[1], "enable")) {
      if (0 != gCtls->idletimerCtrl.enableIdletimerControl()) {
        cli->sendMsg(ResponseCode::CommandSyntaxError, "Missing argument", false);
      } else {
        cli->sendMsg(ResponseCode::CommandOkay, "Enable success", false);
      }
      return 0;

    }
    if (!strcmp(argv[1], "disable")) {
      if (0 != gCtls->idletimerCtrl.disableIdletimerControl()) {
        cli->sendMsg(ResponseCode::CommandSyntaxError, "Missing argument", false);
      } else {
        cli->sendMsg(ResponseCode::CommandOkay, "Disable success", false);
      }
      return 0;
    }
    if (!strcmp(argv[1], "add")) {
        if (argc != 5) {
            cli->sendMsg(ResponseCode::CommandSyntaxError, "Missing argument", false);
            return 0;
        }
        if(0 != gCtls->idletimerCtrl.addInterfaceIdletimer(
                                        argv[2], atoi(argv[3]), argv[4])) {
          cli->sendMsg(ResponseCode::OperationFailed, "Failed to add interface", false);
        } else {
          cli->sendMsg(ResponseCode::CommandOkay,  "Add success", false);
        }
        return 0;
    }
    if (!strcmp(argv[1], "remove")) {
        if (argc != 5) {
            cli->sendMsg(ResponseCode::CommandSyntaxError, "Missing argument", false);
            return 0;
        }
        // ashish: fixme timeout
        if (0 != gCtls->idletimerCtrl.removeInterfaceIdletimer(
                                        argv[2], atoi(argv[3]), argv[4])) {
          cli->sendMsg(ResponseCode::OperationFailed, "Failed to remove interface", false);
        } else {
          cli->sendMsg(ResponseCode::CommandOkay, "Remove success", false);
        }
        return 0;
    }

    cli->sendMsg(ResponseCode::CommandSyntaxError, "Unknown idletimer cmd", false);
    return 0;
}

CommandListener::FirewallCmd::FirewallCmd() :
    NetdCommand("firewall") {
}

int CommandListener::FirewallCmd::sendGenericOkFail(SocketClient *cli, int cond) {
    if (!cond) {
        cli->sendMsg(ResponseCode::CommandOkay, "Firewall command succeeded", false);
    } else {
        cli->sendMsg(ResponseCode::OperationFailed, "Firewall command failed", false);
    }
    return 0;
}

FirewallRule CommandListener::FirewallCmd::parseRule(const char* arg) {
    if (!strcmp(arg, "allow")) {
        return ALLOW;
    } else if (!strcmp(arg, "deny")) {
        return DENY;
    } else {
        ALOGE("failed to parse uid rule (%s)", arg);
        return ALLOW;
    }
}

FirewallType CommandListener::FirewallCmd::parseFirewallType(const char* arg) {
    if (!strcmp(arg, "whitelist")) {
        return WHITELIST;
    } else if (!strcmp(arg, "blacklist")) {
        return BLACKLIST;
    } else {
        ALOGE("failed to parse firewall type (%s)", arg);
        return BLACKLIST;
    }
}

ChildChain CommandListener::FirewallCmd::parseChildChain(const char* arg) {
    if (!strcmp(arg, "dozable")) {
        return DOZABLE;
    } else if (!strcmp(arg, "standby")) {
        return STANDBY;
    } else if (!strcmp(arg, "powersave")) {
        return POWERSAVE;
    } else if (!strcmp(arg, "none")) {
        return NONE;
    } else {
        ALOGE("failed to parse child firewall chain (%s)", arg);
        return INVALID_CHAIN;
    }
}

int CommandListener::FirewallCmd::runCommand(SocketClient *cli, int argc,
        char **argv) {
    if (argc < 2) {
        cli->sendMsg(ResponseCode::CommandSyntaxError, "Missing command", false);
        return 0;
    }

    if (!strcmp(argv[1], "enable")) {
        if (argc != 3) {
            cli->sendMsg(ResponseCode::CommandSyntaxError,
                        "Usage: firewall enable <whitelist|blacklist>", false);
            return 0;
        }
        FirewallType firewallType = parseFirewallType(argv[2]);

        int res = gCtls->firewallCtrl.enableFirewall(firewallType);
        return sendGenericOkFail(cli, res);
    }
    if (!strcmp(argv[1], "disable")) {
        int res = gCtls->firewallCtrl.disableFirewall();
        return sendGenericOkFail(cli, res);
    }
    if (!strcmp(argv[1], "is_enabled")) {
        int res = gCtls->firewallCtrl.isFirewallEnabled();
        return sendGenericOkFail(cli, res);
    }

    if (!strcmp(argv[1], "set_interface_rule")) {
        if (argc != 4) {
            cli->sendMsg(ResponseCode::CommandSyntaxError,
                         "Usage: firewall set_interface_rule <rmnet0> <allow|deny>", false);
            return 0;
        }

        const char* iface = argv[2];
        FirewallRule rule = parseRule(argv[3]);

        int res = gCtls->firewallCtrl.setInterfaceRule(iface, rule);
        return sendGenericOkFail(cli, res);
    }

    if (!strcmp(argv[1], "set_egress_source_rule")) {
        if (argc != 4) {
            cli->sendMsg(ResponseCode::CommandSyntaxError,
                         "Usage: firewall set_egress_source_rule <192.168.0.1> <allow|deny>",
                         false);
            return 0;
        }

        const char* addr = argv[2];
        FirewallRule rule = parseRule(argv[3]);

        int res = gCtls->firewallCtrl.setEgressSourceRule(addr, rule);
        return sendGenericOkFail(cli, res);
    }

    if (!strcmp(argv[1], "set_egress_dest_rule")) {
        if (argc != 5) {
            cli->sendMsg(ResponseCode::CommandSyntaxError,
                         "Usage: firewall set_egress_dest_rule <192.168.0.1> <80> <allow|deny>",
                         false);
            return 0;
        }

        const char* addr = argv[2];
        int port = atoi(argv[3]);
        FirewallRule rule = parseRule(argv[4]);

        int res = 0;
        res |= gCtls->firewallCtrl.setEgressDestRule(addr, PROTOCOL_TCP, port, rule);
        res |= gCtls->firewallCtrl.setEgressDestRule(addr, PROTOCOL_UDP, port, rule);
        return sendGenericOkFail(cli, res);
    }

    if (!strcmp(argv[1], "set_uid_rule")) {
        if (argc != 5) {
            cli->sendMsg(ResponseCode::CommandSyntaxError,
                         "Usage: firewall set_uid_rule <dozable|standby|none> <1000> <allow|deny>",
                         false);
            return 0;
        }

        ChildChain childChain = parseChildChain(argv[2]);
        if (childChain == INVALID_CHAIN) {
            cli->sendMsg(ResponseCode::CommandSyntaxError,
                         "Invalid chain name. Valid names are: <dozable|standby|none>",
                         false);
            return 0;
        }
        int uid = atoi(argv[3]);
        FirewallRule rule = parseRule(argv[4]);
        int res = gCtls->firewallCtrl.setUidRule(childChain, uid, rule);
        return sendGenericOkFail(cli, res);
    }

    if (!strcmp(argv[1], "enable_chain")) {
        if (argc != 3) {
            cli->sendMsg(ResponseCode::CommandSyntaxError,
                         "Usage: firewall enable_chain <dozable|standby>",
                         false);
            return 0;
        }

        ChildChain childChain = parseChildChain(argv[2]);
        int res = gCtls->firewallCtrl.enableChildChains(childChain, true);
        return sendGenericOkFail(cli, res);
    }

    if (!strcmp(argv[1], "disable_chain")) {
        if (argc != 3) {
            cli->sendMsg(ResponseCode::CommandSyntaxError,
                         "Usage: firewall disable_chain <dozable|standby>",
                         false);
            return 0;
        }

        ChildChain childChain = parseChildChain(argv[2]);
        int res = gCtls->firewallCtrl.enableChildChains(childChain, false);
        return sendGenericOkFail(cli, res);
    }

    cli->sendMsg(ResponseCode::CommandSyntaxError, "Unknown command", false);
    return 0;
}

CommandListener::ClatdCmd::ClatdCmd() : NetdCommand("clatd") {
}

int CommandListener::ClatdCmd::runCommand(SocketClient *cli, int argc,
                                                            char **argv) {
    int rc = 0;
    if (argc < 3) {
        cli->sendMsg(ResponseCode::CommandSyntaxError, "Missing argument", false);
        return 0;
    }

    if (!strcmp(argv[1], "stop")) {
        rc = gCtls->clatdCtrl.stopClatd(argv[2]);
    } else if (!strcmp(argv[1], "status")) {
        char *tmp = NULL;
        asprintf(&tmp, "Clatd status: %s", (gCtls->clatdCtrl.isClatdStarted(argv[2]) ?
                                            "started" : "stopped"));
        cli->sendMsg(ResponseCode::ClatdStatusResult, tmp, false);
        free(tmp);
        return 0;
    } else if (!strcmp(argv[1], "start")) {
        rc = gCtls->clatdCtrl.startClatd(argv[2]);
    } else {
        cli->sendMsg(ResponseCode::CommandSyntaxError, "Unknown clatd cmd", false);
        return 0;
    }

    if (!rc) {
        cli->sendMsg(ResponseCode::CommandOkay, "Clatd operation succeeded", false);
    } else {
        cli->sendMsg(ResponseCode::OperationFailed, "Clatd operation failed", false);
    }

    return 0;
}

CommandListener::StrictCmd::StrictCmd() :
    NetdCommand("strict") {
}

int CommandListener::StrictCmd::sendGenericOkFail(SocketClient *cli, int cond) {
    if (!cond) {
        cli->sendMsg(ResponseCode::CommandOkay, "Strict command succeeded", false);
    } else {
        cli->sendMsg(ResponseCode::OperationFailed, "Strict command failed", false);
    }
    return 0;
}

StrictPenalty CommandListener::StrictCmd::parsePenalty(const char* arg) {
    if (!strcmp(arg, "reject")) {
        return REJECT;
    } else if (!strcmp(arg, "log")) {
        return LOG;
    } else if (!strcmp(arg, "accept")) {
        return ACCEPT;
    } else {
        return INVALID;
    }
}

int CommandListener::StrictCmd::runCommand(SocketClient *cli, int argc,
        char **argv) {
    if (argc < 2) {
        cli->sendMsg(ResponseCode::CommandSyntaxError, "Missing command", false);
        return 0;
    }

    if (!strcmp(argv[1], "enable")) {
        int res = gCtls->strictCtrl.enableStrict();
        return sendGenericOkFail(cli, res);
    }
    if (!strcmp(argv[1], "disable")) {
        int res = gCtls->strictCtrl.disableStrict();
        return sendGenericOkFail(cli, res);
    }

    if (!strcmp(argv[1], "set_uid_cleartext_policy")) {
        if (argc != 4) {
            cli->sendMsg(ResponseCode::CommandSyntaxError,
                         "Usage: strict set_uid_cleartext_policy <uid> <accept|log|reject>",
                         false);
            return 0;
        }

        errno = 0;
        unsigned long int uid = strtoul(argv[2], NULL, 0);
        if (errno || uid > UID_MAX) {
            cli->sendMsg(ResponseCode::CommandSyntaxError, "Invalid UID", false);
            return 0;
        }

        StrictPenalty penalty = parsePenalty(argv[3]);
        if (penalty == INVALID) {
            cli->sendMsg(ResponseCode::CommandSyntaxError, "Invalid penalty argument", false);
            return 0;
        }

        int res = gCtls->strictCtrl.setUidCleartextPenalty((uid_t) uid, penalty);
        return sendGenericOkFail(cli, res);
    }

    cli->sendMsg(ResponseCode::CommandSyntaxError, "Unknown command", false);
    return 0;
}

CommandListener::NetworkCommand::NetworkCommand() : NetdCommand("network") {
}

int CommandListener::NetworkCommand::syntaxError(SocketClient* client, const char* message) {
    client->sendMsg(ResponseCode::CommandSyntaxError, message, false);
    return 0;
}

int CommandListener::NetworkCommand::operationError(SocketClient* client, const char* message,
                                                    int ret) {
    errno = -ret;
    client->sendMsg(ResponseCode::OperationFailed, message, true);
    return 0;
}

int CommandListener::NetworkCommand::success(SocketClient* client) {
    client->sendMsg(ResponseCode::CommandOkay, "success", false);
    return 0;
}

int CommandListener::NetworkCommand::runCommand(SocketClient* client, int argc, char** argv) {
    if (argc < 2) {
        return syntaxError(client, "Missing argument");
    }

    //    0      1      2      3      4       5         6            7           8
    // network route [legacy <uid>]  add   <netId> <interface> <destination> [nexthop]
    // network route [legacy <uid>] remove <netId> <interface> <destination> [nexthop]
    //
    // nexthop may be either an IPv4/IPv6 address or one of "unreachable" or "throw".
    if (!strcmp(argv[1], "route")) {
        if (argc < 6 || argc > 9) {
            return syntaxError(client, "Incorrect number of arguments");
        }

        int nextArg = 2;
        bool legacy = false;
        uid_t uid = 0;
        if (!strcmp(argv[nextArg], "legacy")) {
            ++nextArg;
            legacy = true;
            uid = strtoul(argv[nextArg++], NULL, 0);
        }

        bool add = false;
        if (!strcmp(argv[nextArg], "add")) {
            add = true;
        } else if (strcmp(argv[nextArg], "remove")) {
            return syntaxError(client, "Unknown argument");
        }
        ++nextArg;

        if (argc < nextArg + 3 || argc > nextArg + 4) {
            return syntaxError(client, "Incorrect number of arguments");
        }

        unsigned netId = stringToNetId(argv[nextArg++]);
        const char* interface = argv[nextArg++];
        const char* destination = argv[nextArg++];
        const char* nexthop = argc > nextArg ? argv[nextArg] : NULL;

        int ret;
        if (add) {
            ret = gCtls->netCtrl.addRoute(netId, interface, destination, nexthop, legacy, uid);
        } else {
            ret = gCtls->netCtrl.removeRoute(netId, interface, destination, nexthop, legacy, uid);
        }
        if (ret) {
            return operationError(client, add ? "addRoute() failed" : "removeRoute() failed", ret);
        }

        return success(client);
    }

    //    0        1       2       3         4
    // network interface  add   <netId> <interface>
    // network interface remove <netId> <interface>
    if (!strcmp(argv[1], "interface")) {
        if (argc != 5) {
            return syntaxError(client, "Missing argument");
        }
        unsigned netId = stringToNetId(argv[3]);
        if (!strcmp(argv[2], "add")) {
            if (int ret = gCtls->netCtrl.addInterfaceToNetwork(netId, argv[4])) {
                return operationError(client, "addInterfaceToNetwork() failed", ret);
            }
        } else if (!strcmp(argv[2], "remove")) {
            if (int ret = gCtls->netCtrl.removeInterfaceFromNetwork(netId, argv[4])) {
                return operationError(client, "removeInterfaceFromNetwork() failed", ret);
            }
        } else {
            return syntaxError(client, "Unknown argument");
        }
        return success(client);
    }

    //    0      1       2         3
    // network create <netId> [permission]
    //
    //    0      1       2     3     4        5
    // network create <netId> vpn <hasDns> <secure>
    if (!strcmp(argv[1], "create")) {
        if (argc < 3) {
            return syntaxError(client, "Missing argument");
        }
        unsigned netId = stringToNetId(argv[2]);
        if (argc == 6 && !strcmp(argv[3], "vpn")) {
            bool hasDns = atoi(argv[4]);
            bool secure = atoi(argv[5]);
            if (int ret = gCtls->netCtrl.createVirtualNetwork(netId, hasDns, secure)) {
                return operationError(client, "createVirtualNetwork() failed", ret);
            }
        } else if (argc > 4) {
            return syntaxError(client, "Unknown trailing argument(s)");
        } else {
            Permission permission = PERMISSION_NONE;
            if (argc == 4) {
                permission = stringToPermission(argv[3]);
                if (permission == PERMISSION_NONE) {
                    return syntaxError(client, "Unknown permission");
                }
            }
            if (int ret = gCtls->netCtrl.createPhysicalNetwork(netId, permission)) {
                return operationError(client, "createPhysicalNetwork() failed", ret);
            }
        }
        return success(client);
    }

    //    0       1       2
    // network destroy <netId>
    if (!strcmp(argv[1], "destroy")) {
        if (argc != 3) {
            return syntaxError(client, "Incorrect number of arguments");
        }
        unsigned netId = stringToNetId(argv[2]);
        if (int ret = gCtls->netCtrl.destroyNetwork(netId)) {
            return operationError(client, "destroyNetwork() failed", ret);
        }
        return success(client);
    }

    //    0       1      2      3
    // network default  set  <netId>
    // network default clear
    if (!strcmp(argv[1], "default")) {
        if (argc < 3) {
            return syntaxError(client, "Missing argument");
        }
        unsigned netId = NETID_UNSET;
        if (!strcmp(argv[2], "set")) {
            if (argc < 4) {
                return syntaxError(client, "Missing netId");
            }
            netId = stringToNetId(argv[3]);
        } else if (strcmp(argv[2], "clear")) {
            return syntaxError(client, "Unknown argument");
        }
        if (int ret = gCtls->netCtrl.setDefaultNetwork(netId)) {
            return operationError(client, "setDefaultNetwork() failed", ret);
        }
        return success(client);
    }

    //    0        1         2      3        4          5
    // network permission   user   set  <permission>  <uid> ...
    // network permission   user  clear    <uid> ...
    // network permission network  set  <permission> <netId> ...
    // network permission network clear   <netId> ...
    if (!strcmp(argv[1], "permission")) {
        if (argc < 5) {
            return syntaxError(client, "Missing argument");
        }
        int nextArg = 4;
        Permission permission = PERMISSION_NONE;
        if (!strcmp(argv[3], "set")) {
            permission = stringToPermission(argv[4]);
            if (permission == PERMISSION_NONE) {
                return syntaxError(client, "Unknown permission");
            }
            nextArg = 5;
        } else if (strcmp(argv[3], "clear")) {
            return syntaxError(client, "Unknown argument");
        }
        if (nextArg == argc) {
            return syntaxError(client, "Missing id");
        }

        bool userPermissions = !strcmp(argv[2], "user");
        bool networkPermissions = !strcmp(argv[2], "network");
        if (!userPermissions && !networkPermissions) {
            return syntaxError(client, "Unknown argument");
        }

        std::vector<unsigned> ids;
        for (; nextArg < argc; ++nextArg) {
            if (userPermissions) {
                char* endPtr;
                unsigned id = strtoul(argv[nextArg], &endPtr, 0);
                if (!*argv[nextArg] || *endPtr) {
                    return syntaxError(client, "Invalid id");
                }
                ids.push_back(id);
            } else {
                // networkPermissions
                ids.push_back(stringToNetId(argv[nextArg]));
            }
        }
        if (userPermissions) {
            gCtls->netCtrl.setPermissionForUsers(permission, ids);
        } else {
            // networkPermissions
            if (int ret = gCtls->netCtrl.setPermissionForNetworks(permission, ids)) {
                return operationError(client, "setPermissionForNetworks() failed", ret);
            }
        }

        return success(client);
    }

    //    0      1     2       3           4
    // network users  add   <netId> [<uid>[-<uid>]] ...
    // network users remove <netId> [<uid>[-<uid>]] ...
    if (!strcmp(argv[1], "users")) {
        if (argc < 4) {
            return syntaxError(client, "Missing argument");
        }
        unsigned netId = stringToNetId(argv[3]);
        UidRanges uidRanges;
        if (!uidRanges.parseFrom(argc - 4, argv + 4)) {
            return syntaxError(client, "Invalid UIDs");
        }
        if (!strcmp(argv[2], "add")) {
            if (int ret = gCtls->netCtrl.addUsersToNetwork(netId, uidRanges)) {
                return operationError(client, "addUsersToNetwork() failed", ret);
            }
        } else if (!strcmp(argv[2], "remove")) {
            if (int ret = gCtls->netCtrl.removeUsersFromNetwork(netId, uidRanges)) {
                return operationError(client, "removeUsersFromNetwork() failed", ret);
            }
        } else {
            return syntaxError(client, "Unknown argument");
        }
        return success(client);
    }

    //    0       1      2     3
    // network protect allow <uid> ...
    // network protect  deny <uid> ...
    if (!strcmp(argv[1], "protect")) {
        if (argc < 4) {
            return syntaxError(client, "Missing argument");
        }
        std::vector<uid_t> uids;
        for (int i = 3; i < argc; ++i) {
            uids.push_back(strtoul(argv[i], NULL, 0));
        }
        if (!strcmp(argv[2], "allow")) {
            gCtls->netCtrl.allowProtect(uids);
        } else if (!strcmp(argv[2], "deny")) {
            gCtls->netCtrl.denyProtect(uids);
        } else {
            return syntaxError(client, "Unknown argument");
        }
        return success(client);
    }

    return syntaxError(client, "Unknown argument");
}
