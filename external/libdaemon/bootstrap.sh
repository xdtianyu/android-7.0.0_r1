#!/bin/sh

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

VERSION=1.9

run_versioned() {
    local P
    local V

    V=$(echo "$2" | sed -e 's,\.,,g')

    if [ -e "`which $1$V 2> /dev/null`" ] ; then
        P="$1$V"
    else
	if [ -e "`which $1-$2 2> /dev/null`" ] ; then
            P="$1-$2"
	else
	    P="$1"
	fi
    fi

    shift 2
    "$P" "$@"
}

set -ex

if [ "x$1" = "xam" ] ; then
    run_versioned automake "$VERSION" -a -c --foreign
    ./config.status
else
    rm -rf autom4te.cache
    rm -f config.cache

    touch config.rpath
    test "x$LIBTOOLIZE" = "x" && LIBTOOLIZE=libtoolize

    mkdir -p common

    "$LIBTOOLIZE" -c --force
    run_versioned aclocal "$VERSION" -I m4
    run_versioned autoconf 2.59 -Wall
    run_versioned autoheader 2.59
    run_versioned automake "$VERSION" -a -c --foreign

    if test "x$NOCONFIGURE" = "x"; then
        CFLAGS="-g -O0" ./configure --sysconfdir=/etc --localstatedir=/var "$@"
        make clean
    fi
fi
