/* linux */
#define SYSCONFDIR	"/system/etc/dhcpcd-6.8.2"
#define SBINDIR	"/system/etc/dhcpcd-6.8.2"
#define LIBEXECDIR	"/system/etc/dhcpcd-6.8.2"
#define DBDIR	"/data/misc/dhcp-6.8.2"
#define RUNDIR	"/data/misc/dhcp-6.8.2"
#define HAVE_EPOLL
#ifndef NBBY
#define NBBY  8
#endif
#include "compat/closefrom.h"
#include "compat/endian.h"
#include "compat/posix_spawn.h"
#include "compat/queue.h"
#include "compat/strtoi.h"

#include <signal.h>
#include <linux/rtnetlink.h>
