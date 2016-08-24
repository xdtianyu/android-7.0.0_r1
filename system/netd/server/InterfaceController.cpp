/*
 * Copyright (C) 2012 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *	  http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <dirent.h>
#include <errno.h>
#include <malloc.h>

#define LOG_TAG "InterfaceController"
#include <android-base/file.h>
#include <android-base/stringprintf.h>
#include <cutils/log.h>
#include <logwrap/logwrap.h>

#include "InterfaceController.h"
#include "RouteController.h"

using android::base::StringPrintf;
using android::base::WriteStringToFile;

namespace {

const char ipv6_proc_path[] = "/proc/sys/net/ipv6/conf";

const char ipv4_neigh_conf_dir[] = "/proc/sys/net/ipv4/neigh";

const char ipv6_neigh_conf_dir[] = "/proc/sys/net/ipv6/neigh";

const char sys_net_path[] = "/sys/class/net";

const char wl_util_path[] = "/vendor/xbin/wlutil";

bool isInterfaceName(const char *name) {
    return strcmp(name, ".") &&
            strcmp(name, "..") &&
            strcmp(name, "default") &&
            strcmp(name, "all");
}

int writeValueToPath(
        const char* dirname, const char* subdirname, const char* basename,
        const char* value) {
    std::string path(StringPrintf("%s/%s/%s", dirname, subdirname, basename));
    return WriteStringToFile(value, path) ? 0 : -1;
}

void setOnAllInterfaces(const char* dirname, const char* basename, const char* value) {
    // Set the default value, which is used by any interfaces that are created in the future.
    writeValueToPath(dirname, "default", basename, value);

    // Set the value on all the interfaces that currently exist.
    DIR* dir = opendir(dirname);
    if (!dir) {
        ALOGE("Can't list %s: %s", dirname, strerror(errno));
        return;
    }
    dirent* d;
    while ((d = readdir(dir))) {
        if ((d->d_type != DT_DIR) || !isInterfaceName(d->d_name)) {
            continue;
        }
        writeValueToPath(dirname, d->d_name, basename, value);
    }
    closedir(dir);
}

void setIPv6UseOutgoingInterfaceAddrsOnly(const char *value) {
    setOnAllInterfaces(ipv6_proc_path, "use_oif_addrs_only", value);
}

}  // namespace

InterfaceController::InterfaceController() {
	// Initial IPv6 settings.
	// By default, accept_ra is set to 1 (accept RAs unless forwarding is on) on all interfaces.
	// This causes RAs to work or not work based on whether forwarding is on, and causes routes
	// learned from RAs to go away when forwarding is turned on. Make this behaviour predictable
	// by always setting accept_ra to 2.
	setAcceptRA("2");

	setAcceptRARouteTable(-RouteController::ROUTE_TABLE_OFFSET_FROM_INDEX);

	// Enable optimistic DAD for IPv6 addresses on all interfaces.
	setIPv6OptimisticMode("1");

	// Reduce the ARP/ND base reachable time from the default (30sec) to 15sec.
	setBaseReachableTimeMs(15 * 1000);

	// When sending traffic via a given interface use only addresses configured
        // on that interface as possible source addresses.
	setIPv6UseOutgoingInterfaceAddrsOnly("1");
}

InterfaceController::~InterfaceController() {
}

int InterfaceController::setEnableIPv6(const char *interface, const int on) {
    if (!isIfaceName(interface)) {
        errno = ENOENT;
        return -1;
    }
    // When disable_ipv6 changes from 1 to 0, the kernel starts autoconf.
    // When disable_ipv6 changes from 0 to 1, the kernel clears all autoconf
    // addresses and routes and disables IPv6 on the interface.
    const char *disable_ipv6 = on ? "0" : "1";
    return writeValueToPath(ipv6_proc_path, interface, "disable_ipv6", disable_ipv6);
}

int InterfaceController::setIPv6PrivacyExtensions(const char *interface, const int on) {
    if (!isIfaceName(interface)) {
        errno = ENOENT;
        return -1;
    }
    // 0: disable IPv6 privacy addresses
    // 0: enable IPv6 privacy addresses and prefer them over non-privacy ones.
    return writeValueToPath(ipv6_proc_path, interface, "use_tempaddr", on ? "2" : "0");
}

// Enables or disables IPv6 ND offload. This is useful for 464xlat on wifi, IPv6 tethering, and
// generally implementing IPv6 neighbour discovery and duplicate address detection properly.
// TODO: This should be implemented in wpa_supplicant via driver commands instead.
int InterfaceController::setIPv6NdOffload(char* interface, const int on) {
    // Only supported on Broadcom chipsets via wlutil for now.
    if (access(wl_util_path, X_OK) == 0) {
        const char *argv[] = {
            wl_util_path,
            "-a",
            interface,
            "ndoe",
            on ? "1" : "0"
        };
        int ret = android_fork_execvp(ARRAY_SIZE(argv), const_cast<char**>(argv), NULL,
                                      false, false);
        ALOGD("%s ND offload on %s: %d (%s)",
              (on ? "enabling" : "disabling"), interface, ret, strerror(errno));
        return ret;
    } else {
        return 0;
    }
}

void InterfaceController::setAcceptRA(const char *value) {
    setOnAllInterfaces(ipv6_proc_path, "accept_ra", value);
}

// |tableOrOffset| is interpreted as:
//     If == 0: default. Routes go into RT6_TABLE_MAIN.
//     If > 0: user set. Routes go into the specified table.
//     If < 0: automatic. The absolute value is intepreted as an offset and added to the interface
//             ID to get the table. If it's set to -1000, routes from interface ID 5 will go into
//             table 1005, etc.
void InterfaceController::setAcceptRARouteTable(int tableOrOffset) {
    std::string value(StringPrintf("%d", tableOrOffset));
    setOnAllInterfaces(ipv6_proc_path, "accept_ra_rt_table", value.c_str());
}

int InterfaceController::setMtu(const char *interface, const char *mtu)
{
    if (!isIfaceName(interface)) {
        errno = ENOENT;
        return -1;
    }
    return writeValueToPath(sys_net_path, interface, "mtu", mtu);
}

void InterfaceController::setBaseReachableTimeMs(unsigned int millis) {
    std::string value(StringPrintf("%u", millis));
    setOnAllInterfaces(ipv4_neigh_conf_dir, "base_reachable_time_ms", value.c_str());
    setOnAllInterfaces(ipv6_neigh_conf_dir, "base_reachable_time_ms", value.c_str());
}

void InterfaceController::setIPv6OptimisticMode(const char *value) {
    setOnAllInterfaces(ipv6_proc_path, "optimistic_dad", value);
    setOnAllInterfaces(ipv6_proc_path, "use_optimistic", value);
}
