/*
 * Copyright 2012 Daniel Drown <dan-android@drown.org>
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
 * setif.c - network interface configuration
 */
#include <errno.h>
#include <netinet/in.h>
#include <net/if.h>

#include <linux/rtnetlink.h>
#include <netlink/handlers.h>
#include <netlink/msg.h>

#include "logging.h"
#include "netlink_msg.h"

#define DEBUG_OPTNAME(a) case (a): { optname = #a; break; }

/* function: add_address
 * adds an IP address to/from an interface, returns 0 on success and <0 on failure
 * ifname    - name of interface to change
 * family    - address family (AF_INET, AF_INET6)
 * address   - pointer to a struct in_addr or in6_addr
 * prefixlen - bitlength of network (example: 24 for AF_INET's 255.255.255.0)
 * broadcast - broadcast address (only for AF_INET, ignored for AF_INET6)
 */
int add_address(const char *ifname, int family, const void *address, int prefixlen, const void *broadcast) {
  int retval;
  size_t addr_size;
  struct ifaddrmsg ifa;
  struct nl_msg *msg = NULL;

  addr_size = inet_family_size(family);
  if(addr_size == 0) {
    retval = -EAFNOSUPPORT;
    goto cleanup;
  }

  memset(&ifa, 0, sizeof(ifa));
  if (!(ifa.ifa_index = if_nametoindex(ifname))) {
    retval = -ENODEV;
    goto cleanup;
  }
  ifa.ifa_family = family;
  ifa.ifa_prefixlen = prefixlen;
  ifa.ifa_scope = RT_SCOPE_UNIVERSE;

  msg = nlmsg_alloc_ifaddr(RTM_NEWADDR, NLM_F_ACK | NLM_F_REQUEST | NLM_F_CREATE | NLM_F_REPLACE, &ifa);
  if(!msg) {
    retval = -ENOMEM;
    goto cleanup;
  }

  if(nla_put(msg, IFA_LOCAL, addr_size, address) < 0) {
    retval = -ENOMEM;
    goto cleanup;
  }
  if(family == AF_INET6) {
    // AF_INET6 gets IFA_LOCAL + IFA_ADDRESS
    if(nla_put(msg, IFA_ADDRESS, addr_size, address) < 0) {
      retval = -ENOMEM;
      goto cleanup;
    }
  } else if(family == AF_INET) {
    // AF_INET gets IFA_LOCAL + IFA_BROADCAST
    if(nla_put(msg, IFA_BROADCAST, addr_size, broadcast) < 0) {
      retval = -ENOMEM;
      goto cleanup;
    }
  } else {
    retval = -EAFNOSUPPORT;
    goto cleanup;
  }

  retval = netlink_sendrecv(msg);

cleanup:
  if(msg)
    nlmsg_free(msg);

  return retval;
}

/* function: if_up
 * sets interface link state to up and sets mtu, returns 0 on success and <0 on failure
 * ifname - interface name to change
 * mtu    - new mtu
 */
int if_up(const char *ifname, int mtu) {
  int retval = -1;
  struct ifinfomsg ifi;
  struct nl_msg *msg = NULL;

  memset(&ifi, 0, sizeof(ifi));
  if (!(ifi.ifi_index = if_nametoindex(ifname))) {
    retval = -ENODEV;
    goto cleanup;
  }
  ifi.ifi_change = IFF_UP;
  ifi.ifi_flags = IFF_UP;

  msg = nlmsg_alloc_ifinfo(RTM_SETLINK, NLM_F_ACK | NLM_F_REQUEST | NLM_F_ROOT, &ifi);
  if(!msg) {
    retval = -ENOMEM;
    goto cleanup;
  }

  if(nla_put(msg, IFLA_MTU, 4, &mtu) < 0) {
    retval = -ENOMEM;
    goto cleanup;
  }

  retval = netlink_sendrecv(msg);

cleanup:
  if(msg)
    nlmsg_free(msg);

  return retval;
}

static int do_anycast_setsockopt(int sock, int what, struct in6_addr *addr, int ifindex) {
  struct ipv6_mreq mreq = { *addr, ifindex };
  char *optname;
  int ret;

  switch (what) {
    DEBUG_OPTNAME(IPV6_JOIN_ANYCAST)
    DEBUG_OPTNAME(IPV6_LEAVE_ANYCAST)
    default:
      optname = "???";
      break;
  }

  ret = setsockopt(sock, SOL_IPV6, what, &mreq, sizeof(mreq));
  if (ret) {
    logmsg(ANDROID_LOG_ERROR, "%s: setsockopt(%s): %s", __func__, optname, strerror(errno));
  }

  return ret;
}

/* function: add_anycast_address
 * adds an anycast IPv6 address to an interface, returns 0 on success and <0 on failure
 * sock      - the socket to add the address to
 * addr      - the IP address to add
 * ifname    - name of interface to add the address to
 */
int add_anycast_address(int sock, struct in6_addr *addr, const char *ifname) {
  int ifindex;

  ifindex = if_nametoindex(ifname);
  if (!ifindex) {
    logmsg(ANDROID_LOG_ERROR, "%s: unknown ifindex for interface %s", __func__, ifname);
    return -ENODEV;
  }

  return do_anycast_setsockopt(sock, IPV6_JOIN_ANYCAST, addr, ifindex);
}

/* function: del_anycast_address
 * removes an anycast IPv6 address from the system, returns 0 on success and <0 on failure
 * sock      - the socket to remove from, must have had the address added via add_anycast_address
 * addr      - the IP address to remove
 */
int del_anycast_address(int sock, struct in6_addr *addr) {
  return do_anycast_setsockopt(sock, IPV6_LEAVE_ANYCAST, addr, 0);
}
