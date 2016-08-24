#!/usr/bin/python
#
# Copyright (c) 2015 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import os
from autotest_lib.client.common_lib import error
from autotest_lib.client.bin import test
from autotest_lib.client.bin import utils
from autotest_lib.client.cros import kernel_config

import ctypes
import hashlib
import logging
import binascii


AF_ALG = 38
SOCK_SEQPACKET = 5

class sockaddr_alg(ctypes.Structure):
    """
    A python definition of the same struct from <linux/if_alg.h>

    struct sockaddr_alg {
            __u16   salg_family;
            __u8    salg_type[14];
            __u32   salg_feat;
            __u32   salg_mask;
            __u8    salg_name[64];
    };
    """
    _fields_ = [
        ('salg_family', ctypes.c_uint16),
        ('salg_type', ctypes.c_char * 14),
        ('salg_feat', ctypes.c_uint32),
        ('salg_mask', ctypes.c_uint32),
        ('salg_name', ctypes.c_char * 64),
    ]


    def __init__(self, alg_family, alg_type, alg_name, alg_feat=0, alg_mask=0):
        super(sockaddr_alg, self).__init__(alg_family, alg_type, alg_feat,
                                           alg_mask, alg_name)


class kernel_CryptoAPI(test.test):
    """
    Verify that the crypto user API can't be used to load arbitrary modules.
    Uses the kernel module 'test_module'
    """
    version = 1
    preserve_srcdir = True

    def initialize(self):
        self.job.require_gcc()


    def setup(self):
        os.chdir(self.srcdir)
        utils.make()


    def try_load_mod(self, module):
        """
        Try to load a (non-crypto) module using the crypto UAPI
        @param module: name of the kernel module to try to load
        """
        if utils.module_is_loaded(module):
            utils.unload_module(module)

        path = os.path.join(self.srcdir, 'crypto_load_mod ')
        utils.system(path + module)

        if utils.module_is_loaded(module):
            utils.unload_module(module)
            raise error.TestFail('Able to load module "%s" using crypto UAPI' %
                                 module)


    def do_ifalg_digest(self, name, data, outlen):
        """
        Use ctypes to run a digest through one of the available kernel ifalg
        digest types

        @param name: digest name
        @param data: string data to digest
        @param outlen: length of the digest output (e.g., SHA1 is 160-bit, so
                outlen==20)
        @param return string containing the output digest, or None if
                experiencing an error
        """
        libc = ctypes.CDLL("libc.so.6", use_errno=True)

        # If you don't specify the function parameters this way, ctypes may try
        # to treat pointers as 32-bit (and then later sign-extend them)
        libc.socket.argtypes = [ ctypes.c_int, ctypes.c_int, ctypes.c_int ]
        libc.bind.argtypes = [ ctypes.c_int, ctypes.c_void_p, ctypes.c_int ]
        libc.send.argtypes = [ ctypes.c_int, ctypes.c_void_p, ctypes.c_size_t,
                               ctypes.c_int ]
        libc.recv.argtypes = [ ctypes.c_int, ctypes.c_void_p, ctypes.c_size_t,
                               ctypes.c_int ]

        sock = libc.socket(AF_ALG, SOCK_SEQPACKET, 0)
        if sock == -1:
            libc.perror("socket")
            return None

        alg = sockaddr_alg(AF_ALG, "hash", name)
        if libc.bind(sock, ctypes.addressof(alg), ctypes.sizeof(alg)) == -1:
            libc.perror("bind")
            return None

        fd = libc.accept(sock, None, 0)
        if fd == -1:
            libc.perror("accept")
            return None

        message = ctypes.create_string_buffer(data, len(data))
        if libc.send(fd, ctypes.addressof(message), ctypes.sizeof(message), 0) == -1:
            libc.perror("send")
            return None

        out = (ctypes.c_uint8 * outlen)()
        ret = libc.recv(fd, ctypes.addressof(out), ctypes.sizeof(out), 0)
        if ret == -1:
            libc.perror("recv")
            return None

        h = ctypes.string_at(ctypes.addressof(out), ret)

        libc.close(sock)

        return h


    def test_digest(self, name, lib, data):
        """
        Run a digest through both the kernel UAPI and through hashlib, throwing
        an error if the two don't match

        @param name: name of the digest (according to AF_ALG)
        @param lib: a hashlib digest object
        @param data: data to digest
        """

        logging.info("Testing digest %s", name)

        h1 = self.do_ifalg_digest(name, data, lib.digestsize)
        if h1 is None:
            raise error.TestFail("ifalg digest %s failed", name)

        lib.update(data)
        h2 = lib.digest()

        if h1 != h2:
            logging.error("%s: digests do not match", name)
            logging.error(" hash 1: %s", binascii.hexlify(h1))
            logging.error(" hash 2: %s", binascii.hexlify(h2))
            raise error.TestFail("digest mismatch (%s)" % name)

        logging.debug("hash 1: %s", binascii.hexlify(h1))
        logging.debug("hash 2: %s", binascii.hexlify(h2))


    def test_digests(self, data):
        """
        Test several digests, using both the kernel crypto APIs and python
        hashlib

        @param data: the data to digest
        """

        digests = [
                ( "sha1", hashlib.sha1()),
                ( "md5", hashlib.md5()),
                ( "sha512", hashlib.sha512()),
        ]

        for (name, lib) in digests:
            self.test_digest(name, lib, data)


    def test_is_valid(self):
        """
        Check if this test is worth running, based on whether the kernel
        .config has the right features
        """
        config = kernel_config.KernelConfig()
        config.initialize()
        config.is_enabled('CRYPTO_USER_API_HASH')
        config.is_enabled('CRYPTO_USER_API')
        return len(config.failures()) == 0


    def run_once(self):
        # crypto tests only work with AF_ALG support
        if not self.test_is_valid():
            raise error.TestNAError("Crypto tests only run with AF_ALG support")

        module = "test_module"
        self.try_load_mod(module)

        self.test_digests("This is a not-so-secret message")
