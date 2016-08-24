//
// detail/mutex.hpp
// ~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2015 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef ASIO_DETAIL_MUTEX_HPP
#define ASIO_DETAIL_MUTEX_HPP


#include "asio/detail/config.hpp"

#if   defined(ASIO_HAS_PTHREADS)
# include "asio/detail/posix_mutex.hpp"
#else
# include "asio/detail/std_mutex.hpp"
#endif

namespace asio {
namespace detail {

#if   defined(ASIO_HAS_PTHREADS)
typedef posix_mutex mutex;
#else
typedef std_mutex mutex;
#endif

} // namespace detail
} // namespace asio

#endif // ASIO_DETAIL_MUTEX_HPP
