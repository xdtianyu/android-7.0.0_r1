/*
 * This is a stripped down version of the iw tool designed for
 * programmatically checking driver/hw capabilities.
 *
 * Copyright 2007, 2008	Johannes Berg <johannes@sipsolutions.net>
 */

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <net/if.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdbool.h>

#include <netlink/netlink.h>
#include <netlink/genl/genl.h>
#include <netlink/genl/family.h>
#include <netlink/genl/ctrl.h>  
#include <netlink/msg.h>
#include <netlink/attr.h>

#include "nl80211.h"

#ifndef CONFIG_LIBNL20
/* libnl 2.0 compatibility code */

#  define nl_sock nl_handle

static inline struct nl_handle *nl_socket_alloc(void)
{
	return nl_handle_alloc();
}

static inline void nl_socket_free(struct nl_sock *h)
{
	nl_handle_destroy(h);
}

static inline int __genl_ctrl_alloc_cache(struct nl_sock *h, struct nl_cache **cache)
{
	struct nl_cache *tmp = genl_ctrl_alloc_cache(h);
	if (!tmp)
		return -ENOMEM;
	*cache = tmp;
	return 0;
}
#define genl_ctrl_alloc_cache __genl_ctrl_alloc_cache
#endif /* CONFIG_LIBNL20 */

struct nl80211_state {
	struct nl_sock *nl_sock;
	struct nl_cache *nl_cache;
	struct genl_family *nl80211;
};

static int nl80211_init(struct nl80211_state *state)
{
	int err;

	state->nl_sock = nl_socket_alloc();
	if (!state->nl_sock) {
		fprintf(stderr, "Failed to allocate netlink socket.\n");
		return -ENOMEM;
	}

	if (genl_connect(state->nl_sock)) {
		fprintf(stderr, "Failed to connect to generic netlink.\n");
		err = -ENOLINK;
		goto out_handle_destroy;
	}

	if (genl_ctrl_alloc_cache(state->nl_sock, &state->nl_cache)) {
		fprintf(stderr, "Failed to allocate generic netlink cache.\n");
		err = -ENOMEM;
		goto out_handle_destroy;
	}

	state->nl80211 = genl_ctrl_search_by_name(state->nl_cache, "nl80211");
	if (!state->nl80211) {
		fprintf(stderr, "nl80211 not found.\n");
		err = -ENOENT;
		goto out_cache_free;
	}

	return 0;

 out_cache_free:
	nl_cache_free(state->nl_cache);
 out_handle_destroy:
	nl_socket_free(state->nl_sock);
	return err;
}

static void nl80211_cleanup(struct nl80211_state *state)
{
	genl_family_put(state->nl80211);
	nl_cache_free(state->nl_cache);
	nl_socket_free(state->nl_sock);
}

static const char *argv0;

static int phy_lookup(char *name)
{
	char buf[200];
	int fd, pos;

	snprintf(buf, sizeof(buf), "/sys/class/ieee80211/%s/index", name);

	fd = open(buf, O_RDONLY);
	if (fd < 0)
		return -1;
	pos = read(fd, buf, sizeof(buf) - 1);
	if (pos < 0)
		return -1;
	buf[pos] = '\0';
	return atoi(buf);
}

enum {
	CHECK_IS_HT20		= 0x00000001,
	CHECK_IS_HT40		= 0x00000002,
	CHECK_IS_PSMP		= 0x00000004,
	CHECK_IS_AMPDU		= 0x00000008,
	CHECK_IS_AMSDU		= 0x00000010,
	CHECK_IS_SMPS		= 0x00000020,
	CHECK_IS_STA		= 0x00000040,
	CHECK_IS_AP		= 0x00000080,
	CHECK_IS_IBSS		= 0x00000100,
	CHECK_IS_MBSS		= 0x00000200,
	CHECK_IS_MONITOR	= 0x00000400,
	CHECK_BANDS		= 0x00000800,
	CHECK_FREQS		= 0x00001000,
	CHECK_RATES		= 0x00002000,
	CHECK_MCS		= 0x00004000,
	CHECK_AMPDU_DENS	= 0x00008000,
	CHECK_AMPDU_FACT	= 0x00010000,
	CHECK_AMSDU_LEN		= 0x00020000,
	CHECK_IS_LPDC		= 0x00040000,
	CHECK_IS_GREENFIELD	= 0x00080000,
	CHECK_IS_SGI20		= 0x00100000,
	CHECK_IS_SGI40		= 0x00200000,
	CHECK_IS_TXSTBC		= 0x00400000,
	CHECK_RXSTBC		= 0x00800000,
	CHECK_IS_DELBA		= 0x01000000,
	/* NB: must be in upper 16-bits to avoid HT caps */
	CHECK_IS_24GHZ		= 0x02000000,
	CHECK_IS_5GHZ		= 0x04000000,
	CHECK_IS_11B		= 0x08000000,
	CHECK_IS_11G		= 0x10000000,
	CHECK_IS_11A		= 0x20000000,
	CHECK_IS_11N		= 0x40000000,
};

struct check {
	const char *name;
	int namelen;
	int bits;
};
static const struct check checks[] = {
	{ "24ghz", 5, CHECK_IS_24GHZ },
	{ "5ghz", 4, CHECK_IS_5GHZ },
	{ "11b", 3, CHECK_IS_11B },
	{ "11g", 3, CHECK_IS_11G },
	{ "11a", 3, CHECK_IS_11A },
	{ "11n", 3, CHECK_IS_11N },
	{ "ht20", 4, CHECK_IS_HT20 },
	{ "ht40", 4, CHECK_IS_HT40 },
	{ "psmp", 5, CHECK_IS_PSMP },
	{ "ampdu", 5, CHECK_IS_AMPDU },
	{ "amsdu", 5, CHECK_IS_AMSDU },
	{ "smps", 4, CHECK_IS_SMPS },
	{ "sta", 3, CHECK_IS_STA },
	{ "ap", 2, CHECK_IS_AP },
	{ "ibss", 4, CHECK_IS_IBSS },
	{ "mbss", 4, CHECK_IS_MBSS },
	{ "mon", 3, CHECK_IS_MONITOR },
	{ "bands", 4, CHECK_BANDS },
	{ "freqs", 4, CHECK_FREQS },
	{ "rates", 4, CHECK_RATES },
	{ "mcs", 3, CHECK_MCS },
	{ "ampdu_dens", 10, CHECK_AMPDU_DENS },
	{ "ampdu_fact", 10, CHECK_AMPDU_FACT },
	{ "amsdu_len", 9, CHECK_AMSDU_LEN },
	{ "lpdc", 4, CHECK_IS_LPDC },
	{ "green", 5, CHECK_IS_GREENFIELD },
	{ "sgi20", 5, CHECK_IS_SGI20 },
	{ "sgi40", 5, CHECK_IS_SGI40 },
	{ "txstbc", 6, CHECK_IS_TXSTBC },
	{ "rxstbc", 6, CHECK_RXSTBC },
	{ "delba", 5, CHECK_IS_DELBA },
	{ "all", 3, -1 },
	{ NULL }
};

static const struct check *find_check_byname(const char *name)
{
	const struct check *p;

	for (p = checks; p->name != NULL; p++)
		if (strncasecmp(p->name, name, p->namelen) == 0)
			return p;
	return NULL;
}

#if 0
static const struct check *find_check_bybits(int bits)
{
	const struct check *p;

	for (p = checks; p->name != NULL; p++)
		if (p->bits == bits)
			return p;
	return NULL;
}
#endif

static int check_iftype(struct nlattr *tb_msg[], int nl_type)
{
	struct nlattr *nl_mode;
	int rem_mode;

	if (!tb_msg[NL80211_ATTR_SUPPORTED_IFTYPES])
		return 0;

	nla_for_each_nested(nl_mode, tb_msg[NL80211_ATTR_SUPPORTED_IFTYPES], rem_mode)
		if (nl_mode->nla_type == nl_type)
			return 1;
	return 0;
}

static unsigned int get_max_mcs(unsigned char *mcs)
{
	unsigned int mcs_bit, max;

	max = 0;
	for (mcs_bit = 0; mcs_bit <= 76; mcs_bit++) {
		unsigned int mcs_octet = mcs_bit/8;
		unsigned int MCS_RATE_BIT = 1 << mcs_bit % 8;
		bool mcs_rate_idx_set;

		mcs_rate_idx_set = !!(mcs[mcs_octet] & MCS_RATE_BIT);

		if (!mcs_rate_idx_set)
			continue;

		if (mcs_bit > max)
			max = mcs_bit;
	}
	return max;
}

static void pbool(const char *tag, int v)
{
	printf("%s: %s\n", tag, v ? "true" : "false");
}

static void pint(const char *tag, int v)
{
	printf("%s: %d\n", tag, v);
}

static void prate(const char *tag, int v)
{
	printf("%s: %2.1f\n", tag, 0.1*v);
}

static int check_phy_handler(struct nl_msg *msg, void *arg)
{
	struct nlattr *tb_msg[NL80211_ATTR_MAX + 1];
	struct genlmsghdr *gnlh = nlmsg_data(nlmsg_hdr(msg));
	uintptr_t checks = (uintptr_t) arg;

	struct nlattr *tb_band[NL80211_BAND_ATTR_MAX + 1];

	struct nlattr *tb_freq[NL80211_FREQUENCY_ATTR_MAX + 1];
	static struct nla_policy freq_policy[NL80211_FREQUENCY_ATTR_MAX + 1] = {
		[NL80211_FREQUENCY_ATTR_FREQ] = { .type = NLA_U32 },
		[NL80211_FREQUENCY_ATTR_DISABLED] = { .type = NLA_FLAG },
		[NL80211_FREQUENCY_ATTR_PASSIVE_SCAN] = { .type = NLA_FLAG },
		[NL80211_FREQUENCY_ATTR_NO_IBSS] = { .type = NLA_FLAG },
		[NL80211_FREQUENCY_ATTR_RADAR] = { .type = NLA_FLAG },
		[NL80211_FREQUENCY_ATTR_MAX_TX_POWER] = { .type = NLA_U32 },
	};

	struct nlattr *tb_rate[NL80211_BITRATE_ATTR_MAX + 1];
	static struct nla_policy rate_policy[NL80211_BITRATE_ATTR_MAX + 1] = {
		[NL80211_BITRATE_ATTR_RATE] = { .type = NLA_U32 },
		[NL80211_BITRATE_ATTR_2GHZ_SHORTPREAMBLE] = { .type = NLA_FLAG },
	};

	struct nlattr *nl_band;
	struct nlattr *nl_freq;
	struct nlattr *nl_rate;
	int rem_band, rem_freq, rem_rate, phy_caps;
	int amsdu_len, ampdu_fact, ampdu_dens, max_mcs, max_rate;

	nla_parse(tb_msg, NL80211_ATTR_MAX, genlmsg_attrdata(gnlh, 0),
		  genlmsg_attrlen(gnlh, 0), NULL);

	if (!tb_msg[NL80211_ATTR_WIPHY_BANDS])
		return NL_SKIP;

	phy_caps = 0;
	amsdu_len = 0;
	ampdu_fact = 0;
	ampdu_dens = 0;
	max_mcs = 0;
	max_rate = 0;
	/* NB: merge each band's findings; this stuff is silly */
	nla_for_each_nested(nl_band, tb_msg[NL80211_ATTR_WIPHY_BANDS], rem_band) {
		nla_parse(tb_band, NL80211_BAND_ATTR_MAX, nla_data(nl_band),
			  nla_len(nl_band), NULL);

		if (tb_band[NL80211_BAND_ATTR_HT_CAPA]) {
			unsigned short caps = nla_get_u16(tb_band[NL80211_BAND_ATTR_HT_CAPA]);
			int len;

			/* XXX not quite right but close enough */
			phy_caps |= CHECK_IS_11N | caps;
			len = 0xeff + ((caps & 0x0800) << 1);
			if (len > amsdu_len)
				amsdu_len = len;
		}
		if (tb_band[NL80211_BAND_ATTR_HT_AMPDU_FACTOR]) {
			unsigned char factor = nla_get_u8(tb_band[NL80211_BAND_ATTR_HT_AMPDU_FACTOR]);
			int fact = (1<<(13+factor))-1;
			if (fact > ampdu_fact)
				ampdu_fact = fact;
		}
		if (tb_band[NL80211_BAND_ATTR_HT_AMPDU_DENSITY]) {
			unsigned char dens = nla_get_u8(tb_band[NL80211_BAND_ATTR_HT_AMPDU_DENSITY]);
			if (dens > ampdu_dens)
				ampdu_dens = dens;
		}
		if (tb_band[NL80211_BAND_ATTR_HT_MCS_SET] &&
		    nla_len(tb_band[NL80211_BAND_ATTR_HT_MCS_SET]) == 16) {
			/* As defined in 7.3.2.57.4 Supported MCS Set field */
			unsigned char *mcs = nla_data(tb_band[NL80211_BAND_ATTR_HT_MCS_SET]);
			int max = get_max_mcs(&mcs[0]);
			if (max > max_mcs)
				max_mcs = max;
		}

		nla_for_each_nested(nl_freq, tb_band[NL80211_BAND_ATTR_FREQS], rem_freq) {
			uint32_t freq;

			nla_parse(tb_freq, NL80211_FREQUENCY_ATTR_MAX,
			    nla_data(nl_freq), nla_len(nl_freq),
			    freq_policy);
			if (!tb_freq[NL80211_FREQUENCY_ATTR_FREQ])
				continue;
#if 0
			/* NB: we care about device caps, not regulatory */
			if (tb_freq[NL80211_FREQUENCY_ATTR_DISABLED])
				continue;
#endif
			freq = nla_get_u32(
			    tb_freq[NL80211_FREQUENCY_ATTR_FREQ]);
			if (checks & CHECK_FREQS)
				pint("freq", freq);

			/* NB: approximate band boundaries, we get no help */
			if (2000 <= freq && freq <= 3000)
				phy_caps |= CHECK_IS_24GHZ;
			else if (4000 <= freq && freq <= 6000)
				phy_caps |= CHECK_IS_5GHZ;
		}

		nla_for_each_nested(nl_rate, tb_band[NL80211_BAND_ATTR_RATES], rem_rate) {
			int rate;

			nla_parse(tb_rate, NL80211_BITRATE_ATTR_MAX, nla_data(nl_rate),
				  nla_len(nl_rate), rate_policy);
			if (!tb_rate[NL80211_BITRATE_ATTR_RATE])
				continue;
			rate = nla_get_u32(tb_rate[NL80211_BITRATE_ATTR_RATE]);
			if (rate > max_rate)
				max_rate = rate;
		}
	}
#if 0
	/* NB: 11n =>'s legacy support */
	if (phy_caps & CHECK_IS_11N) {
		if (phy_caps & CHECK_IS_24GHZ)
			phy_caps |= CHECK_IS_11B | CHECK_IS_11G;
		if (phy_caps & CHECK_IS_5GHZ)
			phy_caps |= CHECK_IS_11A;
	}
#else
	/* XXX no way to figure this out; just force 'em */
	if (phy_caps & CHECK_IS_24GHZ)
		phy_caps |= CHECK_IS_11B | CHECK_IS_11G;
	if (phy_caps & CHECK_IS_5GHZ)
		phy_caps |= CHECK_IS_11A;
#endif

#define	PBOOL(c, b, name) if (checks & (c)) pbool(name, phy_caps & (b))
	PBOOL(CHECK_IS_24GHZ, CHECK_IS_24GHZ, "24ghz");
	PBOOL(CHECK_IS_5GHZ, CHECK_IS_5GHZ, "5ghz");
	PBOOL(CHECK_IS_11B, CHECK_IS_11B, "11b");
	PBOOL(CHECK_IS_11G, CHECK_IS_11G, "11g");
	PBOOL(CHECK_IS_11A, CHECK_IS_11A, "11a");
	PBOOL(CHECK_IS_11N, CHECK_IS_11N, "11n");
	PBOOL(CHECK_IS_LPDC, 0x1, "lpdc");
	PBOOL(CHECK_IS_HT20, CHECK_IS_11N, "ht20");
	PBOOL(CHECK_IS_HT40, 0x2, "ht40");
	if (checks & CHECK_IS_SMPS)
		pbool("smps", ((phy_caps & 0x000c) >> 2) < 2);
	PBOOL(CHECK_IS_GREENFIELD, 0x10, "green");
	PBOOL(CHECK_IS_SGI20, 0x20, "sgi20");
	PBOOL(CHECK_IS_SGI40, 0x40, "sgi40");
	PBOOL(CHECK_IS_TXSTBC, 0x40, "txstbc");
	PBOOL(CHECK_RXSTBC, 0x300, "rxstbc");
	PBOOL(CHECK_IS_DELBA, 0x400, "delba");
#if 0
	PBOOL(CHECK_IS_DSSCCK, 0x1000, "dsscck");
#endif
	PBOOL(CHECK_IS_PSMP, 0x2000, "psmp");
#if 0
	PBOOL(CHECK_IS_INTOL, 0x4000, "intol");
#endif
#if 0
	PBOOL(CHECK_IS_LSIGTXOP, 0x8000, "lsigtxop");
#endif
#undef PBOOL
	if (checks & CHECK_AMSDU_LEN)
		pint("amsdu_len", amsdu_len);
	if (checks & CHECK_AMPDU_FACT)
		pint("ampdu_fact", ampdu_fact);
	if (checks & CHECK_AMPDU_DENS)
		pint("ampdu_dens", ampdu_dens);
	if (checks & CHECK_RATES)
		prate("rate", max_rate);
	if (checks & CHECK_MCS)
		pint("mcs", max_mcs);

	if (checks & CHECK_IS_STA)
		pbool("sta", check_iftype(tb_msg, NL80211_IFTYPE_STATION));
	if (checks & CHECK_IS_IBSS)
		pbool("ibss", check_iftype(tb_msg, NL80211_IFTYPE_ADHOC));
	if (checks & CHECK_IS_AP)
		pbool("ap", check_iftype(tb_msg, NL80211_IFTYPE_AP));
	if (checks & CHECK_IS_MBSS)
		pbool("mbss", check_iftype(tb_msg, NL80211_IFTYPE_MESH_POINT));
	if (checks & CHECK_IS_MONITOR)
		pbool("mon", check_iftype(tb_msg, NL80211_IFTYPE_MONITOR));

	return NL_SKIP;
}

static int check_phy_caps(struct nl80211_state *state,
		       struct nl_cb *cb,
		       struct nl_msg *msg,
		       int argc, char **argv)
{
	int checks = 0;
	for (; argc > 0; argc--, argv++) {
		const struct check *p = find_check_byname(argv[0]);
		if (p == NULL) {
			fprintf(stderr, "invalid check %s\n", argv[0]);
			return 3;		/* XXX whatever? */
		}
		checks |= p->bits;
	}
	nl_cb_set(cb, NL_CB_VALID, NL_CB_CUSTOM, check_phy_handler,
	    (void *)(uintptr_t) checks);
	return 0;
}

static int error_handler(struct sockaddr_nl *nla, struct nlmsgerr *err,
			 void *arg)
{
	int *ret = arg;
	*ret = err->error;
	return NL_STOP;
}

static int finish_handler(struct nl_msg *msg, void *arg)
{
	int *ret = arg;
	*ret = 0;
	return NL_SKIP;
}

static int ack_handler(struct nl_msg *msg, void *arg)
{
	int *ret = arg;
	*ret = 0;
	return NL_STOP;
}

static int __handle_cmd(struct nl80211_state *state, int argc, char **argv)
{
	struct nl_cb *cb;
	struct nl_msg *msg;
	int devidx, err;

	if (argc <= 1)
		return 1;

	devidx = phy_lookup(*argv);
	if (devidx < 0)
		return -errno;
	argc--, argv++;

	msg = nlmsg_alloc();
	if (!msg) {
		fprintf(stderr, "failed to allocate netlink message\n");
		return 2;
	}

	cb = nl_cb_alloc(NL_CB_DEFAULT);
	if (!cb) {
		fprintf(stderr, "failed to allocate netlink callbacks\n");
		err = 2;
		goto out_free_msg;
	}

	genlmsg_put(msg, 0, 0, genl_family_get_id(state->nl80211), 0,
		    0, NL80211_CMD_GET_WIPHY, 0);
	NLA_PUT_U32(msg, NL80211_ATTR_WIPHY, devidx);

	err = check_phy_caps(state, cb, msg, argc, argv);
	if (err)
		goto out;

	err = nl_send_auto_complete(state->nl_sock, msg);
	if (err < 0)
		goto out;

	err = 1;

	nl_cb_err(cb, NL_CB_CUSTOM, error_handler, &err);
	nl_cb_set(cb, NL_CB_FINISH, NL_CB_CUSTOM, finish_handler, &err);
	nl_cb_set(cb, NL_CB_ACK, NL_CB_CUSTOM, ack_handler, &err);

	while (err > 0)
		nl_recvmsgs(state->nl_sock, cb);
 out:
	nl_cb_put(cb);
 out_free_msg:
	nlmsg_free(msg);
	return err;
 nla_put_failure:
	fprintf(stderr, "building message failed\n");
	return 2;
}

int main(int argc, char **argv)
{
	struct nl80211_state nlstate;
	int err;

	argc--;
	argv0 = *argv++;

	err = nl80211_init(&nlstate);
	if (err == 0) {
		if (argc > 1 && strncmp(*argv, "phy", 3) == 0) {
			err = __handle_cmd(&nlstate, argc, argv);
			if (err < 0)
				fprintf(stderr, "command failed: %s (%d)\n",
				    strerror(-err), err);
			else if (err)
				fprintf(stderr, "command failed: err %d\n", err);
		} else {
			fprintf(stderr, "usage: %s phyX [args]\n", argv0);
			err = 1;
		}
		nl80211_cleanup(&nlstate);
	}
	return err;
}
