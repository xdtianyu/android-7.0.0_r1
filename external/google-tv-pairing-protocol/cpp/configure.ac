## Process this file with autoconf to produce configure.
## In general, the safest way to proceed is to run ./autogen.sh

# make sure we're interpreted by some minimal autoconf
AC_PREREQ(2.57)

AC_INIT(polo, 0.1, opensource@google.com)

SO_VERSION=0:0:0

AC_ARG_WITH([gtest],
  [AS_HELP_STRING([--with-gtest], [location of Google Test source])],
  [GTEST_DIR=$withval])
AC_SUBST([GTEST_DIR])

AC_ARG_WITH([gmock],
  [AS_HELP_STRING([--with-gmock], [location of Google Mock source])],
  [GMOCK_DIR=$withval])
AC_SUBST([GMOCK_DIR])

# The argument here is just something that should be in the current directory
# (for sanity checking)
AC_CONFIG_SRCDIR(README)
AC_CONFIG_MACRO_DIR([m4])
AM_INIT_AUTOMAKE
AM_CONFIG_HEADER(src/config.h)

AC_LANG(C++)

# Checks for programs.
AC_PROG_CC
AC_PROG_CPP
AC_PROG_CXX
AM_CONDITIONAL(GCC, test "$GCC" = yes)   # let the Makefile know if we're gcc

# Check for required packages.
PKG_CHECK_MODULES(PROTOBUF, protobuf)
PKG_CHECK_MODULES(GLOG, libglog)
PKG_CHECK_MODULES(SSL, libssl)
PKG_CHECK_MODULES(CRYPTO, libcrypto)

AC_PATH_PROG(PROTOC, protoc,
             [AC_MSG_ERROR([protoc was not found in your path])])

AC_PROG_LIBTOOL
AC_SUBST(LIBTOOL_DEPS)
AC_SUBST(SO_VERSION)

# Check whether some low-level functions/files are available
AC_HEADER_STDC

# Write generated configuration file
AC_CONFIG_FILES([Makefile polo.pc])
AC_OUTPUT
