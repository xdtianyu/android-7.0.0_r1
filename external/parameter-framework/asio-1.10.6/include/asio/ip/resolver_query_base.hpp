//
// ip/resolver_query_base.hpp
// ~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2015 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef ASIO_IP_RESOLVER_QUERY_BASE_HPP
#define ASIO_IP_RESOLVER_QUERY_BASE_HPP


#include "asio/detail/config.hpp"
#include "asio/detail/socket_types.hpp"

#include "asio/detail/push_options.hpp"

namespace asio {
namespace ip {

/// The resolver_query_base class is used as a base for the
/// basic_resolver_query class templates to provide a common place to define
/// the flag constants.
class resolver_query_base
{
public:
  enum flags
  {
    canonical_name = ASIO_OS_DEF(AI_CANONNAME),
    passive = ASIO_OS_DEF(AI_PASSIVE),
    numeric_host = ASIO_OS_DEF(AI_NUMERICHOST),
    numeric_service = ASIO_OS_DEF(AI_NUMERICSERV),
    v4_mapped = ASIO_OS_DEF(AI_V4MAPPED),
    all_matching = ASIO_OS_DEF(AI_ALL),
    address_configured = ASIO_OS_DEF(AI_ADDRCONFIG)
  };

  // Implement bitmask operations as shown in C++ Std [lib.bitmask.types].

  friend flags operator&(flags x, flags y)
  {
    return static_cast<flags>(
        static_cast<unsigned int>(x) & static_cast<unsigned int>(y));
  }

  friend flags operator|(flags x, flags y)
  {
    return static_cast<flags>(
        static_cast<unsigned int>(x) | static_cast<unsigned int>(y));
  }

  friend flags operator^(flags x, flags y)
  {
    return static_cast<flags>(
        static_cast<unsigned int>(x) ^ static_cast<unsigned int>(y));
  }

  friend flags operator~(flags x)
  {
    return static_cast<flags>(~static_cast<unsigned int>(x));
  }

  friend flags& operator&=(flags& x, flags y)
  {
    x = x & y;
    return x;
  }

  friend flags& operator|=(flags& x, flags y)
  {
    x = x | y;
    return x;
  }

  friend flags& operator^=(flags& x, flags y)
  {
    x = x ^ y;
    return x;
  }

protected:
  /// Protected destructor to prevent deletion through this type.
  ~resolver_query_base()
  {
  }
};

} // namespace ip
} // namespace asio

#include "asio/detail/pop_options.hpp"

#endif // ASIO_IP_RESOLVER_QUERY_BASE_HPP
