/*
 * Copyright (C) 2016 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless requied by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */

#include <arpa/inet.h>
#include <errno.h>
#include <netdb.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <cutils/sockets.h>
#include <android-base/stringprintf.h>
#include <private/android_filesystem_config.h>

#include <algorithm>
#include <chrono>
#include <iterator>
#include <numeric>
#include <thread>

#define LOG_TAG "netd_test"
// TODO: make this dynamic and stop depending on implementation details.
#define TEST_OEM_NETWORK "oem29"
#define TEST_NETID 30

#include "NetdClient.h"

#include <gtest/gtest.h>

#include <utils/Log.h>

#include <testUtil.h>

#include "dns_responder.h"
#include "resolv_params.h"
#include "ResolverStats.h"

#include "android/net/INetd.h"
#include "binder/IServiceManager.h"

using android::base::StringPrintf;
using android::base::StringAppendF;
using android::net::ResolverStats;

// Emulates the behavior of UnorderedElementsAreArray, which currently cannot be used.
// TODO: Use UnorderedElementsAreArray, which depends on being able to compile libgmock_host,
// if that is not possible, improve this hacky algorithm, which is O(n**2)
template <class A, class B>
bool UnorderedCompareArray(const A& a, const B& b) {
    if (a.size() != b.size()) return false;
    for (const auto& a_elem : a) {
        size_t a_count = 0;
        for (const auto& a_elem2 : a) {
            if (a_elem == a_elem2) {
                ++a_count;
            }
        }
        size_t b_count = 0;
        for (const auto& b_elem : b) {
            if (a_elem == b_elem) ++b_count;
        }
        if (a_count != b_count) return false;
    }
    return true;
}

// The only response code used in this test, see
// frameworks/base/services/java/com/android/server/NetworkManagementService.java
// for others.
static constexpr int ResponseCodeOK = 200;

// Returns ResponseCode.
int netdCommand(const char* sockname, const char* command) {
    int sock = socket_local_client(sockname,
                                   ANDROID_SOCKET_NAMESPACE_RESERVED,
                                   SOCK_STREAM);
    if (sock < 0) {
        perror("Error connecting");
        return -1;
    }

    // FrameworkListener expects the whole command in one read.
    char buffer[256];
    int nwritten = snprintf(buffer, sizeof(buffer), "0 %s", command);
    if (write(sock, buffer, nwritten + 1) < 0) {
        perror("Error sending netd command");
        close(sock);
        return -1;
    }

    int nread = read(sock, buffer, sizeof(buffer));
    if (nread < 0) {
        perror("Error reading response");
        close(sock);
        return -1;
    }
    close(sock);
    return atoi(buffer);
}

bool expectNetdResult(int expected, const char* sockname, const char* format, ...) {
    char command[256];
    va_list args;
    va_start(args, format);
    vsnprintf(command, sizeof(command), format, args);
    va_end(args);
    int result = netdCommand(sockname, command);
    EXPECT_EQ(expected, result) << command;
    return (200 <= expected && expected < 300);
}

class AddrInfo {
  public:
    AddrInfo() : ai_(nullptr), error_(0) {}

    AddrInfo(const char* node, const char* service, const addrinfo& hints) : ai_(nullptr) {
        init(node, service, hints);
    }

    AddrInfo(const char* node, const char* service) : ai_(nullptr) {
        init(node, service);
    }

    ~AddrInfo() { clear(); }

    int init(const char* node, const char* service, const addrinfo& hints) {
        clear();
        error_ = getaddrinfo(node, service, &hints, &ai_);
        return error_;
    }

    int init(const char* node, const char* service) {
        clear();
        error_ = getaddrinfo(node, service, nullptr, &ai_);
        return error_;
    }

    void clear() {
        if (ai_ != nullptr) {
            freeaddrinfo(ai_);
            ai_ = nullptr;
            error_ = 0;
        }
    }

    const addrinfo& operator*() const { return *ai_; }
    const addrinfo* get() const { return ai_; }
    const addrinfo* operator&() const { return ai_; }
    int error() const { return error_; }

  private:
    addrinfo* ai_;
    int error_;
};

class ResolverTest : public ::testing::Test {
protected:
    struct Mapping {
        std::string host;
        std::string entry;
        std::string ip4;
        std::string ip6;
    };

    virtual void SetUp() {
        // Ensure resolutions go via proxy.
        setenv("ANDROID_DNS_MODE", "", 1);
        uid = getuid();
        pid = getpid();
        SetupOemNetwork();

        // binder setup
        auto binder = android::defaultServiceManager()->getService(android::String16("netd"));
        ASSERT_TRUE(binder != nullptr);
        mNetdSrv = android::interface_cast<android::net::INetd>(binder);
    }

    virtual void TearDown() {
        TearDownOemNetwork();
        netdCommand("netd", "network destroy " TEST_OEM_NETWORK);
    }

    void SetupOemNetwork() {
        netdCommand("netd", "network destroy " TEST_OEM_NETWORK);
        if (expectNetdResult(ResponseCodeOK, "netd",
                             "network create %s", TEST_OEM_NETWORK)) {
            oemNetId = TEST_NETID;
        }
        setNetworkForProcess(oemNetId);
        ASSERT_EQ((unsigned) oemNetId, getNetworkForProcess());
    }

    void SetupMappings(unsigned num_hosts, const std::vector<std::string>& domains,
            std::vector<Mapping>* mappings) const {
        mappings->resize(num_hosts * domains.size());
        auto mappings_it = mappings->begin();
        for (unsigned i = 0 ; i < num_hosts ; ++i) {
            for (const auto& domain : domains) {
                ASSERT_TRUE(mappings_it != mappings->end());
                mappings_it->host = StringPrintf("host%u", i);
                mappings_it->entry = StringPrintf("%s.%s.", mappings_it->host.c_str(),
                        domain.c_str());
                mappings_it->ip4 = StringPrintf("192.0.2.%u", i%253 + 1);
                mappings_it->ip6 = StringPrintf("2001:db8::%x", i%65534 + 1);
                ++mappings_it;
            }
        }
    }

    void SetupDNSServers(unsigned num_servers, const std::vector<Mapping>& mappings,
            std::vector<std::unique_ptr<test::DNSResponder>>* dns,
            std::vector<std::string>* servers) const {
        ASSERT_TRUE(num_servers != 0 && num_servers < 100);
        const char* listen_srv = "53";
        dns->resize(num_servers);
        servers->resize(num_servers);
        for (unsigned i = 0 ; i < num_servers ; ++i) {
            auto& server = (*servers)[i];
            auto& d = (*dns)[i];
            server = StringPrintf("127.0.0.%u", i + 100);
            d = std::make_unique<test::DNSResponder>(server, listen_srv, 250,
                    ns_rcode::ns_r_servfail, 1.0);
            ASSERT_TRUE(d.get() != nullptr);
            for (const auto& mapping : mappings) {
                d->addMapping(mapping.entry.c_str(), ns_type::ns_t_a, mapping.ip4.c_str());
                d->addMapping(mapping.entry.c_str(), ns_type::ns_t_aaaa, mapping.ip6.c_str());
            }
            ASSERT_TRUE(d->startServer());
        }
    }

    void ShutdownDNSServers(std::vector<std::unique_ptr<test::DNSResponder>>* dns) const {
        for (const auto& d : *dns) {
            ASSERT_TRUE(d.get() != nullptr);
            d->stopServer();
        }
        dns->clear();
    }

    void TearDownOemNetwork() {
        if (oemNetId != -1) {
            expectNetdResult(ResponseCodeOK, "netd",
                             "network destroy %s", TEST_OEM_NETWORK);
        }
    }

    bool SetResolversForNetwork(const std::vector<std::string>& servers,
            const std::vector<std::string>& domains, const std::vector<int>& params) {
        auto rv = mNetdSrv->setResolverConfiguration(TEST_NETID, servers, domains, params);
        return rv.isOk();
    }

    bool SetResolversForNetwork(const std::vector<std::string>& searchDomains,
            const std::vector<std::string>& servers, const std::string& params) {
        std::string cmd = StringPrintf("resolver setnetdns %d \"", oemNetId);
        if (!searchDomains.empty()) {
            cmd += searchDomains[0].c_str();
            for (size_t i = 1 ; i < searchDomains.size() ; ++i) {
                cmd += " ";
                cmd += searchDomains[i];
            }
        }
        cmd += "\"";

        for (const auto& str : servers) {
            cmd += " ";
            cmd += str;
        }

        if (!params.empty()) {
            cmd += " --params \"";
            cmd += params;
            cmd += "\"";
        }

        int rv = netdCommand("netd", cmd.c_str());
        if (rv != ResponseCodeOK) {
            return false;
        }
        return true;
    }

    bool GetResolverInfo(std::vector<std::string>* servers, std::vector<std::string>* domains,
            __res_params* params, std::vector<ResolverStats>* stats) {
        using android::net::INetd;
        std::vector<int32_t> params32;
        std::vector<int32_t> stats32;
        auto rv = mNetdSrv->getResolverInfo(TEST_NETID, servers, domains, &params32, &stats32);
        if (!rv.isOk() || params32.size() != INetd::RESOLVER_PARAMS_COUNT) {
            return false;
        }
        *params = __res_params {
            .sample_validity = static_cast<uint16_t>(
                    params32[INetd::RESOLVER_PARAMS_SAMPLE_VALIDITY]),
            .success_threshold = static_cast<uint8_t>(
                    params32[INetd::RESOLVER_PARAMS_SUCCESS_THRESHOLD]),
            .min_samples = static_cast<uint8_t>(
                    params32[INetd::RESOLVER_PARAMS_MIN_SAMPLES]),
            .max_samples = static_cast<uint8_t>(
                    params32[INetd::RESOLVER_PARAMS_MAX_SAMPLES])
        };
        return ResolverStats::decodeAll(stats32, stats);
    }

    std::string ToString(const hostent* he) const {
        if (he == nullptr) return "<null>";
        char buffer[INET6_ADDRSTRLEN];
        if (!inet_ntop(he->h_addrtype, he->h_addr_list[0], buffer, sizeof(buffer))) {
            return "<invalid>";
        }
        return buffer;
    }

    std::string ToString(const addrinfo* ai) const {
        if (!ai)
            return "<null>";
        for (const auto* aip = ai ; aip != nullptr ; aip = aip->ai_next) {
            char host[NI_MAXHOST];
            int rv = getnameinfo(aip->ai_addr, aip->ai_addrlen, host, sizeof(host), nullptr, 0,
                    NI_NUMERICHOST);
            if (rv != 0)
                return gai_strerror(rv);
            return host;
        }
        return "<invalid>";
    }

    size_t GetNumQueries(const test::DNSResponder& dns, const char* name) const {
        auto queries = dns.queries();
        size_t found = 0;
        for (const auto& p : queries) {
            if (p.first == name) {
                ++found;
            }
        }
        return found;
    }

    size_t GetNumQueriesForType(const test::DNSResponder& dns, ns_type type,
            const char* name) const {
        auto queries = dns.queries();
        size_t found = 0;
        for (const auto& p : queries) {
            if (p.second == type && p.first == name) {
                ++found;
            }
        }
        return found;
    }

    void RunGetAddrInfoStressTest_Binder(unsigned num_hosts, unsigned num_threads,
            unsigned num_queries) {
        std::vector<std::string> domains = { "example.com" };
        std::vector<std::unique_ptr<test::DNSResponder>> dns;
        std::vector<std::string> servers;
        std::vector<Mapping> mappings;
        ASSERT_NO_FATAL_FAILURE(SetupMappings(num_hosts, domains, &mappings));
        ASSERT_NO_FATAL_FAILURE(SetupDNSServers(MAXNS, mappings, &dns, &servers));

        ASSERT_TRUE(SetResolversForNetwork(servers, domains, mDefaultParams_Binder));

        auto t0 = std::chrono::steady_clock::now();
        std::vector<std::thread> threads(num_threads);
        for (std::thread& thread : threads) {
           thread = std::thread([this, &servers, &dns, &mappings, num_queries]() {
                for (unsigned i = 0 ; i < num_queries ; ++i) {
                    uint32_t ofs = arc4random_uniform(mappings.size());
                    ASSERT_TRUE(ofs < mappings.size());
                    auto& mapping = mappings[i];
                    addrinfo* result = nullptr;
                    int rv = getaddrinfo(mapping.host.c_str(), nullptr, nullptr, &result);
                    EXPECT_EQ(0, rv) << "error [" << rv << "] " << gai_strerror(rv);
                    if (rv == 0) {
                        std::string result_str = ToString(result);
                        EXPECT_TRUE(result_str == mapping.ip4 || result_str == mapping.ip6)
                            << "result='" << result_str << "', ip4='" << mapping.ip4
                            << "', ip6='" << mapping.ip6;
                    }
                    if (result) {
                        freeaddrinfo(result);
                        result = nullptr;
                    }
                }
            });
        }

        for (std::thread& thread : threads) {
            thread.join();
        }
        auto t1 = std::chrono::steady_clock::now();
        ALOGI("%u hosts, %u threads, %u queries, %Es", num_hosts, num_threads, num_queries,
                std::chrono::duration<double>(t1 - t0).count());
        ASSERT_NO_FATAL_FAILURE(ShutdownDNSServers(&dns));
    }

    int pid;
    int uid;
    int oemNetId = -1;
    android::sp<android::net::INetd> mNetdSrv = nullptr;
    const std::vector<std::string> mDefaultSearchDomains = { "example.com" };
    // <sample validity in s> <success threshold in percent> <min samples> <max samples>
    const std::string mDefaultParams = "300 25 8 8";
    const std::vector<int> mDefaultParams_Binder = { 300, 25, 8, 8 };
};

TEST_F(ResolverTest, GetHostByName) {
    const char* listen_addr = "127.0.0.3";
    const char* listen_srv = "53";
    const char* host_name = "hello.example.com.";
    test::DNSResponder dns(listen_addr, listen_srv, 250, ns_rcode::ns_r_servfail, 1.0);
    dns.addMapping(host_name, ns_type::ns_t_a, "1.2.3.3");
    ASSERT_TRUE(dns.startServer());
    std::vector<std::string> servers = { listen_addr };
    ASSERT_TRUE(SetResolversForNetwork(mDefaultSearchDomains, servers, mDefaultParams));

    dns.clearQueries();
    const hostent* result = gethostbyname("hello");
    EXPECT_EQ(1U, GetNumQueriesForType(dns, ns_type::ns_t_a, host_name));
    ASSERT_FALSE(result == nullptr);
    ASSERT_EQ(4, result->h_length);
    ASSERT_FALSE(result->h_addr_list[0] == nullptr);
    EXPECT_EQ("1.2.3.3", ToString(result));
    EXPECT_TRUE(result->h_addr_list[1] == nullptr);
    dns.stopServer();
}

TEST_F(ResolverTest, TestBinderSerialization) {
    using android::net::INetd;
    std::vector<int> params_offsets = {
        INetd::RESOLVER_PARAMS_SAMPLE_VALIDITY,
        INetd::RESOLVER_PARAMS_SUCCESS_THRESHOLD,
        INetd::RESOLVER_PARAMS_MIN_SAMPLES,
        INetd::RESOLVER_PARAMS_MAX_SAMPLES
    };
    int size = static_cast<int>(params_offsets.size());
    EXPECT_EQ(size, INetd::RESOLVER_PARAMS_COUNT);
    std::sort(params_offsets.begin(), params_offsets.end());
    for (int i = 0 ; i < size ; ++i) {
        EXPECT_EQ(params_offsets[i], i);
    }
}

TEST_F(ResolverTest, GetHostByName_Binder) {
    using android::net::INetd;

    std::vector<std::string> domains = { "example.com" };
    std::vector<std::unique_ptr<test::DNSResponder>> dns;
    std::vector<std::string> servers;
    std::vector<Mapping> mappings;
    ASSERT_NO_FATAL_FAILURE(SetupMappings(1, domains, &mappings));
    ASSERT_NO_FATAL_FAILURE(SetupDNSServers(4, mappings, &dns, &servers));
    ASSERT_EQ(1U, mappings.size());
    const Mapping& mapping = mappings[0];

    ASSERT_TRUE(SetResolversForNetwork(servers, domains, mDefaultParams_Binder));

    const hostent* result = gethostbyname(mapping.host.c_str());
    size_t total_queries = std::accumulate(dns.begin(), dns.end(), 0,
            [this, &mapping](size_t total, auto& d) {
                return total + GetNumQueriesForType(*d, ns_type::ns_t_a, mapping.entry.c_str());
            });

    EXPECT_LE(1U, total_queries);
    ASSERT_FALSE(result == nullptr);
    ASSERT_EQ(4, result->h_length);
    ASSERT_FALSE(result->h_addr_list[0] == nullptr);
    EXPECT_EQ(mapping.ip4, ToString(result));
    EXPECT_TRUE(result->h_addr_list[1] == nullptr);

    std::vector<std::string> res_servers;
    std::vector<std::string> res_domains;
    __res_params res_params;
    std::vector<ResolverStats> res_stats;
    ASSERT_TRUE(GetResolverInfo(&res_servers, &res_domains, &res_params, &res_stats));
    EXPECT_EQ(servers.size(), res_servers.size());
    EXPECT_EQ(domains.size(), res_domains.size());
    ASSERT_EQ(INetd::RESOLVER_PARAMS_COUNT, mDefaultParams_Binder.size());
    EXPECT_EQ(mDefaultParams_Binder[INetd::RESOLVER_PARAMS_SAMPLE_VALIDITY],
            res_params.sample_validity);
    EXPECT_EQ(mDefaultParams_Binder[INetd::RESOLVER_PARAMS_SUCCESS_THRESHOLD],
            res_params.success_threshold);
    EXPECT_EQ(mDefaultParams_Binder[INetd::RESOLVER_PARAMS_MIN_SAMPLES], res_params.min_samples);
    EXPECT_EQ(mDefaultParams_Binder[INetd::RESOLVER_PARAMS_MAX_SAMPLES], res_params.max_samples);
    EXPECT_EQ(servers.size(), res_stats.size());

    EXPECT_TRUE(UnorderedCompareArray(res_servers, servers));
    EXPECT_TRUE(UnorderedCompareArray(res_domains, domains));

    ASSERT_NO_FATAL_FAILURE(ShutdownDNSServers(&dns));
}

TEST_F(ResolverTest, GetAddrInfo) {
    addrinfo* result = nullptr;

    const char* listen_addr = "127.0.0.4";
    const char* listen_addr2 = "127.0.0.5";
    const char* listen_srv = "53";
    const char* host_name = "howdy.example.com.";
    test::DNSResponder dns(listen_addr, listen_srv, 250,
                           ns_rcode::ns_r_servfail, 1.0);
    dns.addMapping(host_name, ns_type::ns_t_a, "1.2.3.4");
    dns.addMapping(host_name, ns_type::ns_t_aaaa, "::1.2.3.4");
    ASSERT_TRUE(dns.startServer());

    test::DNSResponder dns2(listen_addr2, listen_srv, 250,
                            ns_rcode::ns_r_servfail, 1.0);
    dns2.addMapping(host_name, ns_type::ns_t_a, "1.2.3.4");
    dns2.addMapping(host_name, ns_type::ns_t_aaaa, "::1.2.3.4");
    ASSERT_TRUE(dns2.startServer());

    for (size_t i = 0 ; i < 1000 ; ++i) {
        std::vector<std::string> servers = { listen_addr };
        ASSERT_TRUE(SetResolversForNetwork(mDefaultSearchDomains, servers, mDefaultParams));
        dns.clearQueries();
        dns2.clearQueries();

        EXPECT_EQ(0, getaddrinfo("howdy", nullptr, nullptr, &result));
        size_t found = GetNumQueries(dns, host_name);
        EXPECT_LE(1U, found);
        // Could be A or AAAA
        std::string result_str = ToString(result);
        EXPECT_TRUE(result_str == "1.2.3.4" || result_str == "::1.2.3.4")
            << ", result_str='" << result_str << "'";
        // TODO: Use ScopedAddrinfo or similar once it is available in a common header file.
        if (result) {
            freeaddrinfo(result);
            result = nullptr;
        }

        // Verify that the name is cached.
        size_t old_found = found;
        EXPECT_EQ(0, getaddrinfo("howdy", nullptr, nullptr, &result));
        found = GetNumQueries(dns, host_name);
        EXPECT_LE(1U, found);
        EXPECT_EQ(old_found, found);
        result_str = ToString(result);
        EXPECT_TRUE(result_str == "1.2.3.4" || result_str == "::1.2.3.4")
            << result_str;
        if (result) {
            freeaddrinfo(result);
            result = nullptr;
        }

        // Change the DNS resolver, ensure that queries are no longer cached.
        servers = { listen_addr2 };
        ASSERT_TRUE(SetResolversForNetwork(mDefaultSearchDomains, servers, mDefaultParams));
        dns.clearQueries();
        dns2.clearQueries();

        EXPECT_EQ(0, getaddrinfo("howdy", nullptr, nullptr, &result));
        found = GetNumQueries(dns, host_name);
        size_t found2 = GetNumQueries(dns2, host_name);
        EXPECT_EQ(0U, found);
        EXPECT_LE(1U, found2);

        // Could be A or AAAA
        result_str = ToString(result);
        EXPECT_TRUE(result_str == "1.2.3.4" || result_str == "::1.2.3.4")
            << ", result_str='" << result_str << "'";
        if (result) {
            freeaddrinfo(result);
            result = nullptr;
        }
    }
    dns.stopServer();
    dns2.stopServer();
}

TEST_F(ResolverTest, GetAddrInfoV4) {
    addrinfo* result = nullptr;

    const char* listen_addr = "127.0.0.5";
    const char* listen_srv = "53";
    const char* host_name = "hola.example.com.";
    test::DNSResponder dns(listen_addr, listen_srv, 250,
                           ns_rcode::ns_r_servfail, 1.0);
    dns.addMapping(host_name, ns_type::ns_t_a, "1.2.3.5");
    ASSERT_TRUE(dns.startServer());
    std::vector<std::string> servers = { listen_addr };
    ASSERT_TRUE(SetResolversForNetwork(mDefaultSearchDomains, servers, mDefaultParams));

    addrinfo hints;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    EXPECT_EQ(0, getaddrinfo("hola", nullptr, &hints, &result));
    EXPECT_EQ(1U, GetNumQueries(dns, host_name));
    EXPECT_EQ("1.2.3.5", ToString(result));
    if (result) {
        freeaddrinfo(result);
        result = nullptr;
    }
}

TEST_F(ResolverTest, MultidomainResolution) {
    std::vector<std::string> searchDomains = { "example1.com", "example2.com", "example3.com" };
    const char* listen_addr = "127.0.0.6";
    const char* listen_srv = "53";
    const char* host_name = "nihao.example2.com.";
    test::DNSResponder dns(listen_addr, listen_srv, 250,
                           ns_rcode::ns_r_servfail, 1.0);
    dns.addMapping(host_name, ns_type::ns_t_a, "1.2.3.3");
    ASSERT_TRUE(dns.startServer());
    std::vector<std::string> servers = { listen_addr };
    ASSERT_TRUE(SetResolversForNetwork(searchDomains, servers, mDefaultParams));

    dns.clearQueries();
    const hostent* result = gethostbyname("nihao");
    EXPECT_EQ(1U, GetNumQueriesForType(dns, ns_type::ns_t_a, host_name));
    ASSERT_FALSE(result == nullptr);
    ASSERT_EQ(4, result->h_length);
    ASSERT_FALSE(result->h_addr_list[0] == nullptr);
    EXPECT_EQ("1.2.3.3", ToString(result));
    EXPECT_TRUE(result->h_addr_list[1] == nullptr);
    dns.stopServer();
}

TEST_F(ResolverTest, GetAddrInfoV6_failing) {
    addrinfo* result = nullptr;

    const char* listen_addr0 = "127.0.0.7";
    const char* listen_addr1 = "127.0.0.8";
    const char* listen_srv = "53";
    const char* host_name = "ohayou.example.com.";
    test::DNSResponder dns0(listen_addr0, listen_srv, 250,
                            ns_rcode::ns_r_servfail, 0.0);
    test::DNSResponder dns1(listen_addr1, listen_srv, 250,
                            ns_rcode::ns_r_servfail, 1.0);
    dns0.addMapping(host_name, ns_type::ns_t_aaaa, "2001:db8::5");
    dns1.addMapping(host_name, ns_type::ns_t_aaaa, "2001:db8::6");
    ASSERT_TRUE(dns0.startServer());
    ASSERT_TRUE(dns1.startServer());
    std::vector<std::string> servers = { listen_addr0, listen_addr1 };
    // <sample validity in s> <success threshold in percent> <min samples> <max samples>
    unsigned sample_validity = 300;
    int success_threshold = 25;
    int sample_count = 8;
    std::string params = StringPrintf("%u %d %d %d", sample_validity, success_threshold,
            sample_count, sample_count);
    ASSERT_TRUE(SetResolversForNetwork(mDefaultSearchDomains, servers, params));

    // Repeatedly perform resolutions for non-existing domains until MAXNSSAMPLES resolutions have
    // reached the dns0, which is set to fail. No more requests should then arrive at that server
    // for the next sample_lifetime seconds.
    // TODO: This approach is implementation-dependent, change once metrics reporting is available.
    addrinfo hints;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET6;
    for (int i = 0 ; i < sample_count ; ++i) {
        std::string domain = StringPrintf("nonexistent%d", i);
        getaddrinfo(domain.c_str(), nullptr, &hints, &result);
        if (result) {
            freeaddrinfo(result);
            result = nullptr;
        }
    }
    // Due to 100% errors for all possible samples, the server should be ignored from now on and
    // only the second one used for all following queries, until NSSAMPLE_VALIDITY is reached.
    dns0.clearQueries();
    dns1.clearQueries();
    EXPECT_EQ(0, getaddrinfo("ohayou", nullptr, &hints, &result));
    EXPECT_EQ(0U, GetNumQueries(dns0, host_name));
    EXPECT_EQ(1U, GetNumQueries(dns1, host_name));
    if (result) {
        freeaddrinfo(result);
        result = nullptr;
    }
}

TEST_F(ResolverTest, GetAddrInfoV6_concurrent) {
    const char* listen_addr0 = "127.0.0.9";
    const char* listen_addr1 = "127.0.0.10";
    const char* listen_addr2 = "127.0.0.11";
    const char* listen_srv = "53";
    const char* host_name = "konbanha.example.com.";
    test::DNSResponder dns0(listen_addr0, listen_srv, 250,
                            ns_rcode::ns_r_servfail, 1.0);
    test::DNSResponder dns1(listen_addr1, listen_srv, 250,
                            ns_rcode::ns_r_servfail, 1.0);
    test::DNSResponder dns2(listen_addr2, listen_srv, 250,
                            ns_rcode::ns_r_servfail, 1.0);
    dns0.addMapping(host_name, ns_type::ns_t_aaaa, "2001:db8::5");
    dns1.addMapping(host_name, ns_type::ns_t_aaaa, "2001:db8::6");
    dns2.addMapping(host_name, ns_type::ns_t_aaaa, "2001:db8::7");
    ASSERT_TRUE(dns0.startServer());
    ASSERT_TRUE(dns1.startServer());
    ASSERT_TRUE(dns2.startServer());
    const std::vector<std::string> servers = { listen_addr0, listen_addr1, listen_addr2 };
    std::vector<std::thread> threads(10);
    for (std::thread& thread : threads) {
       thread = std::thread([this, &servers, &dns0, &dns1, &dns2]() {
            unsigned delay = arc4random_uniform(1*1000*1000); // <= 1s
            usleep(delay);
            std::vector<std::string> serverSubset;
            for (const auto& server : servers) {
                if (arc4random_uniform(2)) {
                    serverSubset.push_back(server);
                }
            }
            if (serverSubset.empty()) serverSubset = servers;
            ASSERT_TRUE(SetResolversForNetwork(mDefaultSearchDomains, serverSubset,
                    mDefaultParams));
            addrinfo hints;
            memset(&hints, 0, sizeof(hints));
            hints.ai_family = AF_INET6;
            addrinfo* result = nullptr;
            int rv = getaddrinfo("konbanha", nullptr, &hints, &result);
            EXPECT_EQ(0, rv) << "error [" << rv << "] " << gai_strerror(rv);
            if (result) {
                freeaddrinfo(result);
                result = nullptr;
            }
        });
    }
    for (std::thread& thread : threads) {
        thread.join();
    }
}

TEST_F(ResolverTest, GetAddrInfoStressTest_Binder_100) {
    const unsigned num_hosts = 100;
    const unsigned num_threads = 100;
    const unsigned num_queries = 100;
    ASSERT_NO_FATAL_FAILURE(RunGetAddrInfoStressTest_Binder(num_hosts, num_threads, num_queries));
}

TEST_F(ResolverTest, GetAddrInfoStressTest_Binder_100000) {
    const unsigned num_hosts = 100000;
    const unsigned num_threads = 100;
    const unsigned num_queries = 100;
    ASSERT_NO_FATAL_FAILURE(RunGetAddrInfoStressTest_Binder(num_hosts, num_threads, num_queries));
}

TEST_F(ResolverTest, EmptySetup) {
    using android::net::INetd;
    std::vector<std::string> servers;
    std::vector<std::string> domains;
    ASSERT_TRUE(SetResolversForNetwork(servers, domains, mDefaultParams_Binder));
    std::vector<std::string> res_servers;
    std::vector<std::string> res_domains;
    __res_params res_params;
    std::vector<ResolverStats> res_stats;
    ASSERT_TRUE(GetResolverInfo(&res_servers, &res_domains, &res_params, &res_stats));
    EXPECT_EQ(0U, res_servers.size());
    EXPECT_EQ(0U, res_domains.size());
    ASSERT_EQ(INetd::RESOLVER_PARAMS_COUNT, mDefaultParams_Binder.size());
    EXPECT_EQ(mDefaultParams_Binder[INetd::RESOLVER_PARAMS_SAMPLE_VALIDITY],
            res_params.sample_validity);
    EXPECT_EQ(mDefaultParams_Binder[INetd::RESOLVER_PARAMS_SUCCESS_THRESHOLD],
            res_params.success_threshold);
    EXPECT_EQ(mDefaultParams_Binder[INetd::RESOLVER_PARAMS_MIN_SAMPLES], res_params.min_samples);
    EXPECT_EQ(mDefaultParams_Binder[INetd::RESOLVER_PARAMS_MAX_SAMPLES], res_params.max_samples);
}

TEST_F(ResolverTest, SearchPathChange) {
    addrinfo* result = nullptr;

    const char* listen_addr = "127.0.0.13";
    const char* listen_srv = "53";
    const char* host_name1 = "test13.domain1.org.";
    const char* host_name2 = "test13.domain2.org.";
    test::DNSResponder dns(listen_addr, listen_srv, 250,
                           ns_rcode::ns_r_servfail, 1.0);
    dns.addMapping(host_name1, ns_type::ns_t_aaaa, "2001:db8::13");
    dns.addMapping(host_name2, ns_type::ns_t_aaaa, "2001:db8::1:13");
    ASSERT_TRUE(dns.startServer());
    std::vector<std::string> servers = { listen_addr };
    std::vector<std::string> domains = { "domain1.org" };
    ASSERT_TRUE(SetResolversForNetwork(domains, servers, mDefaultParams));

    addrinfo hints;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET6;
    EXPECT_EQ(0, getaddrinfo("test13", nullptr, &hints, &result));
    EXPECT_EQ(1U, dns.queries().size());
    EXPECT_EQ(1U, GetNumQueries(dns, host_name1));
    EXPECT_EQ("2001:db8::13", ToString(result));
    if (result) freeaddrinfo(result);

    // Test that changing the domain search path on its own works.
    domains = { "domain2.org" };
    ASSERT_TRUE(SetResolversForNetwork(domains, servers, mDefaultParams));
    dns.clearQueries();

    EXPECT_EQ(0, getaddrinfo("test13", nullptr, &hints, &result));
    EXPECT_EQ(1U, dns.queries().size());
    EXPECT_EQ(1U, GetNumQueries(dns, host_name2));
    EXPECT_EQ("2001:db8::1:13", ToString(result));
    if (result) freeaddrinfo(result);
}

TEST_F(ResolverTest, MaxServerPrune_Binder) {
    using android::net::INetd;

    std::vector<std::string> domains = { "example.com" };
    std::vector<std::unique_ptr<test::DNSResponder>> dns;
    std::vector<std::string> servers;
    std::vector<Mapping> mappings;
    ASSERT_NO_FATAL_FAILURE(SetupMappings(1, domains, &mappings));
    ASSERT_NO_FATAL_FAILURE(SetupDNSServers(MAXNS + 1, mappings, &dns, &servers));

    ASSERT_TRUE(SetResolversForNetwork(servers, domains,  mDefaultParams_Binder));

    std::vector<std::string> res_servers;
    std::vector<std::string> res_domains;
    __res_params res_params;
    std::vector<ResolverStats> res_stats;
    ASSERT_TRUE(GetResolverInfo(&res_servers, &res_domains, &res_params, &res_stats));
    EXPECT_EQ(static_cast<size_t>(MAXNS), res_servers.size());

    ASSERT_NO_FATAL_FAILURE(ShutdownDNSServers(&dns));
}
