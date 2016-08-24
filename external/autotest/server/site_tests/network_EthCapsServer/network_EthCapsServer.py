# Copyright (c) 2009 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging, os, socket, subprocess, tempfile, threading, time

from autotest_lib.client.common_lib import error
from autotest_lib.server import autotest, hosts, site_host_attributes
from autotest_lib.server import subcommand, test, utils


class WolWake(threading.Thread):
    """Class to allow waking of DUT via Wake-on-LAN capabilities (WOL)."""


    def __init__(self, hostname, mac_addr, sleep_secs):
        """Constructor for waking DUT.

        Args:
          mac_addr: string of mac address tuple
          sleep_secs: seconds to sleep prior to attempting WOL
        """
        threading.Thread.__init__(self)
        self._hostname = hostname
        self._mac_addr = mac_addr
        self._sleep_secs = sleep_secs


    # TODO(tbroch) Borrowed from class ServoTest.  Refactor for code re-use
    def _ping_test(self, hostname, timeout=5):
        """Verify whether a host responds to a ping.

        Args:
          hostname: Hostname to ping.
          timeout: Time in seconds to wait for a response.

        Returns: True if success False otherwise
        """
        with open(os.devnull, 'w') as fnull:
            ping_good = False
            elapsed_time = 0
            while not ping_good and elapsed_time < timeout:
                ping_good = subprocess.call(
                    ['ping', '-c', '1', '-W', str(timeout), hostname],
                    stdout=fnull, stderr=fnull) == 0
                time.sleep(1)
                elapsed_time += 1
            return ping_good


    def _send_wol_magic_packet(self):
        """Perform Wake-on-LAN magic wake.

        WOL magic packet consists of:
          0xff repeated for 6 bytes
          <mac addr> repeated 16 times

        Sent as a broadcast packet.
        """
        mac_tuple = self._mac_addr.split(':')
        assert len(mac_tuple) == 6
        magic = '\xff' * 6
        submagic = ''.join("%c" % int(value, 16) for value in mac_tuple)
        magic += submagic * 16
        assert len(magic) == 102

        sock=socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        sock.setsockopt(socket.SOL_SOCKET, socket.SO_BROADCAST, 1)
        sock.sendto(magic, ('<broadcast>', 7))
        sock.close()
        logging.info("Wake thread sent WOL wakeup")


    def run(self):
        # ping device to make sure its network is off presumably from suspend
        # not another malfunction.
        ping_secs = 0
        while self._ping_test(self._hostname, timeout=2) and \
                ping_secs < self._sleep_secs:
            time.sleep(1)
            ping_secs += 1

        self._send_wol_magic_packet()


class network_EthCapsServer(test.test):
    version = 1

    def _parse_ifconfig(self, filename):
        """Retrieve ifconfig information.

        Raises
          error.TestError if unable to parse mac address
        """
        self._mac_addr = None

        fd = open(filename)
        for ln in fd.readlines():
            info_str = ln.strip()
            logging.debug(ln)
            index = info_str.find('HWaddr ')
            if index != -1:
                self._mac_addr = info_str[index + len('HWaddr '):]
                logging.info("mac addr = %s" % self._mac_addr)
                break
        fd.close()

        if not self._mac_addr:
            raise error.TestError("Unable to find mac addresss")


    def _client_cmd(self, cmd, results=None):
        """Execute a command on the client.

        Args:
          results: string of filename to save results on client.

        Returns:
          string of filename on server side with stdout results of command
        """
        if results:
            client_tmpdir = self._client.get_tmp_dir()
            client_results = os.path.join(client_tmpdir, "%s" % results)
            cmd = "%s > %s 2>&1" % (cmd, client_results)

        logging.info("Client cmd = %s", cmd)
        self._client.run(cmd)

        if results:
            server_tmpfile = tempfile.NamedTemporaryFile(delete=False)
            server_tmpfile.close()
            self._client.get_file(client_results, server_tmpfile.name)
            return server_tmpfile.name

        return None


    def run_once(self, client_ip=None, ethname='eth0'):
        """Run the test.

        Args:
          client_ip: string of client's ip address
          ethname: string of ethernet device under test
        """
        if not client_ip:
            error.TestError("Must provide client's IP address to test")

        sleep_secs = 20

        self._ethname = ethname
        self._client_ip = client_ip
        self._client = hosts.create_host(client_ip)
        client_at = autotest.Autotest(self._client)

        # retrieve ifconfig info for mac address of client
        cmd = "ifconfig %s" % self._ethname
        ifconfig_filename = self._client_cmd(cmd, results="ifconfig.log")
        self._parse_ifconfig(ifconfig_filename)

        # thread to wake the device using WOL
        wol_wake = WolWake(self._client_ip, self._mac_addr, sleep_secs)
        wol_wake.start()

        # create and run client test to prepare and suspend device
        client_at.run_test("network_EthCaps", ethname=ethname,
                           threshold_secs=sleep_secs * 2)

        wol_wake.join()
