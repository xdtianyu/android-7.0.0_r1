# Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# Description:
#
# Python version of include/asm-generic/ioctl.h


import struct


# ioctl command encoding: 32 bits total, command in lower 16 bits,
# size of the parameter structure in the lower 14 bits of the
# upper 16 bits.
# Encoding the size of the parameter structure in the ioctl request
# is useful for catching programs compiled with old versions
# and to avoid overwriting user space outside the user buffer area.
# The highest 2 bits are reserved for indicating the ``access mode''.
# NOTE: This limits the max parameter size to 16kB -1 !

_IOC_NRBITS    = 8
_IOC_TYPEBITS  = 8
_IOC_SIZEBITS  = 14
_IOC_DIRBITS   = 2

_IOC_NRMASK    = ((1 << _IOC_NRBITS) - 1)
_IOC_TYPEMASK  = ((1 << _IOC_TYPEBITS) - 1)
_IOC_SIZEMASK  = ((1 << _IOC_SIZEBITS) - 1)
_IOC_DIRMASK   = ((1 << _IOC_DIRBITS) - 1)

_IOC_NRSHIFT   = 0
_IOC_TYPESHIFT = (_IOC_NRSHIFT + _IOC_NRBITS)
_IOC_SIZESHIFT = (_IOC_TYPESHIFT + _IOC_TYPEBITS)
_IOC_DIRSHIFT  = (_IOC_SIZESHIFT + _IOC_SIZEBITS)

IOC_NONE      = 0
IOC_WRITE     = 1
IOC_READ      = 2

# Return the byte size of a python struct format string
def sizeof(t):
    return struct.calcsize(t)

def IOC(d, t, nr, size):
    return ((d << _IOC_DIRSHIFT) | (ord(t) << _IOC_TYPESHIFT) |
            (nr << _IOC_NRSHIFT) | (size << _IOC_SIZESHIFT))

# used to create numbers
def IO(t, nr, t_format):
    return IOC(IOC_NONE, t, nr, 0)

def IOW(t, nr, t_format):
    return IOC(IOC_WRITE, t, nr, sizeof(t_format))

def IOR(t, nr, t_format):
    return IOC(IOC_READ, t, nr, sizeof(t_format))

def IOWR(t, nr, t_format):
    return IOC(IOC_READ|_IOC_WRITE, t, nr, sizeof(t_format))

# used to decode ioctl numbers..
def IOC_DIR(nr):
    return ((nr >> _IOC_DIRSHIFT) & _IOC_DIRMASK)

def IOC_TYPE(nr):
    return ((nr >> _IOC_TYPESHIFT) & _IOC_TYPEMASK)

def IOC_NR(nr):
    return ((nr >> _IOC_NRSHIFT) & _IOC_NRMASK)

def IOC_SIZE(nr):
    return ((nr >> _IOC_SIZESHIFT) & _IOC_SIZEMASK)

# ...and for the drivers/sound files...
IOC_IN          = (IOC_WRITE << _IOC_DIRSHIFT)
IOC_OUT         = (IOC_READ << _IOC_DIRSHIFT)
IOC_INOUT       = ((IOC_WRITE | IOC_READ) << _IOC_DIRSHIFT)
IOCSIZE_MASK    = (_IOC_SIZEMASK << _IOC_SIZESHIFT)
IOCSIZE_SHIFT   = (_IOC_SIZESHIFT)

