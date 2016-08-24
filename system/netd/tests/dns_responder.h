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

#ifndef DNS_RESPONDER_H
#define DNS_RESPONDER_H

#include <arpa/nameser.h>

#include <atomic>
#include <mutex>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

#include <android-base/thread_annotations.h>

namespace test {

struct DNSHeader;
struct DNSQuestion;
struct DNSRecord;

/*
 * Simple DNS responder, which replies to queries with the registered response
 * for that type. Class is assumed to be IN. If no response is registered, the
 * default error response code is returned.
 */
class DNSResponder {
public:
    DNSResponder(std::string listen_address, std::string listen_service,
                 int poll_timeout_ms, uint16_t error_rcode,
                 double response_probability);
    ~DNSResponder();
    void addMapping(const char* name, ns_type type, const char* addr);
    void removeMapping(const char* name, ns_type type);
    void setResponseProbability(double response_probability);
    bool running() const;
    bool startServer();
    bool stopServer();
    const std::string& listen_address() const {
        return listen_address_;
    }
    const std::string& listen_service() const {
        return listen_service_;
    }
    std::vector<std::pair<std::string, ns_type>> queries() const;
    void clearQueries();

private:
    // Key used for accessing mappings.
    struct QueryKey {
        std::string name;
        unsigned type;
        QueryKey(std::string n, unsigned t) : name(n), type(t) {}
        bool operator == (const QueryKey& o) const {
            return name == o.name && type == o.type;
        }
        bool operator < (const QueryKey& o) const {
            if (name < o.name) return true;
            if (name > o.name) return false;
            return type < o.type;
        }
    };

    struct QueryKeyHash {
        size_t operator() (const QueryKey& key) const {
            return std::hash<std::string>()(key.name) +
                   static_cast<size_t>(key.type);
        }
    };

    // DNS request handler.
    void requestHandler();

    // Parses and generates a response message for incoming DNS requests.
    // Returns false on parsing errors.
    bool handleDNSRequest(const char* buffer, ssize_t buffer_len,
                          char* response, size_t* response_len) const;

    bool addAnswerRecords(const DNSQuestion& question,
                          std::vector<DNSRecord>* answers) const;

    bool generateErrorResponse(DNSHeader* header, ns_rcode rcode,
                               char* response, size_t* response_len) const;
    bool makeErrorResponse(DNSHeader* header, ns_rcode rcode, char* response,
                           size_t* response_len) const;


    // Address and service to listen on, currently limited to UDP.
    const std::string listen_address_;
    const std::string listen_service_;
    // epoll_wait() timeout in ms.
    const int poll_timeout_ms_;
    // Error code to return for requests for an unknown name.
    const uint16_t error_rcode_;
    // Probability that a valid response is being sent instead of being sent
    // instead of returning error_rcode_.
    std::atomic<double> response_probability_;

    // Mappings from (name, type) to registered response and the
    // mutex protecting them.
    std::unordered_map<QueryKey, std::string, QueryKeyHash> mappings_
        GUARDED_BY(mappings_mutex_);
    // TODO(imaipi): enable GUARDED_BY(mappings_mutex_);
    std::mutex mappings_mutex_;
    // Query names received so far and the corresponding mutex.
    mutable std::vector<std::pair<std::string, ns_type>> queries_
        GUARDED_BY(queries_mutex_);
    mutable std::mutex queries_mutex_;
    // Socket on which the server is listening.
    int socket_;
    // File descriptor for epoll.
    int epoll_fd_;
    // Signal for request handler termination.
    std::atomic<bool> terminate_ GUARDED_BY(update_mutex_);
    // Thread for handling incoming threads.
    std::thread handler_thread_ GUARDED_BY(update_mutex_);
    std::mutex update_mutex_;
};

}  // namespace test

#endif  // DNS_RESPONDER_H
