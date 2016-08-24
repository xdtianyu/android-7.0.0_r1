/*
 * dhcpcd - DHCP client daemon
 * Copyright (c) 2006-2015 Roy Marples <roy@marples.name>
 * All rights reserved

 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <errno.h>
#include <poll.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <unistd.h>

#include <dbus/dbus.h>

#include "../config.h"
#include "../eloop.h"
#include "../dhcp.h"
#ifdef INET6
#include "../dhcp6.h"
#endif
#include "../rpc-interface.h"
#include "dbus-dict.h"

#define SERVICE_NAME 	"org.chromium.dhcpcd"
#define SERVICE_PATH    "/org/chromium/dhcpcd"
#define S_EINVAL	SERVICE_NAME ".InvalidArgument"
#define S_ARGS		"Not enough arguments"

static DBusConnection *connection;
static struct dhcpcd_ctx *dhcpcd_ctx;

static const char dhcpcd_introspection_xml[] =
    "    <method name=\"GetVersion\">\n"
    "      <arg name=\"version\" direction=\"out\" type=\"s\"/>\n"
    "    </method>\n"
    "    <method name=\"Rebind\">\n"
    "      <arg name=\"interface\" direction=\"in\" type=\"s\"/>\n"
    "    </method>\n"
    "    <method name=\"Release\">\n"
    "      <arg name=\"interface\" direction=\"in\" type=\"s\"/>\n"
    "    </method>\n"
    "    <method name=\"Stop\">\n"
    "      <arg name=\"interface\" direction=\"in\" type=\"s\"/>\n"
    "    </method>\n"
    "    <signal name=\"Event\">\n"
    "      <arg name=\"configuration\" type=\"usa{sv}\"/>\n"
    "    </signal>\n"
    "    <signal name=\"StatusChanged\">\n"
    "      <arg name=\"status\" type=\"us\"/>\n"
    "    </signal>\n";

static const char service_watch_rule[] = "interface=" DBUS_INTERFACE_DBUS
	",type=signal,member=NameOwnerChanged";

static const char introspection_header_xml[] =
    "<!DOCTYPE node PUBLIC \"-//freedesktop//"
    "DTD D-BUS Object Introspection 1.0//EN\"\n"
    "\"http://www.freedesktop.org/standards/dbus/1.0/introspect.dtd\">\n"
    "<node name=\"" SERVICE_PATH "\">\n"
    "  <interface name=\"org.freedesktop.DBus.Introspectable\">\n"
    "    <method name=\"Introspect\">\n"
    "      <arg name=\"data\" direction=\"out\" type=\"s\"/>\n"
    "    </method>\n"
    "  </interface>\n"
    "  <interface name=\"" SERVICE_NAME "\">\n";

static const char introspection_footer_xml[] =
    "  </interface>\n"
    "</node>\n";

static const struct o_dbus dhos[] = {
	{ "ip_address=", DBUS_TYPE_UINT32, 0, "IPAddress" },
	{ "server_name=", DBUS_TYPE_STRING, 0, "ServerName"},
	{ "subnet_mask=", DBUS_TYPE_UINT32, 0, "SubnetMask" },
	{ "subnet_cidr=", DBUS_TYPE_BYTE, 0, "SubnetCIDR" },
	{ "network_number=", DBUS_TYPE_UINT32, 0, "NetworkNumber" },
	{ "classless_static_routes=", DBUS_TYPE_STRING, 0,
	  "ClasslessStaticRoutes" },
	{ "ms_classless_static_routes=", DBUS_TYPE_STRING, 0,
	  "MSClasslessStaticRoutes" },
	{ "static_routes=", DBUS_TYPE_ARRAY, DBUS_TYPE_UINT32,
	  "StaticRoutes"} ,
	{ "routers=", DBUS_TYPE_ARRAY, DBUS_TYPE_UINT32, "Routers" },
	{ "time_offset=", DBUS_TYPE_UINT32, 0, "TimeOffset" },
	{ "time_servers=", DBUS_TYPE_ARRAY, DBUS_TYPE_UINT32, "TimeServers" },
	{ "ien116_name_servers=", DBUS_TYPE_ARRAY, DBUS_TYPE_UINT32,
	  "IEN116NameServers" },
	{ "domain_name_servers=", DBUS_TYPE_ARRAY, DBUS_TYPE_UINT32,
	  "DomainNameServers" },
	{ "log_servers=", DBUS_TYPE_ARRAY, DBUS_TYPE_UINT32, "LogServers" },
	{ "cookie_servers=", DBUS_TYPE_ARRAY, DBUS_TYPE_UINT32,
	  "CookieServers" },
	{ "lpr_servers=", DBUS_TYPE_ARRAY, DBUS_TYPE_UINT32, "LPRServers" },
	{ "impress_servers=", DBUS_TYPE_ARRAY, DBUS_TYPE_UINT32,
	  "ImpressServers" },
	{ "resource_location_servers=", DBUS_TYPE_ARRAY, DBUS_TYPE_UINT32,
	  "ResourceLocationServers" },
	{ "host_name=", DBUS_TYPE_STRING, 0, "Hostname" },
	{ "boot_size=", DBUS_TYPE_UINT16, 0, "BootSize" },
	{ "merit_dump=", DBUS_TYPE_STRING, 0, "MeritDump" },
	{ "domain_name=", DBUS_TYPE_STRING, 0, "DomainName" },
	{ "swap_server=", DBUS_TYPE_UINT32, 0, "SwapServer" },
	{ "root_path=", DBUS_TYPE_STRING, 0, "RootPath" },
	{ "extensions_path=", DBUS_TYPE_STRING, 0, "ExtensionsPath" },
	{ "ip_forwarding=", DBUS_TYPE_BOOLEAN, 0, "IPForwarding" },
	{ "non_local_source_routing=", DBUS_TYPE_BOOLEAN, 0,
	  "NonLocalSourceRouting" },
	{ "policy_filter=", DBUS_TYPE_ARRAY, DBUS_TYPE_UINT32,
	  "PolicyFilter" },
	{ "max_dgram_reassembly=", DBUS_TYPE_INT16, 0,
	  "MaxDatagramReassembly" },
	{ "default_ip_ttl=", DBUS_TYPE_UINT16, 0, "DefaultIPTTL" },
	{ "path_mtu_aging_timeout=", DBUS_TYPE_UINT32, 0,
	  "PathMTUAgingTimeout" },
	{ "path_mtu_plateau_table=" ,DBUS_TYPE_ARRAY, DBUS_TYPE_UINT16,
	  "PolicyFilter"} ,
	{ "interface_mtu=", DBUS_TYPE_UINT16, 0, "InterfaceMTU" },
	{ "all_subnets_local=", DBUS_TYPE_BOOLEAN, 0, "AllSubnetsLocal" },
	{ "broadcast_address=", DBUS_TYPE_UINT32, 0, "BroadcastAddress" },
	{ "perform_mask_discovery=", DBUS_TYPE_BOOLEAN, 0,
	  "PerformMaskDiscovery" },
	{ "mask_supplier=", DBUS_TYPE_BOOLEAN, 0, "MaskSupplier" },
	{ "router_discovery=", DBUS_TYPE_BOOLEAN, 0, "RouterDiscovery" },
	{ "router_solicitiation_address=", DBUS_TYPE_UINT32, 0,
	  "RouterSolicationAddress" },
	{ "trailer_encapsulation=", DBUS_TYPE_BOOLEAN, 0,
	  "TrailerEncapsulation" },
	{ "arp_cache_timeout=", DBUS_TYPE_UINT32, 0, "ARPCacheTimeout" },
	{ "ieee802_3_encapsulation=", DBUS_TYPE_UINT16, 0,
	  "IEEE8023Encapsulation" },
	{ "default_tcp_ttl=", DBUS_TYPE_BYTE, 0, "DefaultTCPTTL" },
	{ "tcp_keepalive_interval=", DBUS_TYPE_UINT32, 0,
	  "TCPKeepAliveInterval" },
	{ "tcp_keepalive_garbage=", DBUS_TYPE_BOOLEAN, 0,
	  "TCPKeepAliveGarbage" },
	{ "nis_domain=", DBUS_TYPE_STRING, 0, "NISDomain" },
	{ "nis_servers=", DBUS_TYPE_ARRAY, DBUS_TYPE_UINT32, "NISServers" },
	{ "ntp_servers=", DBUS_TYPE_ARRAY, DBUS_TYPE_UINT32, "NTPServers" },
	{ "vendor_encapsulated_options=", DBUS_TYPE_ARRAY, DBUS_TYPE_BYTE,
	  "VendorEncapsulatedOptions" },
	{ "netbios_name_servers=" ,DBUS_TYPE_ARRAY, DBUS_TYPE_UINT32,
	  "NetBIOSNameServers" },
	{ "netbios_dd_server=", DBUS_TYPE_UINT32, 0, "NetBIOSDDServer" },
	{ "netbios_node_type=", DBUS_TYPE_BYTE, 0, "NetBIOSNodeType" },
	{ "netbios_scope=", DBUS_TYPE_STRING, 0, "NetBIOSScope" },
	{ "font_servers=", DBUS_TYPE_ARRAY, DBUS_TYPE_UINT32, "FontServers" },
	{ "x_display_manager=", DBUS_TYPE_ARRAY, DBUS_TYPE_UINT32,
	  "XDisplayManager" },
	{ "dhcp_requested_address=", DBUS_TYPE_UINT32, 0,
	  "DHCPRequestedAddress" },
	{ "dhcp_lease_time=", DBUS_TYPE_UINT32, 0, "DHCPLeaseTime" },
	{ "dhcp_option_overload=", DBUS_TYPE_BOOLEAN, 0,
	  "DHCPOptionOverload" },
	{ "dhcp_message_type=", DBUS_TYPE_BYTE, 0, "DHCPMessageType" },
	{ "dhcp_server_identifier=", DBUS_TYPE_UINT32, 0,
	  "DHCPServerIdentifier" },
	{ "dhcp_message=", DBUS_TYPE_STRING, 0, "DHCPMessage" },
	{ "dhcp_max_message_size=", DBUS_TYPE_UINT16, 0,
	  "DHCPMaxMessageSize" },
	{ "dhcp_renewal_time=", DBUS_TYPE_UINT32, 0, "DHCPRenewalTime" },
	{ "dhcp_rebinding_time=", DBUS_TYPE_UINT32, 0, "DHCPRebindingTime" },
	{ "nisplus_domain=", DBUS_TYPE_STRING, 0, "NISPlusDomain" },
	{ "nisplus_servers=", DBUS_TYPE_ARRAY, DBUS_TYPE_UINT32,
	  "NISPlusServers" },
	{ "tftp_server_name=", DBUS_TYPE_STRING, 0, "TFTPServerName" },
	{ "bootfile_name=", DBUS_TYPE_STRING, 0, "BootFileName" },
	{ "mobile_ip_home_agent=", DBUS_TYPE_ARRAY, DBUS_TYPE_UINT32,
	  "MobileIPHomeAgent" },
	{ "smtp_server=", DBUS_TYPE_ARRAY, DBUS_TYPE_UINT32, "SMTPServer" },
	{ "pop_server=", DBUS_TYPE_ARRAY, DBUS_TYPE_UINT32, "POPServer" },
	{ "nntp_server=", DBUS_TYPE_ARRAY, DBUS_TYPE_UINT32, "NNTPServer" },
	{ "www_server=", DBUS_TYPE_ARRAY, DBUS_TYPE_UINT32, "WWWServer" },
	{ "finger_server=", DBUS_TYPE_ARRAY, DBUS_TYPE_UINT32,
	  "FingerServer" },
	{ "irc_server=", DBUS_TYPE_ARRAY, DBUS_TYPE_UINT32, "IRCServer" },
	{ "streettalk_server=", DBUS_TYPE_ARRAY, DBUS_TYPE_UINT32,
	  "StreetTalkServer" },
	{ "streettalk_directory_assistance_server=", DBUS_TYPE_ARRAY,
	  DBUS_TYPE_UINT32, "StreetTalkDirectoryAssistanceServer" },
	{ "user_class=", DBUS_TYPE_STRING, 0, "UserClass" },
	{ "new_fqdn_name=", DBUS_TYPE_STRING, 0, "FQDNName" },
	{ "nds_servers=", DBUS_TYPE_ARRAY, DBUS_TYPE_UINT32, "NDSServers" },
	{ "nds_tree_name=", DBUS_TYPE_STRING, 0, "NDSTreeName" },
	{ "nds_context=", DBUS_TYPE_STRING, 0, "NDSContext" },
	{ "bcms_controller_names=", DBUS_TYPE_STRING, 0,
	  "BCMSControllerNames" },
	{ "client_last_transaction_time=", DBUS_TYPE_UINT32, 0,
	  "ClientLastTransactionTime" },
	{ "associated_ip=", DBUS_TYPE_UINT32, 0, "AssociatedIP" },
	{ "uap_servers=", DBUS_TYPE_ARRAY, DBUS_TYPE_UINT32, "UAPServers" },
	{ "netinfo_server_address=", DBUS_TYPE_ARRAY, DBUS_TYPE_UINT32,
	  "NetinfoServerAddress" },
	{ "netinfo_server_tag=", DBUS_TYPE_STRING, 0, "NetinfoServerTag" },
	{ "default_url=", DBUS_TYPE_STRING, 0, "DefaultURL" },
	{ "subnet_selection=", DBUS_TYPE_UINT32, 0, "SubnetSelection" },
	{ "domain_search=", DBUS_TYPE_ARRAY, DBUS_TYPE_STRING,
	  "DomainSearch" },
	{ "wpad_url=", DBUS_TYPE_STRING, 0, "WebProxyAutoDiscoveryUrl" },
#ifdef INET6
	{ "dhcp6_server_id=", DBUS_TYPE_STRING, 0,
	  "DHCPv6ServerIdentifier" },
	{ "dhcp6_ia_na1_ia_addr1=", DBUS_TYPE_STRING, 0, "DHCPv6Address" },
	{ "dhcp6_ia_na1_ia_addr1_vltime=", DBUS_TYPE_UINT32, 0,
	  "DHCPv6AddressLeaseTime" },
	{ "dhcp6_name_servers=", DBUS_TYPE_ARRAY, DBUS_TYPE_STRING,
	  "DHCPv6NameServers" },
	{ "dhcp6_domain_search=", DBUS_TYPE_ARRAY, DBUS_TYPE_STRING,
	  "DHCPv6DomainSearch" },
	{ "dhcp6_ia_pd1_prefix1=", DBUS_TYPE_STRING, 0,
	  "DHCPv6DelegatedPrefix" },
	{ "dhcp6_ia_pd1_prefix1_length=", DBUS_TYPE_UINT32, 0,
	  "DHCPv6DelegatedPrefixLength" },
	{ "dhcp6_ia_pd1_prefix1_vltime=", DBUS_TYPE_UINT32, 0,
	  "DHCPv6DelegatedPrefixLeaseTime" },
#endif
	{ NULL, 0, 0, NULL }
};

static int
append_config(DBusMessageIter *iter,
    const char *prefix, char **env, ssize_t elen)
{
	char **eenv, *p;
	const struct o_dbus *dhop;
	size_t l, lp;
	int retval;

	retval = 0;
	lp = strlen(prefix);
	for (eenv = env + elen; env < eenv; env++) {
		p = env[0];
		for (dhop = dhos; dhop->var; dhop++) {
			l = strlen(dhop->var);
			if (strncmp(p, dhop->var, l) == 0) {
				retval = dict_append_config_item(iter,
				    dhop, p + l);
				break;
			}
			if (strncmp(p, prefix, lp) == 0 &&
			    strncmp(p + lp, dhop->var, l) == 0)
			{
				retval = dict_append_config_item(iter,
				    dhop, p + l + lp);
				break;
			}
		}
		if (retval == -1)
			break;
	}
	return retval;
}

static DBusHandlerResult
get_dbus_error(DBusConnection *con, DBusMessage *msg,
		  const char *name, const char *fmt, ...)
{
	char buffer[1024];
	DBusMessage *reply;
	va_list args;

	va_start(args, fmt);
	vsnprintf(buffer, sizeof(buffer), fmt, args);
	va_end(args);
	reply = dbus_message_new_error(msg, name, buffer);
	dbus_connection_send(con, reply, NULL);
	dbus_message_unref(reply);
	return DBUS_HANDLER_RESULT_HANDLED;
}

static dbus_bool_t
dbus_send_message(const struct interface *ifp, const char *reason,
    const char *prefix, struct dhcp_message *message)
{
	const struct if_options *ifo = ifp->options;
	DBusMessage* msg;
	DBusMessageIter args, dict;
	int pid = getpid();
	char **env = NULL;
	ssize_t e, elen;
	int retval;
	int success = FALSE;

	syslog(LOG_INFO, "event %s on interface %s", reason, ifp->name);

	msg = dbus_message_new_signal(SERVICE_PATH, SERVICE_NAME, "Event");
	if (msg == NULL) {
		syslog(LOG_ERR, "failed to make a configure message");
		return FALSE;
	}
	dbus_message_iter_init_append(msg, &args);
	dbus_message_iter_append_basic(&args, DBUS_TYPE_UINT32, &pid);
	dbus_message_iter_append_basic(&args, DBUS_TYPE_STRING, &reason);
	dbus_message_iter_open_container(&args, DBUS_TYPE_ARRAY,
	    DBUS_DICT_ENTRY_BEGIN_CHAR_AS_STRING
	    DBUS_TYPE_STRING_AS_STRING
	    DBUS_TYPE_VARIANT_AS_STRING
	    DBUS_DICT_ENTRY_END_CHAR_AS_STRING,
	    &dict);
	if (prefix == NULL || message == NULL)
		retval = 0;
	else {
		e = dhcp_env(NULL, NULL, message, ifp);
		if (e > 0) {
			char *config_prefix = strdup(prefix);
			if (config_prefix == NULL) {
				logger(dhcpcd_ctx, LOG_ERR,
				       "Memory exhausted (strdup)");
				eloop_exit(dhcpcd_ctx->eloop, EXIT_FAILURE);
			}
			char *p = config_prefix + strlen(config_prefix) - 1;
			if (p >= config_prefix && *p == '_')
				*p = '\0';
			env = calloc(e + 1, sizeof(char *));
			if (env == NULL) {
				logger(dhcpcd_ctx, LOG_ERR,
				       "Memory exhausted (calloc)");
				eloop_exit(dhcpcd_ctx->eloop, EXIT_FAILURE);
			}
			elen = dhcp_env(env, config_prefix, message, ifp);
			free(config_prefix);
		}
		retval = append_config(&dict, prefix, env, elen);
	}

	/* Release memory allocated for env. */
	if (env) {
		char **current = env;
		while (*current)
			free(*current++);
		free(env);
	}

	dbus_message_iter_close_container(&args, &dict);
	if (retval == 0) {
		success = dbus_connection_send(connection, msg, NULL);
		if (!success)
			syslog(LOG_ERR, "failed to send dhcp to dbus");
	} else
		syslog(LOG_ERR, "failed to construct dbus message");
	dbus_message_unref(msg);

	return success;
}

#ifdef INET6
static dbus_bool_t
dbus_send_dhcpv6_message(const struct interface *ifp, const char *reason,
    const char *prefix, struct dhcp6_message *message, size_t length)
{
	const struct if_options *ifo = ifp->options;
	DBusMessage* msg;
	DBusMessageIter args, dict;
	int pid = getpid();
	char **env = NULL;
	ssize_t e, elen;
	int retval;
	int success = FALSE;

	syslog(LOG_INFO, "event %s on interface %s", reason, ifp->name);

	msg = dbus_message_new_signal(SERVICE_PATH, SERVICE_NAME, "Event");
	if (msg == NULL) {
		syslog(LOG_ERR, "failed to make a configure message");
		return FALSE;
	}
	dbus_message_iter_init_append(msg, &args);
	dbus_message_iter_append_basic(&args, DBUS_TYPE_UINT32, &pid);
	dbus_message_iter_append_basic(&args, DBUS_TYPE_STRING, &reason);
	dbus_message_iter_open_container(&args, DBUS_TYPE_ARRAY,
	    DBUS_DICT_ENTRY_BEGIN_CHAR_AS_STRING
	    DBUS_TYPE_STRING_AS_STRING
	    DBUS_TYPE_VARIANT_AS_STRING
	    DBUS_DICT_ENTRY_END_CHAR_AS_STRING,
	    &dict);
	if (prefix == NULL || message == NULL)
		retval = 0;
	else {
		e = dhcp6_env(NULL, NULL, ifp, message, length);
		if (e > 0) {
			char *config_prefix = strdup(prefix);
			if (config_prefix == NULL) {
				logger(dhcpcd_ctx, LOG_ERR,
				       "Memory exhausted (strdup)");
				eloop_exit(dhcpcd_ctx->eloop, EXIT_FAILURE);
			}
			char *p = config_prefix + strlen(config_prefix) - 1;
			if (p >= config_prefix && *p == '_')
				*p = '\0';
			env = calloc(e + 1, sizeof(char *));
			if (env == NULL) {
				logger(dhcpcd_ctx, LOG_ERR,
				       "Memory exhausted (calloc)");
				eloop_exit(dhcpcd_ctx->eloop, EXIT_FAILURE);
			}
			elen = dhcp6_env(env, "new", ifp, message, length);
			free(config_prefix);
		}
		retval = append_config(&dict, prefix, env, elen);
	}

	/* Release memory allocated for env. */
	if (env) {
		char **current = env;
		while (*current)
			free(*current++);
		free(env);
	}

	dbus_message_iter_close_container(&args, &dict);
	if (retval == 0) {
		success = dbus_connection_send(connection, msg, NULL);
		if (!success)
			syslog(LOG_ERR, "failed to send dhcpv6 to dbus");
	} else
		syslog(LOG_ERR, "failed to construct dbus message");
	dbus_message_unref(msg);

	return success;
}
#endif

static DBusHandlerResult
introspect(DBusConnection *con, DBusMessage *msg)
{
	DBusMessage *reply;
	char *xml;
	size_t len;

	len = sizeof(introspection_header_xml) - 1
	    + sizeof(dhcpcd_introspection_xml) - 1
	    + sizeof(introspection_footer_xml) - 1
	    + 1; /* terminal \0 */
	xml = malloc(len);
	if (xml == NULL)
		return DBUS_HANDLER_RESULT_HANDLED;
	snprintf(xml, len, "%s%s%s",
	    introspection_header_xml,
	    dhcpcd_introspection_xml,
	    introspection_footer_xml);
	reply = dbus_message_new_method_return(msg);
	dbus_message_append_args(reply,
	    DBUS_TYPE_STRING, &xml,
	    DBUS_TYPE_INVALID);
	dbus_connection_send(con, reply, NULL);
	dbus_message_unref(reply);
	free(xml);
	return DBUS_HANDLER_RESULT_HANDLED;
}

static DBusHandlerResult
version(DBusConnection *con, DBusMessage *msg, const char *ver)
{
	DBusMessage *reply;

	reply = dbus_message_new_method_return(msg);
	dbus_message_append_args(reply,
	    DBUS_TYPE_STRING, &ver,
	    DBUS_TYPE_INVALID);
	dbus_connection_send(con, reply, NULL);
	dbus_message_unref(reply);
	return DBUS_HANDLER_RESULT_HANDLED;
}

static DBusHandlerResult
dbus_ack(DBusConnection *con, DBusMessage *msg)
{
	DBusMessage *reply;

	reply = dbus_message_new_method_return(msg);
	dbus_connection_send(con, reply, NULL);
	dbus_message_unref(reply);
	return DBUS_HANDLER_RESULT_HANDLED;
}

static DBusHandlerResult
msg_handler(DBusConnection *con, DBusMessage *msg, __unused void *data)
{
#define	IsMethod(msg, method) \
	dbus_message_is_method_call(msg, SERVICE_NAME, method)

	if (dbus_message_is_method_call(msg, DBUS_INTERFACE_INTROSPECTABLE,
					"Introspect")) {
		return introspect(con, msg);
	} else if (IsMethod(msg, "GetVersion")) {
		return version(con, msg, VERSION);
	} else if (IsMethod(msg, "Rebind")) {
		const char *iface_name;
		if (!dbus_message_get_args(msg, NULL,
				   DBUS_TYPE_STRING, &iface_name,
				   DBUS_TYPE_INVALID)) {
			logger(dhcpcd_ctx, LOG_ERR,
			       "Invalid arguments for Rebind");
			return get_dbus_error(con, msg, S_EINVAL, S_ARGS);
		}
		dhcpcd_start_interface(dhcpcd_ctx, iface_name);
		return dbus_ack(con, msg);
	} else if (IsMethod(msg, "Release")) {
		const char *iface_name;
		if (!dbus_message_get_args(msg, NULL,
				   DBUS_TYPE_STRING, &iface_name,
				   DBUS_TYPE_INVALID)) {
			logger(dhcpcd_ctx, LOG_ERR,
			       "Invalid arguments for Release");
			return get_dbus_error(con, msg, S_EINVAL, S_ARGS);
		}
		dhcpcd_release_ipv4(dhcpcd_ctx, iface_name);
		return dbus_ack(con, msg);
	} else if (IsMethod(msg, "Stop")) {
		const char *iface_name;
		if (!dbus_message_get_args(msg, NULL,
				   DBUS_TYPE_STRING, &iface_name,
				   DBUS_TYPE_INVALID)) {
			logger(dhcpcd_ctx, LOG_ERR,
			       "Invalid arguments for Stop");
			return get_dbus_error(con, msg, S_EINVAL, S_ARGS);
		}
		dhcpcd_stop_interface(dhcpcd_ctx, iface_name);
		(void) dbus_ack(con, msg);
		exit(EXIT_FAILURE);
	} else if (dbus_message_is_signal(msg, DBUS_INTERFACE_LOCAL,
					  "Disconnected")) {
		dhcpcd_stop_interfaces(dhcpcd_ctx);
		exit(EXIT_FAILURE);
	}
	return get_dbus_error(con, msg, S_EINVAL, S_ARGS);
#undef IsMethod
}

static void
dbus_handle_event(DBusWatch *watch, int flags)
{
	dbus_watch_handle((DBusWatch *)watch, flags);

	if (connection != NULL) {
		dbus_connection_ref(connection);
		while (dbus_connection_dispatch(connection) ==
				DBUS_DISPATCH_DATA_REMAINS)
				;
		dbus_connection_unref(connection);
	}
}

static void
dbus_read_event(void *watch)
{
	dbus_handle_event((DBusWatch *)watch, DBUS_WATCH_READABLE);
}

static void
dbus_write_event(void *watch)
{
	dbus_handle_event((DBusWatch *)watch, DBUS_WATCH_WRITABLE);
}

static dbus_bool_t
add_watch(DBusWatch *watch, __unused void *data)
{
	int fd, flags;
	void (*read_event)(void *) = NULL;
	void *read_arg = NULL;
	void (*write_event)(void *) = NULL;
	void *write_arg = NULL;

	fd = dbus_watch_get_unix_fd(watch);
	flags = dbus_watch_get_flags(watch);
	if (flags & DBUS_WATCH_READABLE) {
		read_event = dbus_read_event;
		read_arg = watch;
	}
	if (flags & DBUS_WATCH_WRITABLE) {
		write_event = dbus_write_event;
		write_arg = watch;
	}

	if (eloop_event_add(dhcpcd_ctx->eloop, fd, read_event, read_arg,
			    write_event, write_arg) == 0)
		return TRUE;
	return FALSE;
}

static void
remove_watch(DBusWatch *watch, __unused void *data)
{
	int fd, flags;
	int write_only = 0;
	fd = dbus_watch_get_unix_fd(watch);
	flags = dbus_watch_get_flags(watch);
	if (!(flags & DBUS_WATCH_READABLE) && (flags & DBUS_WATCH_WRITABLE))
		write_only = 1;
	eloop_event_delete(dhcpcd_ctx->eloop, fd, write_only);
}

static DBusHandlerResult
dhcpcd_dbus_filter(DBusConnection *conn, DBusMessage *msg, void *user_data)
{
	const char *service = NULL;
	const char *old_owner = NULL;
	const char *new_owner = NULL;

	if (!dbus_message_is_signal(msg, DBUS_INTERFACE_DBUS,
				    "NameOwnerChanged"))
		return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;

	if (!dbus_message_get_args(msg, NULL,
				   DBUS_TYPE_STRING, &service,
				   DBUS_TYPE_STRING, &old_owner,
				   DBUS_TYPE_STRING, &new_owner,
				   DBUS_TYPE_INVALID)) {
		syslog(LOG_ERR,
		       "Invalid arguments for NameOwnerChanged signal");
		return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
	}
	if (strcmp(service, "org.chromium.flimflam") == 0 &&
	    strlen(new_owner) == 0) {
		syslog(LOG_INFO, "exiting because flimflamd has died");
		dhcpcd_stop_interfaces(dhcpcd_ctx);
		exit(EXIT_FAILURE);
	}
	return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
}

int
rpc_init(struct dhcpcd_ctx *ctx)
{
	DBusObjectPathVTable vt = {
		NULL, &msg_handler, NULL, NULL, NULL, NULL
	};
	DBusError err;
	int ret;

	dhcpcd_ctx = ctx;

	dbus_error_init(&err);
	connection = dbus_bus_get(DBUS_BUS_SYSTEM, &err);
	if (connection == NULL) {
		if (dbus_error_is_set(&err))
			syslog(LOG_ERR, "%s", err.message);
		else
			syslog(LOG_ERR, "failed to get a dbus connection");
		return -1;
	}
	atexit(rpc_close);

	if (!dbus_connection_set_watch_functions(connection,
		add_watch, remove_watch, NULL, NULL, NULL))
	{
		syslog(LOG_ERR, "dbus: failed to set watch functions");
		return -1;
	}
	if (!dbus_connection_register_object_path(connection,
		SERVICE_PATH, &vt, NULL))
	{
		syslog(LOG_ERR, "dbus: failed to register object path");
		return -1;
	}
	dbus_connection_add_filter(connection, dhcpcd_dbus_filter, NULL, NULL);
	dbus_bus_add_match(connection, service_watch_rule, &err);
	if (dbus_error_is_set(&err)) {
		syslog(LOG_ERR, "Cannot add rule: %s", err.message);
		return -1;
	}
	return 0;
}

void
rpc_close(void)
{
	if (connection) {
		dbus_bus_remove_match(connection, service_watch_rule, NULL);
		dbus_connection_remove_filter(connection,
					      dhcpcd_dbus_filter,
					      NULL);
		dbus_connection_unref(connection);
		connection = NULL;
	}
}

void
rpc_signal_status(const char *status)
{
	DBusMessage *msg;
	DBusMessageIter args;
	int pid = getpid();

	syslog(LOG_INFO, "status changed to %s", status);

	msg = dbus_message_new_signal(SERVICE_PATH, SERVICE_NAME,
	    "StatusChanged");
	if (msg == NULL) {
		syslog(LOG_ERR, "failed to make a status changed message");
		return;
	}
	dbus_message_iter_init_append(msg, &args);
	dbus_message_iter_append_basic(&args, DBUS_TYPE_UINT32, &pid);
	dbus_message_iter_append_basic(&args, DBUS_TYPE_STRING, &status);
	if (!dbus_connection_send(connection, msg, NULL))
		syslog(LOG_ERR, "failed to send status to dbus");
	dbus_message_unref(msg);
}


int
rpc_update_ipv4(struct interface *ifp)
{
	struct dhcp_state *state = D_STATE(ifp);
	if (state->new != NULL) {
		/* push state over d-bus */
		dbus_send_message(ifp, state->reason, "new_", state->new);
		rpc_signal_status("Bound");
	} else {
		rpc_signal_status("Release");
	}
	return 0;
}

#ifdef INET6
int
rpc_update_ipv6(struct interface *ifp)
{
	struct dhcp6_state *state = D6_STATE(ifp);
	if (state->new != NULL) {
		/* push state over d-bus */
		dbus_send_dhcpv6_message(ifp, state->reason, "new_",
					 state->new, state->new_len);
		rpc_signal_status("Bound6");
	} else {
		rpc_signal_status("Release6");
	}
	return 0;
}
#endif

int
rpc_notify_unicast_arp(struct interface *ifp) {
	struct dhcp_state *state = D_STATE(ifp);
	return dbus_send_message(ifp, "GATEWAY-ARP", "saved_", state->offer);
}
