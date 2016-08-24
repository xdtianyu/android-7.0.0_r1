/*
 * Original implementation on libmnl:
 * (C) 2011 by Pablo Neira Ayuso <pablo@netfilter.org>
 * (C) 2011 by Intra2net AG <http://www.intra2net.com>
 *
 * Port to libnl:
 * (C) 2013 by Mathieu J. Poirier <mathieu.poirier@linaro.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published
 * by the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <dirent.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>
#include <errno.h>

#include <netlink-local.h>
#include <linux/netlink.h>
#include <linux/netfilter/nfnetlink.h>
#include <linux/netfilter/nfnetlink_acct.h>
#include <netlink/netfilter/nfnl.h>
#include <netlink/netlink.h>
#include <netlink/socket.h>
#include <netlink/msg.h>

#define VERSION "1.0.1"

enum {
	NFACCT_CMD_NONE = 0,
	NFACCT_CMD_LIST,
	NFACCT_CMD_ADD,
	NFACCT_CMD_DELETE,
	NFACCT_CMD_GET,
	NFACCT_CMD_FLUSH,
	NFACCT_CMD_VERSION,
	NFACCT_CMD_HELP,
	NFACCT_CMD_RESTORE,
};

static int nfacct_cmd_list(int argc, char *argv[]);
static int nfacct_cmd_add(int argc, char *argv[]);
static int nfacct_cmd_delete(int argc, char *argv[]);
static int nfacct_cmd_get(int argc, char *argv[]);
static int nfacct_cmd_flush(int argc, char *argv[]);
static int nfacct_cmd_version(int argc, char *argv[]);
static int nfacct_cmd_help(int argc, char *argv[]);
static int nfacct_cmd_restore(int argc, char *argv[]);

#ifndef HAVE_LIBNL20
#define nl_sock nl_handle
#define nl_socket_alloc nl_handle_alloc
#define nl_socket_free nl_handle_destroy
#endif

static void usage(char *argv[])
{
	fprintf(stderr, "Usage: %s command [parameters]...\n", argv[0]);
}

static void nfacct_perror(const char *msg)
{
	if (errno == 0) {
		fprintf(stderr, "nfacct v%s: %s\n", VERSION, msg);
	} else {
		fprintf(stderr, "nfacct v%s: %s: %s\n",
			VERSION, msg, strerror(errno));
	}
}

int main(int argc, char *argv[])
{
	int cmd = NFACCT_CMD_NONE, ret = 0;

	if (argc < 2) {
		usage(argv);
		exit(EXIT_FAILURE);
	}

	if (strncmp(argv[1], "list", strlen(argv[1])) == 0)
		cmd = NFACCT_CMD_LIST;
	else if (strncmp(argv[1], "add", strlen(argv[1])) == 0)
		cmd = NFACCT_CMD_ADD;
	else if (strncmp(argv[1], "delete", strlen(argv[1])) == 0)
		cmd = NFACCT_CMD_DELETE;
	else if (strncmp(argv[1], "get", strlen(argv[1])) == 0)
		cmd = NFACCT_CMD_GET;
	else if (strncmp(argv[1], "flush", strlen(argv[1])) == 0)
		cmd = NFACCT_CMD_FLUSH;
	else if (strncmp(argv[1], "version", strlen(argv[1])) == 0)
		cmd = NFACCT_CMD_VERSION;
	else if (strncmp(argv[1], "help", strlen(argv[1])) == 0)
		cmd = NFACCT_CMD_HELP;
	else if (strncmp(argv[1], "restore", strlen(argv[1])) == 0)
		cmd = NFACCT_CMD_RESTORE;
	else {
		fprintf(stderr, "nfacct v%s: Unknown command: %s\n",
			VERSION, argv[1]);
		usage(argv);
		exit(EXIT_FAILURE);
	}

	switch(cmd) {
	case NFACCT_CMD_LIST:
		ret = nfacct_cmd_list(argc, argv);
		break;
	case NFACCT_CMD_ADD:
		ret = nfacct_cmd_add(argc, argv);
		break;
	case NFACCT_CMD_DELETE:
		ret = nfacct_cmd_delete(argc, argv);
		break;
	case NFACCT_CMD_GET:
		ret = nfacct_cmd_get(argc, argv);
		break;
	case NFACCT_CMD_FLUSH:
		ret = nfacct_cmd_flush(argc, argv);
		break;
	case NFACCT_CMD_VERSION:
		ret = nfacct_cmd_version(argc, argv);
		break;
	case NFACCT_CMD_HELP:
		ret = nfacct_cmd_help(argc, argv);
		break;
	case NFACCT_CMD_RESTORE:
		ret = nfacct_cmd_restore(argc, argv);
		break;
	}
	return ret < 0 ? EXIT_FAILURE : EXIT_SUCCESS;
}


static int message_received(struct nl_msg *msg, void *arg)
{
	struct nlmsghdr *hdr = msg->nm_nlh;

	if (hdr->nlmsg_type == NLMSG_ERROR) {
		struct nlmsgerr *err = nlmsg_data(hdr);

		if (err->error == 0)
			return NL_STOP;
	}

	return NL_OK;
}

static int valid_input(struct nl_msg *msg, void *arg)
{
	struct nlattr *nla = nlmsg_attrdata(nlmsg_hdr(msg),
					 sizeof(struct nfgenmsg));
	struct nlattr *tb[NFACCT_NAME_MAX+1] = {};
	char buf[4096];
	int ret;

	ret = nlmsg_parse(nlmsg_hdr(msg),
			 sizeof(struct nfgenmsg), tb, NFACCT_MAX, NULL);

	if (ret < 0) {
		nfacct_perror("Can't parse message\n");
		return ret;
	}

	ret = snprintf(buf, sizeof(buf),
		"{ pkts = %.20llu, bytes = %.20llu } = %s;",
		(unsigned long long)be64toh(nla_get_u64(tb[NFACCT_PKTS])),
		(unsigned long long)be64toh(nla_get_u64(tb[NFACCT_BYTES])),
		nla_get_string(tb[NFACCT_NAME]));

	printf("%s\n", buf);

	return 0;
}

static int nfacct_cmd_list(int argc, char *argv[])
{
	struct nl_msg *msg;
	struct nl_sock *handle;
	int zeroctr = 0;
	int ret, i;

	for (i=2; i<argc; i++) {
		if (strncmp(argv[i], "reset", strlen(argv[i])) == 0) {
			zeroctr = 1;
		} else if (strncmp(argv[i], "xml", strlen(argv[i])) == 0) {
			nfacct_perror("xml feature not implemented");
			return -1;
		} else {
			nfacct_perror("unknown argument");
			return -1;
		}
	}

	msg = nlmsg_alloc();
	if (!msg)
		return -1;

	ret = nfnlmsg_put(msg,
			NL_AUTO_PID,
			NL_AUTO_SEQ,
			NFNL_SUBSYS_ACCT,
			zeroctr ?
			NFNL_MSG_ACCT_GET_CTRZERO : NFNL_MSG_ACCT_GET,
			NLM_F_DUMP | NLM_F_REQUEST,
			AF_UNSPEC,
			0);

	if (ret) {
		NL_DBG(2, "Can't append payload to message: %s line: %d\n",
							__FUNCTION__, __LINE__);
		goto fail;
	}

	handle = nl_socket_alloc();
	if ((ret = nfnl_connect(handle))) {
		NL_DBG(2, "Can't connect handle: %s line: %d\n",
							__FUNCTION__, __LINE__);
		goto fail;
	}

	if ((ret = nl_send_auto_complete(handle, msg)) < 0) {
		NL_DBG(2, "Can't send msg: %s line: %d\n",
							__FUNCTION__, __LINE__);
		goto fail_send;
        }

	nl_socket_modify_cb(handle, NL_CB_VALID, NL_CB_CUSTOM, valid_input, NULL);
	ret = nl_recvmsgs_default(handle);
	if (ret < 0) {
		NL_DBG(2, "Can't receice msg: %s line: %d\n",
							__FUNCTION__, __LINE__);
	}

fail_send:
	nl_close(handle);
	nl_socket_free(handle);
fail:
	nlmsg_free(msg);
	return ret;
}

static int _nfacct_cmd_add(char *name, int pkts, int bytes)
{
	struct nl_msg *msg;
	struct nl_sock *handle;
	char nfname[NFACCT_NAME_MAX];
	int ret;

	strncpy(nfname, name, NFACCT_NAME_MAX);
	nfname[NFACCT_NAME_MAX-1] = '\0';

	msg = nlmsg_alloc();
	if (!msg)
		return -1;

	ret = nfnlmsg_put(msg,
			NL_AUTO_PID,
			NL_AUTO_SEQ,
			NFNL_SUBSYS_ACCT,
			NFNL_MSG_ACCT_NEW,
			NLM_F_CREATE | NLM_F_ACK | NLM_F_REQUEST,
			AF_UNSPEC,
			0);

	if (ret) {
		NL_DBG(2, "Can't append payload to message: %s line: %d\n",
							__FUNCTION__, __LINE__);
		goto fail;
	}

	nla_put_string(msg, NFACCT_NAME, nfname);
	nla_put_u64(msg, NFACCT_PKTS, htobe64(pkts));
	nla_put_u64(msg, NFACCT_BYTES, htobe64(bytes));

	handle = nl_socket_alloc();
	if ((ret = nfnl_connect(handle))) {
		NL_DBG(2, "Can't connect handle: %s line: %d\n",
							__FUNCTION__, __LINE__);
		goto fail;
	}

	if ((ret = nl_send_auto_complete(handle, msg)) < 0) {
		NL_DBG(2, "Can't send msg: %s line: %d\n",
							__FUNCTION__, __LINE__);
		goto fail_send;
        }

	ret = nl_recvmsgs_default(handle);
	if (ret < 0) {
		NL_DBG(2, "Can't receice msg: %s line: %d\n",
							__FUNCTION__, __LINE__);
	}

fail_send:
	nl_close(handle);
	nl_socket_free(handle);
fail:
	nlmsg_free(msg);
	return ret;
}



static int nfacct_cmd_add(int argc, char *argv[])
{
	if (argc < 3) {
		nfacct_perror("missing object name");
		return -1;
	} else if (argc > 3) {
		nfacct_perror("too many arguments");
		return -1;
	}

	return _nfacct_cmd_add(argv[2], 0, 0);
}

static int nfacct_cmd_delete(int argc, char *argv[])
{
	struct nl_msg *msg;
	struct nl_sock *handle;
	char nfname[NFACCT_NAME_MAX];
	int ret;

	if (argc < 3) {
		nfacct_perror("missing object name");
		return -1;
	} else if (argc > 3) {
		nfacct_perror("too many arguments");
		return -1;
	}

	strncpy(nfname, argv[2], NFACCT_NAME_MAX);
	nfname[NFACCT_NAME_MAX-1] = '\0';

	msg = nlmsg_alloc();
	if (!msg)
		return -1;

	ret = nfnlmsg_put(msg,
			NL_AUTO_PID,
			NL_AUTO_SEQ,
			NFNL_SUBSYS_ACCT,
			NFNL_MSG_ACCT_DEL,
			NLM_F_ACK | NLM_F_REQUEST,
			AF_UNSPEC,
			0);

	if (ret) {
		NL_DBG(2, "Can't append payload to message: %s line: %d\n",
							__FUNCTION__, __LINE__);
		goto fail;
	}

	nla_put_string(msg, NFACCT_NAME, nfname);

	handle = nl_socket_alloc();
	if ((ret = nfnl_connect(handle))) {
		NL_DBG(2, "Can't connect handle: %s line: %d\n",
							__FUNCTION__, __LINE__);
		goto fail;
	}

	if ((ret = nl_send_auto_complete(handle, msg)) < 0) {
		NL_DBG(2, "Can't send msg: %s line: %d\n",
							__FUNCTION__, __LINE__);
		goto fail_send;
        }

	ret = nl_recvmsgs_default(handle);
	if (ret < 0) {
		NL_DBG(2, "Can't receice msg: %s line: %d\n",
							__FUNCTION__, __LINE__);
	}

fail_send:
	nl_close(handle);
	nl_socket_free(handle);
fail:
	nlmsg_free(msg);
	return ret;
	return 0;
}


static int nfacct_cmd_get(int argc, char *argv[])
{
	struct nl_msg *msg;
	struct nl_sock *handle;
	struct nl_cb *cb;
	char nfname[NFACCT_NAME_MAX];
	int zeroctr = 0;
	int ret, i;

	if (argc < 3) {
		nfacct_perror("missing object name");
		 return -1;
	}

	for (i=3; i<argc; i++) {
		if (strncmp(argv[i], "reset", strlen(argv[i])) == 0) {
			zeroctr = 1;
		} else if (strncmp(argv[i], "xml", strlen(argv[i])) == 0) {
			nfacct_perror("xml feature not implemented");
			return -1;
		} else {
			nfacct_perror("unknown argument");
			return -1;
		}
	}

	strncpy(nfname, argv[2], NFACCT_NAME_MAX);
	nfname[NFACCT_NAME_MAX-1] = '\0';

	msg = nlmsg_alloc();
	if (!msg)
		return -1;

	ret = nfnlmsg_put(msg,
			NL_AUTO_PID,
			NL_AUTO_SEQ,
			NFNL_SUBSYS_ACCT,
			zeroctr ?
			NFNL_MSG_ACCT_GET_CTRZERO : NFNL_MSG_ACCT_GET,
			NLM_F_ACK | NLM_F_REQUEST,
			AF_UNSPEC,
			0);

	if (ret) {
		NL_DBG(2, "Can't append payload to message: %s line: %d\n",
							__FUNCTION__, __LINE__);
		goto fail;
	}

	nla_put_string(msg, NFACCT_NAME, nfname);

	handle = nl_socket_alloc();

	if (handle) {
		cb = nl_cb_alloc(NL_CB_DEFAULT);
		if (!cb)
			goto fail;

		if (nl_cb_set(cb, NL_CB_MSG_IN,
				 NL_CB_CUSTOM,
				 message_received, NULL) < 0)
			goto fail;

		nl_socket_set_cb(handle,cb);
	} else {
		goto fail;
	}

	if ((ret = nfnl_connect(handle))) {
		NL_DBG(2, "Can't connect handle: %s line: %d\n",
							__FUNCTION__, __LINE__);
		goto fail;
	}

	if ((ret = nl_send_auto_complete(handle, msg)) < 0) {
		NL_DBG(2, "Can't send msg: %s line: %d\n",
							__FUNCTION__, __LINE__);
		goto fail_send;
        }

	nl_socket_modify_cb(handle, NL_CB_VALID, NL_CB_CUSTOM, valid_input, NULL);
	ret = nl_recvmsgs_default(handle);
	if (ret < 0) {
		NL_DBG(2, "Can't receice msg: %s line: %d\n",
							__FUNCTION__, __LINE__);
	}

fail_send:
	nl_close(handle);
	nl_socket_free(handle);
fail:
	nlmsg_free(msg);
	return ret;
}

static int nfacct_cmd_flush(int argc, char *argv[])
{
	struct nl_msg *msg;
	struct nl_sock *handle;
	int ret;

	if (argc > 2) {
		nfacct_perror("too many arguments");
		return -1;
	}

	msg = nlmsg_alloc();
	if (!msg)
		return -1;

	ret = nfnlmsg_put(msg,
			NL_AUTO_PID,
			NL_AUTO_SEQ,
			NFNL_SUBSYS_ACCT,
			NFNL_MSG_ACCT_DEL,
			NLM_F_ACK | NLM_F_REQUEST,
			AF_UNSPEC,
			0);

	if (ret) {
		NL_DBG(2, "Can't append payload to message: %s line: %d\n",
							__FUNCTION__, __LINE__);
		goto fail;
	}

	handle = nl_socket_alloc();
	if ((ret = nfnl_connect(handle))) {
		NL_DBG(2, "Can't connect handle: %s line: %d\n",
							__FUNCTION__, __LINE__);
		goto fail;
	}

	if ((ret = nl_send_auto_complete(handle, msg)) < 0) {
		NL_DBG(2, "Can't send msg: %s line: %d\n",
							__FUNCTION__, __LINE__);
		goto fail_send;
        }

	ret = nl_recvmsgs_default(handle);
	if (ret < 0) {
		NL_DBG(2, "Can't receice msg: %s line: %d\n",
							__FUNCTION__, __LINE__);
	}

fail_send:
	nl_close(handle);
	nl_socket_free(handle);
fail:
	nlmsg_free(msg);
	return ret;
}

static const char version_msg[] =
	"nfacct v%s: utility for the Netfilter extended accounting "
	"infrastructure\n"
	"Copyright (C) 2011 Pablo Neira Ayuso <pablo@netfilter.org>\n"
	"Copyright (C) 2011 Intra2net AG <http://www.intra2net.com>\n"
	"Copyright (C) 2013 Mathieu Poirier <mathieu.poirier@linaro.org>\n"
	"This program comes with ABSOLUTELY NO WARRANTY.\n"
	"This is free software, and you are welcome to redistribute it under "
	"certain \nconditions; see LICENSE file distributed in this package "
	"for details.\n";

static int nfacct_cmd_version(int argc, char *argv[])
{
	printf(version_msg, VERSION);
	return 0;
}

static const char help_msg[] =
	"nfacct v%s: utility for the Netfilter extended accounting "
	"infrastructure\n"
	"Usage: %s command [parameters]...\n\n"
	"Commands:\n"
	"  list [reset]\t\tList the accounting object table (and reset)\n"
	"  add object-name\tAdd new accounting object to table\n"
	"  delete object-name\tDelete existing accounting object\n"
	"  get object-name\tGet existing accounting object\n"
	"  flush\t\t\tFlush accounting object table\n"
	"  restore\t\tRestore accounting object table reading 'list' output from stdin\n"
	"  version\t\tDisplay version and disclaimer\n"
	"  help\t\t\tDisplay this help message\n";

static int nfacct_cmd_help(int argc, char *argv[])
{
	printf(help_msg, VERSION, argv[0]);
	return 0;
}

static int nfacct_cmd_restore(int argc, char *argv[])
{
	uint64_t pkts, bytes;
	char name[512];
	char buffer[512];
	int ret;
	while (fgets(buffer, sizeof(buffer), stdin)) {
		char *semicolon = strchr(buffer, ';');
		if (semicolon == NULL) {
			nfacct_perror("invalid line");
			return -1;
		}
		*semicolon = 0;
		ret = sscanf(buffer, "{ pkts = %llu, bytes = %llu } = %s",
		       &pkts, &bytes, name);
		if (ret != 3) {
			nfacct_perror("error reading input");
			return -1;
		}
		if ((ret = _nfacct_cmd_add(name, pkts, bytes)) != 0)
			return ret;

	}
	return 0;
}
