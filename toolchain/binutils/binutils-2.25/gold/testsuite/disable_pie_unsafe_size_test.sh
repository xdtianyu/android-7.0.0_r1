#!/bin/sh

# disable_pie_unsafe_size_test.sh -- test if PIE disabled for large data sizes.

# Copyright (C) 2015 Free Software Foundation, Inc.
# Written by Sriraman Tallam <tmsriram@google.com>.

# This file is part of gold.

# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 3 of the License, or
# (at your option) any later version.

# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.

# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street - Fifth Floor, Boston,
# MA 02110-1301, USA.

# This tests a call to a weak undefined function.  We use LD_PRELOAD
# to force the function to be defined.

# Check if PIE is disabled, output of readelf must have EXEC instead of DYN
# for file type.  PIE must be disabled for executable
# disable_pie_unsafe_size_test.cc since it has a data segment size of > 128 MB.

grep 'Elf file type is EXEC (Executable file)' ./disable_pie_unsafe_size_test_readelf.stdout
