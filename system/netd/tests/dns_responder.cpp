/*
 * Copyright (C) 2016 The Android Open Source Project
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

#include "dns_responder.h"

#include <arpa/inet.h>
#include <fcntl.h>
#include <netdb.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include <iostream>
#include <vector>

#include <log/log.h>

namespace test {

std::string errno2str() {
    char error_msg[512] = { 0 };
    if (strerror_r(errno, error_msg, sizeof(error_msg)))
        return std::string();
    return std::string(error_msg);
}

#define APLOGI(fmt, ...) ALOGI(fmt ": [%d] %s", __VA_ARGS__, errno, errno2str().c_str())

std::string str2hex(const char* buffer, size_t len) {
    std::string str(len*2, '\0');
    for (size_t i = 0 ; i < len ; ++i) {
        static const char* hex = "0123456789ABCDEF";
        uint8_t c = buffer[i];
        str[i*2] = hex[c >> 4];
        str[i*2 + 1] = hex[c & 0x0F];
    }
    return str;
}

std::string addr2str(const sockaddr* sa, socklen_t sa_len) {
    char host_str[NI_MAXHOST] = { 0 };
    int rv = getnameinfo(sa, sa_len, host_str, sizeof(host_str), nullptr, 0,
                         NI_NUMERICHOST);
    if (rv == 0) return std::string(host_str);
    return std::string();
}

/* DNS struct helpers */

const char* dnstype2str(unsigned dnstype) {
    static std::unordered_map<unsigned, const char*> kTypeStrs = {
        { ns_type::ns_t_a, "A" },
        { ns_type::ns_t_ns, "NS" },
        { ns_type::ns_t_md, "MD" },
        { ns_type::ns_t_mf, "MF" },
        { ns_type::ns_t_cname, "CNAME" },
        { ns_type::ns_t_soa, "SOA" },
        { ns_type::ns_t_mb, "MB" },
        { ns_type::ns_t_mb, "MG" },
        { ns_type::ns_t_mr, "MR" },
        { ns_type::ns_t_null, "NULL" },
        { ns_type::ns_t_wks, "WKS" },
        { ns_type::ns_t_ptr, "PTR" },
        { ns_type::ns_t_hinfo, "HINFO" },
        { ns_type::ns_t_minfo, "MINFO" },
        { ns_type::ns_t_mx, "MX" },
        { ns_type::ns_t_txt, "TXT" },
        { ns_type::ns_t_rp, "RP" },
        { ns_type::ns_t_afsdb, "AFSDB" },
        { ns_type::ns_t_x25, "X25" },
        { ns_type::ns_t_isdn, "ISDN" },
        { ns_type::ns_t_rt, "RT" },
        { ns_type::ns_t_nsap, "NSAP" },
        { ns_type::ns_t_nsap_ptr, "NSAP-PTR" },
        { ns_type::ns_t_sig, "SIG" },
        { ns_type::ns_t_key, "KEY" },
        { ns_type::ns_t_px, "PX" },
        { ns_type::ns_t_gpos, "GPOS" },
        { ns_type::ns_t_aaaa, "AAAA" },
        { ns_type::ns_t_loc, "LOC" },
        { ns_type::ns_t_nxt, "NXT" },
        { ns_type::ns_t_eid, "EID" },
        { ns_type::ns_t_nimloc, "NIMLOC" },
        { ns_type::ns_t_srv, "SRV" },
        { ns_type::ns_t_naptr, "NAPTR" },
        { ns_type::ns_t_kx, "KX" },
        { ns_type::ns_t_cert, "CERT" },
        { ns_type::ns_t_a6, "A6" },
        { ns_type::ns_t_dname, "DNAME" },
        { ns_type::ns_t_sink, "SINK" },
        { ns_type::ns_t_opt, "OPT" },
        { ns_type::ns_t_apl, "APL" },
        { ns_type::ns_t_tkey, "TKEY" },
        { ns_type::ns_t_tsig, "TSIG" },
        { ns_type::ns_t_ixfr, "IXFR" },
        { ns_type::ns_t_axfr, "AXFR" },
        { ns_type::ns_t_mailb, "MAILB" },
        { ns_type::ns_t_maila, "MAILA" },
        { ns_type::ns_t_any, "ANY" },
        { ns_type::ns_t_zxfr, "ZXFR" },
    };
    auto it = kTypeStrs.find(dnstype);
    static const char* kUnknownStr{ "UNKNOWN" };
    if (it == kTypeStrs.end()) return kUnknownStr;
    return it->second;
}

const char* dnsclass2str(unsigned dnsclass) {
    static std::unordered_map<unsigned, const char*> kClassStrs = {
        { ns_class::ns_c_in , "Internet" },
        { 2, "CSNet" },
        { ns_class::ns_c_chaos, "ChaosNet" },
        { ns_class::ns_c_hs, "Hesiod" },
        { ns_class::ns_c_none, "none" },
        { ns_class::ns_c_any, "any" }
    };
    auto it = kClassStrs.find(dnsclass);
    static const char* kUnknownStr{ "UNKNOWN" };
    if (it == kClassStrs.end()) return kUnknownStr;
    return it->second;
    return "unknown";
}

struct DNSName {
    std::string name;
    const char* read(const char* buffer, const char* buffer_end);
    char* write(char* buffer, const char* buffer_end) const;
    const char* toString() const;
private:
    const char* parseField(const char* buffer, const char* buffer_end,
                           bool* last);
};

const char* DNSName::toString() const {
    return name.c_str();
}

const char* DNSName::read(const char* buffer, const char* buffer_end) {
    const char* cur = buffer;
    bool last = false;
    do {
        cur = parseField(cur, buffer_end, &last);
        if (cur == nullptr) {
            ALOGI("parsing failed at line %d", __LINE__);
            return nullptr;
        }
    } while (!last);
    return cur;
}

char* DNSName::write(char* buffer, const char* buffer_end) const {
    char* buffer_cur = buffer;
    for (size_t pos = 0 ; pos < name.size() ; ) {
        size_t dot_pos = name.find('.', pos);
        if (dot_pos == std::string::npos) {
            // Sanity check, should never happen unless parseField is broken.
            ALOGI("logic error: all names are expected to end with a '.'");
            return nullptr;
        }
        size_t len = dot_pos - pos;
        if (len >= 256) {
            ALOGI("name component '%s' is %zu long, but max is 255",
                    name.substr(pos, dot_pos - pos).c_str(), len);
            return nullptr;
        }
        if (buffer_cur + sizeof(uint8_t) + len > buffer_end) {
            ALOGI("buffer overflow at line %d", __LINE__);
            return nullptr;
        }
        *buffer_cur++ = len;
        buffer_cur = std::copy(std::next(name.begin(), pos),
                               std::next(name.begin(), dot_pos),
                               buffer_cur);
        pos = dot_pos + 1;
    }
    // Write final zero.
    *buffer_cur++ = 0;
    return buffer_cur;
}

const char* DNSName::parseField(const char* buffer, const char* buffer_end,
                                bool* last) {
    if (buffer + sizeof(uint8_t) > buffer_end) {
        ALOGI("parsing failed at line %d", __LINE__);
        return nullptr;
    }
    unsigned field_type = *buffer >> 6;
    unsigned ofs = *buffer & 0x3F;
    const char* cur = buffer + sizeof(uint8_t);
    if (field_type == 0) {
        // length + name component
        if (ofs == 0) {
            *last = true;
            return cur;
        }
        if (cur + ofs > buffer_end) {
            ALOGI("parsing failed at line %d", __LINE__);
            return nullptr;
        }
        name.append(cur, ofs);
        name.push_back('.');
        return cur + ofs;
    } else if (field_type == 3) {
        ALOGI("name compression not implemented");
        return nullptr;
    }
    ALOGI("invalid name field type");
    return nullptr;
}

struct DNSQuestion {
    DNSName qname;
    unsigned qtype;
    unsigned qclass;
    const char* read(const char* buffer, const char* buffer_end);
    char* write(char* buffer, const char* buffer_end) const;
    std::string toString() const;
};

const char* DNSQuestion::read(const char* buffer, const char* buffer_end) {
    const char* cur = qname.read(buffer, buffer_end);
    if (cur == nullptr) {
        ALOGI("parsing failed at line %d", __LINE__);
        return nullptr;
    }
    if (cur + 2*sizeof(uint16_t) > buffer_end) {
        ALOGI("parsing failed at line %d", __LINE__);
        return nullptr;
    }
    qtype = ntohs(*reinterpret_cast<const uint16_t*>(cur));
    qclass = ntohs(*reinterpret_cast<const uint16_t*>(cur + sizeof(uint16_t)));
    return cur + 2*sizeof(uint16_t);
}

char* DNSQuestion::write(char* buffer, const char* buffer_end) const {
    char* buffer_cur = qname.write(buffer, buffer_end);
    if (buffer_cur == nullptr) return nullptr;
    if (buffer_cur + 2*sizeof(uint16_t) > buffer_end) {
        ALOGI("buffer overflow on line %d", __LINE__);
        return nullptr;
    }
    *reinterpret_cast<uint16_t*>(buffer_cur) = htons(qtype);
    *reinterpret_cast<uint16_t*>(buffer_cur + sizeof(uint16_t)) =
            htons(qclass);
    return buffer_cur + 2*sizeof(uint16_t);
}

std::string DNSQuestion::toString() const {
    char buffer[4096];
    int len = snprintf(buffer, sizeof(buffer), "Q<%s,%s,%s>", qname.toString(),
                       dnstype2str(qtype), dnsclass2str(qclass));
    return std::string(buffer, len);
}

struct DNSRecord {
    DNSName name;
    unsigned rtype;
    unsigned rclass;
    unsigned ttl;
    std::vector<char> rdata;
    const char* read(const char* buffer, const char* buffer_end);
    char* write(char* buffer, const char* buffer_end) const;
    std::string toString() const;
private:
    struct IntFields {
        uint16_t rtype;
        uint16_t rclass;
        uint32_t ttl;
        uint16_t rdlen;
    } __attribute__((__packed__));

    const char* readIntFields(const char* buffer, const char* buffer_end,
            unsigned* rdlen);
    char* writeIntFields(unsigned rdlen, char* buffer,
                         const char* buffer_end) const;
};

const char* DNSRecord::read(const char* buffer, const char* buffer_end) {
    const char* cur = name.read(buffer, buffer_end);
    if (cur == nullptr) {
        ALOGI("parsing failed at line %d", __LINE__);
        return nullptr;
    }
    unsigned rdlen = 0;
    cur = readIntFields(cur, buffer_end, &rdlen);
    if (cur == nullptr) {
        ALOGI("parsing failed at line %d", __LINE__);
        return nullptr;
    }
    if (cur + rdlen > buffer_end) {
        ALOGI("parsing failed at line %d", __LINE__);
        return nullptr;
    }
    rdata.assign(cur, cur + rdlen);
    return cur + rdlen;
}

char* DNSRecord::write(char* buffer, const char* buffer_end) const {
    char* buffer_cur = name.write(buffer, buffer_end);
    if (buffer_cur == nullptr) return nullptr;
    buffer_cur = writeIntFields(rdata.size(), buffer_cur, buffer_end);
    if (buffer_cur == nullptr) return nullptr;
    if (buffer_cur + rdata.size() > buffer_end) {
        ALOGI("buffer overflow on line %d", __LINE__);
        return nullptr;
    }
    return std::copy(rdata.begin(), rdata.end(), buffer_cur);
}

std::string DNSRecord::toString() const {
    char buffer[4096];
    int len = snprintf(buffer, sizeof(buffer), "R<%s,%s,%s>", name.toString(),
                       dnstype2str(rtype), dnsclass2str(rclass));
    return std::string(buffer, len);
}

const char* DNSRecord::readIntFields(const char* buffer, const char* buffer_end,
                                     unsigned* rdlen) {
    if (buffer + sizeof(IntFields) > buffer_end ) {
        ALOGI("parsing failed at line %d", __LINE__);
        return nullptr;
    }
    const auto& intfields = *reinterpret_cast<const IntFields*>(buffer);
    rtype = ntohs(intfields.rtype);
    rclass = ntohs(intfields.rclass);
    ttl = ntohl(intfields.ttl);
    *rdlen = ntohs(intfields.rdlen);
    return buffer + sizeof(IntFields);
}

char* DNSRecord::writeIntFields(unsigned rdlen, char* buffer,
                                const char* buffer_end) const {
    if (buffer + sizeof(IntFields) > buffer_end ) {
        ALOGI("buffer overflow on line %d", __LINE__);
        return nullptr;
    }
    auto& intfields = *reinterpret_cast<IntFields*>(buffer);
    intfields.rtype = htons(rtype);
    intfields.rclass = htons(rclass);
    intfields.ttl = htonl(ttl);
    intfields.rdlen = htons(rdlen);
    return buffer + sizeof(IntFields);
}

struct DNSHeader {
    unsigned id;
    bool ra;
    uint8_t rcode;
    bool qr;
    uint8_t opcode;
    bool aa;
    bool tr;
    bool rd;
    std::vector<DNSQuestion> questions;
    std::vector<DNSRecord> answers;
    std::vector<DNSRecord> authorities;
    std::vector<DNSRecord> additionals;
    const char* read(const char* buffer, const char* buffer_end);
    char* write(char* buffer, const char* buffer_end) const;
    std::string toString() const;

private:
    struct Header {
        uint16_t id;
        uint8_t flags0;
        uint8_t flags1;
        uint16_t qdcount;
        uint16_t ancount;
        uint16_t nscount;
        uint16_t arcount;
    } __attribute__((__packed__));

    const char* readHeader(const char* buffer, const char* buffer_end,
                           unsigned* qdcount, unsigned* ancount,
                           unsigned* nscount, unsigned* arcount);
};

const char* DNSHeader::read(const char* buffer, const char* buffer_end) {
    unsigned qdcount;
    unsigned ancount;
    unsigned nscount;
    unsigned arcount;
    const char* cur = readHeader(buffer, buffer_end, &qdcount, &ancount,
                                 &nscount, &arcount);
    if (cur == nullptr) {
        ALOGI("parsing failed at line %d", __LINE__);
        return nullptr;
    }
    if (qdcount) {
        questions.resize(qdcount);
        for (unsigned i = 0 ; i < qdcount ; ++i) {
            cur = questions[i].read(cur, buffer_end);
            if (cur == nullptr) {
                ALOGI("parsing failed at line %d", __LINE__);
                return nullptr;
            }
        }
    }
    if (ancount) {
        answers.resize(ancount);
        for (unsigned i = 0 ; i < ancount ; ++i) {
            cur = answers[i].read(cur, buffer_end);
            if (cur == nullptr) {
                ALOGI("parsing failed at line %d", __LINE__);
                return nullptr;
            }
        }
    }
    if (nscount) {
        authorities.resize(nscount);
        for (unsigned i = 0 ; i < nscount ; ++i) {
            cur = authorities[i].read(cur, buffer_end);
            if (cur == nullptr) {
                ALOGI("parsing failed at line %d", __LINE__);
                return nullptr;
            }
        }
    }
    if (arcount) {
        additionals.resize(arcount);
        for (unsigned i = 0 ; i < arcount ; ++i) {
            cur = additionals[i].read(cur, buffer_end);
            if (cur == nullptr) {
                ALOGI("parsing failed at line %d", __LINE__);
                return nullptr;
            }
        }
    }
    return cur;
}

char* DNSHeader::write(char* buffer, const char* buffer_end) const {
    if (buffer + sizeof(Header) > buffer_end) {
        ALOGI("buffer overflow on line %d", __LINE__);
        return nullptr;
    }
    Header& header = *reinterpret_cast<Header*>(buffer);
    // bytes 0-1
    header.id = htons(id);
    // byte 2: 7:qr, 3-6:opcode, 2:aa, 1:tr, 0:rd
    header.flags0 = (qr << 7) | (opcode << 3) | (aa << 2) | (tr << 1) | rd;
    // byte 3: 7:ra, 6:zero, 5:ad, 4:cd, 0-3:rcode
    header.flags1 = rcode;
    // rest of header
    header.qdcount = htons(questions.size());
    header.ancount = htons(answers.size());
    header.nscount = htons(authorities.size());
    header.arcount = htons(additionals.size());
    char* buffer_cur = buffer + sizeof(Header);
    for (const DNSQuestion& question : questions) {
        buffer_cur = question.write(buffer_cur, buffer_end);
        if (buffer_cur == nullptr) return nullptr;
    }
    for (const DNSRecord& answer : answers) {
        buffer_cur = answer.write(buffer_cur, buffer_end);
        if (buffer_cur == nullptr) return nullptr;
    }
    for (const DNSRecord& authority : authorities) {
        buffer_cur = authority.write(buffer_cur, buffer_end);
        if (buffer_cur == nullptr) return nullptr;
    }
    for (const DNSRecord& additional : additionals) {
        buffer_cur = additional.write(buffer_cur, buffer_end);
        if (buffer_cur == nullptr) return nullptr;
    }
    return buffer_cur;
}

std::string DNSHeader::toString() const {
    // TODO
    return std::string();
}

const char* DNSHeader::readHeader(const char* buffer, const char* buffer_end,
                                  unsigned* qdcount, unsigned* ancount,
                                  unsigned* nscount, unsigned* arcount) {
    if (buffer + sizeof(Header) > buffer_end)
        return 0;
    const auto& header = *reinterpret_cast<const Header*>(buffer);
    // bytes 0-1
    id = ntohs(header.id);
    // byte 2: 7:qr, 3-6:opcode, 2:aa, 1:tr, 0:rd
    qr = header.flags0 >> 7;
    opcode = (header.flags0 >> 3) & 0x0F;
    aa = (header.flags0 >> 2) & 1;
    tr = (header.flags0 >> 1) & 1;
    rd = header.flags0 & 1;
    // byte 3: 7:ra, 6:zero, 5:ad, 4:cd, 0-3:rcode
    ra = header.flags1 >> 7;
    rcode = header.flags1 & 0xF;
    // rest of header
    *qdcount = ntohs(header.qdcount);
    *ancount = ntohs(header.ancount);
    *nscount = ntohs(header.nscount);
    *arcount = ntohs(header.arcount);
    return buffer + sizeof(Header);
}

/* DNS responder */

DNSResponder::DNSResponder(std::string listen_address,
                           std::string listen_service, int poll_timeout_ms,
                           uint16_t error_rcode, double response_probability) :
    listen_address_(std::move(listen_address)), listen_service_(std::move(listen_service)),
    poll_timeout_ms_(poll_timeout_ms), error_rcode_(error_rcode),
    response_probability_(response_probability),
    socket_(-1), epoll_fd_(-1), terminate_(false) { }

DNSResponder::~DNSResponder() {
    stopServer();
}

void DNSResponder::addMapping(const char* name, ns_type type,
        const char* addr) {
    std::lock_guard<std::mutex> lock(mappings_mutex_);
    auto it = mappings_.find(QueryKey(name, type));
    if (it != mappings_.end()) {
        ALOGI("Overwriting mapping for (%s, %s), previous address %s, new "
            "address %s", name, dnstype2str(type), it->second.c_str(),
            addr);
        it->second = addr;
        return;
    }
    mappings_.emplace(std::piecewise_construct,
                      std::forward_as_tuple(name, type),
                      std::forward_as_tuple(addr));
}

void DNSResponder::removeMapping(const char* name, ns_type type) {
    std::lock_guard<std::mutex> lock(mappings_mutex_);
    auto it = mappings_.find(QueryKey(name, type));
    if (it != mappings_.end()) {
        ALOGI("Cannot remove mapping mapping from (%s, %s), not present", name,
            dnstype2str(type));
        return;
    }
    mappings_.erase(it);
}

void DNSResponder::setResponseProbability(double response_probability) {
    response_probability_ = response_probability;
}

bool DNSResponder::running() const {
    return socket_ != -1;
}

bool DNSResponder::startServer() {
    if (running()) {
        ALOGI("server already running");
        return false;
    }
    addrinfo ai_hints{
        .ai_family = AF_UNSPEC,
        .ai_socktype = SOCK_DGRAM,
        .ai_flags = AI_PASSIVE
    };
    addrinfo* ai_res;
    int rv = getaddrinfo(listen_address_.c_str(), listen_service_.c_str(),
                         &ai_hints, &ai_res);
    if (rv) {
        ALOGI("getaddrinfo(%s, %s) failed: %s", listen_address_.c_str(),
            listen_service_.c_str(), gai_strerror(rv));
        return false;
    }
    int s = -1;
    for (const addrinfo* ai = ai_res ; ai ; ai = ai->ai_next) {
        s = socket(ai->ai_family, ai->ai_socktype, ai->ai_protocol);
        if (s < 0) continue;
        const int one = 1;
        setsockopt(s, SOL_SOCKET, SO_REUSEPORT, &one, sizeof(one));
        if (bind(s, ai->ai_addr, ai->ai_addrlen)) {
            APLOGI("bind failed for socket %d", s);
            close(s);
            s = -1;
            continue;
        }
        std::string host_str = addr2str(ai->ai_addr, ai->ai_addrlen);
        ALOGI("bound to UDP %s:%s", host_str.c_str(), listen_service_.c_str());
        break;
    }
    freeaddrinfo(ai_res);
    if (s < 0) {
        ALOGI("bind() failed");
        return false;
    }

    int flags = fcntl(s, F_GETFL, 0);
    if (flags < 0) flags = 0;
    if (fcntl(s, F_SETFL, flags | O_NONBLOCK) < 0) {
        APLOGI("fcntl(F_SETFL) failed for socket %d", s);
        close(s);
        return false;
    }

    int ep_fd = epoll_create(1);
    if (ep_fd < 0) {
        char error_msg[512] = { 0 };
        if (strerror_r(errno, error_msg, sizeof(error_msg)))
            strncpy(error_msg, "UNKNOWN", sizeof(error_msg));
        APLOGI("epoll_create() failed: %s", error_msg);
        close(s);
        return false;
    }
    epoll_event ev;
    ev.events = EPOLLIN;
    ev.data.fd = s;
    if (epoll_ctl(ep_fd, EPOLL_CTL_ADD, s, &ev) < 0) {
        APLOGI("epoll_ctl() failed for socket %d", s);
        close(ep_fd);
        close(s);
        return false;
    }

    epoll_fd_ = ep_fd;
    socket_ = s;
    {
        std::lock_guard<std::mutex> lock(update_mutex_);
        handler_thread_ = std::thread(&DNSResponder::requestHandler, this);
    }
    ALOGI("server started successfully");
    return true;
}

bool DNSResponder::stopServer() {
    std::lock_guard<std::mutex> lock(update_mutex_);
    if (!running()) {
        ALOGI("server not running");
        return false;
    }
    if (terminate_) {
        ALOGI("LOGIC ERROR");
        return false;
    }
    ALOGI("stopping server");
    terminate_ = true;
    handler_thread_.join();
    close(epoll_fd_);
    close(socket_);
    terminate_ = false;
    socket_ = -1;
    ALOGI("server stopped successfully");
    return true;
}

std::vector<std::pair<std::string, ns_type >> DNSResponder::queries() const {
    std::lock_guard<std::mutex> lock(queries_mutex_);
    return queries_;
}

void DNSResponder::clearQueries() {
    std::lock_guard<std::mutex> lock(queries_mutex_);
    queries_.clear();
}

void DNSResponder::requestHandler() {
    epoll_event evs[1];
    while (!terminate_) {
        int n = epoll_wait(epoll_fd_, evs, 1, poll_timeout_ms_);
        if (n == 0) continue;
        if (n < 0) {
            ALOGI("epoll_wait() failed");
            // TODO(imaipi): terminate on error.
            return;
        }
        char buffer[4096];
        sockaddr_storage sa;
        socklen_t sa_len = sizeof(sa);
        ssize_t len;
        do {
            len = recvfrom(socket_, buffer, sizeof(buffer), 0,
                           (sockaddr*) &sa, &sa_len);
        } while (len < 0 && (errno == EAGAIN || errno == EINTR));
        if (len <= 0) {
            ALOGI("recvfrom() failed");
            continue;
        }
        ALOGI("read %zd bytes", len);
        char response[4096];
        size_t response_len = sizeof(response);
        if (handleDNSRequest(buffer, len, response, &response_len) &&
            response_len > 0) {
            len = sendto(socket_, response, response_len, 0,
                         reinterpret_cast<const sockaddr*>(&sa), sa_len);
            std::string host_str =
                addr2str(reinterpret_cast<const sockaddr*>(&sa), sa_len);
            if (len > 0) {
                ALOGI("sent %zu bytes to %s", len, host_str.c_str());
            } else {
                APLOGI("sendto() failed for %s", host_str.c_str());
            }
            // Test that the response is actually a correct DNS message.
            const char* response_end = response + len;
            DNSHeader header;
            const char* cur = header.read(response, response_end);
            if (cur == nullptr) ALOGI("response is flawed");

        } else {
            ALOGI("not responding");
        }
    }
}

bool DNSResponder::handleDNSRequest(const char* buffer, ssize_t len,
                                    char* response, size_t* response_len)
                                    const {
    ALOGI("request: '%s'", str2hex(buffer, len).c_str());
    const char* buffer_end = buffer + len;
    DNSHeader header;
    const char* cur = header.read(buffer, buffer_end);
    // TODO(imaipi): for now, unparsable messages are silently dropped, fix.
    if (cur == nullptr) {
        ALOGI("failed to parse query");
        return false;
    }
    if (header.qr) {
        ALOGI("response received instead of a query");
        return false;
    }
    if (header.opcode != ns_opcode::ns_o_query) {
        ALOGI("unsupported request opcode received");
        return makeErrorResponse(&header, ns_rcode::ns_r_notimpl, response,
                                 response_len);
    }
    if (header.questions.empty()) {
        ALOGI("no questions present");
        return makeErrorResponse(&header, ns_rcode::ns_r_formerr, response,
                                 response_len);
    }
    if (!header.answers.empty()) {
        ALOGI("already %zu answers present in query", header.answers.size());
        return makeErrorResponse(&header, ns_rcode::ns_r_formerr, response,
                                 response_len);
    }
    {
        std::lock_guard<std::mutex> lock(queries_mutex_);
        for (const DNSQuestion& question : header.questions) {
            queries_.push_back(make_pair(question.qname.name,
                                         ns_type(question.qtype)));
        }
    }

    // Ignore requests with the preset probability.
    auto constexpr bound = std::numeric_limits<unsigned>::max();
    if (arc4random_uniform(bound) > bound*response_probability_) {
        ALOGI("returning SRVFAIL in accordance with probability distribution");
        return makeErrorResponse(&header, ns_rcode::ns_r_servfail, response,
                                 response_len);
    }

    for (const DNSQuestion& question : header.questions) {
        if (question.qclass != ns_class::ns_c_in &&
            question.qclass != ns_class::ns_c_any) {
            ALOGI("unsupported question class %u", question.qclass);
            return makeErrorResponse(&header, ns_rcode::ns_r_notimpl, response,
                                     response_len);
        }
        if (!addAnswerRecords(question, &header.answers)) {
            return makeErrorResponse(&header, ns_rcode::ns_r_servfail, response,
                                     response_len);
        }
    }
    header.qr = true;
    char* response_cur = header.write(response, response + *response_len);
    if (response_cur == nullptr) {
        return false;
    }
    *response_len = response_cur - response;
    return true;
}

bool DNSResponder::addAnswerRecords(const DNSQuestion& question,
                                    std::vector<DNSRecord>* answers) const {
    auto it = mappings_.find(QueryKey(question.qname.name, question.qtype));
    if (it == mappings_.end()) {
        // TODO(imaipi): handle correctly
        ALOGI("no mapping found for %s %s, lazily refusing to add an answer",
            question.qname.name.c_str(), dnstype2str(question.qtype));
        return true;
    }
    ALOGI("mapping found for %s %s: %s", question.qname.name.c_str(),
        dnstype2str(question.qtype), it->second.c_str());
    DNSRecord record;
    record.name = question.qname;
    record.rtype = question.qtype;
    record.rclass = ns_class::ns_c_in;
    record.ttl = 5;  // seconds
    if (question.qtype == ns_type::ns_t_a) {
        record.rdata.resize(4);
        if (inet_pton(AF_INET, it->second.c_str(), record.rdata.data()) != 1) {
            ALOGI("inet_pton(AF_INET, %s) failed", it->second.c_str());
            return false;
        }
    } else if (question.qtype == ns_type::ns_t_aaaa) {
        record.rdata.resize(16);
        if (inet_pton(AF_INET6, it->second.c_str(), record.rdata.data()) != 1) {
            ALOGI("inet_pton(AF_INET6, %s) failed", it->second.c_str());
            return false;
        }
    } else {
        ALOGI("unhandled qtype %s", dnstype2str(question.qtype));
        return false;
    }
    answers->push_back(std::move(record));
    return true;
}

bool DNSResponder::makeErrorResponse(DNSHeader* header, ns_rcode rcode,
                                     char* response, size_t* response_len)
                                     const {
    header->answers.clear();
    header->authorities.clear();
    header->additionals.clear();
    header->rcode = rcode;
    header->qr = true;
    char* response_cur = header->write(response, response + *response_len);
    if (response_cur == nullptr) return false;
    *response_len = response_cur - response;
    return true;
}

}  // namespace test

