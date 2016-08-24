/*
 * Copyright 2014 The Android Open Source Project
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
 * clatd_test.cpp - unit tests for clatd
 */

#include <iostream>

#include <stdio.h>
#include <arpa/inet.h>
#include <netinet/in6.h>
#include <sys/uio.h>

#include <gtest/gtest.h>

extern "C" {
#include "checksum.h"
#include "translate.h"
#include "config.h"
#include "clatd.h"
}

// For convenience.
#define ARRAYSIZE(x) sizeof((x)) / sizeof((x)[0])

// Default translation parameters.
static const char kIPv4LocalAddr[] = "192.0.0.4";
static const char kIPv6LocalAddr[] = "2001:db8:0:b11::464";
static const char kIPv6PlatSubnet[] = "64:ff9b::";

// Test packet portions. Defined as macros because it's easy to concatenate them to make packets.
#define IPV4_HEADER(p, c1, c2) \
    0x45, 0x00,    0,   41,  /* Version=4, IHL=5, ToS=0x80, len=41 */     \
    0x00, 0x00, 0x40, 0x00,  /* ID=0x0000, flags=IP_DF, offset=0 */       \
      55,  (p), (c1), (c2),  /* TTL=55, protocol=p, checksum=c1,c2 */     \
     192,    0,    0,    4,  /* Src=192.0.0.4 */                          \
       8,    8,    8,    8,  /* Dst=8.8.8.8 */
#define IPV4_UDP_HEADER IPV4_HEADER(IPPROTO_UDP, 0x73, 0xb0)
#define IPV4_ICMP_HEADER IPV4_HEADER(IPPROTO_ICMP, 0x73, 0xc0)

#define IPV6_HEADER(p) \
    0x60, 0x00,    0,    0,  /* Version=6, tclass=0x00, flowlabel=0 */    \
       0,   21,  (p),   55,  /* plen=11, nxthdr=p, hlim=55 */             \
    0x20, 0x01, 0x0d, 0xb8,  /* Src=2001:db8:0:b11::464 */                \
    0x00, 0x00, 0x0b, 0x11,                                               \
    0x00, 0x00, 0x00, 0x00,                                               \
    0x00, 0x00, 0x04, 0x64,                                               \
    0x00, 0x64, 0xff, 0x9b,  /* Dst=64:ff9b::8.8.8.8 */                   \
    0x00, 0x00, 0x00, 0x00,                                               \
    0x00, 0x00, 0x00, 0x00,                                               \
    0x08, 0x08, 0x08, 0x08,
#define IPV6_UDP_HEADER IPV6_HEADER(IPPROTO_UDP)
#define IPV6_ICMPV6_HEADER IPV6_HEADER(IPPROTO_ICMPV6)

#define UDP_LEN 21
#define UDP_HEADER \
    0xc8, 0x8b,    0,   53,  /* Port 51339->53 */                         \
    0x00, UDP_LEN, 0,    0,  /* Length 21, checksum empty for now */

#define PAYLOAD 'H', 'e', 'l', 'l', 'o', ' ', 0x4e, 0xb8, 0x96, 0xe7, 0x95, 0x8c, 0x00

#define IPV4_PING \
    0x08, 0x00, 0x88, 0xd0,  /* Type 8, code 0, checksum 0x88d0 */        \
    0xd0, 0x0d, 0x00, 0x03,  /* ID=0xd00d, seq=3 */

#define IPV6_PING \
    0x80, 0x00, 0xc3, 0x42,  /* Type 128, code 0, checksum 0xc342 */      \
    0xd0, 0x0d, 0x00, 0x03,  /* ID=0xd00d, seq=3 */

// Macros to return pseudo-headers from packets.
#define IPV4_PSEUDOHEADER(ip, tlen)                                  \
  ip[12], ip[13], ip[14], ip[15],        /* Source address      */   \
  ip[16], ip[17], ip[18], ip[19],        /* Destination address */   \
  0, ip[9],                              /* 0, protocol         */   \
  ((tlen) >> 16) & 0xff, (tlen) & 0xff,  /* Transport length */

#define IPV6_PSEUDOHEADER(ip6, protocol, tlen)                       \
  ip6[8],  ip6[9],  ip6[10], ip6[11],  /* Source address */          \
  ip6[12], ip6[13], ip6[14], ip6[15],                                \
  ip6[16], ip6[17], ip6[18], ip6[19],                                \
  ip6[20], ip6[21], ip6[22], ip6[23],                                \
  ip6[24], ip6[25], ip6[26], ip6[27],  /* Destination address */     \
  ip6[28], ip6[29], ip6[30], ip6[31],                                \
  ip6[32], ip6[33], ip6[34], ip6[35],                                \
  ip6[36], ip6[37], ip6[38], ip6[39],                                \
  ((tlen) >> 24) & 0xff,               /* Transport length */        \
  ((tlen) >> 16) & 0xff,                                             \
  ((tlen) >> 8) & 0xff,                                              \
  (tlen) & 0xff,                                                     \
  0, 0, 0, (protocol),

// A fragmented DNS request.
static const uint8_t kIPv4Frag1[] = {
    0x45, 0x00, 0x00, 0x24, 0xfe, 0x47, 0x20, 0x00, 0x40, 0x11,
    0x8c, 0x6d, 0xc0, 0x00, 0x00, 0x04, 0x08, 0x08, 0x08, 0x08,
    0x14, 0x5d, 0x00, 0x35, 0x00, 0x29, 0x68, 0xbb, 0x50, 0x47,
    0x01, 0x00, 0x00, 0x01, 0x00, 0x00
};
static const uint8_t kIPv4Frag2[] = {
    0x45, 0x00, 0x00, 0x24, 0xfe, 0x47, 0x20, 0x02, 0x40, 0x11,
    0x8c, 0x6b, 0xc0, 0x00, 0x00, 0x04, 0x08, 0x08, 0x08, 0x08,
    0x00, 0x00, 0x00, 0x00, 0x04, 0x69, 0x70, 0x76, 0x34, 0x06,
    0x67, 0x6f, 0x6f, 0x67, 0x6c, 0x65
};
static const uint8_t kIPv4Frag3[] = {
    0x45, 0x00, 0x00, 0x1d, 0xfe, 0x47, 0x00, 0x04, 0x40, 0x11,
    0xac, 0x70, 0xc0, 0x00, 0x00, 0x04, 0x08, 0x08, 0x08, 0x08,
    0x03, 0x63, 0x6f, 0x6d, 0x00, 0x00, 0x01, 0x00, 0x01
};
static const uint8_t *kIPv4Fragments[] = { kIPv4Frag1, kIPv4Frag2, kIPv4Frag3 };
static const size_t kIPv4FragLengths[] = { sizeof(kIPv4Frag1), sizeof(kIPv4Frag2),
                                           sizeof(kIPv4Frag3) };

static const uint8_t kIPv6Frag1[] = {
    0x60, 0x00, 0x00, 0x00, 0x00, 0x18, 0x2c, 0x40, 0x20, 0x01,
    0x0d, 0xb8, 0x00, 0x00, 0x0b, 0x11, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x04, 0x64, 0x00, 0x64, 0xff, 0x9b, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x08, 0x08, 0x08, 0x08,
    0x11, 0x00, 0x00, 0x01, 0x00, 0x00, 0xfe, 0x47, 0x14, 0x5d,
    0x00, 0x35, 0x00, 0x29, 0xeb, 0x91, 0x50, 0x47, 0x01, 0x00,
    0x00, 0x01, 0x00, 0x00
};

static const uint8_t kIPv6Frag2[] = {
    0x60, 0x00, 0x00, 0x00, 0x00, 0x18, 0x2c, 0x40, 0x20, 0x01,
    0x0d, 0xb8, 0x00, 0x00, 0x0b, 0x11, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x04, 0x64, 0x00, 0x64, 0xff, 0x9b, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x08, 0x08, 0x08, 0x08,
    0x11, 0x00, 0x00, 0x11, 0x00, 0x00, 0xfe, 0x47, 0x00, 0x00,
    0x00, 0x00, 0x04, 0x69, 0x70, 0x76, 0x34, 0x06, 0x67, 0x6f,
    0x6f, 0x67, 0x6c, 0x65
};

static const uint8_t kIPv6Frag3[] = {
    0x60, 0x00, 0x00, 0x00, 0x00, 0x11, 0x2c, 0x40, 0x20, 0x01,
    0x0d, 0xb8, 0x00, 0x00, 0x0b, 0x11, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x04, 0x64, 0x00, 0x64, 0xff, 0x9b, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x08, 0x08, 0x08, 0x08,
    0x11, 0x00, 0x00, 0x20, 0x00, 0x00, 0xfe, 0x47, 0x03, 0x63,
    0x6f, 0x6d, 0x00, 0x00, 0x01, 0x00, 0x01
};
static const uint8_t *kIPv6Fragments[] = { kIPv6Frag1, kIPv6Frag2, kIPv6Frag3 };
static const size_t kIPv6FragLengths[] = { sizeof(kIPv6Frag1), sizeof(kIPv6Frag2),
                                           sizeof(kIPv6Frag3) };

static const uint8_t kReassembledIPv4[] = {
    0x45, 0x00, 0x00, 0x3d, 0xfe, 0x47, 0x00, 0x00, 0x40, 0x11,
    0xac, 0x54, 0xc0, 0x00, 0x00, 0x04, 0x08, 0x08, 0x08, 0x08,
    0x14, 0x5d, 0x00, 0x35, 0x00, 0x29, 0x68, 0xbb, 0x50, 0x47,
    0x01, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x04, 0x69, 0x70, 0x76, 0x34, 0x06, 0x67, 0x6f, 0x6f, 0x67,
    0x6c, 0x65, 0x03, 0x63, 0x6f, 0x6d, 0x00, 0x00, 0x01, 0x00,
    0x01
};

// Expected checksums.
static const uint32_t kUdpPartialChecksum     = 0xd5c8;
static const uint32_t kPayloadPartialChecksum = 0x31e9c;
static const uint16_t kUdpV4Checksum          = 0xd0c7;
static const uint16_t kUdpV6Checksum          = 0xa74a;

uint8_t ip_version(const uint8_t *packet) {
  uint8_t version = packet[0] >> 4;
  return version;
}

int is_ipv4_fragment(struct iphdr *ip) {
  // A packet is a fragment if its fragment offset is nonzero or if the MF flag is set.
  return ntohs(ip->frag_off) & (IP_OFFMASK | IP_MF);
}

int is_ipv6_fragment(struct ip6_hdr *ip6, size_t len) {
  if (ip6->ip6_nxt != IPPROTO_FRAGMENT) {
    return 0;
  }
  struct ip6_frag *frag = (struct ip6_frag *) (ip6 + 1);
  return len >= sizeof(*ip6) + sizeof(*frag) &&
          (frag->ip6f_offlg & (IP6F_OFF_MASK | IP6F_MORE_FRAG));
}

int ipv4_fragment_offset(struct iphdr *ip) {
  return ntohs(ip->frag_off) & IP_OFFMASK;
}

int ipv6_fragment_offset(struct ip6_frag *frag) {
  return ntohs((frag->ip6f_offlg & IP6F_OFF_MASK) >> 3);
}

void check_packet(const uint8_t *packet, size_t len, const char *msg) {
  void *payload;
  size_t payload_length = 0;
  uint32_t pseudo_checksum = 0;
  uint8_t protocol = 0;
  int version = ip_version(packet);
  switch (version) {
    case 4: {
      struct iphdr *ip = (struct iphdr *) packet;
      ASSERT_GE(len, sizeof(*ip)) << msg << ": IPv4 packet shorter than IPv4 header\n";
      EXPECT_EQ(5, ip->ihl) << msg << ": Unsupported IP header length\n";
      EXPECT_EQ(len, ntohs(ip->tot_len)) << msg << ": Incorrect IPv4 length\n";
      EXPECT_EQ(0, ip_checksum(ip, sizeof(*ip))) << msg << ": Incorrect IP checksum\n";
      protocol = ip->protocol;
      payload = ip + 1;
      if (!is_ipv4_fragment(ip)) {
        payload_length = len - sizeof(*ip);
        pseudo_checksum = ipv4_pseudo_header_checksum(ip, payload_length);
      }
      ASSERT_TRUE(protocol == IPPROTO_TCP || protocol == IPPROTO_UDP || protocol == IPPROTO_ICMP)
          << msg << ": Unsupported IPv4 protocol " << protocol << "\n";
      break;
    }
    case 6: {
      struct ip6_hdr *ip6 = (struct ip6_hdr *) packet;
      ASSERT_GE(len, sizeof(*ip6)) << msg << ": IPv6 packet shorter than IPv6 header\n";
      EXPECT_EQ(len - sizeof(*ip6), htons(ip6->ip6_plen)) << msg << ": Incorrect IPv6 length\n";

      if (ip6->ip6_nxt == IPPROTO_FRAGMENT) {
        struct ip6_frag *frag = (struct ip6_frag *) (ip6 + 1);
        ASSERT_GE(len, sizeof(*ip6) + sizeof(*frag))
            << msg << ": IPv6 fragment: short fragment header\n";
        protocol = frag->ip6f_nxt;
        payload = frag + 1;
        // Even though the packet has a Fragment header, it might not be a fragment.
        if (!is_ipv6_fragment(ip6, len)) {
          payload_length = len - sizeof(*ip6) - sizeof(*frag);
        }
      } else {
        // Since there are no extension headers except Fragment, this must be the payload.
        protocol = ip6->ip6_nxt;
        payload = ip6 + 1;
        payload_length = len - sizeof(*ip6);
      }
      ASSERT_TRUE(protocol == IPPROTO_TCP || protocol == IPPROTO_UDP || protocol == IPPROTO_ICMPV6)
          << msg << ": Unsupported IPv6 next header " << protocol;
      if (payload_length) {
        pseudo_checksum = ipv6_pseudo_header_checksum(ip6, payload_length, protocol);
      }
      break;
    }
    default:
      FAIL() << msg << ": Unsupported IP version " << version << "\n";
      return;
  }

  // If we understand the payload, verify the checksum.
  if (payload_length) {
    uint16_t checksum;
    switch(protocol) {
      case IPPROTO_UDP:
      case IPPROTO_TCP:
      case IPPROTO_ICMPV6:
        checksum = ip_checksum_finish(ip_checksum_add(pseudo_checksum, payload, payload_length));
        break;
      case IPPROTO_ICMP:
        checksum = ip_checksum(payload, payload_length);
        break;
      default:
        checksum = 0;  // Don't check.
        break;
    }
    EXPECT_EQ(0, checksum) << msg << ": Incorrect transport checksum\n";
  }

  if (protocol == IPPROTO_UDP) {
    struct udphdr *udp = (struct udphdr *) payload;
    EXPECT_NE(0, udp->check) << msg << ": UDP checksum 0 should be 0xffff";
    // If this is not a fragment, check the UDP length field.
    if (payload_length) {
      EXPECT_EQ(payload_length, ntohs(udp->len)) << msg << ": Incorrect UDP length\n";
    }
  }
}

void reassemble_packet(const uint8_t **fragments, const size_t lengths[], int numpackets,
                       uint8_t *reassembled, size_t *reassembled_len, const char *msg) {
  struct iphdr *ip = NULL;
  struct ip6_hdr *ip6 = NULL;
  size_t  total_length, pos = 0;
  uint8_t protocol = 0;
  uint8_t version = ip_version(fragments[0]);

  for (int i = 0; i < numpackets; i++) {
    const uint8_t *packet = fragments[i];
    int len = lengths[i];
    int headersize, payload_offset;

    ASSERT_EQ(ip_version(packet), version) << msg << ": Inconsistent fragment versions\n";
    check_packet(packet, len, "Fragment sanity check");

    switch (version) {
      case 4: {
        struct iphdr *ip_orig = (struct iphdr *) packet;
        headersize = sizeof(*ip_orig);
        ASSERT_TRUE(is_ipv4_fragment(ip_orig))
            << msg << ": IPv4 fragment #" << i + 1 << " not a fragment\n";
        ASSERT_EQ(pos, ipv4_fragment_offset(ip_orig) * 8 + ((i != 0) ? sizeof(*ip): 0))
            << msg << ": IPv4 fragment #" << i + 1 << ": inconsistent offset\n";

        headersize = sizeof(*ip_orig);
        payload_offset = headersize;
        if (pos == 0) {
          ip = (struct iphdr *) reassembled;
        }
        break;
      }
      case 6: {
        struct ip6_hdr *ip6_orig = (struct ip6_hdr *) packet;
        struct ip6_frag *frag = (struct ip6_frag *) (ip6_orig + 1);
        ASSERT_TRUE(is_ipv6_fragment(ip6_orig, len))
            << msg << ": IPv6 fragment #" << i + 1 << " not a fragment\n";
        ASSERT_EQ(pos, ipv6_fragment_offset(frag) * 8 + ((i != 0) ? sizeof(*ip6): 0))
            << msg << ": IPv6 fragment #" << i + 1 << ": inconsistent offset\n";

        headersize = sizeof(*ip6_orig);
        payload_offset = sizeof(*ip6_orig) + sizeof(*frag);
        if (pos == 0) {
          ip6 = (struct ip6_hdr *) reassembled;
          protocol = frag->ip6f_nxt;
        }
        break;
      }
      default:
        FAIL() << msg << ": Invalid IP version << " << version;
    }

    // If this is the first fragment, copy the header.
    if (pos == 0) {
      ASSERT_LT(headersize, (int) *reassembled_len) << msg << ": Reassembly buffer too small\n";
      memcpy(reassembled, packet, headersize);
      total_length = headersize;
      pos += headersize;
    }

    // Copy the payload.
    int payload_length = len - payload_offset;
    total_length += payload_length;
    ASSERT_LT(total_length, *reassembled_len) << msg << ": Reassembly buffer too small\n";
    memcpy(reassembled + pos, packet + payload_offset, payload_length);
    pos += payload_length;
  }


  // Fix up the reassembled headers to reflect fragmentation and length (and IPv4 checksum).
  ASSERT_EQ(total_length, pos) << msg << ": Reassembled packet length incorrect\n";
  if (ip) {
    ip->frag_off &= ~htons(IP_MF);
    ip->tot_len = htons(total_length);
    ip->check = 0;
    ip->check = ip_checksum(ip, sizeof(*ip));
    ASSERT_FALSE(is_ipv4_fragment(ip)) << msg << ": reassembled IPv4 packet is a fragment!\n";
  }
  if (ip6) {
    ip6->ip6_nxt = protocol;
    ip6->ip6_plen = htons(total_length - sizeof(*ip6));
    ASSERT_FALSE(is_ipv6_fragment(ip6, ip6->ip6_plen))
        << msg << ": reassembled IPv6 packet is a fragment!\n";
  }

  *reassembled_len = total_length;
}

void check_data_matches(const void *expected, const void *actual, size_t len, const char *msg) {
  if (memcmp(expected, actual, len)) {
    // Hex dump, 20 bytes per line, one space between bytes (1 byte = 3 chars), indented by 4.
    int hexdump_len = len * 3 + (len / 20 + 1) * 5;
    char expected_hexdump[hexdump_len], actual_hexdump[hexdump_len];
    unsigned pos = 0;
    for (unsigned i = 0; i < len; i++) {
      if (i % 20 == 0) {
        sprintf(expected_hexdump + pos, "\n   ");
        sprintf(actual_hexdump + pos, "\n   ");
        pos += 4;
      }
      sprintf(expected_hexdump + pos, " %02x", ((uint8_t *) expected)[i]);
      sprintf(actual_hexdump + pos, " %02x", ((uint8_t *) actual)[i]);
      pos += 3;
    }
    FAIL() << msg << ": Data doesn't match"
           << "\n  Expected:" << (char *) expected_hexdump
           << "\n  Actual:" << (char *) actual_hexdump << "\n";
  }
}

void fix_udp_checksum(uint8_t* packet) {
  uint32_t pseudo_checksum;
  uint8_t version = ip_version(packet);
  struct udphdr *udp;
  switch (version) {
    case 4: {
      struct iphdr *ip = (struct iphdr *) packet;
      udp = (struct udphdr *) (ip + 1);
      pseudo_checksum = ipv4_pseudo_header_checksum(ip, ntohs(udp->len));
      break;
    }
    case 6: {
      struct ip6_hdr *ip6 = (struct ip6_hdr *) packet;
      udp = (struct udphdr *) (ip6 + 1);
      pseudo_checksum = ipv6_pseudo_header_checksum(ip6, ntohs(udp->len), IPPROTO_UDP);
      break;
    }
    default:
      FAIL() << "unsupported IP version" << version << "\n";
      return;
    }

  udp->check = 0;
  udp->check = ip_checksum_finish(ip_checksum_add(pseudo_checksum, udp, ntohs(udp->len)));
}

// Testing stub for send_rawv6. The real version uses sendmsg() with a
// destination IPv6 address, and attempting to call that on our test socketpair
// fd results in EINVAL.
extern "C" void send_rawv6(int fd, clat_packet out, int iov_len) {
    writev(fd, out, iov_len);
}

void do_translate_packet(const uint8_t *original, size_t original_len, uint8_t *out, size_t *outlen,
                         const char *msg) {
  int fds[2];
  if (socketpair(AF_UNIX, SOCK_DGRAM | SOCK_NONBLOCK, 0, fds)) {
    abort();
  }

  char foo[512];
  snprintf(foo, sizeof(foo), "%s: Invalid original packet", msg);
  check_packet(original, original_len, foo);

  int read_fd, write_fd;
  uint16_t expected_proto;
  int version = ip_version(original);
  switch (version) {
    case 4:
      expected_proto = htons(ETH_P_IPV6);
      read_fd = fds[1];
      write_fd = fds[0];
      break;
    case 6:
      expected_proto = htons(ETH_P_IP);
      read_fd = fds[0];
      write_fd = fds[1];
      break;
    default:
      FAIL() << msg << ": Unsupported IP version " << version << "\n";
      break;
  }

  translate_packet(write_fd, (version == 4), original, original_len);

  snprintf(foo, sizeof(foo), "%s: Invalid translated packet", msg);
  if (version == 6) {
    // Translating to IPv4. Expect a tun header.
    struct tun_pi new_tun_header;
    struct iovec iov[] = {
      { &new_tun_header, sizeof(new_tun_header) },
      { out, *outlen }
    };
    int len = readv(read_fd, iov, 2);
    if (len > (int) sizeof(new_tun_header)) {
      ASSERT_LT((size_t) len, *outlen) << msg << ": Translated packet buffer too small\n";
      EXPECT_EQ(expected_proto, new_tun_header.proto) << msg << "Unexpected tun proto\n";
      *outlen = len - sizeof(new_tun_header);
      check_packet(out, *outlen, msg);
    } else {
      FAIL() << msg << ": Packet was not translated: len=" << len;
      *outlen = 0;
    }
  } else {
    // Translating to IPv6. Expect raw packet.
    *outlen = read(read_fd, out, *outlen);
    check_packet(out, *outlen, msg);
  }
}

void check_translated_packet(const uint8_t *original, size_t original_len,
                             const uint8_t *expected, size_t expected_len, const char *msg) {
  uint8_t translated[MAXMTU];
  size_t translated_len = sizeof(translated);
  do_translate_packet(original, original_len, translated, &translated_len, msg);
  EXPECT_EQ(expected_len, translated_len) << msg << ": Translated packet length incorrect\n";
  check_data_matches(expected, translated, translated_len, msg);
}

void check_fragment_translation(const uint8_t *original[], const size_t original_lengths[],
                                const uint8_t *expected[], const size_t expected_lengths[],
                                int numfragments, const char *msg) {
  for (int i = 0; i < numfragments; i++) {
    // Check that each of the fragments translates as expected.
    char frag_msg[512];
    snprintf(frag_msg, sizeof(frag_msg), "%s: fragment #%d", msg, i + 1);
    check_translated_packet(original[i], original_lengths[i],
                            expected[i], expected_lengths[i], frag_msg);
  }

  // Sanity check that reassembling the original and translated fragments produces valid packets.
  uint8_t reassembled[MAXMTU];
  size_t reassembled_len = sizeof(reassembled);
  reassemble_packet(original, original_lengths, numfragments, reassembled, &reassembled_len, msg);
  check_packet(reassembled, reassembled_len, msg);

  uint8_t translated[MAXMTU];
  size_t translated_len = sizeof(translated);
  do_translate_packet(reassembled, reassembled_len, translated, &translated_len, msg);
  check_packet(translated, translated_len, msg);
}

int get_transport_checksum(const uint8_t *packet) {
  struct iphdr *ip;
  struct ip6_hdr *ip6;
  uint8_t protocol;
  const void *payload;

  int version = ip_version(packet);
  switch (version) {
    case 4:
      ip = (struct iphdr *) packet;
      if (is_ipv4_fragment(ip)) {
          return -1;
      }
      protocol = ip->protocol;
      payload = ip + 1;
      break;
    case 6:
      ip6 = (struct ip6_hdr *) packet;
      protocol = ip6->ip6_nxt;
      payload = ip6 + 1;
      break;
    default:
      return -1;
  }

  switch (protocol) {
    case IPPROTO_UDP:
      return ((struct udphdr *) payload)->check;

    case IPPROTO_TCP:
      return ((struct tcphdr *) payload)->check;

    case IPPROTO_FRAGMENT:
    default:
      return -1;
  }
}

struct clat_config Global_Clatd_Config;

class ClatdTest : public ::testing::Test {
 protected:
  virtual void SetUp() {
    inet_pton(AF_INET, kIPv4LocalAddr, &Global_Clatd_Config.ipv4_local_subnet);
    inet_pton(AF_INET6, kIPv6PlatSubnet, &Global_Clatd_Config.plat_subnet);
    inet_pton(AF_INET6, kIPv6LocalAddr, &Global_Clatd_Config.ipv6_local_subnet);
    Global_Clatd_Config.ipv6_host_id = in6addr_any;
    Global_Clatd_Config.use_dynamic_iid = 1;
  }
};

void expect_ipv6_addr_equal(struct in6_addr *expected, struct in6_addr *actual) {
  if (!IN6_ARE_ADDR_EQUAL(expected, actual)) {
    char expected_str[INET6_ADDRSTRLEN], actual_str[INET6_ADDRSTRLEN];
    inet_ntop(AF_INET6, expected, expected_str, sizeof(expected_str));
    inet_ntop(AF_INET6, actual, actual_str, sizeof(actual_str));
    FAIL()
        << "Unexpected IPv6 address:: "
        << "\n  Expected: " << expected_str
        << "\n  Actual:   " << actual_str
        << "\n";
  }
}

TEST_F(ClatdTest, TestIPv6PrefixEqual) {
  EXPECT_TRUE(ipv6_prefix_equal(&Global_Clatd_Config.plat_subnet,
                                &Global_Clatd_Config.plat_subnet));
  EXPECT_FALSE(ipv6_prefix_equal(&Global_Clatd_Config.plat_subnet,
                                 &Global_Clatd_Config.ipv6_local_subnet));

  struct in6_addr subnet2 = Global_Clatd_Config.ipv6_local_subnet;
  EXPECT_TRUE(ipv6_prefix_equal(&Global_Clatd_Config.ipv6_local_subnet, &subnet2));
  EXPECT_TRUE(ipv6_prefix_equal(&subnet2, &Global_Clatd_Config.ipv6_local_subnet));

  subnet2.s6_addr[6] = 0xff;
  EXPECT_FALSE(ipv6_prefix_equal(&Global_Clatd_Config.ipv6_local_subnet, &subnet2));
  EXPECT_FALSE(ipv6_prefix_equal(&subnet2, &Global_Clatd_Config.ipv6_local_subnet));
}

int count_onebits(const void *data, size_t size) {
  int onebits = 0;
  for (size_t pos = 0; pos < size; pos++) {
    uint8_t *byte = ((uint8_t*) data) + pos;
    for (int shift = 0; shift < 8; shift++) {
      onebits += (*byte >> shift) & 1;
    }
  }
  return onebits;
}

TEST_F(ClatdTest, TestCountOnebits) {
  uint64_t i;
  i = 1;
  ASSERT_EQ(1, count_onebits(&i, sizeof(i)));
  i <<= 61;
  ASSERT_EQ(1, count_onebits(&i, sizeof(i)));
  i |= ((uint64_t) 1 << 33);
  ASSERT_EQ(2, count_onebits(&i, sizeof(i)));
  i = 0xf1000202020000f0;
  ASSERT_EQ(5 + 1 + 1 + 1 + 4, count_onebits(&i, sizeof(i)));
}

TEST_F(ClatdTest, TestGenIIDConfigured) {
  struct in6_addr myaddr, expected;
  Global_Clatd_Config.use_dynamic_iid = 0;
  ASSERT_TRUE(inet_pton(AF_INET6, "::bad:ace:d00d", &Global_Clatd_Config.ipv6_host_id));
  ASSERT_TRUE(inet_pton(AF_INET6, "2001:db8:1:2:0:bad:ace:d00d", &expected));
  ASSERT_TRUE(inet_pton(AF_INET6, "2001:db8:1:2:f076:ae99:124e:aa54", &myaddr));
  config_generate_local_ipv6_subnet(&myaddr);
  expect_ipv6_addr_equal(&expected, &myaddr);

  Global_Clatd_Config.use_dynamic_iid = 1;
  config_generate_local_ipv6_subnet(&myaddr);
  EXPECT_FALSE(IN6_ARE_ADDR_EQUAL(&expected, &myaddr));
}

TEST_F(ClatdTest, TestGenIIDRandom) {
  struct in6_addr interface_ipv6;
  ASSERT_TRUE(inet_pton(AF_INET6, "2001:db8:1:2:f076:ae99:124e:aa54", &interface_ipv6));
  Global_Clatd_Config.ipv6_host_id = in6addr_any;

  // Generate a boatload of random IIDs.
  int onebits = 0;
  uint64_t prev_iid = 0;
  for (int i = 0; i < 100000; i++) {
    struct in6_addr myaddr =  interface_ipv6;

    config_generate_local_ipv6_subnet(&myaddr);

    // Check the generated IP address is in the same prefix as the interface IPv6 address.
    EXPECT_TRUE(ipv6_prefix_equal(&interface_ipv6, &myaddr));

    // Check that consecutive IIDs are not the same.
    uint64_t iid = * (uint64_t*) (&myaddr.s6_addr[8]);
    ASSERT_TRUE(iid != prev_iid)
        << "Two consecutive random IIDs are the same: "
        << std::showbase << std::hex
        << iid << "\n";
    prev_iid = iid;

    // Check that the IID is checksum-neutral with the NAT64 prefix and the
    // local prefix.
    struct in_addr *ipv4addr = &Global_Clatd_Config.ipv4_local_subnet;
    struct in6_addr *plat_subnet = &Global_Clatd_Config.plat_subnet;

    uint16_t c1 = ip_checksum_finish(ip_checksum_add(0, ipv4addr, sizeof(*ipv4addr)));
    uint16_t c2 = ip_checksum_finish(ip_checksum_add(0, plat_subnet, sizeof(*plat_subnet)) +
                                     ip_checksum_add(0, &myaddr, sizeof(myaddr)));

    if (c1 != c2) {
      char myaddr_str[INET6_ADDRSTRLEN], plat_str[INET6_ADDRSTRLEN], ipv4_str[INET6_ADDRSTRLEN];
      inet_ntop(AF_INET6, &myaddr, myaddr_str, sizeof(myaddr_str));
      inet_ntop(AF_INET6, plat_subnet, plat_str, sizeof(plat_str));
      inet_ntop(AF_INET, ipv4addr, ipv4_str, sizeof(ipv4_str));
      FAIL()
          << "Bad IID: " << myaddr_str
          << " not checksum-neutral with " << ipv4_str << " and " << plat_str
          << std::showbase << std::hex
          << "\n  IPv4 checksum: " << c1
          << "\n  IPv6 checksum: " << c2
          << "\n";
    }

    // Check that IIDs are roughly random and use all the bits by counting the
    // total number of bits set to 1 in a random sample of 100000 generated IIDs.
    onebits += count_onebits(&iid, sizeof(iid));
  }
  EXPECT_LE(3190000, onebits);
  EXPECT_GE(3210000, onebits);
}

extern "C" addr_free_func config_is_ipv4_address_free;
int never_free(in_addr_t /* addr */) { return 0; }
int always_free(in_addr_t /* addr */) { return 1; }
int only2_free(in_addr_t addr) { return (ntohl(addr) & 0xff) == 2; }
int over6_free(in_addr_t addr) { return (ntohl(addr) & 0xff) >= 6; }
int only10_free(in_addr_t addr) { return (ntohl(addr) & 0xff) == 10; }

TEST_F(ClatdTest, SelectIPv4Address) {
  struct in_addr addr;

  inet_pton(AF_INET, kIPv4LocalAddr, &addr);

  addr_free_func orig_config_is_ipv4_address_free = config_is_ipv4_address_free;

  // If no addresses are free, return INADDR_NONE.
  config_is_ipv4_address_free = never_free;
  EXPECT_EQ(INADDR_NONE, config_select_ipv4_address(&addr, 29));
  EXPECT_EQ(INADDR_NONE, config_select_ipv4_address(&addr, 16));

  // If the configured address is free, pick that. But a prefix that's too big is invalid.
  config_is_ipv4_address_free = always_free;
  EXPECT_EQ(inet_addr(kIPv4LocalAddr), config_select_ipv4_address(&addr, 29));
  EXPECT_EQ(inet_addr(kIPv4LocalAddr), config_select_ipv4_address(&addr, 20));
  EXPECT_EQ(INADDR_NONE, config_select_ipv4_address(&addr, 15));

  // A prefix length of 32 works, but anything above it is invalid.
  EXPECT_EQ(inet_addr(kIPv4LocalAddr), config_select_ipv4_address(&addr, 32));
  EXPECT_EQ(INADDR_NONE, config_select_ipv4_address(&addr, 33));

  // If another address is free, pick it.
  config_is_ipv4_address_free = over6_free;
  EXPECT_EQ(inet_addr("192.0.0.6"), config_select_ipv4_address(&addr, 29));

  // Check that we wrap around to addresses that are lower than the first address.
  config_is_ipv4_address_free = only2_free;
  EXPECT_EQ(inet_addr("192.0.0.2"), config_select_ipv4_address(&addr, 29));
  EXPECT_EQ(INADDR_NONE, config_select_ipv4_address(&addr, 30));

  // If a free address exists outside the prefix, we don't pick it.
  config_is_ipv4_address_free = only10_free;
  EXPECT_EQ(INADDR_NONE, config_select_ipv4_address(&addr, 29));
  EXPECT_EQ(inet_addr("192.0.0.10"), config_select_ipv4_address(&addr, 24));

  // Now try using the real function which sees if IP addresses are free using bind().
  // Assume that the machine running the test has the address 127.0.0.1, but not 8.8.8.8.
  config_is_ipv4_address_free = orig_config_is_ipv4_address_free;
  addr.s_addr = inet_addr("8.8.8.8");
  EXPECT_EQ(inet_addr("8.8.8.8"), config_select_ipv4_address(&addr, 29));

  addr.s_addr = inet_addr("127.0.0.1");
  EXPECT_EQ(inet_addr("127.0.0.2"), config_select_ipv4_address(&addr, 29));
}

TEST_F(ClatdTest, DataSanitycheck) {
  // Sanity checks the data.
  uint8_t v4_header[] = { IPV4_UDP_HEADER };
  ASSERT_EQ(sizeof(struct iphdr), sizeof(v4_header)) << "Test IPv4 header: incorrect length\n";

  uint8_t v6_header[] = { IPV6_UDP_HEADER };
  ASSERT_EQ(sizeof(struct ip6_hdr), sizeof(v6_header)) << "Test IPv6 header: incorrect length\n";

  uint8_t udp_header[] = { UDP_HEADER };
  ASSERT_EQ(sizeof(struct udphdr), sizeof(udp_header)) << "Test UDP header: incorrect length\n";

  // Sanity checks check_packet.
  struct udphdr *udp;
  uint8_t v4_udp_packet[] = { IPV4_UDP_HEADER UDP_HEADER PAYLOAD };
  udp = (struct udphdr *) (v4_udp_packet + sizeof(struct iphdr));
  fix_udp_checksum(v4_udp_packet);
  ASSERT_EQ(kUdpV4Checksum, udp->check) << "UDP/IPv4 packet checksum sanity check\n";
  check_packet(v4_udp_packet, sizeof(v4_udp_packet), "UDP/IPv4 packet sanity check");

  uint8_t v6_udp_packet[] = { IPV6_UDP_HEADER UDP_HEADER PAYLOAD };
  udp = (struct udphdr *) (v6_udp_packet + sizeof(struct ip6_hdr));
  fix_udp_checksum(v6_udp_packet);
  ASSERT_EQ(kUdpV6Checksum, udp->check) << "UDP/IPv6 packet checksum sanity check\n";
  check_packet(v6_udp_packet, sizeof(v6_udp_packet), "UDP/IPv6 packet sanity check");

  uint8_t ipv4_ping[] = { IPV4_ICMP_HEADER IPV4_PING PAYLOAD };
  check_packet(ipv4_ping, sizeof(ipv4_ping), "IPv4 ping sanity check");

  uint8_t ipv6_ping[] = { IPV6_ICMPV6_HEADER IPV6_PING PAYLOAD };
  check_packet(ipv6_ping, sizeof(ipv6_ping), "IPv6 ping sanity check");

  // Sanity checks reassemble_packet.
  uint8_t reassembled[MAXMTU];
  size_t total_length = sizeof(reassembled);
  reassemble_packet(kIPv4Fragments, kIPv4FragLengths, ARRAYSIZE(kIPv4Fragments),
                    reassembled, &total_length, "Reassembly sanity check");
  check_packet(reassembled, total_length, "IPv4 Reassembled packet is valid");
  ASSERT_EQ(sizeof(kReassembledIPv4), total_length) << "IPv4 reassembly sanity check: length\n";
  ASSERT_TRUE(!is_ipv4_fragment((struct iphdr *) reassembled))
      << "Sanity check: reassembled packet is a fragment!\n";
  check_data_matches(kReassembledIPv4, reassembled, total_length, "IPv4 reassembly sanity check");

  total_length = sizeof(reassembled);
  reassemble_packet(kIPv6Fragments, kIPv6FragLengths, ARRAYSIZE(kIPv6Fragments),
                    reassembled, &total_length, "IPv6 reassembly sanity check");
  ASSERT_TRUE(!is_ipv6_fragment((struct ip6_hdr *) reassembled, total_length))
      << "Sanity check: reassembled packet is a fragment!\n";
  check_packet(reassembled, total_length, "IPv6 Reassembled packet is valid");
}

TEST_F(ClatdTest, PseudoChecksum) {
  uint32_t pseudo_checksum;

  uint8_t v4_header[] = { IPV4_UDP_HEADER };
  uint8_t v4_pseudo_header[] = { IPV4_PSEUDOHEADER(v4_header, UDP_LEN) };
  pseudo_checksum = ipv4_pseudo_header_checksum((struct iphdr *) v4_header, UDP_LEN);
  EXPECT_EQ(ip_checksum_finish(pseudo_checksum),
            ip_checksum(v4_pseudo_header, sizeof(v4_pseudo_header)))
            << "ipv4_pseudo_header_checksum incorrect\n";

  uint8_t v6_header[] = { IPV6_UDP_HEADER };
  uint8_t v6_pseudo_header[] = { IPV6_PSEUDOHEADER(v6_header, IPPROTO_UDP, UDP_LEN) };
  pseudo_checksum = ipv6_pseudo_header_checksum((struct ip6_hdr *) v6_header, UDP_LEN, IPPROTO_UDP);
  EXPECT_EQ(ip_checksum_finish(pseudo_checksum),
            ip_checksum(v6_pseudo_header, sizeof(v6_pseudo_header)))
            << "ipv6_pseudo_header_checksum incorrect\n";
}

TEST_F(ClatdTest, TransportChecksum) {
  uint8_t udphdr[] = { UDP_HEADER };
  uint8_t payload[] = { PAYLOAD };
  EXPECT_EQ(kUdpPartialChecksum, ip_checksum_add(0, udphdr, sizeof(udphdr)))
            << "UDP partial checksum\n";
  EXPECT_EQ(kPayloadPartialChecksum, ip_checksum_add(0, payload, sizeof(payload)))
            << "Payload partial checksum\n";

  uint8_t ip[] = { IPV4_UDP_HEADER };
  uint8_t ip6[] = { IPV6_UDP_HEADER };
  uint32_t ipv4_pseudo_sum = ipv4_pseudo_header_checksum((struct iphdr *) ip, UDP_LEN);
  uint32_t ipv6_pseudo_sum = ipv6_pseudo_header_checksum((struct ip6_hdr *) ip6, UDP_LEN,
                                                         IPPROTO_UDP);

  EXPECT_EQ(0x3ad0U, ipv4_pseudo_sum) << "IPv4 pseudo-checksum sanity check\n";
  EXPECT_EQ(0x2644bU, ipv6_pseudo_sum) << "IPv6 pseudo-checksum sanity check\n";
  EXPECT_EQ(
      kUdpV4Checksum,
      ip_checksum_finish(ipv4_pseudo_sum + kUdpPartialChecksum + kPayloadPartialChecksum))
      << "Unexpected UDP/IPv4 checksum\n";
  EXPECT_EQ(
      kUdpV6Checksum,
      ip_checksum_finish(ipv6_pseudo_sum + kUdpPartialChecksum + kPayloadPartialChecksum))
      << "Unexpected UDP/IPv6 checksum\n";

  EXPECT_EQ(kUdpV6Checksum,
      ip_checksum_adjust(kUdpV4Checksum, ipv4_pseudo_sum, ipv6_pseudo_sum))
      << "Adjust IPv4/UDP checksum to IPv6\n";
  EXPECT_EQ(kUdpV4Checksum,
      ip_checksum_adjust(kUdpV6Checksum, ipv6_pseudo_sum, ipv4_pseudo_sum))
      << "Adjust IPv6/UDP checksum to IPv4\n";
}

TEST_F(ClatdTest, AdjustChecksum) {
  struct checksum_data {
    uint16_t checksum;
    uint32_t old_hdr_sum;
    uint32_t new_hdr_sum;
    uint16_t result;
  } DATA[] = {
    { 0x1423, 0xb8ec, 0x2d757, 0xf5b5 },
    { 0xf5b5, 0x2d757, 0xb8ec, 0x1423 },
    { 0xdd2f, 0x5555, 0x3285, 0x0000 },
    { 0x1215, 0x5560, 0x15560 + 20, 0x1200 },
    { 0xd0c7, 0x3ad0, 0x2644b, 0xa74a },
  };
  unsigned i = 0;

  for (i = 0; i < ARRAYSIZE(DATA); i++) {
    struct checksum_data *data = DATA + i;
    uint16_t result = ip_checksum_adjust(data->checksum, data->old_hdr_sum, data->new_hdr_sum);
    EXPECT_EQ(result, data->result)
        << "Incorrect checksum" << std::showbase << std::hex
        << "\n  Expected: " << data->result
        << "\n  Actual:   " << result
        << "\n    checksum=" << data->checksum
        << " old_sum=" << data->old_hdr_sum << " new_sum=" << data->new_hdr_sum << "\n";
  }
}

TEST_F(ClatdTest, Translate) {
  uint8_t udp_ipv4[] = { IPV4_UDP_HEADER UDP_HEADER PAYLOAD };
  uint8_t udp_ipv6[] = { IPV6_UDP_HEADER UDP_HEADER PAYLOAD };
  fix_udp_checksum(udp_ipv4);
  fix_udp_checksum(udp_ipv6);
  check_translated_packet(udp_ipv4, sizeof(udp_ipv4), udp_ipv6, sizeof(udp_ipv6),
                          "UDP/IPv4 -> UDP/IPv6 translation");
  check_translated_packet(udp_ipv6, sizeof(udp_ipv6), udp_ipv4, sizeof(udp_ipv4),
                          "UDP/IPv6 -> UDP/IPv4 translation");

  uint8_t ipv4_ping[] = { IPV4_ICMP_HEADER IPV4_PING PAYLOAD };
  uint8_t ipv6_ping[] = { IPV6_ICMPV6_HEADER IPV6_PING PAYLOAD };
  check_translated_packet(ipv4_ping, sizeof(ipv4_ping), ipv6_ping, sizeof(ipv6_ping),
                          "ICMP->ICMPv6 translation");
  check_translated_packet(ipv6_ping, sizeof(ipv6_ping), ipv4_ping, sizeof(ipv4_ping),
                          "ICMPv6->ICMP translation");
}

TEST_F(ClatdTest, Fragmentation) {
  check_fragment_translation(kIPv4Fragments, kIPv4FragLengths,
                             kIPv6Fragments, kIPv6FragLengths,
                             ARRAYSIZE(kIPv4Fragments), "IPv4->IPv6 fragment translation");

  check_fragment_translation(kIPv6Fragments, kIPv6FragLengths,
                             kIPv4Fragments, kIPv4FragLengths,
                             ARRAYSIZE(kIPv6Fragments), "IPv6->IPv4 fragment translation");
}

void check_translate_checksum_neutral(const uint8_t *original, size_t original_len,
                                      size_t expected_len, const char *msg) {
  uint8_t translated[MAXMTU];
  size_t translated_len = sizeof(translated);
  do_translate_packet(original, original_len, translated, &translated_len, msg);
  EXPECT_EQ(expected_len, translated_len) << msg << ": Translated packet length incorrect\n";
  // do_translate_packet already checks packets for validity and verifies the checksum.
  int original_check = get_transport_checksum(original);
  int translated_check = get_transport_checksum(translated);
  ASSERT_NE(-1, original_check);
  ASSERT_NE(-1, translated_check);
  ASSERT_EQ(original_check, translated_check)
      << "Not checksum neutral: original and translated checksums differ\n";
}

TEST_F(ClatdTest, TranslateChecksumNeutral) {
  // Generate a random clat IPv6 address and check that translation is checksum-neutral.
  Global_Clatd_Config.ipv6_host_id = in6addr_any;
  ASSERT_TRUE(inet_pton(AF_INET6, "2001:db8:1:2:f076:ae99:124e:aa54",
                        &Global_Clatd_Config.ipv6_local_subnet));
  config_generate_local_ipv6_subnet(&Global_Clatd_Config.ipv6_local_subnet);
  ASSERT_NE((uint32_t) 0x00000464, Global_Clatd_Config.ipv6_local_subnet.s6_addr32[3]);
  ASSERT_NE((uint32_t) 0, Global_Clatd_Config.ipv6_local_subnet.s6_addr32[3]);

  // Check that translating UDP packets is checksum-neutral. First, IPv4.
  uint8_t udp_ipv4[] = { IPV4_UDP_HEADER UDP_HEADER PAYLOAD };
  fix_udp_checksum(udp_ipv4);
  check_translate_checksum_neutral(udp_ipv4, sizeof(udp_ipv4), sizeof(udp_ipv4) + 20,
                                   "UDP/IPv4 -> UDP/IPv6 checksum neutral");

  // Now try IPv6.
  uint8_t udp_ipv6[] = { IPV6_UDP_HEADER UDP_HEADER PAYLOAD };
  // The test packet uses the static IID, not the random IID. Fix up the source address.
  struct ip6_hdr *ip6 = (struct ip6_hdr *) udp_ipv6;
  memcpy(&ip6->ip6_src, &Global_Clatd_Config.ipv6_local_subnet, sizeof(ip6->ip6_src));
  fix_udp_checksum(udp_ipv6);
  check_translate_checksum_neutral(udp_ipv4, sizeof(udp_ipv4), sizeof(udp_ipv4) + 20,
                                   "UDP/IPv4 -> UDP/IPv6 checksum neutral");
}
