//
// detail/config.hpp
// ~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2015 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef ASIO_DETAIL_CONFIG_HPP
#define ASIO_DETAIL_CONFIG_HPP

# define ASIO_DISABLE_BOOST_ARRAY 1
# define ASIO_DISABLE_BOOST_ASSERT 1
# define ASIO_DISABLE_BOOST_BIND 1
# define ASIO_DISABLE_BOOST_CHRONO 1
# define ASIO_DISABLE_BOOST_DATE_TIME 1
# define ASIO_DISABLE_BOOST_LIMITS 1
# define ASIO_DISABLE_BOOST_REGEX 1
# define ASIO_DISABLE_BOOST_STATIC_CONSTANT 1
# define ASIO_DISABLE_BOOST_THROW_EXCEPTION 1
# define ASIO_DISABLE_BOOST_WORKAROUND 1

// Default to a header-only implementation. The user must specifically request
// separate compilation by defining either ASIO_SEPARATE_COMPILATION or
// ASIO_DYN_LINK (as a DLL/shared library implies separate compilation).

# define ASIO_DECL inline

// If ASIO_DECL isn't defined yet define it now.
#if !defined(ASIO_DECL)
# define ASIO_DECL
#endif // !defined(ASIO_DECL)

// Microsoft Visual C++ detection.

// Clang / libc++ detection.
# if (__cplusplus >= 201103)
#  if __has_include(<__config>)
#   include <__config>
#   if defined(_LIBCPP_VERSION)
#    define ASIO_HAS_CLANG_LIBCXX 1
#   endif // defined(_LIBCPP_VERSION)
#  endif // __has_include(<__config>)
# endif // (__cplusplus >= 201103)

// Support move construction and assignment on compilers known to allow it.

// If ASIO_MOVE_CAST isn't defined, and move support is available, define
// ASIO_MOVE_ARG and ASIO_MOVE_CAST to take advantage of rvalue
// references and perfect forwarding.
#if defined(ASIO_HAS_MOVE) && !defined(ASIO_MOVE_CAST)
# define ASIO_MOVE_ARG(type) type&&
# define ASIO_MOVE_CAST(type) static_cast<type&&>
# define ASIO_MOVE_CAST2(type1, type2) static_cast<type1, type2&&>
#endif // defined(ASIO_HAS_MOVE) && !defined(ASIO_MOVE_CAST)

// If ASIO_MOVE_CAST still isn't defined, default to a C++03-compatible
// implementation. Note that older g++ and MSVC versions don't like it when you
// pass a non-member function through a const reference, so for most compilers
// we'll play it safe and stick with the old approach of passing the handler by
// value.
#if !defined(ASIO_MOVE_CAST)
# if defined(__GNUC__)
#  if ((__GNUC__ == 4) && (__GNUC_MINOR__ >= 1)) || (__GNUC__ > 4)
#   define ASIO_MOVE_ARG(type) const type&
#  else // ((__GNUC__ == 4) && (__GNUC_MINOR__ >= 1)) || (__GNUC__ > 4)
#   define ASIO_MOVE_ARG(type) type
#  endif // ((__GNUC__ == 4) && (__GNUC_MINOR__ >= 1)) || (__GNUC__ > 4)
# else
#  define ASIO_MOVE_ARG(type) type
# endif
# define ASIO_MOVE_CAST(type) static_cast<const type&>
# define ASIO_MOVE_CAST2(type1, type2) static_cast<const type1, type2&>
#endif // !defined(ASIO_MOVE_CAST)

// Support variadic templates on compilers known to allow it.

// Support constexpr on compilers known to allow it.
#if !defined(ASIO_CONSTEXPR)
#  define ASIO_CONSTEXPR constexpr
#endif // !defined(ASIO_CONSTEXPR)

// Standard library support for system errors.

// Compliant C++11 compilers put noexcept specifiers on error_category members.
#if !defined(ASIO_ERROR_CATEGORY_NOEXCEPT)
# if (BOOST_VERSION >= 105300)
#  define ASIO_ERROR_CATEGORY_NOEXCEPT BOOST_NOEXCEPT
# else
#  if __has_feature(__cxx_noexcept__)
#   define ASIO_ERROR_CATEGORY_NOEXCEPT noexcept(true)
#  endif // __has_feature(__cxx_noexcept__)
# endif
# if !defined(ASIO_ERROR_CATEGORY_NOEXCEPT)
#  define ASIO_ERROR_CATEGORY_NOEXCEPT
# endif // !defined(ASIO_ERROR_CATEGORY_NOEXCEPT)
#endif // !defined(ASIO_ERROR_CATEGORY_NOEXCEPT)

// Standard library support for arrays.

// Standard library support for shared_ptr and weak_ptr.

// Standard library support for atomic operations.

// Standard library support for chrono. Some standard libraries (such as the
// libstdc++ shipped with gcc 4.6) provide monotonic_clock as per early C++0x
// drafts, rather than the eventually standardised name of steady_clock.

// Boost support for chrono.

// Boost support for the DateTime library.

// Standard library support for addressof.

// Standard library support for the function class.

// Standard library support for type traits.

// Standard library support for the cstdint header.

// Standard library support for the thread class.

// Standard library support for the mutex and condition variable classes.

// WinRT target.
# if defined(__cplusplus_winrt)
#  include <winapifamily.h>
#  if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_APP)     && !WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
#   define ASIO_WINDOWS_RUNTIME 1
#  endif // WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_APP)
         // && !WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
# endif // defined(__cplusplus_winrt)

// Windows target. Excludes WinRT.

// Windows: target OS version.

// Windows: minimise header inclusion.

// Windows: suppress definition of "min" and "max" macros.

// Windows: IO Completion Ports.

// On POSIX (and POSIX-like) platforms we need to include unistd.h in order to
// get access to the various platform feature macros, e.g. to be able to test
// for threads support.
#if !defined(ASIO_HAS_UNISTD_H)
#  if defined(unix)     || defined(__unix)     || defined(_XOPEN_SOURCE)     || defined(_POSIX_SOURCE)     || (defined(__MACH__) && defined(__APPLE__))     || defined(__FreeBSD__)     || defined(__NetBSD__)     || defined(__OpenBSD__)     || defined(__linux__)
#   define ASIO_HAS_UNISTD_H 1
#  endif
#endif // !defined(ASIO_HAS_UNISTD_H)
#if defined(ASIO_HAS_UNISTD_H)
# include <unistd.h>
#endif // defined(ASIO_HAS_UNISTD_H)

// Linux: epoll, eventfd and timerfd.
#if defined(__linux__)
# include <linux/version.h>
# if !defined(ASIO_HAS_EPOLL)
# endif // !defined(ASIO_HAS_EPOLL)
#  if defined(ASIO_HAS_EPOLL)
#   if (__GLIBC__ > 2) || (__GLIBC__ == 2 && __GLIBC_MINOR__ >= 8)
#    define ASIO_HAS_TIMERFD 1
#   endif // (__GLIBC__ > 2) || (__GLIBC__ == 2 && __GLIBC_MINOR__ >= 8)
#  endif // defined(ASIO_HAS_EPOLL)
#endif // defined(__linux__)

// Mac OS X, FreeBSD, NetBSD, OpenBSD: kqueue.
#if (defined(__MACH__) && defined(__APPLE__))    || defined(__FreeBSD__)    || defined(__NetBSD__)    || defined(__OpenBSD__)
#endif // (defined(__MACH__) && defined(__APPLE__))
       //   || defined(__FreeBSD__)
       //   || defined(__NetBSD__)
       //   || defined(__OpenBSD__)

// Solaris: /dev/poll.
#if defined(__sun)
# if !defined(ASIO_HAS_DEV_POLL)
#  if !defined(ASIO_DISABLE_DEV_POLL)
#   define ASIO_HAS_DEV_POLL 1
#  endif // !defined(ASIO_DISABLE_DEV_POLL)
# endif // !defined(ASIO_HAS_DEV_POLL)
#endif // defined(__sun)

// Serial ports.
        //   || !defined(ASIO_WINDOWS)
        //   && !defined(ASIO_WINDOWS_RUNTIME)
        //   && !defined(__CYGWIN__)

// Windows: stream handles.
# if !defined(ASIO_DISABLE_WINDOWS_STREAM_HANDLE)
# endif // !defined(ASIO_DISABLE_WINDOWS_STREAM_HANDLE)

// Windows: random access handles.
# if !defined(ASIO_DISABLE_WINDOWS_RANDOM_ACCESS_HANDLE)
# endif // !defined(ASIO_DISABLE_WINDOWS_RANDOM_ACCESS_HANDLE)

// Windows: object handles.
# if !defined(ASIO_DISABLE_WINDOWS_OBJECT_HANDLE)
# endif // !defined(ASIO_DISABLE_WINDOWS_OBJECT_HANDLE)

// Windows: OVERLAPPED wrapper.
# if !defined(ASIO_DISABLE_WINDOWS_OVERLAPPED_PTR)
# endif // !defined(ASIO_DISABLE_WINDOWS_OVERLAPPED_PTR)

// POSIX: stream-oriented file descriptors.

// UNIX domain sockets.

// Can use sigaction() instead of signal().
#if !defined(ASIO_HAS_SIGACTION)
# if !defined(ASIO_DISABLE_SIGACTION)
#   define ASIO_HAS_SIGACTION 1
         //   && !defined(ASIO_WINDOWS_RUNTIME)
         //   && !defined(__CYGWIN__)
# endif // !defined(ASIO_DISABLE_SIGACTION)
#endif // !defined(ASIO_HAS_SIGACTION)

// Can use signal().
#if !defined(ASIO_HAS_SIGNAL)
# if !defined(ASIO_DISABLE_SIGNAL)
#  if !defined(UNDER_CE)
#   define ASIO_HAS_SIGNAL 1
#  endif // !defined(UNDER_CE)
# endif // !defined(ASIO_DISABLE_SIGNAL)
#endif // !defined(ASIO_HAS_SIGNAL)

// Can use getaddrinfo() and getnameinfo().

// Whether standard iostreams are disabled.

// Whether exception handling is disabled.
#if !defined(ASIO_NO_EXCEPTIONS)
#endif // !defined(ASIO_NO_EXCEPTIONS)

// Whether the typeid operator is supported.
#if !defined(ASIO_NO_TYPEID)
#endif // !defined(ASIO_NO_TYPEID)

// Threads.

// POSIX threads.
#if !defined(ASIO_HAS_PTHREADS)
#  if   defined(_POSIX_THREADS)
#   define ASIO_HAS_PTHREADS 1
#  endif // defined(ASIO_HAS_BOOST_CONFIG) && defined(BOOST_HAS_PTHREADS)
#endif // !defined(ASIO_HAS_PTHREADS)

// Helper to prevent macro expansion.
#define ASIO_PREVENT_MACRO_SUBSTITUTION

// Helper to define in-class constants.
#if !defined(ASIO_STATIC_CONSTANT)
#  define ASIO_STATIC_CONSTANT(type, assignment)      static const type assignment
#endif // !defined(ASIO_STATIC_CONSTANT)

// Boost array library.

// Boost assert macro.

// Boost limits header.

// Boost throw_exception function.

// Boost regex library.

// Boost bind function.

// Boost's BOOST_WORKAROUND macro.

// Microsoft Visual C++'s secure C runtime library.
#if !defined(ASIO_HAS_SECURE_RTL)
# if !defined(ASIO_DISABLE_SECURE_RTL)
         // && (ASIO_MSVC >= 1400)
         // && !defined(UNDER_CE)
# endif // !defined(ASIO_DISABLE_SECURE_RTL)
#endif // !defined(ASIO_HAS_SECURE_RTL)

// Handler hooking. Disabled for ancient Borland C++ and gcc compilers.
#if !defined(ASIO_HAS_HANDLER_HOOKS)
# if !defined(ASIO_DISABLE_HANDLER_HOOKS)
#  if defined(__GNUC__)
#   if (__GNUC__ >= 3)
#    define ASIO_HAS_HANDLER_HOOKS 1
#   endif // (__GNUC__ >= 3)
#  else
#   define ASIO_HAS_HANDLER_HOOKS 1
#  endif // !defined(__BORLANDC__)
# endif // !defined(ASIO_DISABLE_HANDLER_HOOKS)
#endif // !defined(ASIO_HAS_HANDLER_HOOKS)

// Support for the __thread keyword extension.
#if !defined(ASIO_THREAD_KEYWORD)
# define ASIO_THREAD_KEYWORD __thread
#endif // !defined(ASIO_THREAD_KEYWORD)

// Support for POSIX ssize_t typedef.
#if !defined(ASIO_DISABLE_SSIZE_T)
# if defined(__linux__)     || (defined(__MACH__) && defined(__APPLE__))
#  define ASIO_HAS_SSIZE_T 1
# endif // defined(__linux__)
        //   || (defined(__MACH__) && defined(__APPLE__))
#endif // !defined(ASIO_DISABLE_SSIZE_T)

#endif // ASIO_DETAIL_CONFIG_HPP
