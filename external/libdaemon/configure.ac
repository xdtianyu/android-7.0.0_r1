# -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

# This file is part of libdaemon.
#
# Copyright 2003-2008 Lennart Poettering
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.

AC_PREREQ(2.63)

AC_INIT([libdaemon],[0.14],[mzqnrzba (at) 0pointer (dot) de])
AC_CONFIG_SRCDIR([libdaemon/dfork.c])
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_MACRO_DIR([m4])

AM_INIT_AUTOMAKE([foreign 1.10 -Wall])

AC_SUBST(PACKAGE_URL, [http://0pointer.de/lennart/projects/libdaemon/])

AC_SUBST(LIBDAEMON_VERSION_INFO, [5:0:5])

if type -p stow > /dev/null && test -d /usr/local/stow ; then
   AC_MSG_NOTICE([*** Found /usr/local/stow: default install prefix set to /usr/local/stow/${PACKAGE_NAME}-${PACKAGE_VERSION} ***])
   ac_default_prefix="/usr/local/stow/${PACKAGE_NAME}-${PACKAGE_VERSION}"
fi

m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES([yes])])

# Checks for programs.
AC_PROG_CC
AC_PROG_CC_C99 dnl This enable gnu99 if present
AM_PROG_CC_C_O

AC_USE_SYSTEM_EXTENSIONS

AC_PROG_INSTALL
AC_PROG_LN_S
AC_PROG_MAKE_SET
AC_PROG_LIBTOOL

CC_CHECK_CFLAGS_APPEND([-pipe -Wall -W -Wextra -pedantic -Wformat -Wold-style-definition -Wdeclaration-after-statement -Wfloat-equal -Wmissing-declarations -Wmissing-prototypes -Wstrict-prototypes -Wredundant-decls -Wmissing-noreturn -Wshadow -Wendif-labels -Wpointer-arith -Wbad-function-cast -Wcast-qual -Wcast-align -Wwrite-strings -Winline -Wno-unused-parameter -Wstrict-aliasing])

# Checks for header files.
AC_HEADER_ASSERT
AC_HEADER_STDC
AC_CHECK_HEADERS([fcntl.h limits.h stdlib.h string.h syslog.h unistd.h sys/ioctl.h sys/time.h])
AC_HEADER_SYS_WAIT

# Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST
AC_TYPE_PID_T
AC_HEADER_TIME
AC_C_VOLATILE

# Checks for library functions.
AC_FUNC_FORK
AC_FUNC_SELECT_ARGTYPES
AC_FUNC_VPRINTF
AC_CHECK_FUNCS([select strerror dup2 memset strrchr asprintf])
AC_TYPE_MODE_T
AC_FUNC_SETPGRP
AC_TYPE_SIGNAL
AC_TYPE_SIZE_T

AC_SYS_LARGEFILE

CC_NOUNDEFINED

ZP_LYNX_DOC

AC_ARG_ENABLE(examples,
       AS_HELP_STRING([--disable-examples], [Don't build examples during make]),,
       enable_examples="yes")
AM_CONDITIONAL(EXAMPLES, [test "$enable_examples" = "yes"])

AC_CONFIG_FILES([libdaemon/Makefile Makefile doc/Makefile doc/README.html doc/doxygen.conf examples/Makefile pkgconfig/libdaemon.pc pkgconfig/libdaemon-uninstalled.pc])
AC_OUTPUT
