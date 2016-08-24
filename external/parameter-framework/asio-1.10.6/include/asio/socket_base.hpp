//
// socket_base.hpp
// ~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2015 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef ASIO_SOCKET_BASE_HPP
#define ASIO_SOCKET_BASE_HPP


#include "asio/detail/config.hpp"
#include "asio/detail/io_control.hpp"
#include "asio/detail/socket_option.hpp"
#include "asio/detail/socket_types.hpp"

#include "asio/detail/push_options.hpp"

namespace asio {

/// The socket_base class is used as a base for the basic_stream_socket and
/// basic_datagram_socket class templates so that we have a common place to
/// define the shutdown_type and enum.
class socket_base
{
public:
  /// Different ways a socket may be shutdown.
  enum shutdown_type
  {
    shutdown_receive = ASIO_OS_DEF(SHUT_RD),
    shutdown_send = ASIO_OS_DEF(SHUT_WR),
    shutdown_both = ASIO_OS_DEF(SHUT_RDWR)
  };

  /// Bitmask type for flags that can be passed to send and receive operations.
  typedef int message_flags;

  ASIO_STATIC_CONSTANT(int,
      message_peek = ASIO_OS_DEF(MSG_PEEK));
  ASIO_STATIC_CONSTANT(int,
      message_out_of_band = ASIO_OS_DEF(MSG_OOB));
  ASIO_STATIC_CONSTANT(int,
      message_do_not_route = ASIO_OS_DEF(MSG_DONTROUTE));
  ASIO_STATIC_CONSTANT(int,
      message_end_of_record = ASIO_OS_DEF(MSG_EOR));

  /// Socket option to permit sending of broadcast messages.
  /**
   * Implements the SOL_SOCKET/SO_BROADCAST socket option.
   *
   * @par Examples
   * Setting the option:
   * @code
   * asio::ip::udp::socket socket(io_service); 
   * ...
   * asio::socket_base::broadcast option(true);
   * socket.set_option(option);
   * @endcode
   *
   * @par
   * Getting the current option value:
   * @code
   * asio::ip::udp::socket socket(io_service); 
   * ...
   * asio::socket_base::broadcast option;
   * socket.get_option(option);
   * bool is_set = option.value();
   * @endcode
   *
   * @par Concepts:
   * Socket_Option, Boolean_Socket_Option.
   */
  typedef asio::detail::socket_option::boolean<
    ASIO_OS_DEF(SOL_SOCKET), ASIO_OS_DEF(SO_BROADCAST)>
      broadcast;

  /// Socket option to enable socket-level debugging.
  /**
   * Implements the SOL_SOCKET/SO_DEBUG socket option.
   *
   * @par Examples
   * Setting the option:
   * @code
   * asio::ip::tcp::socket socket(io_service); 
   * ...
   * asio::socket_base::debug option(true);
   * socket.set_option(option);
   * @endcode
   *
   * @par
   * Getting the current option value:
   * @code
   * asio::ip::tcp::socket socket(io_service); 
   * ...
   * asio::socket_base::debug option;
   * socket.get_option(option);
   * bool is_set = option.value();
   * @endcode
   *
   * @par Concepts:
   * Socket_Option, Boolean_Socket_Option.
   */
  typedef asio::detail::socket_option::boolean<
    ASIO_OS_DEF(SOL_SOCKET), ASIO_OS_DEF(SO_DEBUG)> debug;

  /// Socket option to prevent routing, use local interfaces only.
  /**
   * Implements the SOL_SOCKET/SO_DONTROUTE socket option.
   *
   * @par Examples
   * Setting the option:
   * @code
   * asio::ip::udp::socket socket(io_service); 
   * ...
   * asio::socket_base::do_not_route option(true);
   * socket.set_option(option);
   * @endcode
   *
   * @par
   * Getting the current option value:
   * @code
   * asio::ip::udp::socket socket(io_service); 
   * ...
   * asio::socket_base::do_not_route option;
   * socket.get_option(option);
   * bool is_set = option.value();
   * @endcode
   *
   * @par Concepts:
   * Socket_Option, Boolean_Socket_Option.
   */
  typedef asio::detail::socket_option::boolean<
    ASIO_OS_DEF(SOL_SOCKET), ASIO_OS_DEF(SO_DONTROUTE)>
      do_not_route;

  /// Socket option to send keep-alives.
  /**
   * Implements the SOL_SOCKET/SO_KEEPALIVE socket option.
   *
   * @par Examples
   * Setting the option:
   * @code
   * asio::ip::tcp::socket socket(io_service); 
   * ...
   * asio::socket_base::keep_alive option(true);
   * socket.set_option(option);
   * @endcode
   *
   * @par
   * Getting the current option value:
   * @code
   * asio::ip::tcp::socket socket(io_service); 
   * ...
   * asio::socket_base::keep_alive option;
   * socket.get_option(option);
   * bool is_set = option.value();
   * @endcode
   *
   * @par Concepts:
   * Socket_Option, Boolean_Socket_Option.
   */
  typedef asio::detail::socket_option::boolean<
    ASIO_OS_DEF(SOL_SOCKET), ASIO_OS_DEF(SO_KEEPALIVE)> keep_alive;

  /// Socket option for the send buffer size of a socket.
  /**
   * Implements the SOL_SOCKET/SO_SNDBUF socket option.
   *
   * @par Examples
   * Setting the option:
   * @code
   * asio::ip::tcp::socket socket(io_service); 
   * ...
   * asio::socket_base::send_buffer_size option(8192);
   * socket.set_option(option);
   * @endcode
   *
   * @par
   * Getting the current option value:
   * @code
   * asio::ip::tcp::socket socket(io_service); 
   * ...
   * asio::socket_base::send_buffer_size option;
   * socket.get_option(option);
   * int size = option.value();
   * @endcode
   *
   * @par Concepts:
   * Socket_Option, Integer_Socket_Option.
   */
  typedef asio::detail::socket_option::integer<
    ASIO_OS_DEF(SOL_SOCKET), ASIO_OS_DEF(SO_SNDBUF)>
      send_buffer_size;

  /// Socket option for the send low watermark.
  /**
   * Implements the SOL_SOCKET/SO_SNDLOWAT socket option.
   *
   * @par Examples
   * Setting the option:
   * @code
   * asio::ip::tcp::socket socket(io_service); 
   * ...
   * asio::socket_base::send_low_watermark option(1024);
   * socket.set_option(option);
   * @endcode
   *
   * @par
   * Getting the current option value:
   * @code
   * asio::ip::tcp::socket socket(io_service); 
   * ...
   * asio::socket_base::send_low_watermark option;
   * socket.get_option(option);
   * int size = option.value();
   * @endcode
   *
   * @par Concepts:
   * Socket_Option, Integer_Socket_Option.
   */
  typedef asio::detail::socket_option::integer<
    ASIO_OS_DEF(SOL_SOCKET), ASIO_OS_DEF(SO_SNDLOWAT)>
      send_low_watermark;

  /// Socket option for the receive buffer size of a socket.
  /**
   * Implements the SOL_SOCKET/SO_RCVBUF socket option.
   *
   * @par Examples
   * Setting the option:
   * @code
   * asio::ip::tcp::socket socket(io_service); 
   * ...
   * asio::socket_base::receive_buffer_size option(8192);
   * socket.set_option(option);
   * @endcode
   *
   * @par
   * Getting the current option value:
   * @code
   * asio::ip::tcp::socket socket(io_service); 
   * ...
   * asio::socket_base::receive_buffer_size option;
   * socket.get_option(option);
   * int size = option.value();
   * @endcode
   *
   * @par Concepts:
   * Socket_Option, Integer_Socket_Option.
   */
  typedef asio::detail::socket_option::integer<
    ASIO_OS_DEF(SOL_SOCKET), ASIO_OS_DEF(SO_RCVBUF)>
      receive_buffer_size;

  /// Socket option for the receive low watermark.
  /**
   * Implements the SOL_SOCKET/SO_RCVLOWAT socket option.
   *
   * @par Examples
   * Setting the option:
   * @code
   * asio::ip::tcp::socket socket(io_service); 
   * ...
   * asio::socket_base::receive_low_watermark option(1024);
   * socket.set_option(option);
   * @endcode
   *
   * @par
   * Getting the current option value:
   * @code
   * asio::ip::tcp::socket socket(io_service); 
   * ...
   * asio::socket_base::receive_low_watermark option;
   * socket.get_option(option);
   * int size = option.value();
   * @endcode
   *
   * @par Concepts:
   * Socket_Option, Integer_Socket_Option.
   */
  typedef asio::detail::socket_option::integer<
    ASIO_OS_DEF(SOL_SOCKET), ASIO_OS_DEF(SO_RCVLOWAT)>
      receive_low_watermark;

  /// Socket option to allow the socket to be bound to an address that is
  /// already in use.
  /**
   * Implements the SOL_SOCKET/SO_REUSEADDR socket option.
   *
   * @par Examples
   * Setting the option:
   * @code
   * asio::ip::tcp::acceptor acceptor(io_service); 
   * ...
   * asio::socket_base::reuse_address option(true);
   * acceptor.set_option(option);
   * @endcode
   *
   * @par
   * Getting the current option value:
   * @code
   * asio::ip::tcp::acceptor acceptor(io_service); 
   * ...
   * asio::socket_base::reuse_address option;
   * acceptor.get_option(option);
   * bool is_set = option.value();
   * @endcode
   *
   * @par Concepts:
   * Socket_Option, Boolean_Socket_Option.
   */
  typedef asio::detail::socket_option::boolean<
    ASIO_OS_DEF(SOL_SOCKET), ASIO_OS_DEF(SO_REUSEADDR)>
      reuse_address;

  /// Socket option to specify whether the socket lingers on close if unsent
  /// data is present.
  /**
   * Implements the SOL_SOCKET/SO_LINGER socket option.
   *
   * @par Examples
   * Setting the option:
   * @code
   * asio::ip::tcp::socket socket(io_service); 
   * ...
   * asio::socket_base::linger option(true, 30);
   * socket.set_option(option);
   * @endcode
   *
   * @par
   * Getting the current option value:
   * @code
   * asio::ip::tcp::socket socket(io_service); 
   * ...
   * asio::socket_base::linger option;
   * socket.get_option(option);
   * bool is_set = option.enabled();
   * unsigned short timeout = option.timeout();
   * @endcode
   *
   * @par Concepts:
   * Socket_Option, Linger_Socket_Option.
   */
  typedef asio::detail::socket_option::linger<
    ASIO_OS_DEF(SOL_SOCKET), ASIO_OS_DEF(SO_LINGER)>
      linger;

  /// Socket option to report aborted connections on accept.
  /**
   * Implements a custom socket option that determines whether or not an accept
   * operation is permitted to fail with asio::error::connection_aborted.
   * By default the option is false.
   *
   * @par Examples
   * Setting the option:
   * @code
   * asio::ip::tcp::acceptor acceptor(io_service); 
   * ...
   * asio::socket_base::enable_connection_aborted option(true);
   * acceptor.set_option(option);
   * @endcode
   *
   * @par
   * Getting the current option value:
   * @code
   * asio::ip::tcp::acceptor acceptor(io_service); 
   * ...
   * asio::socket_base::enable_connection_aborted option;
   * acceptor.get_option(option);
   * bool is_set = option.value();
   * @endcode
   *
   * @par Concepts:
   * Socket_Option, Boolean_Socket_Option.
   */
  typedef asio::detail::socket_option::boolean<
    asio::detail::custom_socket_option_level,
    asio::detail::enable_connection_aborted_option>
    enable_connection_aborted;

  /// (Deprecated: Use non_blocking().) IO control command to
  /// set the blocking mode of the socket.
  /**
   * Implements the FIONBIO IO control command.
   *
   * @par Example
   * @code
   * asio::ip::tcp::socket socket(io_service); 
   * ...
   * asio::socket_base::non_blocking_io command(true);
   * socket.io_control(command);
   * @endcode
   *
   * @par Concepts:
   * IO_Control_Command, Boolean_IO_Control_Command.
   */
  typedef asio::detail::io_control::non_blocking_io non_blocking_io;

  /// IO control command to get the amount of data that can be read without
  /// blocking.
  /**
   * Implements the FIONREAD IO control command.
   *
   * @par Example
   * @code
   * asio::ip::tcp::socket socket(io_service); 
   * ...
   * asio::socket_base::bytes_readable command(true);
   * socket.io_control(command);
   * std::size_t bytes_readable = command.get();
   * @endcode
   *
   * @par Concepts:
   * IO_Control_Command, Size_IO_Control_Command.
   */
  typedef asio::detail::io_control::bytes_readable bytes_readable;

  /// The maximum length of the queue of pending incoming connections.
  ASIO_STATIC_CONSTANT(int, max_connections
      = ASIO_OS_DEF(SOMAXCONN));

protected:
  /// Protected destructor to prevent deletion through this type.
  ~socket_base()
  {
  }
};

} // namespace asio

#include "asio/detail/pop_options.hpp"

#endif // ASIO_SOCKET_BASE_HPP
