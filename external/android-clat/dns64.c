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
 * dns64.c - find the nat64 prefix with a dns64 lookup
 */

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <strings.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "dns64.h"
#include "logging.h"
#include "resolv_netid.h"

/* function: plat_prefix
 * looks up an ipv4-only hostname and looks for a nat64 /96 prefix, returns 1 on success, 0 on failure
 * ipv4_name  - name to lookup
 * net_id     - (optional) netId to use, NETID_UNSET indicates use of default network
 * prefix     - the plat /96 prefix
 */
int plat_prefix(const char *ipv4_name, unsigned net_id, struct in6_addr *prefix) {
  const struct addrinfo hints = {
    .ai_family = AF_INET6,
  };
  int status;
  struct addrinfo *result = NULL;
  struct in6_addr plat_addr;
  char plat_addr_str[INET6_ADDRSTRLEN];

  logmsg(ANDROID_LOG_INFO, "Detecting NAT64 prefix from DNS...");

  status = android_getaddrinfofornet(ipv4_name, NULL, &hints, net_id, MARK_UNSET, &result);
  if (status != 0 || result == NULL) {
    logmsg(ANDROID_LOG_ERROR, "plat_prefix/dns(%s) status = %d/%s",
           ipv4_name, status, gai_strerror(status));
    return 0;
  }

  // Use only the first result.  If other records are present, possibly with
  // differing DNS64 prefixes they are ignored (there is very little sensible
  // that could be done with them at this time anyway).

  if (result->ai_family != AF_INET6) {
    logmsg(ANDROID_LOG_WARN, "plat_prefix/unexpected address family: %d", result->ai_family);
    return 0;
  }
  plat_addr = ((struct sockaddr_in6 *)result->ai_addr)->sin6_addr;
  // Only /96 DNS64 prefixes are supported at this time.
  plat_addr.s6_addr32[3] = 0;
  freeaddrinfo(result);

  logmsg(ANDROID_LOG_INFO, "Detected NAT64 prefix %s/96",
         inet_ntop(AF_INET6, &plat_addr, plat_addr_str, sizeof(plat_addr_str)));
  *prefix = plat_addr;
  return 1;
}
