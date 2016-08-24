/*
 * Copyright 2016 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * binder_test.cpp - unit tests for netd binder RPCs.
 */

#include <cerrno>
#include <cinttypes>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <set>
#include <vector>

#include <sys/socket.h>
#include <netinet/in.h>

#include <android-base/stringprintf.h>
#include <android-base/strings.h>
#include <cutils/multiuser.h>
#include <gtest/gtest.h>
#include <logwrap/logwrap.h>

#include "NetdConstants.h"
#include "android/net/INetd.h"
#include "android/net/UidRange.h"
#include "binder/IServiceManager.h"

using namespace android;
using namespace android::base;
using namespace android::binder;
using android::net::INetd;
using android::net::UidRange;

static const char* IP_RULE_V4 = "-4";
static const char* IP_RULE_V6 = "-6";

class BinderTest : public ::testing::Test {

public:
    BinderTest() {
        sp<IServiceManager> sm = defaultServiceManager();
        sp<IBinder> binder = sm->getService(String16("netd"));
        if (binder != nullptr) {
            mNetd = interface_cast<INetd>(binder);
        }
    }

    void SetUp() {
        ASSERT_NE(nullptr, mNetd.get());
    }

protected:
    sp<INetd> mNetd;
};


class TimedOperation : public Stopwatch {
public:
    TimedOperation(std::string name): mName(name) {}
    virtual ~TimedOperation() {
        fprintf(stderr, "    %s: %6.1f ms\n", mName.c_str(), timeTaken());
    }

private:
    std::string mName;
};

TEST_F(BinderTest, TestIsAlive) {
    TimedOperation t("isAlive RPC");
    bool isAlive = false;
    mNetd->isAlive(&isAlive);
    ASSERT_TRUE(isAlive);
}

static int randomUid() {
    return 100000 * arc4random_uniform(7) + 10000 + arc4random_uniform(5000);
}

static std::vector<std::string> runCommand(const std::string& command) {
    std::vector<std::string> lines;
    FILE *f;

    if ((f = popen(command.c_str(), "r")) == nullptr) {
        perror("popen");
        return lines;
    }

    char *line = nullptr;
    size_t bufsize = 0;
    ssize_t linelen = 0;
    while ((linelen = getline(&line, &bufsize, f)) >= 0) {
        lines.push_back(std::string(line, linelen));
        free(line);
        line = nullptr;
    }

    pclose(f);
    return lines;
}

static std::vector<std::string> listIpRules(const char *ipVersion) {
    std::string command = StringPrintf("%s %s rule list", IP_PATH, ipVersion);
    return runCommand(command);
}

static std::vector<std::string> listIptablesRule(const char *binary, const char *chainName) {
    std::string command = StringPrintf("%s -n -L %s", binary, chainName);
    return runCommand(command);
}

static int iptablesRuleLineLength(const char *binary, const char *chainName) {
    return listIptablesRule(binary, chainName).size();
}

TEST_F(BinderTest, TestFirewallReplaceUidChain) {
    std::string chainName = StringPrintf("netd_binder_test_%u", arc4random_uniform(10000));
    const int kNumUids = 500;
    std::vector<int32_t> noUids(0);
    std::vector<int32_t> uids(kNumUids);
    for (int i = 0; i < kNumUids; i++) {
        uids[i] = randomUid();
    }

    bool ret;
    {
        TimedOperation op(StringPrintf("Programming %d-UID whitelist chain", kNumUids));
        mNetd->firewallReplaceUidChain(String16(chainName.c_str()), true, uids, &ret);
    }
    EXPECT_EQ(true, ret);
    EXPECT_EQ((int) uids.size() + 5, iptablesRuleLineLength(IPTABLES_PATH, chainName.c_str()));
    EXPECT_EQ((int) uids.size() + 11, iptablesRuleLineLength(IP6TABLES_PATH, chainName.c_str()));
    {
        TimedOperation op("Clearing whitelist chain");
        mNetd->firewallReplaceUidChain(String16(chainName.c_str()), false, noUids, &ret);
    }
    EXPECT_EQ(true, ret);
    EXPECT_EQ(3, iptablesRuleLineLength(IPTABLES_PATH, chainName.c_str()));
    EXPECT_EQ(3, iptablesRuleLineLength(IP6TABLES_PATH, chainName.c_str()));

    {
        TimedOperation op(StringPrintf("Programming %d-UID blacklist chain", kNumUids));
        mNetd->firewallReplaceUidChain(String16(chainName.c_str()), false, uids, &ret);
    }
    EXPECT_EQ(true, ret);
    EXPECT_EQ((int) uids.size() + 3, iptablesRuleLineLength(IPTABLES_PATH, chainName.c_str()));
    EXPECT_EQ((int) uids.size() + 3, iptablesRuleLineLength(IP6TABLES_PATH, chainName.c_str()));

    {
        TimedOperation op("Clearing blacklist chain");
        mNetd->firewallReplaceUidChain(String16(chainName.c_str()), false, noUids, &ret);
    }
    EXPECT_EQ(true, ret);
    EXPECT_EQ(3, iptablesRuleLineLength(IPTABLES_PATH, chainName.c_str()));
    EXPECT_EQ(3, iptablesRuleLineLength(IP6TABLES_PATH, chainName.c_str()));

    // Check that the call fails if iptables returns an error.
    std::string veryLongStringName = "netd_binder_test_UnacceptablyLongIptablesChainName";
    mNetd->firewallReplaceUidChain(String16(veryLongStringName.c_str()), true, noUids, &ret);
    EXPECT_EQ(false, ret);
}

static int bandwidthDataSaverEnabled(const char *binary) {
    std::vector<std::string> lines = listIptablesRule(binary, "bw_data_saver");

    // Output looks like this:
    //
    // Chain bw_data_saver (1 references)
    // target     prot opt source               destination
    // RETURN     all  --  0.0.0.0/0            0.0.0.0/0
    EXPECT_EQ(3U, lines.size());
    if (lines.size() != 3) return -1;

    EXPECT_TRUE(android::base::StartsWith(lines[2], "RETURN ") ||
                android::base::StartsWith(lines[2], "REJECT "));

    return android::base::StartsWith(lines[2], "REJECT");
}

bool enableDataSaver(sp<INetd>& netd, bool enable) {
    TimedOperation op(enable ? " Enabling data saver" : "Disabling data saver");
    bool ret;
    netd->bandwidthEnableDataSaver(enable, &ret);
    return ret;
}

int getDataSaverState() {
    const int enabled4 = bandwidthDataSaverEnabled(IPTABLES_PATH);
    const int enabled6 = bandwidthDataSaverEnabled(IP6TABLES_PATH);
    EXPECT_EQ(enabled4, enabled6);
    EXPECT_NE(-1, enabled4);
    EXPECT_NE(-1, enabled6);
    if (enabled4 != enabled6 || (enabled6 != 0 && enabled6 != 1)) {
        return -1;
    }
    return enabled6;
}

TEST_F(BinderTest, TestBandwidthEnableDataSaver) {
    const int wasEnabled = getDataSaverState();
    ASSERT_NE(-1, wasEnabled);

    if (wasEnabled) {
        ASSERT_TRUE(enableDataSaver(mNetd, false));
        EXPECT_EQ(0, getDataSaverState());
    }

    ASSERT_TRUE(enableDataSaver(mNetd, false));
    EXPECT_EQ(0, getDataSaverState());

    ASSERT_TRUE(enableDataSaver(mNetd, true));
    EXPECT_EQ(1, getDataSaverState());

    ASSERT_TRUE(enableDataSaver(mNetd, true));
    EXPECT_EQ(1, getDataSaverState());

    if (!wasEnabled) {
        ASSERT_TRUE(enableDataSaver(mNetd, false));
        EXPECT_EQ(0, getDataSaverState());
    }
}

static bool ipRuleExistsForRange(const uint32_t priority, const UidRange& range,
        const std::string& action, const char* ipVersion) {
    // Output looks like this:
    //   "12500:\tfrom all fwmark 0x0/0x20000 iif lo uidrange 1000-2000 prohibit"
    std::vector<std::string> rules = listIpRules(ipVersion);

    std::string prefix = StringPrintf("%" PRIu32 ":", priority);
    std::string suffix = StringPrintf(" iif lo uidrange %d-%d %s\n",
            range.getStart(), range.getStop(), action.c_str());
    for (std::string line : rules) {
        if (android::base::StartsWith(line, prefix.c_str())
                && android::base::EndsWith(line, suffix.c_str())) {
            return true;
        }
    }
    return false;
}

static bool ipRuleExistsForRange(const uint32_t priority, const UidRange& range,
        const std::string& action) {
    bool existsIp4 = ipRuleExistsForRange(priority, range, action, IP_RULE_V4);
    bool existsIp6 = ipRuleExistsForRange(priority, range, action, IP_RULE_V6);
    EXPECT_EQ(existsIp4, existsIp6);
    return existsIp4;
}

TEST_F(BinderTest, TestNetworkRejectNonSecureVpn) {
    constexpr uint32_t RULE_PRIORITY = 12500;

    constexpr int baseUid = MULTIUSER_APP_PER_USER_RANGE * 5;
    std::vector<UidRange> uidRanges = {
        {baseUid + 150, baseUid + 224},
        {baseUid + 226, baseUid + 300}
    };

    const std::vector<std::string> initialRulesV4 = listIpRules(IP_RULE_V4);
    const std::vector<std::string> initialRulesV6 = listIpRules(IP_RULE_V6);

    // Create two valid rules.
    ASSERT_TRUE(mNetd->networkRejectNonSecureVpn(true, uidRanges).isOk());
    EXPECT_EQ(initialRulesV4.size() + 2, listIpRules(IP_RULE_V4).size());
    EXPECT_EQ(initialRulesV6.size() + 2, listIpRules(IP_RULE_V6).size());
    for (auto const& range : uidRanges) {
        EXPECT_TRUE(ipRuleExistsForRange(RULE_PRIORITY, range, "prohibit"));
    }

    // Remove the rules.
    ASSERT_TRUE(mNetd->networkRejectNonSecureVpn(false, uidRanges).isOk());
    EXPECT_EQ(initialRulesV4.size(), listIpRules(IP_RULE_V4).size());
    EXPECT_EQ(initialRulesV6.size(), listIpRules(IP_RULE_V6).size());
    for (auto const& range : uidRanges) {
        EXPECT_FALSE(ipRuleExistsForRange(RULE_PRIORITY, range, "prohibit"));
    }

    // Fail to remove the rules a second time after they are already deleted.
    binder::Status status = mNetd->networkRejectNonSecureVpn(false, uidRanges);
    ASSERT_EQ(binder::Status::EX_SERVICE_SPECIFIC, status.exceptionCode());
    EXPECT_EQ(ENOENT, status.serviceSpecificErrorCode());

    // All rules should be the same as before.
    EXPECT_EQ(initialRulesV4, listIpRules(IP_RULE_V4));
    EXPECT_EQ(initialRulesV6, listIpRules(IP_RULE_V6));
}

void socketpair(int *clientSocket, int *serverSocket, int *acceptedSocket) {
    *serverSocket = socket(AF_INET6, SOCK_STREAM, 0);
    struct sockaddr_in6 server6 = { .sin6_family = AF_INET6 };
    ASSERT_EQ(0, bind(*serverSocket, (struct sockaddr *) &server6, sizeof(server6)));

    socklen_t addrlen = sizeof(server6);
    ASSERT_EQ(0, getsockname(*serverSocket, (struct sockaddr *) &server6, &addrlen));
    ASSERT_EQ(0, listen(*serverSocket, 10));

    *clientSocket = socket(AF_INET6, SOCK_STREAM, 0);
    struct sockaddr_in6 client6;
    ASSERT_EQ(0, connect(*clientSocket, (struct sockaddr *) &server6, sizeof(server6)));
    ASSERT_EQ(0, getsockname(*clientSocket, (struct sockaddr *) &client6, &addrlen));

    *acceptedSocket = accept(*serverSocket, (struct sockaddr *) &server6, &addrlen);
    ASSERT_NE(-1, *acceptedSocket);

    ASSERT_EQ(0, memcmp(&client6, &server6, sizeof(client6)));
}

void checkSocketpairOpen(int clientSocket, int acceptedSocket) {
    char buf[4096];
    EXPECT_EQ(4, write(clientSocket, "foo", sizeof("foo")));
    EXPECT_EQ(4, read(acceptedSocket, buf, sizeof(buf)));
    EXPECT_EQ(0, memcmp(buf, "foo", sizeof("foo")));
}

void checkSocketpairClosed(int clientSocket, int acceptedSocket) {
    // Check that the client socket was closed with ECONNABORTED.
    int ret = write(clientSocket, "foo", sizeof("foo"));
    int err = errno;
    EXPECT_EQ(-1, ret);
    EXPECT_EQ(ECONNABORTED, err);

    // Check that it sent a RST to the server.
    ret = write(acceptedSocket, "foo", sizeof("foo"));
    err = errno;
    EXPECT_EQ(-1, ret);
    EXPECT_EQ(ECONNRESET, err);
}

TEST_F(BinderTest, TestSocketDestroy) {
    int clientSocket, serverSocket, acceptedSocket;
    ASSERT_NO_FATAL_FAILURE(socketpair(&clientSocket, &serverSocket, &acceptedSocket));

    // Pick a random UID in the system UID range.
    constexpr int baseUid = AID_APP - 2000;
    static_assert(baseUid > 0, "Not enough UIDs? Please fix this test.");
    int uid = baseUid + 500 + arc4random_uniform(1000);
    EXPECT_EQ(0, fchown(clientSocket, uid, -1));

    // UID ranges that don't contain uid.
    std::vector<UidRange> uidRanges = {
        {baseUid + 42, baseUid + 449},
        {baseUid + 1536, AID_APP - 4},
        {baseUid + 498, uid - 1},
        {uid + 1, baseUid + 1520},
    };
    // A skip list that doesn't contain UID.
    std::vector<int32_t> skipUids { baseUid + 123, baseUid + 1600 };

    // Close sockets. Our test socket should be intact.
    EXPECT_TRUE(mNetd->socketDestroy(uidRanges, skipUids).isOk());
    checkSocketpairOpen(clientSocket, acceptedSocket);

    // UID ranges that do contain uid.
    uidRanges = {
        {baseUid + 42, baseUid + 449},
        {baseUid + 1536, AID_APP - 4},
        {baseUid + 498, baseUid + 1520},
    };
    // Add uid to the skip list.
    skipUids.push_back(uid);

    // Close sockets. Our test socket should still be intact because it's in the skip list.
    EXPECT_TRUE(mNetd->socketDestroy(uidRanges, skipUids).isOk());
    checkSocketpairOpen(clientSocket, acceptedSocket);

    // Now remove uid from skipUids, and close sockets. Our test socket should have been closed.
    skipUids.resize(skipUids.size() - 1);
    EXPECT_TRUE(mNetd->socketDestroy(uidRanges, skipUids).isOk());
    checkSocketpairClosed(clientSocket, acceptedSocket);

    close(clientSocket);
    close(serverSocket);
    close(acceptedSocket);
}
