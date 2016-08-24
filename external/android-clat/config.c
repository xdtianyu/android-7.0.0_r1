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
 * config.c - configuration settings
 */

#include <string.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <limits.h>
#include <errno.h>
#include <unistd.h>

#include <cutils/config_utils.h>
#include <netutils/ifc.h>

#include "config.h"
#include "dns64.h"
#include "logging.h"
#include "getaddr.h"
#include "clatd.h"
#include "checksum.h"

struct clat_config Global_Clatd_Config;

/* function: config_item_str
 * locates the config item and returns the pointer to a string, or NULL on failure.  Caller frees pointer
 * root       - parsed configuration
 * item_name  - name of config item to locate
 * defaultvar - value to use if config item isn't present
 */
char *config_item_str(cnode *root, const char *item_name, const char *defaultvar) {
  const char *tmp;

  if(!(tmp = config_str(root, item_name, defaultvar))) {
    logmsg(ANDROID_LOG_FATAL,"%s config item needed",item_name);
    return NULL;
  }
  return strdup(tmp);
}

/* function: config_item_int16_t
 * locates the config item, parses the integer, and returns the pointer ret_val_ptr, or NULL on failure
 * root        - parsed configuration
 * item_name   - name of config item to locate
 * defaultvar  - value to use if config item isn't present
 * ret_val_ptr - pointer for return value storage
 */
int16_t *config_item_int16_t(cnode *root, const char *item_name, const char *defaultvar, int16_t *ret_val_ptr) {
  const char *tmp;
  char *endptr;
  long int conf_int;

  if(!(tmp = config_str(root, item_name, defaultvar))) {
    logmsg(ANDROID_LOG_FATAL,"%s config item needed",item_name);
    return NULL;
  }

  errno = 0;
  conf_int = strtol(tmp,&endptr,10);
  if(errno > 0) {
    logmsg(ANDROID_LOG_FATAL,"%s config item is not numeric: %s (error=%s)",item_name,tmp,strerror(errno));
    return NULL;
  }
  if(endptr == tmp || *tmp == '\0') {
    logmsg(ANDROID_LOG_FATAL,"%s config item is not numeric: %s",item_name,tmp);
    return NULL;
  }
  if(*endptr != '\0') {
    logmsg(ANDROID_LOG_FATAL,"%s config item contains non-numeric characters: %s",item_name,endptr);
    return NULL;
  }
  if(conf_int > INT16_MAX || conf_int < INT16_MIN) {
    logmsg(ANDROID_LOG_FATAL,"%s config item is too big/small: %d",item_name,conf_int);
    return NULL;
  }
  *ret_val_ptr = conf_int;
  return ret_val_ptr;
}

/* function: config_item_ip
 * locates the config item, parses the ipv4 address, and returns the pointer ret_val_ptr, or NULL on failure
 * root        - parsed configuration
 * item_name   - name of config item to locate
 * defaultvar  - value to use if config item isn't present
 * ret_val_ptr - pointer for return value storage
 */
struct in_addr *config_item_ip(cnode *root, const char *item_name, const char *defaultvar, struct in_addr *ret_val_ptr) {
  const char *tmp;
  int status;

  if(!(tmp = config_str(root, item_name, defaultvar))) {
    logmsg(ANDROID_LOG_FATAL,"%s config item needed",item_name);
    return NULL;
  }

  status = inet_pton(AF_INET, tmp, ret_val_ptr);
  if(status <= 0) {
    logmsg(ANDROID_LOG_FATAL,"invalid IPv4 address specified for %s: %s", item_name, tmp);
    return NULL;
  }

  return ret_val_ptr;
}

/* function: config_item_ip6
 * locates the config item, parses the ipv6 address, and returns the pointer ret_val_ptr, or NULL on failure
 * root        - parsed configuration
 * item_name   - name of config item to locate
 * defaultvar  - value to use if config item isn't present
 * ret_val_ptr - pointer for return value storage
 */
struct in6_addr *config_item_ip6(cnode *root, const char *item_name, const char *defaultvar, struct in6_addr *ret_val_ptr) {
  const char *tmp;
  int status;

  if(!(tmp = config_str(root, item_name, defaultvar))) {
    logmsg(ANDROID_LOG_FATAL,"%s config item needed",item_name);
    return NULL;
  }

  status = inet_pton(AF_INET6, tmp, ret_val_ptr);
  if(status <= 0) {
    logmsg(ANDROID_LOG_FATAL,"invalid IPv6 address specified for %s: %s", item_name, tmp);
    return NULL;
  }

  return ret_val_ptr;
}

/* function: free_config
 * frees the memory used by the global config variable
 */
void free_config() {
  if(Global_Clatd_Config.plat_from_dns64_hostname) {
    free(Global_Clatd_Config.plat_from_dns64_hostname);
    Global_Clatd_Config.plat_from_dns64_hostname = NULL;
  }
}

/* function: ipv6_prefix_equal
 * compares the prefixes two ipv6 addresses. assumes the prefix lengths are both /64.
 * a1 - first address
 * a2 - second address
 * returns: 0 if the subnets are different, 1 if they are the same.
 */
int ipv6_prefix_equal(struct in6_addr *a1, struct in6_addr *a2) {
    return !memcmp(a1, a2, 8);
}

/* function: dns64_detection
 * does dns lookups to set the plat subnet or exits on failure, waits forever for a dns response with a query backoff timer
 * net_id - (optional) netId to use, NETID_UNSET indicates use of default network
 */
void dns64_detection(unsigned net_id) {
  int backoff_sleep, status;
  struct in6_addr tmp_ptr;

  backoff_sleep = 1;

  while(1) {
    status = plat_prefix(Global_Clatd_Config.plat_from_dns64_hostname,net_id,&tmp_ptr);
    if(status > 0) {
      memcpy(&Global_Clatd_Config.plat_subnet, &tmp_ptr, sizeof(struct in6_addr));
      return;
    }
    logmsg(ANDROID_LOG_WARN, "dns64_detection -- error, sleeping for %d seconds", backoff_sleep);
    sleep(backoff_sleep);
    backoff_sleep *= 2;
    if(backoff_sleep >= 120) {
      backoff_sleep = 120;
    }
  }
}

/* function: gen_random_iid
 * picks a random interface ID that is checksum neutral with the IPv4 address and the NAT64 prefix
 * myaddr            - IPv6 address to write to
 * ipv4_local_subnet - clat IPv4 address
 * plat_subnet       - NAT64 prefix
 */
void gen_random_iid(struct in6_addr *myaddr, struct in_addr *ipv4_local_subnet,
                    struct in6_addr *plat_subnet) {
  // Fill last 8 bytes of IPv6 address with random bits.
  arc4random_buf(&myaddr->s6_addr[8], 8);

  // Make the IID checksum-neutral. That is, make it so that:
  //   checksum(Local IPv4 | Remote IPv4) = checksum(Local IPv6 | Remote IPv6)
  // in other words (because remote IPv6 = NAT64 prefix | Remote IPv4):
  //   checksum(Local IPv4) = checksum(Local IPv6 | NAT64 prefix)
  // Do this by adjusting the two bytes in the middle of the IID.

  uint16_t middlebytes = (myaddr->s6_addr[11] << 8) + myaddr->s6_addr[12];

  uint32_t c1 = ip_checksum_add(0, ipv4_local_subnet, sizeof(*ipv4_local_subnet));
  uint32_t c2 = ip_checksum_add(0, plat_subnet, sizeof(*plat_subnet)) +
                ip_checksum_add(0, myaddr, sizeof(*myaddr));

  uint16_t delta = ip_checksum_adjust(middlebytes, c1, c2);
  myaddr->s6_addr[11] = delta >> 8;
  myaddr->s6_addr[12] = delta & 0xff;
}

// Factored out to a separate function for testability.
int connect_is_ipv4_address_free(in_addr_t addr) {
  int s = socket(AF_INET, SOCK_DGRAM, 0);
  if (s == -1) {
    return 0;
  }

  // Attempt to connect to the address. If the connection succeeds and getsockname returns the same
  // the address then the address is already assigned to the system and we can't use it.
  struct sockaddr_in sin = { .sin_family = AF_INET, .sin_addr = { addr }, .sin_port = 53 };
  socklen_t len = sizeof(sin);
  int inuse = connect(s, (struct sockaddr *) &sin, sizeof(sin)) == 0 &&
              getsockname(s, (struct sockaddr *) &sin, &len) == 0 &&
              (size_t) len >= sizeof(sin) &&
              sin.sin_addr.s_addr == addr;

  close(s);
  return !inuse;
}

addr_free_func config_is_ipv4_address_free = connect_is_ipv4_address_free;

/* function: config_select_ipv4_address
 * picks a free IPv4 address, starting from ip and trying all addresses in the prefix in order
 * ip        - the IP address from the configuration file
 * prefixlen - the length of the prefix from which addresses may be selected.
 * returns: the IPv4 address, or INADDR_NONE if no addresses were available
 */
in_addr_t config_select_ipv4_address(const struct in_addr *ip, int16_t prefixlen) {
  in_addr_t chosen = INADDR_NONE;

  // Don't accept prefixes that are too large because we scan addresses one by one.
  if (prefixlen < 16 || prefixlen > 32) {
      return chosen;
  }

  // All these are in host byte order.
  in_addr_t mask = 0xffffffff >> (32 - prefixlen) << (32 - prefixlen);
  in_addr_t ipv4 = ntohl(ip->s_addr);
  in_addr_t first_ipv4 = ipv4;
  in_addr_t prefix = ipv4 & mask;

  // Pick the first IPv4 address in the pool, wrapping around if necessary.
  // So, for example, 192.0.0.4 -> 192.0.0.5 -> 192.0.0.6 -> 192.0.0.7 -> 192.0.0.0.
  do {
     if (config_is_ipv4_address_free(htonl(ipv4))) {
       chosen = htonl(ipv4);
       break;
     }
     ipv4 = prefix | ((ipv4 + 1) & ~mask);
  } while (ipv4 != first_ipv4);

  return chosen;
}

/* function: config_generate_local_ipv6_subnet
 * generates the local ipv6 subnet when given the interface ip
 * requires config.ipv6_host_id
 * interface_ip - in: interface ip, out: local ipv6 host address
 */
void config_generate_local_ipv6_subnet(struct in6_addr *interface_ip) {
  int i;

  if (Global_Clatd_Config.use_dynamic_iid) {
    /* Generate a random interface ID. */
    gen_random_iid(interface_ip,
                   &Global_Clatd_Config.ipv4_local_subnet,
                   &Global_Clatd_Config.plat_subnet);
  } else {
    /* Use the specified interface ID. */
    for(i = 2; i < 4; i++) {
      interface_ip->s6_addr32[i] = Global_Clatd_Config.ipv6_host_id.s6_addr32[i];
    }
  }
}

/* function: read_config
 * reads the config file and parses it into the global variable Global_Clatd_Config. returns 0 on failure, 1 on success
 * file             - filename to parse
 * uplink_interface - interface to use to reach the internet and supplier of address space
 * plat_prefix      - (optional) plat prefix to use, otherwise follow config file
 * net_id           - (optional) netId to use, NETID_UNSET indicates use of default network
 */
int read_config(const char *file, const char *uplink_interface, const char *plat_prefix,
        unsigned net_id) {
  cnode *root = config_node("", "");
  void *tmp_ptr = NULL;
  unsigned flags;

  if(!root) {
    logmsg(ANDROID_LOG_FATAL,"out of memory");
    return 0;
  }

  memset(&Global_Clatd_Config, '\0', sizeof(Global_Clatd_Config));

  config_load_file(root, file);
  if(root->first_child == NULL) {
    logmsg(ANDROID_LOG_FATAL,"Could not read config file %s", file);
    goto failed;
  }

  Global_Clatd_Config.default_pdp_interface = strdup(uplink_interface);
  if (!Global_Clatd_Config.default_pdp_interface)
    goto failed;

  if(!config_item_int16_t(root, "mtu", "-1", &Global_Clatd_Config.mtu))
    goto failed;

  if(!config_item_int16_t(root, "ipv4mtu", "-1", &Global_Clatd_Config.ipv4mtu))
    goto failed;

  if(!config_item_ip(root, "ipv4_local_subnet", DEFAULT_IPV4_LOCAL_SUBNET,
                     &Global_Clatd_Config.ipv4_local_subnet))
    goto failed;

  if(!config_item_int16_t(root, "ipv4_local_prefixlen", DEFAULT_IPV4_LOCAL_PREFIXLEN,
                          &Global_Clatd_Config.ipv4_local_prefixlen))
    goto failed;

  if(plat_prefix) { // plat subnet is coming from the command line
    if(inet_pton(AF_INET6, plat_prefix, &Global_Clatd_Config.plat_subnet) <= 0) {
      logmsg(ANDROID_LOG_FATAL,"invalid IPv6 address specified for plat prefix: %s", plat_prefix);
      goto failed;
    }
  } else {
    tmp_ptr = (void *)config_item_str(root, "plat_from_dns64", "yes");
    if(!tmp_ptr || strcmp(tmp_ptr, "no") == 0) {
      free(tmp_ptr);

      if(!config_item_ip6(root, "plat_subnet", NULL, &Global_Clatd_Config.plat_subnet)) {
        logmsg(ANDROID_LOG_FATAL, "plat_from_dns64 disabled, but no plat_subnet specified");
        goto failed;
      }
    } else {
      free(tmp_ptr);

      if(!(Global_Clatd_Config.plat_from_dns64_hostname = config_item_str(root, "plat_from_dns64_hostname", DEFAULT_DNS64_DETECTION_HOSTNAME)))
        goto failed;
      dns64_detection(net_id);
    }
  }

  if (!config_item_ip6(root, "ipv6_host_id", "::", &Global_Clatd_Config.ipv6_host_id))
    goto failed;

  /* In order to prevent multiple devices attempting to use the same clat address, never use a
     statically-configured interface ID on a broadcast interface such as wifi. */
  if (!IN6_IS_ADDR_UNSPECIFIED(&Global_Clatd_Config.ipv6_host_id)) {
    ifc_init();
    ifc_get_info(Global_Clatd_Config.default_pdp_interface, NULL, NULL, &flags);
    ifc_close();
    Global_Clatd_Config.use_dynamic_iid = (flags & IFF_BROADCAST) != 0;
  } else {
    Global_Clatd_Config.use_dynamic_iid = 1;
  }

  return 1;

failed:
  free(root);
  free_config();
  return 0;
}

/* function; dump_config
 * prints the current config
 */
void dump_config() {
  char charbuffer[INET6_ADDRSTRLEN];

  logmsg(ANDROID_LOG_DEBUG,"mtu = %d",Global_Clatd_Config.mtu);
  logmsg(ANDROID_LOG_DEBUG,"ipv4mtu = %d",Global_Clatd_Config.ipv4mtu);
  logmsg(ANDROID_LOG_DEBUG,"ipv6_local_subnet = %s",inet_ntop(AF_INET6, &Global_Clatd_Config.ipv6_local_subnet, charbuffer, sizeof(charbuffer)));
  logmsg(ANDROID_LOG_DEBUG,"ipv4_local_subnet = %s",inet_ntop(AF_INET, &Global_Clatd_Config.ipv4_local_subnet, charbuffer, sizeof(charbuffer)));
  logmsg(ANDROID_LOG_DEBUG,"ipv4_local_prefixlen = %d", Global_Clatd_Config.ipv4_local_prefixlen);
  logmsg(ANDROID_LOG_DEBUG,"plat_subnet = %s",inet_ntop(AF_INET6, &Global_Clatd_Config.plat_subnet, charbuffer, sizeof(charbuffer)));
  logmsg(ANDROID_LOG_DEBUG,"default_pdp_interface = %s",Global_Clatd_Config.default_pdp_interface);
}
