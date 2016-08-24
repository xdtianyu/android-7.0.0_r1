/*
 * Copyright 2011 Daniel Drown
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
 * translate.h - translate from one version of ip to another
 */
#ifndef __TRANSLATE_H__
#define __TRANSLATE_H__

#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>
#include <netinet/udp.h>
#include <netinet/tcp.h>
#include <netinet/ip6.h>
#include <netinet/icmp6.h>
#include <linux/icmp.h>
#include <linux/if_tun.h>

#include "clatd.h"

#define MAX_TCP_HDR (15 * 4)   // Data offset field is 4 bits and counts in 32-bit words.

// Calculates the checksum over all the packet components starting from pos.
uint16_t packet_checksum(uint32_t checksum, clat_packet packet, clat_packet_index pos);

// Returns the total length of the packet components after pos.
uint16_t packet_length(clat_packet packet, clat_packet_index pos);

// Returns true iff the given IPv6 address is in the plat subnet.
int is_in_plat_subnet(const struct in6_addr *addr6);

// Functions to create tun, IPv4, and IPv6 headers.
void fill_tun_header(struct tun_pi *tun_header, uint16_t proto);
void fill_ip_header(struct iphdr *ip_targ, uint16_t payload_len, uint8_t protocol,
                    const struct ip6_hdr *old_header);
void fill_ip6_header(struct ip6_hdr *ip6, uint16_t payload_len, uint8_t protocol,
                     const struct iphdr *old_header);

// Translate and send packets.
void translate_packet(int fd, int to_ipv6, const uint8_t *packet, size_t packetsize);

// Translate IPv4 and IPv6 packets.
int ipv4_packet(clat_packet out, clat_packet_index pos, const uint8_t *packet, size_t len);
int ipv6_packet(clat_packet out, clat_packet_index pos, const uint8_t *packet, size_t len);

// Deal with fragmented packets.
size_t maybe_fill_frag_header(struct ip6_frag *frag_hdr, struct ip6_hdr *ip6_targ,
                              const struct iphdr *old_header);
uint8_t parse_frag_header(const struct ip6_frag *frag_hdr, struct iphdr *ip_targ);

// Deal with fragmented packets.
size_t maybe_fill_frag_header(struct ip6_frag *frag_hdr, struct ip6_hdr *ip6_targ,
                              const struct iphdr *old_header);
uint8_t parse_frag_header(const struct ip6_frag *frag_hdr, struct iphdr *ip_targ);

// Translate ICMP packets.
int icmp_to_icmp6(clat_packet out, clat_packet_index pos, const struct icmphdr *icmp,
                  uint32_t checksum, const uint8_t *payload, size_t payload_size);
int icmp6_to_icmp(clat_packet out, clat_packet_index pos, const struct icmp6_hdr *icmp6,
                  const uint8_t *payload, size_t payload_size);

// Translate generic IP packets.
int generic_packet(clat_packet out, clat_packet_index pos, const uint8_t *payload, size_t len);

// Translate TCP and UDP packets.
int tcp_packet(clat_packet out, clat_packet_index pos, const struct tcphdr *tcp,
               uint32_t old_sum, uint32_t new_sum, size_t len);
int udp_packet(clat_packet out, clat_packet_index pos, const struct udphdr *udp,
               uint32_t old_sum, uint32_t new_sum, size_t len);

int tcp_translate(clat_packet out, clat_packet_index pos, const struct tcphdr *tcp,
                  size_t header_size, uint32_t old_sum, uint32_t new_sum,
                  const uint8_t *payload, size_t payload_size);
int udp_translate(clat_packet out, clat_packet_index pos, const struct udphdr *udp,
                  uint32_t old_sum, uint32_t new_sum,
                  const uint8_t *payload, size_t payload_size);

#endif /* __TRANSLATE_H__ */
