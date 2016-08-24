//
// detail/atomic_count.hpp
// ~~~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2015 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef ASIO_DETAIL_ATOMIC_COUNT_HPP
#define ASIO_DETAIL_ATOMIC_COUNT_HPP


#include "asio/detail/config.hpp"

# include <atomic>

namespace asio {
namespace detail {

typedef std::atomic<long> atomic_count;
inline void increment(atomic_count& a, long b) { a += b; }

} // namespace detail
} // namespace asio

#endif // ASIO_DETAIL_ATOMIC_COUNT_HPP
