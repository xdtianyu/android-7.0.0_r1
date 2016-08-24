//
// ip/resolver_service.hpp
// ~~~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2015 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef ASIO_IP_RESOLVER_SERVICE_HPP
#define ASIO_IP_RESOLVER_SERVICE_HPP


#include "asio/detail/config.hpp"
#include "asio/async_result.hpp"
#include "asio/error_code.hpp"
#include "asio/io_service.hpp"
#include "asio/ip/basic_resolver_iterator.hpp"
#include "asio/ip/basic_resolver_query.hpp"

# include "asio/detail/resolver_service.hpp"

#include "asio/detail/push_options.hpp"

namespace asio {
namespace ip {

/// Default service implementation for a resolver.
template <typename InternetProtocol>
class resolver_service
  : public asio::detail::service_base<
      resolver_service<InternetProtocol> >
{
public:

  /// The protocol type.
  typedef InternetProtocol protocol_type;

  /// The endpoint type.
  typedef typename InternetProtocol::endpoint endpoint_type;

  /// The query type.
  typedef basic_resolver_query<InternetProtocol> query_type;

  /// The iterator type.
  typedef basic_resolver_iterator<InternetProtocol> iterator_type;

private:
  // The type of the platform-specific implementation.
  typedef asio::detail::resolver_service<InternetProtocol>
    service_impl_type;

public:
  /// The type of a resolver implementation.
  typedef typename service_impl_type::implementation_type implementation_type;

  /// Construct a new resolver service for the specified io_service.
  explicit resolver_service(asio::io_service& io_service)
    : asio::detail::service_base<
        resolver_service<InternetProtocol> >(io_service),
      service_impl_(io_service)
  {
  }

  /// Construct a new resolver implementation.
  void construct(implementation_type& impl)
  {
    service_impl_.construct(impl);
  }

  /// Destroy a resolver implementation.
  void destroy(implementation_type& impl)
  {
    service_impl_.destroy(impl);
  }

  /// Cancel pending asynchronous operations.
  void cancel(implementation_type& impl)
  {
    service_impl_.cancel(impl);
  }

  /// Resolve a query to a list of entries.
  iterator_type resolve(implementation_type& impl, const query_type& query,
      asio::error_code& ec)
  {
    return service_impl_.resolve(impl, query, ec);
  }

  /// Asynchronously resolve a query to a list of entries.
  template <typename ResolveHandler>
  ASIO_INITFN_RESULT_TYPE(ResolveHandler,
      void (asio::error_code, iterator_type))
  async_resolve(implementation_type& impl, const query_type& query,
      ASIO_MOVE_ARG(ResolveHandler) handler)
  {
    asio::detail::async_result_init<
      ResolveHandler, void (asio::error_code, iterator_type)> init(
        ASIO_MOVE_CAST(ResolveHandler)(handler));

    service_impl_.async_resolve(impl, query, init.handler);

    return init.result.get();
  }

  /// Resolve an endpoint to a list of entries.
  iterator_type resolve(implementation_type& impl,
      const endpoint_type& endpoint, asio::error_code& ec)
  {
    return service_impl_.resolve(impl, endpoint, ec);
  }

  /// Asynchronously resolve an endpoint to a list of entries.
  template <typename ResolveHandler>
  ASIO_INITFN_RESULT_TYPE(ResolveHandler,
      void (asio::error_code, iterator_type))
  async_resolve(implementation_type& impl, const endpoint_type& endpoint,
      ASIO_MOVE_ARG(ResolveHandler) handler)
  {
    asio::detail::async_result_init<
      ResolveHandler, void (asio::error_code, iterator_type)> init(
        ASIO_MOVE_CAST(ResolveHandler)(handler));

    service_impl_.async_resolve(impl, endpoint, init.handler);

    return init.result.get();
  }

private:
  // Destroy all user-defined handler objects owned by the service.
  void shutdown_service()
  {
    service_impl_.shutdown_service();
  }

  // Perform any fork-related housekeeping.
  void fork_service(asio::io_service::fork_event event)
  {
    service_impl_.fork_service(event);
  }

  // The platform-specific implementation.
  service_impl_type service_impl_;
};

} // namespace ip
} // namespace asio

#include "asio/detail/pop_options.hpp"

#endif // ASIO_IP_RESOLVER_SERVICE_HPP
