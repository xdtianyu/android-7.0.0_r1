//
// detail/thread.hpp
// ~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2015 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef ASIO_DETAIL_THREAD_HPP
#define ASIO_DETAIL_THREAD_HPP


#include "asio/detail/config.hpp"

#if   defined(ASIO_HAS_PTHREADS)
# include "asio/detail/posix_thread.hpp"
#else
# include "asio/detail/std_thread.hpp"
#endif

namespace asio {
namespace detail {

#if   defined(ASIO_HAS_PTHREADS)
typedef posix_thread thread;
#else
typedef std_thread thread;
#endif

} // namespace detail
} // namespace asio

#endif // ASIO_DETAIL_THREAD_HPP
