//
// error_code.hpp
// ~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2015 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef ASIO_ERROR_CODE_HPP
#define ASIO_ERROR_CODE_HPP


#include "asio/detail/config.hpp"

# include <system_error>

#include "asio/detail/push_options.hpp"

namespace asio {


typedef std::error_category error_category;


/// Returns the error category used for the system errors produced by asio.
extern ASIO_DECL const error_category& system_category();


typedef std::error_code error_code;


} // namespace asio

#include "asio/detail/pop_options.hpp"

# include "asio/impl/error_code.ipp"

#endif // ASIO_ERROR_CODE_HPP
