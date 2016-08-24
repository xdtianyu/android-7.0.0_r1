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
 * ipv6.c - takes ipv6 packets, finds their headers, and then calls translation functions on them
 */
#include <string.h>

#include <arpa/inet.h>

#include "translate.h"
#include "checksum.h"
#include "logging.h"
#include "dump.h"
#include "config.h"
#include "debug.h"

/* function: icmp6_packet
 * takes an icmp6 packet and sets it up for translation
 * out      - output packet
 * icmp6    - pointer to icmp6 header in packet
 * checksum - pseudo-header checksum (unused)
 * len      - size of ip payload
 * returns: the highest position in the output clat_packet that's filled in
 */
int icmp6_packet(clat_packet out, clat_packet_index pos, const struct icmp6_hdr *icmp6,
                 size_t len) {
  const uint8_t *payload;
  size_t payload_size;

  if(len < sizeof(struct icmp6_hdr)) {
    logmsg_dbg(ANDROID_LOG_ERROR, "icmp6_packet/(too small)");
    return 0;
  }

  payload = (const uint8_t *) (icmp6 + 1);
  payload_size = len - sizeof(struct icmp6_hdr);

  return icmp6_to_icmp(out, pos, icmp6, payload, payload_size);
}

/* function: log_bad_address
 * logs a bad address to android's log buffer if debugging is turned on
 * fmt     - printf-style format, use %s to place the address
 * badaddr - the bad address in question
 */
#if CLAT_DEBUG
void log_bad_address(const char *fmt, const struct in6_addr *src, const struct in6_addr *dst) {
  char srcstr[INET6_ADDRSTRLEN];
  char dststr[INET6_ADDRSTRLEN];

  inet_ntop(AF_INET6, src, srcstr, sizeof(srcstr));
  inet_ntop(AF_INET6, dst, dststr, sizeof(dststr));
  logmsg_dbg(ANDROID_LOG_ERROR, fmt, srcstr, dststr);
}
#else
#define log_bad_address(fmt, src, dst)
#endif

/* function: ipv6_packet
 * takes an ipv6 packet and hands it off to the layer 4 protocol function
 * out    - output packet
 * packet - packet data
 * len    - size of packet
 * returns: the highest position in the output clat_packet that's filled in
 */
int ipv6_packet(clat_packet out, clat_packet_index pos, const uint8_t *packet, size_t len) {
  const struct ip6_hdr *ip6 = (struct ip6_hdr *) packet;
  struct iphdr *ip_targ = (struct iphdr *) out[pos].iov_base;
  struct ip6_frag *frag_hdr = NULL;
  uint8_t protocol;
  const uint8_t *next_header;
  size_t len_left;
  uint32_t old_sum, new_sum;
  int iov_len;

  if(len < sizeof(struct ip6_hdr)) {
    logmsg_dbg(ANDROID_LOG_ERROR, "ipv6_packet/too short for an ip6 header: %d", len);
    return 0;
  }

  if(IN6_IS_ADDR_MULTICAST(&ip6->ip6_dst)) {
    log_bad_address("ipv6_packet/multicast %s->%s", &ip6->ip6_src, &ip6->ip6_dst);
    return 0; // silently ignore
  }

  // If the packet is not from the plat subnet to the local subnet, or vice versa, drop it, unless
  // it's an ICMP packet (which can come from anywhere). We do not send IPv6 packets from the plat
  // subnet to the local subnet, but these can appear as inner packets in ICMP errors, so we need
  // to translate them. We accept third-party ICMPv6 errors, even though their source addresses
  // cannot be translated, so that things like unreachables and traceroute will work. fill_ip_header
  // takes care of faking a source address for them.
  if (!(is_in_plat_subnet(&ip6->ip6_src) &&
        IN6_ARE_ADDR_EQUAL(&ip6->ip6_dst, &Global_Clatd_Config.ipv6_local_subnet)) &&
      !(is_in_plat_subnet(&ip6->ip6_dst) &&
        IN6_ARE_ADDR_EQUAL(&ip6->ip6_src, &Global_Clatd_Config.ipv6_local_subnet)) &&
      ip6->ip6_nxt != IPPROTO_ICMPV6) {
    log_bad_address("ipv6_packet/wrong source address: %s->%s", &ip6->ip6_src, &ip6->ip6_dst);
    return 0;
  }

  next_header = packet + sizeof(struct ip6_hdr);
  len_left = len - sizeof(struct ip6_hdr);

  protocol = ip6->ip6_nxt;

  /* Fill in the IPv4 header. We need to do this before we translate the packet because TCP and
   * UDP include parts of the IP header in the checksum. Set the length to zero because we don't
   * know it yet.
   */
  fill_ip_header(ip_targ, 0, protocol, ip6);
  out[pos].iov_len = sizeof(struct iphdr);

  // If there's a Fragment header, parse it and decide what the next header is.
  // Do this before calculating the pseudo-header checksum because it updates the next header value.
  if (protocol == IPPROTO_FRAGMENT) {
    frag_hdr = (struct ip6_frag *) next_header;
    if (len_left < sizeof(*frag_hdr)) {
      logmsg_dbg(ANDROID_LOG_ERROR, "ipv6_packet/too short for fragment header: %d", len);
      return 0;
    }

    next_header += sizeof(*frag_hdr);
    len_left -= sizeof(*frag_hdr);

    protocol = parse_frag_header(frag_hdr, ip_targ);
  }

  // ICMP and ICMPv6 have different protocol numbers.
  if (protocol == IPPROTO_ICMPV6) {
    protocol = IPPROTO_ICMP;
    ip_targ->protocol = IPPROTO_ICMP;
  }

  /* Calculate the pseudo-header checksum.
   * Technically, the length that is used in the pseudo-header checksum is the transport layer
   * length, which is not the same as len_left in the case of fragmented packets. But since
   * translation does not change the transport layer length, the checksum is unaffected.
   */
  old_sum = ipv6_pseudo_header_checksum(ip6, len_left, protocol);
  new_sum = ipv4_pseudo_header_checksum(ip_targ, len_left);

  // Does not support IPv6 extension headers except Fragment.
  if (frag_hdr && (frag_hdr->ip6f_offlg & IP6F_OFF_MASK)) {
    iov_len = generic_packet(out, pos + 2, next_header, len_left);
  } else if (protocol == IPPROTO_ICMP) {
    iov_len = icmp6_packet(out, pos + 2, (const struct icmp6_hdr *) next_header, len_left);
  } else if (protocol == IPPROTO_TCP) {
    iov_len = tcp_packet(out, pos + 2, (const struct tcphdr *) next_header, old_sum, new_sum,
                         len_left);
  } else if (protocol == IPPROTO_UDP) {
    iov_len = udp_packet(out, pos + 2, (const struct udphdr *) next_header, old_sum, new_sum,
                         len_left);
  } else if (protocol == IPPROTO_GRE) {
    iov_len = generic_packet(out, pos + 2, next_header, len_left);
  } else {
#if CLAT_DEBUG
    logmsg(ANDROID_LOG_ERROR, "ipv6_packet/unknown next header type: %x", ip6->ip6_nxt);
    logcat_hexdump("ipv6/nxthdr", packet, len);
#endif
    return 0;
  }

  // Set the length and calculate the checksum.
  ip_targ->tot_len = htons(ntohs(ip_targ->tot_len) + packet_length(out, pos));
  ip_targ->check = ip_checksum(ip_targ, sizeof(struct iphdr));
  return iov_len;
}
