//
// detail/socket_types.hpp
// ~~~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2015 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef ASIO_DETAIL_SOCKET_TYPES_HPP
#define ASIO_DETAIL_SOCKET_TYPES_HPP


#include "asio/detail/config.hpp"

# include <sys/ioctl.h>
#  include <sys/poll.h>
# include <sys/types.h>
# include <sys/stat.h>
# include <fcntl.h>
# if defined(__hpux)
#  include <sys/time.h>
# endif
# if !defined(__hpux) || defined(__SELECT)
#  include <sys/select.h>
# endif
# include <sys/socket.h>
# include <sys/uio.h>
# include <sys/un.h>
# include <netinet/in.h>
#  include <netinet/tcp.h>
# include <arpa/inet.h>
# include <netdb.h>
# include <net/if.h>
# include <limits.h>
# if defined(__sun)
#  include <sys/filio.h>
#  include <sys/sockio.h>
# endif

#include "asio/detail/push_options.hpp"

namespace asio {
namespace detail {

typedef int socket_type;
const int invalid_socket = -1;
const int socket_error_retval = -1;
const int max_addr_v4_str_len = INET_ADDRSTRLEN;
#if defined(INET6_ADDRSTRLEN)
const int max_addr_v6_str_len = INET6_ADDRSTRLEN + 1 + IF_NAMESIZE;
#else // defined(INET6_ADDRSTRLEN)
const int max_addr_v6_str_len = 256;
#endif // defined(INET6_ADDRSTRLEN)
typedef sockaddr socket_addr_type;
typedef in_addr in4_addr_type;
# if defined(__hpux)
// HP-UX doesn't provide ip_mreq when _XOPEN_SOURCE_EXTENDED is defined.
struct in4_mreq_type
{
  struct in_addr imr_multiaddr;
  struct in_addr imr_interface;
};
# else
typedef ip_mreq in4_mreq_type;
# endif
typedef sockaddr_in sockaddr_in4_type;
typedef in6_addr in6_addr_type;
typedef ipv6_mreq in6_mreq_type;
typedef sockaddr_in6 sockaddr_in6_type;
typedef sockaddr_storage sockaddr_storage_type;
typedef sockaddr_un sockaddr_un_type;
typedef addrinfo addrinfo_type;
typedef ::linger linger_type;
typedef int ioctl_arg_type;
typedef uint32_t u_long_type;
typedef uint16_t u_short_type;
#if defined(ASIO_HAS_SSIZE_T)
typedef ssize_t signed_size_type;
#else // defined(ASIO_HAS_SSIZE_T)
typedef int signed_size_type;
#endif // defined(ASIO_HAS_SSIZE_T)
# define ASIO_OS_DEF(c) ASIO_OS_DEF_##c
# define ASIO_OS_DEF_AF_UNSPEC AF_UNSPEC
# define ASIO_OS_DEF_AF_INET AF_INET
# define ASIO_OS_DEF_AF_INET6 AF_INET6
# define ASIO_OS_DEF_SOCK_STREAM SOCK_STREAM
# define ASIO_OS_DEF_SOCK_DGRAM SOCK_DGRAM
# define ASIO_OS_DEF_SOCK_RAW SOCK_RAW
# define ASIO_OS_DEF_SOCK_SEQPACKET SOCK_SEQPACKET
# define ASIO_OS_DEF_IPPROTO_IP IPPROTO_IP
# define ASIO_OS_DEF_IPPROTO_IPV6 IPPROTO_IPV6
# define ASIO_OS_DEF_IPPROTO_TCP IPPROTO_TCP
# define ASIO_OS_DEF_IPPROTO_UDP IPPROTO_UDP
# define ASIO_OS_DEF_IPPROTO_ICMP IPPROTO_ICMP
# define ASIO_OS_DEF_IPPROTO_ICMPV6 IPPROTO_ICMPV6
# define ASIO_OS_DEF_FIONBIO FIONBIO
# define ASIO_OS_DEF_FIONREAD FIONREAD
# define ASIO_OS_DEF_INADDR_ANY INADDR_ANY
# define ASIO_OS_DEF_MSG_OOB MSG_OOB
# define ASIO_OS_DEF_MSG_PEEK MSG_PEEK
# define ASIO_OS_DEF_MSG_DONTROUTE MSG_DONTROUTE
# define ASIO_OS_DEF_MSG_EOR MSG_EOR
# define ASIO_OS_DEF_SHUT_RD SHUT_RD
# define ASIO_OS_DEF_SHUT_WR SHUT_WR
# define ASIO_OS_DEF_SHUT_RDWR SHUT_RDWR
# define ASIO_OS_DEF_SOMAXCONN SOMAXCONN
# define ASIO_OS_DEF_SOL_SOCKET SOL_SOCKET
# define ASIO_OS_DEF_SO_BROADCAST SO_BROADCAST
# define ASIO_OS_DEF_SO_DEBUG SO_DEBUG
# define ASIO_OS_DEF_SO_DONTROUTE SO_DONTROUTE
# define ASIO_OS_DEF_SO_KEEPALIVE SO_KEEPALIVE
# define ASIO_OS_DEF_SO_LINGER SO_LINGER
# define ASIO_OS_DEF_SO_SNDBUF SO_SNDBUF
# define ASIO_OS_DEF_SO_RCVBUF SO_RCVBUF
# define ASIO_OS_DEF_SO_SNDLOWAT SO_SNDLOWAT
# define ASIO_OS_DEF_SO_RCVLOWAT SO_RCVLOWAT
# define ASIO_OS_DEF_SO_REUSEADDR SO_REUSEADDR
# define ASIO_OS_DEF_TCP_NODELAY TCP_NODELAY
# define ASIO_OS_DEF_IP_MULTICAST_IF IP_MULTICAST_IF
# define ASIO_OS_DEF_IP_MULTICAST_TTL IP_MULTICAST_TTL
# define ASIO_OS_DEF_IP_MULTICAST_LOOP IP_MULTICAST_LOOP
# define ASIO_OS_DEF_IP_ADD_MEMBERSHIP IP_ADD_MEMBERSHIP
# define ASIO_OS_DEF_IP_DROP_MEMBERSHIP IP_DROP_MEMBERSHIP
# define ASIO_OS_DEF_IP_TTL IP_TTL
# define ASIO_OS_DEF_IPV6_UNICAST_HOPS IPV6_UNICAST_HOPS
# define ASIO_OS_DEF_IPV6_MULTICAST_IF IPV6_MULTICAST_IF
# define ASIO_OS_DEF_IPV6_MULTICAST_HOPS IPV6_MULTICAST_HOPS
# define ASIO_OS_DEF_IPV6_MULTICAST_LOOP IPV6_MULTICAST_LOOP
# define ASIO_OS_DEF_IPV6_JOIN_GROUP IPV6_JOIN_GROUP
# define ASIO_OS_DEF_IPV6_LEAVE_GROUP IPV6_LEAVE_GROUP
# define ASIO_OS_DEF_AI_CANONNAME AI_CANONNAME
# define ASIO_OS_DEF_AI_PASSIVE AI_PASSIVE
# define ASIO_OS_DEF_AI_NUMERICHOST AI_NUMERICHOST
# if defined(AI_NUMERICSERV)
#  define ASIO_OS_DEF_AI_NUMERICSERV AI_NUMERICSERV
# else
#  define ASIO_OS_DEF_AI_NUMERICSERV 0
# endif
// Note: QNX Neutrino 6.3 defines AI_V4MAPPED, AI_ALL and AI_ADDRCONFIG but
// does not implement them. Therefore they are specifically excluded here.
# if defined(AI_V4MAPPED) && !defined(__QNXNTO__)
#  define ASIO_OS_DEF_AI_V4MAPPED AI_V4MAPPED
# else
#  define ASIO_OS_DEF_AI_V4MAPPED 0
# endif
# if defined(AI_ALL) && !defined(__QNXNTO__)
#  define ASIO_OS_DEF_AI_ALL AI_ALL
# else
#  define ASIO_OS_DEF_AI_ALL 0
# endif
# if defined(AI_ADDRCONFIG) && !defined(__QNXNTO__)
#  define ASIO_OS_DEF_AI_ADDRCONFIG AI_ADDRCONFIG
# else
#  define ASIO_OS_DEF_AI_ADDRCONFIG 0
# endif
# if defined(IOV_MAX)
const int max_iov_len = IOV_MAX;
# else
// POSIX platforms are not required to define IOV_MAX.
const int max_iov_len = 16;
# endif
const int custom_socket_option_level = 0xA5100000;
const int enable_connection_aborted_option = 1;
const int always_fail_option = 2;

} // namespace detail
} // namespace asio

#include "asio/detail/pop_options.hpp"

#endif // ASIO_DETAIL_SOCKET_TYPES_HPP
