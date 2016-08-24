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
 * ipv4.c - takes ipv4 packets, finds their headers, and then calls translation functions on them
 */
#include <string.h>

#include "translate.h"
#include "checksum.h"
#include "logging.h"
#include "debug.h"
#include "dump.h"

/* function: icmp_packet
 * translates an icmp packet
 * out      - output packet
 * icmp     - pointer to icmp header in packet
 * checksum - pseudo-header checksum
 * len      - size of ip payload
 * returns: the highest position in the output clat_packet that's filled in
 */
int icmp_packet(clat_packet out, clat_packet_index pos, const struct icmphdr *icmp,
                uint32_t checksum, size_t len) {
  const uint8_t *payload;
  size_t payload_size;

  if(len < sizeof(struct icmphdr)) {
    logmsg_dbg(ANDROID_LOG_ERROR, "icmp_packet/(too small)");
    return 0;
  }

  payload = (const uint8_t *) (icmp + 1);
  payload_size = len - sizeof(struct icmphdr);

  return icmp_to_icmp6(out, pos, icmp, checksum, payload, payload_size);
}

/* function: ipv4_packet
 * translates an ipv4 packet
 * out    - output packet
 * packet - packet data
 * len    - size of packet
 * returns: the highest position in the output clat_packet that's filled in
 */
int ipv4_packet(clat_packet out, clat_packet_index pos, const uint8_t *packet, size_t len) {
  const struct iphdr *header = (struct iphdr *) packet;
  struct ip6_hdr *ip6_targ = (struct ip6_hdr *) out[pos].iov_base;
  struct ip6_frag *frag_hdr;
  size_t frag_hdr_len;
  uint8_t nxthdr;
  const uint8_t *next_header;
  size_t len_left;
  uint32_t old_sum, new_sum;
  int iov_len;

  if(len < sizeof(struct iphdr)) {
    logmsg_dbg(ANDROID_LOG_ERROR, "ip_packet/too short for an ip header");
    return 0;
  }

  if(header->ihl < 5) {
    logmsg_dbg(ANDROID_LOG_ERROR, "ip_packet/ip header length set to less than 5: %x", header->ihl);
    return 0;
  }

  if((size_t) header->ihl * 4 > len) { // ip header length larger than entire packet
    logmsg_dbg(ANDROID_LOG_ERROR, "ip_packet/ip header length set too large: %x", header->ihl);
    return 0;
  }

  if(header->version != 4) {
    logmsg_dbg(ANDROID_LOG_ERROR, "ip_packet/ip header version not 4: %x", header->version);
    return 0;
  }

  /* rfc6145 - If any IPv4 options are present in the IPv4 packet, they MUST be
   * ignored and the packet translated normally; there is no attempt to
   * translate the options.
   */

  next_header = packet + header->ihl*4;
  len_left = len - header->ihl * 4;

  nxthdr = header->protocol;
  if (nxthdr == IPPROTO_ICMP) {
    // ICMP and ICMPv6 have different protocol numbers.
    nxthdr = IPPROTO_ICMPV6;
  }

  /* Fill in the IPv6 header. We need to do this before we translate the packet because TCP and
   * UDP include parts of the IP header in the checksum. Set the length to zero because we don't
   * know it yet.
   */
  fill_ip6_header(ip6_targ, 0, nxthdr, header);
  out[pos].iov_len = sizeof(struct ip6_hdr);

  /* Calculate the pseudo-header checksum.
   * Technically, the length that is used in the pseudo-header checksum is the transport layer
   * length, which is not the same as len_left in the case of fragmented packets. But since
   * translation does not change the transport layer length, the checksum is unaffected.
   */
  old_sum = ipv4_pseudo_header_checksum(header, len_left);
  new_sum = ipv6_pseudo_header_checksum(ip6_targ, len_left, nxthdr);

  // If the IPv4 packet is fragmented, add a Fragment header.
  frag_hdr = (struct ip6_frag *) out[pos + 1].iov_base;
  frag_hdr_len = maybe_fill_frag_header(frag_hdr, ip6_targ, header);
  out[pos + 1].iov_len = frag_hdr_len;

  if (frag_hdr_len && frag_hdr->ip6f_offlg & IP6F_OFF_MASK) {
    // Non-first fragment. Copy the rest of the packet as is.
    iov_len = generic_packet(out, pos + 2, next_header, len_left);
  } else if (nxthdr == IPPROTO_ICMPV6) {
    iov_len = icmp_packet(out, pos + 2, (const struct icmphdr *) next_header, new_sum, len_left);
  } else if (nxthdr == IPPROTO_TCP) {
    iov_len = tcp_packet(out, pos + 2, (const struct tcphdr *) next_header, old_sum, new_sum,
                         len_left);
  } else if (nxthdr == IPPROTO_UDP) {
    iov_len = udp_packet(out, pos + 2, (const struct udphdr *) next_header, old_sum, new_sum,
                         len_left);
  } else if (nxthdr == IPPROTO_GRE) {
    iov_len = generic_packet(out, pos + 2, next_header, len_left);
  } else {
#if CLAT_DEBUG
    logmsg_dbg(ANDROID_LOG_ERROR, "ip_packet/unknown protocol: %x",header->protocol);
    logcat_hexdump("ipv4/protocol", packet, len);
#endif
    return 0;
  }

  // Set the length.
  ip6_targ->ip6_plen = htons(packet_length(out, pos));
  return iov_len;
}
