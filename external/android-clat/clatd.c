/*
 * Copyright 2012 Daniel Drown
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
 * clatd.c - tun interface setup and main event loop
 */
#include <poll.h>
#include <signal.h>
#include <time.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/prctl.h>
#include <sys/stat.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <fcntl.h>

#include <sys/capability.h>
#include <sys/uio.h>
#include <linux/filter.h>
#include <linux/if.h>
#include <linux/if_tun.h>
#include <linux/if_ether.h>
#include <linux/if_packet.h>
#include <net/if.h>

#include <private/android_filesystem_config.h>

#include "translate.h"
#include "clatd.h"
#include "config.h"
#include "logging.h"
#include "resolv_netid.h"
#include "setif.h"
#include "mtu.h"
#include "getaddr.h"
#include "dump.h"
#include "tun.h"
#include "ring.h"

#define DEVICEPREFIX "v4-"

/* 40 bytes IPv6 header - 20 bytes IPv4 header + 8 bytes fragment header */
#define MTU_DELTA 28

volatile sig_atomic_t running = 1;

/* function: stop_loop
 * signal handler: stop the event loop
 */
void stop_loop() {
  running = 0;
}

/* function: configure_packet_socket
 * Binds the packet socket and attaches the receive filter to it.
 * sock - the socket to configure
 */
int configure_packet_socket(int sock) {
  struct sockaddr_ll sll = {
    .sll_family   = AF_PACKET,
    .sll_protocol = htons(ETH_P_IPV6),
    .sll_ifindex  = if_nametoindex(Global_Clatd_Config.default_pdp_interface),
    .sll_pkttype  = PACKET_OTHERHOST,  // The 464xlat IPv6 address is not assigned to the kernel.
  };
  if (bind(sock, (struct sockaddr *) &sll, sizeof(sll))) {
    logmsg(ANDROID_LOG_FATAL, "binding packet socket: %s", strerror(errno));
    return 0;
  }

  uint32_t *ipv6 = Global_Clatd_Config.ipv6_local_subnet.s6_addr32;
  struct sock_filter filter_code[] = {
    // Load the first four bytes of the IPv6 destination address (starts 24 bytes in).
    // Compare it against the first four bytes of our IPv6 address, in host byte order (BPF loads
    // are always in host byte order). If it matches, continue with next instruction (JMP 0). If it
    // doesn't match, jump ahead to statement that returns 0 (ignore packet). Repeat for the other
    // three words of the IPv6 address, and if they all match, return PACKETLEN (accept packet).
    BPF_STMT(BPF_LD  | BPF_W   | BPF_ABS,  24),
    BPF_JUMP(BPF_JMP | BPF_JEQ | BPF_K,    htonl(ipv6[0]), 0, 7),
    BPF_STMT(BPF_LD  | BPF_W   | BPF_ABS,  28),
    BPF_JUMP(BPF_JMP | BPF_JEQ | BPF_K,    htonl(ipv6[1]), 0, 5),
    BPF_STMT(BPF_LD  | BPF_W   | BPF_ABS,  32),
    BPF_JUMP(BPF_JMP | BPF_JEQ | BPF_K,    htonl(ipv6[2]), 0, 3),
    BPF_STMT(BPF_LD  | BPF_W   | BPF_ABS,  36),
    BPF_JUMP(BPF_JMP | BPF_JEQ | BPF_K,    htonl(ipv6[3]), 0, 1),
    BPF_STMT(BPF_RET | BPF_K,              PACKETLEN),
    BPF_STMT(BPF_RET | BPF_K, 0)
  };
  struct sock_fprog filter = {
    sizeof(filter_code) / sizeof(filter_code[0]),
    filter_code
  };

  if (setsockopt(sock, SOL_SOCKET, SO_ATTACH_FILTER, &filter, sizeof(filter))) {
    logmsg(ANDROID_LOG_FATAL, "attach packet filter failed: %s", strerror(errno));
    return 0;
  }

  return 1;
}

/* function: configure_tun_ip
 * configures the ipv4 and ipv6 addresses on the tunnel interface
 * tunnel - tun device data
 */
void configure_tun_ip(const struct tun_data *tunnel) {
  int status;

  // Pick an IPv4 address to use by finding a free address in the configured prefix. Technically,
  // there is a race here - if another clatd calls config_select_ipv4_address after we do, but
  // before we call add_address, it can end up having the same IP address as we do. But the time
  // window in which this can happen is extremely small, and even if we end up with a duplicate
  // address, the only damage is that IPv4 TCP connections won't be reset until both interfaces go
  // down.
  in_addr_t localaddr = config_select_ipv4_address(&Global_Clatd_Config.ipv4_local_subnet,
                                                   Global_Clatd_Config.ipv4_local_prefixlen);
  if (localaddr == INADDR_NONE) {
    logmsg(ANDROID_LOG_FATAL,"No free IPv4 address in %s/%d",
           inet_ntoa(Global_Clatd_Config.ipv4_local_subnet),
           Global_Clatd_Config.ipv4_local_prefixlen);
    exit(1);
  }
  Global_Clatd_Config.ipv4_local_subnet.s_addr = localaddr;

  // Configure the interface before bringing it up. As soon as we bring the interface up, the
  // framework will be notified and will assume the interface's configuration has been finalized.
  status = add_address(tunnel->device4, AF_INET, &Global_Clatd_Config.ipv4_local_subnet,
      32, &Global_Clatd_Config.ipv4_local_subnet);
  if(status < 0) {
    logmsg(ANDROID_LOG_FATAL,"configure_tun_ip/if_address(4) failed: %s",strerror(-status));
    exit(1);
  }

  char addrstr[INET_ADDRSTRLEN];
  inet_ntop(AF_INET, &Global_Clatd_Config.ipv4_local_subnet, addrstr, sizeof(addrstr));
  logmsg(ANDROID_LOG_INFO, "Using IPv4 address %s on %s", addrstr, tunnel->device4);

  if((status = if_up(tunnel->device4, Global_Clatd_Config.ipv4mtu)) < 0) {
    logmsg(ANDROID_LOG_FATAL,"configure_tun_ip/if_up(4) failed: %s",strerror(-status));
    exit(1);
  }
}

/* function: drop_root
 * drops root privs but keeps the needed capability
 */
void drop_root() {
  gid_t groups[] = { AID_INET, AID_VPN };
  if(setgroups(sizeof(groups)/sizeof(groups[0]), groups) < 0) {
    logmsg(ANDROID_LOG_FATAL,"drop_root/setgroups failed: %s",strerror(errno));
    exit(1);
  }

  prctl(PR_SET_KEEPCAPS, 1, 0, 0, 0);

  if(setgid(AID_CLAT) < 0) {
    logmsg(ANDROID_LOG_FATAL,"drop_root/setgid failed: %s",strerror(errno));
    exit(1);
  }
  if(setuid(AID_CLAT) < 0) {
    logmsg(ANDROID_LOG_FATAL,"drop_root/setuid failed: %s",strerror(errno));
    exit(1);
  }

  struct __user_cap_header_struct header;
  struct __user_cap_data_struct cap;
  memset(&header, 0, sizeof(header));
  memset(&cap, 0, sizeof(cap));

  header.version = _LINUX_CAPABILITY_VERSION;
  header.pid = 0; // 0 = change myself
  cap.effective = cap.permitted = (1 << CAP_NET_ADMIN);

  if(capset(&header, &cap) < 0) {
    logmsg(ANDROID_LOG_FATAL,"drop_root/capset failed: %s",strerror(errno));
    exit(1);
  }
}

/* function: open_sockets
 * opens a packet socket to receive IPv6 packets and a raw socket to send them
 * tunnel - tun device data
 * mark - the socket mark to use for the sending raw socket
 */
void open_sockets(struct tun_data *tunnel, uint32_t mark) {
  int rawsock = socket(AF_INET6, SOCK_RAW | SOCK_NONBLOCK, IPPROTO_RAW);
  if (rawsock < 0) {
    logmsg(ANDROID_LOG_FATAL, "raw socket failed: %s", strerror(errno));
    exit(1);
  }

  int off = 0;
  if (setsockopt(rawsock, SOL_IPV6, IPV6_CHECKSUM, &off, sizeof(off)) < 0) {
    logmsg(ANDROID_LOG_WARN, "could not disable checksum on raw socket: %s", strerror(errno));
  }
  if (mark != MARK_UNSET && setsockopt(rawsock, SOL_SOCKET, SO_MARK, &mark, sizeof(mark)) < 0) {
    logmsg(ANDROID_LOG_ERROR, "could not set mark on raw socket: %s", strerror(errno));
  }

  tunnel->write_fd6 = rawsock;

  tunnel->read_fd6 = ring_create(tunnel);
  if (tunnel->read_fd6 < 0) {
    exit(1);
  }
}

/* function: update_clat_ipv6_address
 * picks the clat IPv6 address and configures packet translation to use it.
 * tunnel - tun device data
 * interface - uplink interface name
 * returns: 1 on success, 0 on failure
 */
int update_clat_ipv6_address(const struct tun_data *tunnel, const char *interface) {
  union anyip *interface_ip;
  char addrstr[INET6_ADDRSTRLEN];

  // TODO: check that the prefix length is /64.
  interface_ip = getinterface_ip(interface, AF_INET6);
  if (!interface_ip) {
    logmsg(ANDROID_LOG_ERROR, "Unable to find an IPv6 address on interface %s", interface);
    return 0;
  }

  // If our prefix hasn't changed, do nothing. (If this is the first time we configure an IPv6
  // address, Global_Clatd_Config.ipv6_local_subnet will be ::, which won't match our new prefix.)
  if (ipv6_prefix_equal(&interface_ip->ip6, &Global_Clatd_Config.ipv6_local_subnet)) {
    free(interface_ip);
    return 1;
  }

  // Generate an interface ID.
  config_generate_local_ipv6_subnet(&interface_ip->ip6);
  inet_ntop(AF_INET6, &interface_ip->ip6, addrstr, sizeof(addrstr));

  if (IN6_IS_ADDR_UNSPECIFIED(&Global_Clatd_Config.ipv6_local_subnet)) {
    // Startup.
    logmsg(ANDROID_LOG_INFO, "Using IPv6 address %s on %s", addrstr, interface);
  } else {
    // Prefix change.
    char from_addr[INET6_ADDRSTRLEN];
    inet_ntop(AF_INET6, &Global_Clatd_Config.ipv6_local_subnet, from_addr, sizeof(from_addr));
    logmsg(ANDROID_LOG_INFO, "clat IPv6 address changed from %s to %s", from_addr, addrstr);
    del_anycast_address(tunnel->write_fd6, &Global_Clatd_Config.ipv6_local_subnet);
  }

  // Start translating packets to the new prefix.
  Global_Clatd_Config.ipv6_local_subnet = interface_ip->ip6;
  add_anycast_address(tunnel->write_fd6, &Global_Clatd_Config.ipv6_local_subnet, interface);
  free(interface_ip);

  // Update our packet socket filter to reflect the new 464xlat IP address.
  if (!configure_packet_socket(tunnel->read_fd6)) {
      // Things aren't going to work. Bail out and hope we have better luck next time.
      // We don't log an error here because configure_packet_socket has already done so.
      exit(1);
  }

  return 1;
}

/* function: configure_interface
 * reads the configuration and applies it to the interface
 * uplink_interface - network interface to use to reach the ipv6 internet
 * plat_prefix      - PLAT prefix to use
 * tunnel           - tun device data
 * net_id           - NetID to use, NETID_UNSET indicates use of default network
 */
void configure_interface(const char *uplink_interface, const char *plat_prefix, struct tun_data *tunnel, unsigned net_id) {
  int error;

  if(!read_config("/system/etc/clatd.conf", uplink_interface, plat_prefix, net_id)) {
    logmsg(ANDROID_LOG_FATAL,"read_config failed");
    exit(1);
  }

  if(Global_Clatd_Config.mtu > MAXMTU) {
    logmsg(ANDROID_LOG_WARN,"Max MTU is %d, requested %d", MAXMTU, Global_Clatd_Config.mtu);
    Global_Clatd_Config.mtu = MAXMTU;
  }
  if(Global_Clatd_Config.mtu <= 0) {
    Global_Clatd_Config.mtu = getifmtu(Global_Clatd_Config.default_pdp_interface);
    logmsg(ANDROID_LOG_WARN,"ifmtu=%d",Global_Clatd_Config.mtu);
  }
  if(Global_Clatd_Config.mtu < 1280) {
    logmsg(ANDROID_LOG_WARN,"mtu too small = %d", Global_Clatd_Config.mtu);
    Global_Clatd_Config.mtu = 1280;
  }

  if(Global_Clatd_Config.ipv4mtu <= 0 ||
     Global_Clatd_Config.ipv4mtu > Global_Clatd_Config.mtu - MTU_DELTA) {
    Global_Clatd_Config.ipv4mtu = Global_Clatd_Config.mtu - MTU_DELTA;
    logmsg(ANDROID_LOG_WARN,"ipv4mtu now set to = %d",Global_Clatd_Config.ipv4mtu);
  }

  error = tun_alloc(tunnel->device4, tunnel->fd4);
  if(error < 0) {
    logmsg(ANDROID_LOG_FATAL,"tun_alloc/4 failed: %s",strerror(errno));
    exit(1);
  }

  error = set_nonblocking(tunnel->fd4);
  if (error < 0) {
    logmsg(ANDROID_LOG_FATAL, "set_nonblocking failed: %s", strerror(errno));
    exit(1);
  }

  configure_tun_ip(tunnel);
}

/* function: read_packet
 * reads a packet from the tunnel fd and translates it
 * read_fd  - file descriptor to read original packet from
 * write_fd - file descriptor to write translated packet to
 * to_ipv6  - whether the packet is to be translated to ipv6 or ipv4
 */
void read_packet(int read_fd, int write_fd, int to_ipv6) {
  ssize_t readlen;
  uint8_t buf[PACKETLEN], *packet;

  readlen = read(read_fd, buf, PACKETLEN);

  if(readlen < 0) {
    if (errno != EAGAIN) {
      logmsg(ANDROID_LOG_WARN,"read_packet/read error: %s", strerror(errno));
    }
    return;
  } else if(readlen == 0) {
    logmsg(ANDROID_LOG_WARN,"read_packet/tun interface removed");
    running = 0;
    return;
  }

  struct tun_pi *tun_header = (struct tun_pi *) buf;
  if (readlen < (ssize_t) sizeof(*tun_header)) {
    logmsg(ANDROID_LOG_WARN,"read_packet/short read: got %ld bytes", readlen);
    return;
  }

  uint16_t proto = ntohs(tun_header->proto);
  if (proto != ETH_P_IP) {
    logmsg(ANDROID_LOG_WARN, "%s: unknown packet type = 0x%x", __func__, proto);
    return;
  }

  if(tun_header->flags != 0) {
    logmsg(ANDROID_LOG_WARN, "%s: unexpected flags = %d", __func__, tun_header->flags);
  }

  packet = (uint8_t *) (tun_header + 1);
  readlen -= sizeof(*tun_header);
  translate_packet(write_fd, to_ipv6, packet, readlen);
}

/* function: event_loop
 * reads packets from the tun network interface and passes them down the stack
 * tunnel - tun device data
 */
void event_loop(struct tun_data *tunnel) {
  time_t last_interface_poll;
  struct pollfd wait_fd[] = {
    { tunnel->read_fd6, POLLIN, 0 },
    { tunnel->fd4, POLLIN, 0 },
  };

  // start the poll timer
  last_interface_poll = time(NULL);

  while(running) {
    if(poll(wait_fd, 2, NO_TRAFFIC_INTERFACE_POLL_FREQUENCY*1000) == -1) {
      if(errno != EINTR) {
        logmsg(ANDROID_LOG_WARN,"event_loop/poll returned an error: %s",strerror(errno));
      }
    } else {
      // Call read_packet if the socket has data to be read, but also if an
      // error is waiting. If we don't call read() after getting POLLERR, a
      // subsequent poll() will return immediately with POLLERR again,
      // causing this code to spin in a loop. Calling read() will clear the
      // socket error flag instead.
      if (wait_fd[0].revents) {
        ring_read(&tunnel->ring, tunnel->fd4, 0 /* to_ipv6 */);
      }
      if (wait_fd[1].revents) {
        read_packet(tunnel->fd4, tunnel->write_fd6, 1 /* to_ipv6 */);
      }
    }

    time_t now = time(NULL);
    if(last_interface_poll < (now - INTERFACE_POLL_FREQUENCY)) {
      update_clat_ipv6_address(tunnel, Global_Clatd_Config.default_pdp_interface);
      last_interface_poll = now;
    }
  }
}

/* function: print_help
 * in case the user is running this on the command line
 */
void print_help() {
  printf("android-clat arguments:\n");
  printf("-i [uplink interface]\n");
  printf("-p [plat prefix]\n");
  printf("-n [NetId]\n");
  printf("-m [socket mark]\n");
}

/* function: parse_unsigned
 * parses a string as a decimal/hex/octal unsigned integer
 * str - the string to parse
 * out - the unsigned integer to write to, gets clobbered on failure
 */
int parse_unsigned(const char *str, unsigned *out) {
    char *end_ptr;
    *out = strtoul(str, &end_ptr, 0);
    return *str && !*end_ptr;
}

/* function: main
 * allocate and setup the tun device, then run the event loop
 */
int main(int argc, char **argv) {
  struct tun_data tunnel;
  int opt;
  char *uplink_interface = NULL, *plat_prefix = NULL, *net_id_str = NULL, *mark_str = NULL;
  unsigned net_id = NETID_UNSET;
  uint32_t mark = MARK_UNSET;
  unsigned len;

  while((opt = getopt(argc, argv, "i:p:n:m:h")) != -1) {
    switch(opt) {
      case 'i':
        uplink_interface = optarg;
        break;
      case 'p':
        plat_prefix = optarg;
        break;
      case 'n':
        net_id_str = optarg;
        break;
      case 'm':
        mark_str = optarg;
        break;
      case 'h':
        print_help();
        exit(0);
      default:
        logmsg(ANDROID_LOG_FATAL, "Unknown option -%c. Exiting.", (char) optopt);
        exit(1);
    }
  }

  if(uplink_interface == NULL) {
    logmsg(ANDROID_LOG_FATAL, "clatd called without an interface");
    exit(1);
  }

  if (net_id_str != NULL && !parse_unsigned(net_id_str, &net_id)) {
    logmsg(ANDROID_LOG_FATAL, "invalid NetID %s", net_id_str);
    exit(1);
  }

  if (mark_str != NULL && !parse_unsigned(mark_str, &mark)) {
    logmsg(ANDROID_LOG_FATAL, "invalid mark %s", mark_str);
    exit(1);
  }

  len = snprintf(tunnel.device4, sizeof(tunnel.device4), "%s%s", DEVICEPREFIX, uplink_interface);
  if (len >= sizeof(tunnel.device4)) {
    logmsg(ANDROID_LOG_FATAL, "interface name too long '%s'", tunnel.device4);
    exit(1);
  }

  logmsg(ANDROID_LOG_INFO, "Starting clat version %s on %s netid=%s mark=%s",
         CLATD_VERSION, uplink_interface,
         net_id_str ? net_id_str : "(none)",
         mark_str ? mark_str : "(none)");

  // open our raw sockets before dropping privs
  open_sockets(&tunnel, mark);

  // run under a regular user
  drop_root();

  // we can create tun devices as non-root because we're in the VPN group.
  tunnel.fd4 = tun_open();
  if(tunnel.fd4 < 0) {
    logmsg(ANDROID_LOG_FATAL, "tun_open4 failed: %s", strerror(errno));
    exit(1);
  }

  // When run from netd, the environment variable ANDROID_DNS_MODE is set to
  // "local", but that only works for the netd process itself.
  unsetenv("ANDROID_DNS_MODE");

  configure_interface(uplink_interface, plat_prefix, &tunnel, net_id);

  update_clat_ipv6_address(&tunnel, uplink_interface);

  // Loop until someone sends us a signal or brings down the tun interface.
  if(signal(SIGTERM, stop_loop) == SIG_ERR) {
    logmsg(ANDROID_LOG_FATAL, "sigterm handler failed: %s", strerror(errno));
    exit(1);
  }

  event_loop(&tunnel);

  logmsg(ANDROID_LOG_INFO,"Shutting down clat on %s", uplink_interface);
  del_anycast_address(tunnel.write_fd6, &Global_Clatd_Config.ipv6_local_subnet);

  return 0;
}
