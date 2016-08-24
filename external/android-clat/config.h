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
 * config.h - configuration settings
 */
#ifndef __CONFIG_H__
#define __CONFIG_H__

#include <netinet/in.h>
#include <linux/if.h>

#define DEFAULT_IPV4_LOCAL_SUBNET "192.0.0.4"
#define DEFAULT_IPV4_LOCAL_PREFIXLEN "29"
#define DEFAULT_DNS64_DETECTION_HOSTNAME "ipv4only.arpa"

struct clat_config {
  int16_t mtu, ipv4mtu;
  struct in6_addr ipv6_local_subnet;
  struct in6_addr ipv6_host_id;
  struct in_addr ipv4_local_subnet;
  int16_t ipv4_local_prefixlen;
  struct in6_addr plat_subnet;
  char *default_pdp_interface;
  char *plat_from_dns64_hostname;
  int use_dynamic_iid;
};

extern struct clat_config Global_Clatd_Config;

int read_config(const char *file, const char *uplink_interface, const char *plat_prefix,
        unsigned net_id);
void config_generate_local_ipv6_subnet(struct in6_addr *interface_ip);
in_addr_t config_select_ipv4_address(const struct in_addr *ip, int16_t prefixlen);
int ipv6_prefix_equal(struct in6_addr *a1, struct in6_addr *a2);

typedef int (*addr_free_func)(in_addr_t addr);

#endif /* __CONFIG_H__ */
