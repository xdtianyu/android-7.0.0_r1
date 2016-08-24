//
// detail/impl/socket_ops.ipp
// ~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2015 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef ASIO_DETAIL_SOCKET_OPS_IPP
#define ASIO_DETAIL_SOCKET_OPS_IPP


#include "asio/detail/config.hpp"

#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cerrno>
#include <new>
#include "asio/detail/assert.hpp"
#include "asio/detail/socket_ops.hpp"
#include "asio/error.hpp"


#if defined(ASIO_WINDOWS) || defined(__CYGWIN__)    || defined(__MACH__) && defined(__APPLE__)
# if defined(ASIO_HAS_PTHREADS)
#  include <pthread.h>
# endif // defined(ASIO_HAS_PTHREADS)
#endif // defined(ASIO_WINDOWS) || defined(__CYGWIN__)
       // || defined(__MACH__) && defined(__APPLE__)

#include "asio/detail/push_options.hpp"

namespace asio {
namespace detail {
namespace socket_ops {



#if defined(__hpux)
// HP-UX doesn't declare these functions extern "C", so they are declared again
// here to avoid linker errors about undefined symbols.
extern "C" char* if_indextoname(unsigned int, char*);
extern "C" unsigned int if_nametoindex(const char*);
#endif // defined(__hpux)


inline void clear_last_error()
{
  errno = 0;
}


template <typename ReturnType>
inline ReturnType error_wrapper(ReturnType return_value,
    asio::error_code& ec)
{
  ec = asio::error_code(errno,
      asio::error::get_system_category());
  return return_value;
}

template <typename SockLenType>
inline socket_type call_accept(SockLenType msghdr::*,
    socket_type s, socket_addr_type* addr, std::size_t* addrlen)
{
  SockLenType tmp_addrlen = addrlen ? (SockLenType)*addrlen : 0;
  socket_type result = ::accept(s, addr, addrlen ? &tmp_addrlen : 0);
  if (addrlen)
    *addrlen = (std::size_t)tmp_addrlen;
  return result;
}

socket_type accept(socket_type s, socket_addr_type* addr,
    std::size_t* addrlen, asio::error_code& ec)
{
  if (s == invalid_socket)
  {
    ec = asio::error::bad_descriptor;
    return invalid_socket;
  }

  clear_last_error();

  socket_type new_s = error_wrapper(call_accept(
        &msghdr::msg_namelen, s, addr, addrlen), ec);
  if (new_s == invalid_socket)
    return new_s;

#if defined(__MACH__) && defined(__APPLE__) || defined(__FreeBSD__)
  int optval = 1;
  int result = error_wrapper(::setsockopt(new_s,
        SOL_SOCKET, SO_NOSIGPIPE, &optval, sizeof(optval)), ec);
  if (result != 0)
  {
    ::close(new_s);
    return invalid_socket;
  }
#endif

  ec = asio::error_code();
  return new_s;
}

socket_type sync_accept(socket_type s, state_type state,
    socket_addr_type* addr, std::size_t* addrlen, asio::error_code& ec)
{
  // Accept a socket.
  for (;;)
  {
    // Try to complete the operation without blocking.
    socket_type new_socket = socket_ops::accept(s, addr, addrlen, ec);

    // Check if operation succeeded.
    if (new_socket != invalid_socket)
      return new_socket;

    // Operation failed.
    if (ec == asio::error::would_block
        || ec == asio::error::try_again)
    {
      if (state & user_set_non_blocking)
        return invalid_socket;
      // Fall through to retry operation.
    }
    else if (ec == asio::error::connection_aborted)
    {
      if (state & enable_connection_aborted)
        return invalid_socket;
      // Fall through to retry operation.
    }
#if defined(EPROTO)
    else if (ec.value() == EPROTO)
    {
      if (state & enable_connection_aborted)
        return invalid_socket;
      // Fall through to retry operation.
    }
#endif // defined(EPROTO)
    else
      return invalid_socket;

    // Wait for socket to become ready.
    if (socket_ops::poll_read(s, 0, ec) < 0)
      return invalid_socket;
  }
}


bool non_blocking_accept(socket_type s,
    state_type state, socket_addr_type* addr, std::size_t* addrlen,
    asio::error_code& ec, socket_type& new_socket)
{
  for (;;)
  {
    // Accept the waiting connection.
    new_socket = socket_ops::accept(s, addr, addrlen, ec);

    // Check if operation succeeded.
    if (new_socket != invalid_socket)
      return true;

    // Retry operation if interrupted by signal.
    if (ec == asio::error::interrupted)
      continue;

    // Operation failed.
    if (ec == asio::error::would_block
        || ec == asio::error::try_again)
    {
      if (state & user_set_non_blocking)
        return true;
      // Fall through to retry operation.
    }
    else if (ec == asio::error::connection_aborted)
    {
      if (state & enable_connection_aborted)
        return true;
      // Fall through to retry operation.
    }
#if defined(EPROTO)
    else if (ec.value() == EPROTO)
    {
      if (state & enable_connection_aborted)
        return true;
      // Fall through to retry operation.
    }
#endif // defined(EPROTO)
    else
      return true;

    return false;
  }
}


template <typename SockLenType>
inline int call_bind(SockLenType msghdr::*,
    socket_type s, const socket_addr_type* addr, std::size_t addrlen)
{
  return ::bind(s, addr, (SockLenType)addrlen);
}

int bind(socket_type s, const socket_addr_type* addr,
    std::size_t addrlen, asio::error_code& ec)
{
  if (s == invalid_socket)
  {
    ec = asio::error::bad_descriptor;
    return socket_error_retval;
  }

  clear_last_error();
  int result = error_wrapper(call_bind(
        &msghdr::msg_namelen, s, addr, addrlen), ec);
  if (result == 0)
    ec = asio::error_code();
  return result;
}

int close(socket_type s, state_type& state,
    bool destruction, asio::error_code& ec)
{
  int result = 0;
  if (s != invalid_socket)
  {
    // We don't want the destructor to block, so set the socket to linger in
    // the background. If the user doesn't like this behaviour then they need
    // to explicitly close the socket.
    if (destruction && (state & user_set_linger))
    {
      ::linger opt;
      opt.l_onoff = 0;
      opt.l_linger = 0;
      asio::error_code ignored_ec;
      socket_ops::setsockopt(s, state, SOL_SOCKET,
          SO_LINGER, &opt, sizeof(opt), ignored_ec);
    }

    clear_last_error();
    result = error_wrapper(::close(s), ec);

    if (result != 0
        && (ec == asio::error::would_block
          || ec == asio::error::try_again))
    {
      // According to UNIX Network Programming Vol. 1, it is possible for
      // close() to fail with EWOULDBLOCK under certain circumstances. What
      // isn't clear is the state of the descriptor after this error. The one
      // current OS where this behaviour is seen, Windows, says that the socket
      // remains open. Therefore we'll put the descriptor back into blocking
      // mode and have another attempt at closing it.
      ioctl_arg_type arg = 0;
      ::ioctl(s, FIONBIO, &arg);
      state &= ~non_blocking;

      clear_last_error();
      result = error_wrapper(::close(s), ec);
    }
  }

  if (result == 0)
    ec = asio::error_code();
  return result;
}

bool set_user_non_blocking(socket_type s,
    state_type& state, bool value, asio::error_code& ec)
{
  if (s == invalid_socket)
  {
    ec = asio::error::bad_descriptor;
    return false;
  }

  clear_last_error();
  ioctl_arg_type arg = (value ? 1 : 0);
  int result = error_wrapper(::ioctl(s, FIONBIO, &arg), ec);

  if (result >= 0)
  {
    ec = asio::error_code();
    if (value)
      state |= user_set_non_blocking;
    else
    {
      // Clearing the user-set non-blocking mode always overrides any
      // internally-set non-blocking flag. Any subsequent asynchronous
      // operations will need to re-enable non-blocking I/O.
      state &= ~(user_set_non_blocking | internal_non_blocking);
    }
    return true;
  }

  return false;
}

bool set_internal_non_blocking(socket_type s,
    state_type& state, bool value, asio::error_code& ec)
{
  if (s == invalid_socket)
  {
    ec = asio::error::bad_descriptor;
    return false;
  }

  if (!value && (state & user_set_non_blocking))
  {
    // It does not make sense to clear the internal non-blocking flag if the
    // user still wants non-blocking behaviour. Return an error and let the
    // caller figure out whether to update the user-set non-blocking flag.
    ec = asio::error::invalid_argument;
    return false;
  }

  clear_last_error();
  ioctl_arg_type arg = (value ? 1 : 0);
  int result = error_wrapper(::ioctl(s, FIONBIO, &arg), ec);

  if (result >= 0)
  {
    ec = asio::error_code();
    if (value)
      state |= internal_non_blocking;
    else
      state &= ~internal_non_blocking;
    return true;
  }

  return false;
}

int shutdown(socket_type s, int what, asio::error_code& ec)
{
  if (s == invalid_socket)
  {
    ec = asio::error::bad_descriptor;
    return socket_error_retval;
  }

  clear_last_error();
  int result = error_wrapper(::shutdown(s, what), ec);
  if (result == 0)
    ec = asio::error_code();
  return result;
}

template <typename SockLenType>
inline int call_connect(SockLenType msghdr::*,
    socket_type s, const socket_addr_type* addr, std::size_t addrlen)
{
  return ::connect(s, addr, (SockLenType)addrlen);
}

int connect(socket_type s, const socket_addr_type* addr,
    std::size_t addrlen, asio::error_code& ec)
{
  if (s == invalid_socket)
  {
    ec = asio::error::bad_descriptor;
    return socket_error_retval;
  }

  clear_last_error();
  int result = error_wrapper(call_connect(
        &msghdr::msg_namelen, s, addr, addrlen), ec);
  if (result == 0)
    ec = asio::error_code();
#if defined(__linux__)
  else if (ec == asio::error::try_again)
    ec = asio::error::no_buffer_space;
#endif // defined(__linux__)
  return result;
}

void sync_connect(socket_type s, const socket_addr_type* addr,
    std::size_t addrlen, asio::error_code& ec)
{
  // Perform the connect operation.
  socket_ops::connect(s, addr, addrlen, ec);
  if (ec != asio::error::in_progress
      && ec != asio::error::would_block)
  {
    // The connect operation finished immediately.
    return;
  }

  // Wait for socket to become ready.
  if (socket_ops::poll_connect(s, ec) < 0)
    return;

  // Get the error code from the connect operation.
  int connect_error = 0;
  size_t connect_error_len = sizeof(connect_error);
  if (socket_ops::getsockopt(s, 0, SOL_SOCKET, SO_ERROR,
        &connect_error, &connect_error_len, ec) == socket_error_retval)
    return;

  // Return the result of the connect operation.
  ec = asio::error_code(connect_error,
      asio::error::get_system_category());
}


bool non_blocking_connect(socket_type s, asio::error_code& ec)
{
  // Check if the connect operation has finished. This is required since we may
  // get spurious readiness notifications from the reactor.
      // || defined(__CYGWIN__)
      // || defined(__SYMBIAN32__)
  pollfd fds;
  fds.fd = s;
  fds.events = POLLOUT;
  fds.revents = 0;
  int ready = ::poll(&fds, 1, 0);
       // || defined(__CYGWIN__)
       // || defined(__SYMBIAN32__)
  if (ready == 0)
  {
    // The asynchronous connect operation is still in progress.
    return false;
  }

  // Get the error code from the connect operation.
  int connect_error = 0;
  size_t connect_error_len = sizeof(connect_error);
  if (socket_ops::getsockopt(s, 0, SOL_SOCKET, SO_ERROR,
        &connect_error, &connect_error_len, ec) == 0)
  {
    if (connect_error)
    {
      ec = asio::error_code(connect_error,
          asio::error::get_system_category());
    }
    else
      ec = asio::error_code();
  }

  return true;
}

int socketpair(int af, int type, int protocol,
    socket_type sv[2], asio::error_code& ec)
{
  clear_last_error();
  int result = error_wrapper(::socketpair(af, type, protocol, sv), ec);
  if (result == 0)
    ec = asio::error_code();
  return result;
}

bool sockatmark(socket_type s, asio::error_code& ec)
{
  if (s == invalid_socket)
  {
    ec = asio::error::bad_descriptor;
    return false;
  }

#if defined(SIOCATMARK)
  ioctl_arg_type value = 0;
  int result = error_wrapper(::ioctl(s, SIOCATMARK, &value), ec);
  if (result == 0)
    ec = asio::error_code();
# if defined(ENOTTY)
  if (ec.value() == ENOTTY)
    ec = asio::error::not_socket;
# endif // defined(ENOTTY)
#else // defined(SIOCATMARK)
  int value = error_wrapper(::sockatmark(s), ec);
  if (value != -1)
    ec = asio::error_code();
#endif // defined(SIOCATMARK)

  return ec ? false : value != 0;
}

size_t available(socket_type s, asio::error_code& ec)
{
  if (s == invalid_socket)
  {
    ec = asio::error::bad_descriptor;
    return 0;
  }

  ioctl_arg_type value = 0;
  int result = error_wrapper(::ioctl(s, FIONREAD, &value), ec);
  if (result == 0)
    ec = asio::error_code();
#if defined(ENOTTY)
  if (ec.value() == ENOTTY)
    ec = asio::error::not_socket;
#endif // defined(ENOTTY)

  return ec ? static_cast<size_t>(0) : static_cast<size_t>(value);
}

int listen(socket_type s, int backlog, asio::error_code& ec)
{
  if (s == invalid_socket)
  {
    ec = asio::error::bad_descriptor;
    return socket_error_retval;
  }

  clear_last_error();
  int result = error_wrapper(::listen(s, backlog), ec);
  if (result == 0)
    ec = asio::error_code();
  return result;
}

inline void init_buf_iov_base(void*& base, void* addr)
{
  base = addr;
}

template <typename T>
inline void init_buf_iov_base(T& base, void* addr)
{
  base = static_cast<T>(addr);
}

typedef iovec buf;

void init_buf(buf& b, void* data, size_t size)
{
  init_buf_iov_base(b.iov_base, data);
  b.iov_len = size;
}

void init_buf(buf& b, const void* data, size_t size)
{
  init_buf_iov_base(b.iov_base, const_cast<void*>(data));
  b.iov_len = size;
}

inline void init_msghdr_msg_name(void*& name, socket_addr_type* addr)
{
  name = addr;
}

inline void init_msghdr_msg_name(void*& name, const socket_addr_type* addr)
{
  name = const_cast<socket_addr_type*>(addr);
}

template <typename T>
inline void init_msghdr_msg_name(T& name, socket_addr_type* addr)
{
  name = reinterpret_cast<T>(addr);
}

template <typename T>
inline void init_msghdr_msg_name(T& name, const socket_addr_type* addr)
{
  name = reinterpret_cast<T>(const_cast<socket_addr_type*>(addr));
}

signed_size_type recv(socket_type s, buf* bufs, size_t count,
    int flags, asio::error_code& ec)
{
  clear_last_error();
  msghdr msg = msghdr();
  msg.msg_iov = bufs;
  msg.msg_iovlen = static_cast<int>(count);
  signed_size_type result = error_wrapper(::recvmsg(s, &msg, flags), ec);
  if (result >= 0)
    ec = asio::error_code();
  return result;
}

size_t sync_recv(socket_type s, state_type state, buf* bufs,
    size_t count, int flags, bool all_empty, asio::error_code& ec)
{
  if (s == invalid_socket)
  {
    ec = asio::error::bad_descriptor;
    return 0;
  }

  // A request to read 0 bytes on a stream is a no-op.
  if (all_empty && (state & stream_oriented))
  {
    ec = asio::error_code();
    return 0;
  }

  // Read some data.
  for (;;)
  {
    // Try to complete the operation without blocking.
    signed_size_type bytes = socket_ops::recv(s, bufs, count, flags, ec);

    // Check if operation succeeded.
    if (bytes > 0)
      return bytes;

    // Check for EOF.
    if ((state & stream_oriented) && bytes == 0)
    {
      ec = asio::error::eof;
      return 0;
    }

    // Operation failed.
    if ((state & user_set_non_blocking)
        || (ec != asio::error::would_block
          && ec != asio::error::try_again))
      return 0;

    // Wait for socket to become ready.
    if (socket_ops::poll_read(s, 0, ec) < 0)
      return 0;
  }
}


bool non_blocking_recv(socket_type s,
    buf* bufs, size_t count, int flags, bool is_stream,
    asio::error_code& ec, size_t& bytes_transferred)
{
  for (;;)
  {
    // Read some data.
    signed_size_type bytes = socket_ops::recv(s, bufs, count, flags, ec);

    // Check for end of stream.
    if (is_stream && bytes == 0)
    {
      ec = asio::error::eof;
      return true;
    }

    // Retry operation if interrupted by signal.
    if (ec == asio::error::interrupted)
      continue;

    // Check if we need to run the operation again.
    if (ec == asio::error::would_block
        || ec == asio::error::try_again)
      return false;

    // Operation is complete.
    if (bytes >= 0)
    {
      ec = asio::error_code();
      bytes_transferred = bytes;
    }
    else
      bytes_transferred = 0;

    return true;
  }
}


signed_size_type recvfrom(socket_type s, buf* bufs, size_t count,
    int flags, socket_addr_type* addr, std::size_t* addrlen,
    asio::error_code& ec)
{
  clear_last_error();
  msghdr msg = msghdr();
  init_msghdr_msg_name(msg.msg_name, addr);
  msg.msg_namelen = static_cast<int>(*addrlen);
  msg.msg_iov = bufs;
  msg.msg_iovlen = static_cast<int>(count);
  signed_size_type result = error_wrapper(::recvmsg(s, &msg, flags), ec);
  *addrlen = msg.msg_namelen;
  if (result >= 0)
    ec = asio::error_code();
  return result;
}

size_t sync_recvfrom(socket_type s, state_type state, buf* bufs,
    size_t count, int flags, socket_addr_type* addr,
    std::size_t* addrlen, asio::error_code& ec)
{
  if (s == invalid_socket)
  {
    ec = asio::error::bad_descriptor;
    return 0;
  }

  // Read some data.
  for (;;)
  {
    // Try to complete the operation without blocking.
    signed_size_type bytes = socket_ops::recvfrom(
        s, bufs, count, flags, addr, addrlen, ec);

    // Check if operation succeeded.
    if (bytes >= 0)
      return bytes;

    // Operation failed.
    if ((state & user_set_non_blocking)
        || (ec != asio::error::would_block
          && ec != asio::error::try_again))
      return 0;

    // Wait for socket to become ready.
    if (socket_ops::poll_read(s, 0, ec) < 0)
      return 0;
  }
}


bool non_blocking_recvfrom(socket_type s,
    buf* bufs, size_t count, int flags,
    socket_addr_type* addr, std::size_t* addrlen,
    asio::error_code& ec, size_t& bytes_transferred)
{
  for (;;)
  {
    // Read some data.
    signed_size_type bytes = socket_ops::recvfrom(
        s, bufs, count, flags, addr, addrlen, ec);

    // Retry operation if interrupted by signal.
    if (ec == asio::error::interrupted)
      continue;

    // Check if we need to run the operation again.
    if (ec == asio::error::would_block
        || ec == asio::error::try_again)
      return false;

    // Operation is complete.
    if (bytes >= 0)
    {
      ec = asio::error_code();
      bytes_transferred = bytes;
    }
    else
      bytes_transferred = 0;

    return true;
  }
}


signed_size_type recvmsg(socket_type s, buf* bufs, size_t count,
    int in_flags, int& out_flags, asio::error_code& ec)
{
  clear_last_error();
  msghdr msg = msghdr();
  msg.msg_iov = bufs;
  msg.msg_iovlen = static_cast<int>(count);
  signed_size_type result = error_wrapper(::recvmsg(s, &msg, in_flags), ec);
  if (result >= 0)
  {
    ec = asio::error_code();
    out_flags = msg.msg_flags;
  }
  else
    out_flags = 0;
  return result;
}

size_t sync_recvmsg(socket_type s, state_type state,
    buf* bufs, size_t count, int in_flags, int& out_flags,
    asio::error_code& ec)
{
  if (s == invalid_socket)
  {
    ec = asio::error::bad_descriptor;
    return 0;
  }

  // Read some data.
  for (;;)
  {
    // Try to complete the operation without blocking.
    signed_size_type bytes = socket_ops::recvmsg(
        s, bufs, count, in_flags, out_flags, ec);

    // Check if operation succeeded.
    if (bytes >= 0)
      return bytes;

    // Operation failed.
    if ((state & user_set_non_blocking)
        || (ec != asio::error::would_block
          && ec != asio::error::try_again))
      return 0;

    // Wait for socket to become ready.
    if (socket_ops::poll_read(s, 0, ec) < 0)
      return 0;
  }
}


bool non_blocking_recvmsg(socket_type s,
    buf* bufs, size_t count, int in_flags, int& out_flags,
    asio::error_code& ec, size_t& bytes_transferred)
{
  for (;;)
  {
    // Read some data.
    signed_size_type bytes = socket_ops::recvmsg(
        s, bufs, count, in_flags, out_flags, ec);

    // Retry operation if interrupted by signal.
    if (ec == asio::error::interrupted)
      continue;

    // Check if we need to run the operation again.
    if (ec == asio::error::would_block
        || ec == asio::error::try_again)
      return false;

    // Operation is complete.
    if (bytes >= 0)
    {
      ec = asio::error_code();
      bytes_transferred = bytes;
    }
    else
      bytes_transferred = 0;

    return true;
  }
}


signed_size_type send(socket_type s, const buf* bufs, size_t count,
    int flags, asio::error_code& ec)
{
  clear_last_error();
  msghdr msg = msghdr();
  msg.msg_iov = const_cast<buf*>(bufs);
  msg.msg_iovlen = static_cast<int>(count);
#if defined(__linux__)
  flags |= MSG_NOSIGNAL;
#endif // defined(__linux__)
  signed_size_type result = error_wrapper(::sendmsg(s, &msg, flags), ec);
  if (result >= 0)
    ec = asio::error_code();
  return result;
}

size_t sync_send(socket_type s, state_type state, const buf* bufs,
    size_t count, int flags, bool all_empty, asio::error_code& ec)
{
  if (s == invalid_socket)
  {
    ec = asio::error::bad_descriptor;
    return 0;
  }

  // A request to write 0 bytes to a stream is a no-op.
  if (all_empty && (state & stream_oriented))
  {
    ec = asio::error_code();
    return 0;
  }

  // Read some data.
  for (;;)
  {
    // Try to complete the operation without blocking.
    signed_size_type bytes = socket_ops::send(s, bufs, count, flags, ec);

    // Check if operation succeeded.
    if (bytes >= 0)
      return bytes;

    // Operation failed.
    if ((state & user_set_non_blocking)
        || (ec != asio::error::would_block
          && ec != asio::error::try_again))
      return 0;

    // Wait for socket to become ready.
    if (socket_ops::poll_write(s, 0, ec) < 0)
      return 0;
  }
}


bool non_blocking_send(socket_type s,
    const buf* bufs, size_t count, int flags,
    asio::error_code& ec, size_t& bytes_transferred)
{
  for (;;)
  {
    // Write some data.
    signed_size_type bytes = socket_ops::send(s, bufs, count, flags, ec);

    // Retry operation if interrupted by signal.
    if (ec == asio::error::interrupted)
      continue;

    // Check if we need to run the operation again.
    if (ec == asio::error::would_block
        || ec == asio::error::try_again)
      return false;

    // Operation is complete.
    if (bytes >= 0)
    {
      ec = asio::error_code();
      bytes_transferred = bytes;
    }
    else
      bytes_transferred = 0;

    return true;
  }
}


signed_size_type sendto(socket_type s, const buf* bufs, size_t count,
    int flags, const socket_addr_type* addr, std::size_t addrlen,
    asio::error_code& ec)
{
  clear_last_error();
  msghdr msg = msghdr();
  init_msghdr_msg_name(msg.msg_name, addr);
  msg.msg_namelen = static_cast<int>(addrlen);
  msg.msg_iov = const_cast<buf*>(bufs);
  msg.msg_iovlen = static_cast<int>(count);
#if defined(__linux__)
  flags |= MSG_NOSIGNAL;
#endif // defined(__linux__)
  signed_size_type result = error_wrapper(::sendmsg(s, &msg, flags), ec);
  if (result >= 0)
    ec = asio::error_code();
  return result;
}

size_t sync_sendto(socket_type s, state_type state, const buf* bufs,
    size_t count, int flags, const socket_addr_type* addr,
    std::size_t addrlen, asio::error_code& ec)
{
  if (s == invalid_socket)
  {
    ec = asio::error::bad_descriptor;
    return 0;
  }

  // Write some data.
  for (;;)
  {
    // Try to complete the operation without blocking.
    signed_size_type bytes = socket_ops::sendto(
        s, bufs, count, flags, addr, addrlen, ec);

    // Check if operation succeeded.
    if (bytes >= 0)
      return bytes;

    // Operation failed.
    if ((state & user_set_non_blocking)
        || (ec != asio::error::would_block
          && ec != asio::error::try_again))
      return 0;

    // Wait for socket to become ready.
    if (socket_ops::poll_write(s, 0, ec) < 0)
      return 0;
  }
}


bool non_blocking_sendto(socket_type s,
    const buf* bufs, size_t count, int flags,
    const socket_addr_type* addr, std::size_t addrlen,
    asio::error_code& ec, size_t& bytes_transferred)
{
  for (;;)
  {
    // Write some data.
    signed_size_type bytes = socket_ops::sendto(
        s, bufs, count, flags, addr, addrlen, ec);

    // Retry operation if interrupted by signal.
    if (ec == asio::error::interrupted)
      continue;

    // Check if we need to run the operation again.
    if (ec == asio::error::would_block
        || ec == asio::error::try_again)
      return false;

    // Operation is complete.
    if (bytes >= 0)
    {
      ec = asio::error_code();
      bytes_transferred = bytes;
    }
    else
      bytes_transferred = 0;

    return true;
  }
}


socket_type socket(int af, int type, int protocol,
    asio::error_code& ec)
{
  clear_last_error();
#if   defined(__MACH__) && defined(__APPLE__) || defined(__FreeBSD__)
  socket_type s = error_wrapper(::socket(af, type, protocol), ec);
  if (s == invalid_socket)
    return s;

  int optval = 1;
  int result = error_wrapper(::setsockopt(s,
        SOL_SOCKET, SO_NOSIGPIPE, &optval, sizeof(optval)), ec);
  if (result != 0)
  {
    ::close(s);
    return invalid_socket;
  }

  return s;
#else
  int s = error_wrapper(::socket(af, type, protocol), ec);
  if (s >= 0)
    ec = asio::error_code();
  return s;
#endif
}

template <typename SockLenType>
inline int call_setsockopt(SockLenType msghdr::*,
    socket_type s, int level, int optname,
    const void* optval, std::size_t optlen)
{
  return ::setsockopt(s, level, optname,
      (const char*)optval, (SockLenType)optlen);
}

int setsockopt(socket_type s, state_type& state, int level, int optname,
    const void* optval, std::size_t optlen, asio::error_code& ec)
{
  if (s == invalid_socket)
  {
    ec = asio::error::bad_descriptor;
    return socket_error_retval;
  }

  if (level == custom_socket_option_level && optname == always_fail_option)
  {
    ec = asio::error::invalid_argument;
    return socket_error_retval;
  }

  if (level == custom_socket_option_level
      && optname == enable_connection_aborted_option)
  {
    if (optlen != sizeof(int))
    {
      ec = asio::error::invalid_argument;
      return socket_error_retval;
    }

    if (*static_cast<const int*>(optval))
      state |= enable_connection_aborted;
    else
      state &= ~enable_connection_aborted;
    ec = asio::error_code();
    return 0;
  }

  if (level == SOL_SOCKET && optname == SO_LINGER)
    state |= user_set_linger;

  clear_last_error();
  int result = error_wrapper(call_setsockopt(&msghdr::msg_namelen,
        s, level, optname, optval, optlen), ec);
  if (result == 0)
  {
    ec = asio::error_code();

#if defined(__MACH__) && defined(__APPLE__)    || defined(__NetBSD__) || defined(__FreeBSD__) || defined(__OpenBSD__)
    // To implement portable behaviour for SO_REUSEADDR with UDP sockets we
    // need to also set SO_REUSEPORT on BSD-based platforms.
    if ((state & datagram_oriented)
        && level == SOL_SOCKET && optname == SO_REUSEADDR)
    {
      call_setsockopt(&msghdr::msg_namelen, s,
          SOL_SOCKET, SO_REUSEPORT, optval, optlen);
    }
#endif
  }

  return result;
}

template <typename SockLenType>
inline int call_getsockopt(SockLenType msghdr::*,
    socket_type s, int level, int optname,
    void* optval, std::size_t* optlen)
{
  SockLenType tmp_optlen = (SockLenType)*optlen;
  int result = ::getsockopt(s, level, optname, (char*)optval, &tmp_optlen);
  *optlen = (std::size_t)tmp_optlen;
  return result;
}

int getsockopt(socket_type s, state_type state, int level, int optname,
    void* optval, size_t* optlen, asio::error_code& ec)
{
  if (s == invalid_socket)
  {
    ec = asio::error::bad_descriptor;
    return socket_error_retval;
  }

  if (level == custom_socket_option_level && optname == always_fail_option)
  {
    ec = asio::error::invalid_argument;
    return socket_error_retval;
  }

  if (level == custom_socket_option_level
      && optname == enable_connection_aborted_option)
  {
    if (*optlen != sizeof(int))
    {
      ec = asio::error::invalid_argument;
      return socket_error_retval;
    }

    *static_cast<int*>(optval) = (state & enable_connection_aborted) ? 1 : 0;
    ec = asio::error_code();
    return 0;
  }

  clear_last_error();
  int result = error_wrapper(call_getsockopt(&msghdr::msg_namelen,
        s, level, optname, optval, optlen), ec);
#if defined(__linux__)
  if (result == 0 && level == SOL_SOCKET && *optlen == sizeof(int)
      && (optname == SO_SNDBUF || optname == SO_RCVBUF))
  {
    // On Linux, setting SO_SNDBUF or SO_RCVBUF to N actually causes the kernel
    // to set the buffer size to N*2. Linux puts additional stuff into the
    // buffers so that only about half is actually available to the application.
    // The retrieved value is divided by 2 here to make it appear as though the
    // correct value has been set.
    *static_cast<int*>(optval) /= 2;
  }
#endif // defined(__linux__)
  if (result == 0)
    ec = asio::error_code();
  return result;
}

template <typename SockLenType>
inline int call_getpeername(SockLenType msghdr::*,
    socket_type s, socket_addr_type* addr, std::size_t* addrlen)
{
  SockLenType tmp_addrlen = (SockLenType)*addrlen;
  int result = ::getpeername(s, addr, &tmp_addrlen);
  *addrlen = (std::size_t)tmp_addrlen;
  return result;
}

int getpeername(socket_type s, socket_addr_type* addr,
    std::size_t* addrlen, bool cached, asio::error_code& ec)
{
  if (s == invalid_socket)
  {
    ec = asio::error::bad_descriptor;
    return socket_error_retval;
  }

  (void)cached;

  clear_last_error();
  int result = error_wrapper(call_getpeername(
        &msghdr::msg_namelen, s, addr, addrlen), ec);
  if (result == 0)
    ec = asio::error_code();
  return result;
}

template <typename SockLenType>
inline int call_getsockname(SockLenType msghdr::*,
    socket_type s, socket_addr_type* addr, std::size_t* addrlen)
{
  SockLenType tmp_addrlen = (SockLenType)*addrlen;
  int result = ::getsockname(s, addr, &tmp_addrlen);
  *addrlen = (std::size_t)tmp_addrlen;
  return result;
}

int getsockname(socket_type s, socket_addr_type* addr,
    std::size_t* addrlen, asio::error_code& ec)
{
  if (s == invalid_socket)
  {
    ec = asio::error::bad_descriptor;
    return socket_error_retval;
  }

  clear_last_error();
  int result = error_wrapper(call_getsockname(
        &msghdr::msg_namelen, s, addr, addrlen), ec);
  if (result == 0)
    ec = asio::error_code();
  return result;
}

int ioctl(socket_type s, state_type& state, int cmd,
    ioctl_arg_type* arg, asio::error_code& ec)
{
  if (s == invalid_socket)
  {
    ec = asio::error::bad_descriptor;
    return socket_error_retval;
  }

  clear_last_error();
#if   defined(__MACH__) && defined(__APPLE__)    || defined(__NetBSD__) || defined(__FreeBSD__) || defined(__OpenBSD__)
  int result = error_wrapper(::ioctl(s,
        static_cast<unsigned int>(cmd), arg), ec);
#else
  int result = error_wrapper(::ioctl(s, cmd, arg), ec);
#endif
  if (result >= 0)
  {
    ec = asio::error_code();

    // When updating the non-blocking mode we always perform the ioctl syscall,
    // even if the flags would otherwise indicate that the socket is already in
    // the correct state. This ensures that the underlying socket is put into
    // the state that has been requested by the user. If the ioctl syscall was
    // successful then we need to update the flags to match.
    if (cmd == static_cast<int>(FIONBIO))
    {
      if (*arg)
      {
        state |= user_set_non_blocking;
      }
      else
      {
        // Clearing the non-blocking mode always overrides any internally-set
        // non-blocking flag. Any subsequent asynchronous operations will need
        // to re-enable non-blocking I/O.
        state &= ~(user_set_non_blocking | internal_non_blocking);
      }
    }
  }

  return result;
}

int select(int nfds, fd_set* readfds, fd_set* writefds,
    fd_set* exceptfds, timeval* timeout, asio::error_code& ec)
{
  clear_last_error();

#if defined(__hpux) && defined(__SELECT)
  timespec ts;
  ts.tv_sec = timeout ? timeout->tv_sec : 0;
  ts.tv_nsec = timeout ? timeout->tv_usec * 1000 : 0;
  return error_wrapper(::pselect(nfds, readfds,
        writefds, exceptfds, timeout ? &ts : 0, 0), ec);
#else
  int result = error_wrapper(::select(nfds, readfds,
        writefds, exceptfds, timeout), ec);
  if (result >= 0)
    ec = asio::error_code();
  return result;
#endif
}

int poll_read(socket_type s, state_type state, asio::error_code& ec)
{
  if (s == invalid_socket)
  {
    ec = asio::error::bad_descriptor;
    return socket_error_retval;
  }

      // || defined(__CYGWIN__)
      // || defined(__SYMBIAN32__)
  pollfd fds;
  fds.fd = s;
  fds.events = POLLIN;
  fds.revents = 0;
  int timeout = (state & user_set_non_blocking) ? 0 : -1;
  clear_last_error();
  int result = error_wrapper(::poll(&fds, 1, timeout), ec);
       // || defined(__CYGWIN__)
       // || defined(__SYMBIAN32__)
  if (result == 0)
    ec = (state & user_set_non_blocking)
      ? asio::error::would_block : asio::error_code();
  else if (result > 0)
    ec = asio::error_code();
  return result;
}

int poll_write(socket_type s, state_type state, asio::error_code& ec)
{
  if (s == invalid_socket)
  {
    ec = asio::error::bad_descriptor;
    return socket_error_retval;
  }

      // || defined(__CYGWIN__)
      // || defined(__SYMBIAN32__)
  pollfd fds;
  fds.fd = s;
  fds.events = POLLOUT;
  fds.revents = 0;
  int timeout = (state & user_set_non_blocking) ? 0 : -1;
  clear_last_error();
  int result = error_wrapper(::poll(&fds, 1, timeout), ec);
       // || defined(__CYGWIN__)
       // || defined(__SYMBIAN32__)
  if (result == 0)
    ec = (state & user_set_non_blocking)
      ? asio::error::would_block : asio::error_code();
  else if (result > 0)
    ec = asio::error_code();
  return result;
}

int poll_connect(socket_type s, asio::error_code& ec)
{
  if (s == invalid_socket)
  {
    ec = asio::error::bad_descriptor;
    return socket_error_retval;
  }

      // || defined(__CYGWIN__)
      // || defined(__SYMBIAN32__)
  pollfd fds;
  fds.fd = s;
  fds.events = POLLOUT;
  fds.revents = 0;
  clear_last_error();
  int result = error_wrapper(::poll(&fds, 1, -1), ec);
  if (result >= 0)
    ec = asio::error_code();
  return result;
       // || defined(__CYGWIN__)
       // || defined(__SYMBIAN32__)
}


const char* inet_ntop(int af, const void* src, char* dest, size_t length,
    unsigned long scope_id, asio::error_code& ec)
{
  clear_last_error();
  const char* result = error_wrapper(::inet_ntop(
        af, src, dest, static_cast<int>(length)), ec);
  if (result == 0 && !ec)
    ec = asio::error::invalid_argument;
  if (result != 0 && af == ASIO_OS_DEF(AF_INET6) && scope_id != 0)
  {
    using namespace std; // For strcat and sprintf.
    char if_name[IF_NAMESIZE + 1] = "%";
    const in6_addr_type* ipv6_address = static_cast<const in6_addr_type*>(src);
    bool is_link_local = ((ipv6_address->s6_addr[0] == 0xfe)
        && ((ipv6_address->s6_addr[1] & 0xc0) == 0x80));
    bool is_multicast_link_local = ((ipv6_address->s6_addr[0] == 0xff)
        && ((ipv6_address->s6_addr[1] & 0x0f) == 0x02));
    if ((!is_link_local && !is_multicast_link_local)
        || if_indextoname(static_cast<unsigned>(scope_id), if_name + 1) == 0)
      sprintf(if_name + 1, "%lu", scope_id);
    strcat(dest, if_name);
  }
  return result;
}

int inet_pton(int af, const char* src, void* dest,
    unsigned long* scope_id, asio::error_code& ec)
{
  clear_last_error();
  using namespace std; // For strchr, memcpy and atoi.

  // On some platforms, inet_pton fails if an address string contains a scope
  // id. Detect and remove the scope id before passing the string to inet_pton.
  const bool is_v6 = (af == ASIO_OS_DEF(AF_INET6));
  const char* if_name = is_v6 ? strchr(src, '%') : 0;
  char src_buf[max_addr_v6_str_len + 1];
  const char* src_ptr = src;
  if (if_name != 0)
  {
    if (if_name - src > max_addr_v6_str_len)
    {
      ec = asio::error::invalid_argument;
      return 0;
    }
    memcpy(src_buf, src, if_name - src);
    src_buf[if_name - src] = 0;
    src_ptr = src_buf;
  }

  int result = error_wrapper(::inet_pton(af, src_ptr, dest), ec);
  if (result <= 0 && !ec)
    ec = asio::error::invalid_argument;
  if (result > 0 && is_v6 && scope_id)
  {
    using namespace std; // For strchr and atoi.
    *scope_id = 0;
    if (if_name != 0)
    {
      in6_addr_type* ipv6_address = static_cast<in6_addr_type*>(dest);
      bool is_link_local = ((ipv6_address->s6_addr[0] == 0xfe)
          && ((ipv6_address->s6_addr[1] & 0xc0) == 0x80));
      bool is_multicast_link_local = ((ipv6_address->s6_addr[0] == 0xff)
          && ((ipv6_address->s6_addr[1] & 0x0f) == 0x02));
      if (is_link_local || is_multicast_link_local)
        *scope_id = if_nametoindex(if_name + 1);
      if (*scope_id == 0)
        *scope_id = atoi(if_name + 1);
    }
  }
  return result;
}

int gethostname(char* name, int namelen, asio::error_code& ec)
{
  clear_last_error();
  int result = error_wrapper(::gethostname(name, namelen), ec);
  return result;
}



inline asio::error_code translate_addrinfo_error(int error)
{
  switch (error)
  {
  case 0:
    return asio::error_code();
  case EAI_AGAIN:
    return asio::error::host_not_found_try_again;
  case EAI_BADFLAGS:
    return asio::error::invalid_argument;
  case EAI_FAIL:
    return asio::error::no_recovery;
  case EAI_FAMILY:
    return asio::error::address_family_not_supported;
  case EAI_MEMORY:
    return asio::error::no_memory;
  case EAI_NONAME:
#if defined(EAI_ADDRFAMILY)
  case EAI_ADDRFAMILY:
#endif
#if defined(EAI_NODATA) && (EAI_NODATA != EAI_NONAME)
  case EAI_NODATA:
#endif
    return asio::error::host_not_found;
  case EAI_SERVICE:
    return asio::error::service_not_found;
  case EAI_SOCKTYPE:
    return asio::error::socket_type_not_supported;
  default: // Possibly the non-portable EAI_SYSTEM.
    return asio::error_code(
        errno, asio::error::get_system_category());
  }
}

asio::error_code getaddrinfo(const char* host,
    const char* service, const addrinfo_type& hints,
    addrinfo_type** result, asio::error_code& ec)
{
  host = (host && *host) ? host : 0;
  service = (service && *service) ? service : 0;
  clear_last_error();
  int error = ::getaddrinfo(host, service, &hints, result);
  return ec = translate_addrinfo_error(error);
}

asio::error_code background_getaddrinfo(
    const weak_cancel_token_type& cancel_token, const char* host,
    const char* service, const addrinfo_type& hints,
    addrinfo_type** result, asio::error_code& ec)
{
  if (cancel_token.expired())
    ec = asio::error::operation_aborted;
  else
    socket_ops::getaddrinfo(host, service, hints, result, ec);
  return ec;
}

void freeaddrinfo(addrinfo_type* ai)
{
  ::freeaddrinfo(ai);
}

asio::error_code getnameinfo(const socket_addr_type* addr,
    std::size_t addrlen, char* host, std::size_t hostlen,
    char* serv, std::size_t servlen, int flags, asio::error_code& ec)
{
  clear_last_error();
  int error = ::getnameinfo(addr, addrlen, host, hostlen, serv, servlen, flags);
  return ec = translate_addrinfo_error(error);
}

asio::error_code sync_getnameinfo(
    const socket_addr_type* addr, std::size_t addrlen,
    char* host, std::size_t hostlen, char* serv,
    std::size_t servlen, int sock_type, asio::error_code& ec)
{
  // First try resolving with the service name. If that fails try resolving
  // but allow the service to be returned as a number.
  int flags = (sock_type == SOCK_DGRAM) ? NI_DGRAM : 0;
  socket_ops::getnameinfo(addr, addrlen, host,
      hostlen, serv, servlen, flags, ec);
  if (ec)
  {
    socket_ops::getnameinfo(addr, addrlen, host, hostlen,
        serv, servlen, flags | NI_NUMERICSERV, ec);
  }

  return ec;
}

asio::error_code background_getnameinfo(
    const weak_cancel_token_type& cancel_token,
    const socket_addr_type* addr, std::size_t addrlen,
    char* host, std::size_t hostlen, char* serv,
    std::size_t servlen, int sock_type, asio::error_code& ec)
{
  if (cancel_token.expired())
  {
    ec = asio::error::operation_aborted;
  }
  else
  {
    // First try resolving with the service name. If that fails try resolving
    // but allow the service to be returned as a number.
    int flags = (sock_type == SOCK_DGRAM) ? NI_DGRAM : 0;
    socket_ops::getnameinfo(addr, addrlen, host,
        hostlen, serv, servlen, flags, ec);
    if (ec)
    {
      socket_ops::getnameinfo(addr, addrlen, host, hostlen,
          serv, servlen, flags | NI_NUMERICSERV, ec);
    }
  }

  return ec;
}


u_long_type network_to_host_long(u_long_type value)
{
  return ntohl(value);
}

u_long_type host_to_network_long(u_long_type value)
{
  return htonl(value);
}

u_short_type network_to_host_short(u_short_type value)
{
  return ntohs(value);
}

u_short_type host_to_network_short(u_short_type value)
{
  return htons(value);
}

} // namespace socket_ops
} // namespace detail
} // namespace asio

#include "asio/detail/pop_options.hpp"

#endif // ASIO_DETAIL_SOCKET_OPS_IPP
