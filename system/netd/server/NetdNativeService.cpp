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

#define LOG_TAG "Netd"

#include <vector>

#include <android-base/stringprintf.h>
#include <cutils/log.h>
#include <utils/Errors.h>
#include <utils/String16.h>

#include <binder/IPCThreadState.h>
#include <binder/IServiceManager.h>
#include "android/net/BnNetd.h"

#include "Controllers.h"
#include "DumpWriter.h"
#include "NetdConstants.h"
#include "NetdNativeService.h"
#include "RouteController.h"
#include "SockDiag.h"
#include "UidRanges.h"

using android::base::StringPrintf;

namespace android {
namespace net {

namespace {

const char CONNECTIVITY_INTERNAL[] = "android.permission.CONNECTIVITY_INTERNAL";
const char DUMP[] = "android.permission.DUMP";

binder::Status checkPermission(const char *permission) {
    pid_t pid;
    uid_t uid;

    if (checkCallingPermission(String16(permission), (int32_t *) &pid, (int32_t *) &uid)) {
        return binder::Status::ok();
    } else {
        auto err = StringPrintf("UID %d / PID %d lacks permission %s", uid, pid, permission);
        return binder::Status::fromExceptionCode(binder::Status::EX_SECURITY, String8(err.c_str()));
    }
}

#define ENFORCE_PERMISSION(permission) {                    \
    binder::Status status = checkPermission((permission));  \
    if (!status.isOk()) {                                   \
        return status;                                      \
    }                                                       \
}

#define NETD_LOCKING_RPC(permission, lock)                  \
    ENFORCE_PERMISSION(permission);                         \
    android::RWLock::AutoWLock _lock(lock);

#define NETD_BIG_LOCK_RPC(permission) NETD_LOCKING_RPC((permission), gBigNetdLock)
}  // namespace


status_t NetdNativeService::start() {
    IPCThreadState::self()->disableBackgroundScheduling(true);
    status_t ret = BinderService<NetdNativeService>::publish();
    if (ret != android::OK) {
        return ret;
    }
    sp<ProcessState> ps(ProcessState::self());
    ps->startThreadPool();
    ps->giveThreadPoolName();
    return android::OK;
}

status_t NetdNativeService::dump(int fd, const Vector<String16> & /* args */) {
    const binder::Status dump_permission = checkPermission(DUMP);
    if (!dump_permission.isOk()) {
        const String8 msg(dump_permission.toString8());
        write(fd, msg.string(), msg.size());
        return PERMISSION_DENIED;
    }

    // This method does not grab any locks. If individual classes need locking
    // their dump() methods MUST handle locking appropriately.
    DumpWriter dw(fd);
    dw.blankline();
    gCtls->netCtrl.dump(dw);
    dw.blankline();

    return NO_ERROR;
}

binder::Status NetdNativeService::isAlive(bool *alive) {
    NETD_BIG_LOCK_RPC(CONNECTIVITY_INTERNAL);

    *alive = true;
    return binder::Status::ok();
}

binder::Status NetdNativeService::firewallReplaceUidChain(const android::String16& chainName,
        bool isWhitelist, const std::vector<int32_t>& uids, bool *ret) {
    NETD_LOCKING_RPC(CONNECTIVITY_INTERNAL, gCtls->firewallCtrl.lock);

    android::String8 name = android::String8(chainName);
    int err = gCtls->firewallCtrl.replaceUidChain(name.string(), isWhitelist, uids);
    *ret = (err == 0);
    return binder::Status::ok();
}

binder::Status NetdNativeService::bandwidthEnableDataSaver(bool enable, bool *ret) {
    NETD_LOCKING_RPC(CONNECTIVITY_INTERNAL, gCtls->bandwidthCtrl.lock);

    int err = gCtls->bandwidthCtrl.enableDataSaver(enable);
    *ret = (err == 0);
    return binder::Status::ok();
}

binder::Status NetdNativeService::networkRejectNonSecureVpn(bool add,
        const std::vector<UidRange>& uidRangeArray) {
    // TODO: elsewhere RouteController is only used from the tethering and network controllers, so
    // it should be possible to use the same lock as NetworkController. However, every call through
    // the CommandListener "network" command will need to hold this lock too, not just the ones that
    // read/modify network internal state (that is sufficient for ::dump() because it doesn't
    // look at routes, but it's not enough here).
    NETD_BIG_LOCK_RPC(CONNECTIVITY_INTERNAL);

    UidRanges uidRanges(uidRangeArray);

    int err;
    if (add) {
        err = RouteController::addUsersToRejectNonSecureNetworkRule(uidRanges);
    } else {
        err = RouteController::removeUsersFromRejectNonSecureNetworkRule(uidRanges);
    }

    if (err != 0) {
        return binder::Status::fromServiceSpecificError(-err,
                String8::format("RouteController error: %s", strerror(-err)));
    }
    return binder::Status::ok();
}

binder::Status NetdNativeService::socketDestroy(const std::vector<UidRange>& uids,
        const std::vector<int32_t>& skipUids) {

    ENFORCE_PERMISSION(CONNECTIVITY_INTERNAL);

    SockDiag sd;
    if (!sd.open()) {
        return binder::Status::fromServiceSpecificError(EIO,
                String8("Could not open SOCK_DIAG socket"));
    }

    UidRanges uidRanges(uids);
    int err = sd.destroySockets(uidRanges, std::set<uid_t>(skipUids.begin(), skipUids.end()));

    if (err) {
        return binder::Status::fromServiceSpecificError(-err,
                String8::format("destroySockets: %s", strerror(-err)));
    }
    return binder::Status::ok();
}

binder::Status NetdNativeService::setResolverConfiguration(int32_t netId,
        const std::vector<std::string>& servers, const std::vector<std::string>& domains,
        const std::vector<int32_t>& params) {
    // This function intentionally does not lock within Netd, as Bionic is thread-safe.
    ENFORCE_PERMISSION(CONNECTIVITY_INTERNAL);

    int err = gCtls->resolverCtrl.setResolverConfiguration(netId, servers, domains, params);
    if (err != 0) {
        return binder::Status::fromServiceSpecificError(-err,
                String8::format("ResolverController error: %s", strerror(-err)));
    }
    return binder::Status::ok();
}

binder::Status NetdNativeService::getResolverInfo(int32_t netId,
        std::vector<std::string>* servers, std::vector<std::string>* domains,
        std::vector<int32_t>* params, std::vector<int32_t>* stats) {
    // This function intentionally does not lock within Netd, as Bionic is thread-safe.
    ENFORCE_PERMISSION(CONNECTIVITY_INTERNAL);

    int err = gCtls->resolverCtrl.getResolverInfo(netId, servers, domains, params, stats);
    if (err != 0) {
        return binder::Status::fromServiceSpecificError(-err,
                String8::format("ResolverController error: %s", strerror(-err)));
    }
    return binder::Status::ok();
}

}  // namespace net
}  // namespace android
