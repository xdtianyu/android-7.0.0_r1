# Copyright 2014 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import dpkt
import logging
import time

from autotest_lib.client.bin import test
from autotest_lib.client.common_lib import error
from autotest_lib.client.common_lib.cros.tendo import peerd_config
from autotest_lib.client.cros import chrooted_avahi
from autotest_lib.client.cros.netprotos import interface_host
from autotest_lib.client.cros.netprotos import zeroconf
from autotest_lib.client.cros.tendo import peerd_dbus_helper


class peerd_HandlesNameConflicts(test.test):
    """Test that peerd can handle mDNS name conflicts."""
    version = 1

    CACHE_REFRESH_PERIOD_SECONDS = 5
    FAKE_HOST_HOSTNAME = 'a-test-host'
    TEST_TIMEOUT_SECONDS = 30
    TEST_SERVICE_ID = 'test-service-0'
    TEST_SERVICE_INFO = {'some_data': 'a value',
                          'other_data': 'another value'}
    INITIAL_MDNS_PREFIX = 'initial_mdns_prefix'
    SERBUS_SERVICE_ID = 'serbus'
    SERBUS_SERVICE_NAME = '_serbus'
    SERBUS_PROTOCOL = '_tcp'
    SERBUS_PORT = 0


    def reset_peerd(self):
        """Start up a peerd instance.

        This instance will have really verbose logging and will attempt
        to use a known MDNS prefix to start out.

        """
        self._peerd = peerd_dbus_helper.make_helper(
                peerd_config.PeerdConfig(verbosity_level=3,
                                         mdns_prefix=self.INITIAL_MDNS_PREFIX))


    def initialize(self):
        # Make sure these are initiallized to None in case we throw
        # during self.initialize().
        self._chrooted_avahi = None
        self._peerd = None
        self._host = None
        self._zc_listener = None
        self._chrooted_avahi = chrooted_avahi.ChrootedAvahi()
        self._chrooted_avahi.start()
        self.reset_peerd()
        # Listen on our half of the interface pair for mDNS advertisements.
        self._host = interface_host.InterfaceHost(
                self._chrooted_avahi.unchrooted_interface_name)
        self._zc_listener = zeroconf.ZeroconfDaemon(self._host,
                                                    self.FAKE_HOST_HOSTNAME)
        # The queries for hostname/dns_domain are IPCs and therefor relatively
        # expensive.  Do them just once.
        hostname = self._chrooted_avahi.hostname
        dns_domain = self._chrooted_avahi.dns_domain
        if not hostname or not dns_domain:
            raise error.TestFail('Failed to get hostname/domain from avahi.')
        self._dns_domain = dns_domain
        self._hostname = '%s.%s' % (hostname, dns_domain)
        self._last_cache_refresh_seconds = 0


    def cleanup(self):
        for obj in (self._chrooted_avahi,
                    self._host,
                    self._peerd):
            if obj is not None:
                obj.close()


    def _get_PTR_prefix(self, service_id):
        ptr_name = '_%s._tcp.%s' % (service_id, self._dns_domain)
        found_records = self._zc_listener.cached_results(
                ptr_name, dpkt.dns.DNS_PTR)
        if len(found_records) == 0:
            logging.debug('Found no PTR records for %s', ptr_name)
            return None
        if len(found_records) > 1:
            logging.debug('Found multiple PTR records for %s', ptr_name)
            return None
        unique_name = found_records[0].data
        expected_suffix = '.' + ptr_name
        if not unique_name.endswith(expected_suffix):
            logging.error('PTR record for "%s" points to odd name: "%s"',
                          ptr_name, unique_name)
            return None
        ptr_prefix = unique_name[0:-len(expected_suffix)]
        logging.debug('PTR record for "%s" points to service with name "%s"',
                      ptr_name, ptr_prefix)
        return ptr_prefix


    def _found_expected_PTR_records(self, forbidden_record_prefix):
        for service_id in (self.SERBUS_SERVICE_ID, self.TEST_SERVICE_ID):
            prefix = self._get_PTR_prefix(service_id)
            if prefix is None:
                break
            if prefix == forbidden_record_prefix:
                logging.debug('Ignoring service with conflicting prefix')
                break
        else:
            return True
        delta_seconds = time.time() - self._last_cache_refresh_seconds
        if delta_seconds > self.CACHE_REFRESH_PERIOD_SECONDS:
            self._zc_listener.clear_cache()
            self._last_cache_refresh_seconds = time.time()
        return False


    def run_once(self):
        # Tell peerd about this exciting new service we have.
        self._peerd.expose_service(self.TEST_SERVICE_ID, self.TEST_SERVICE_INFO)
        # Wait for advertisements of that service to appear from avahi.
        # They should be prefixed with our special name, since there are no
        # conflicts.
        logging.info('Waiting to receive mDNS advertisements of '
                     'peerd services.')
        success, duration = self._host.run_until(
                lambda: self._found_expected_PTR_records(None),
                self.TEST_TIMEOUT_SECONDS)
        if not success:
            raise error.TestFail('Did not receive mDNS advertisements in time.')
        actual_prefix = self._get_PTR_prefix(self.SERBUS_SERVICE_ID)
        if actual_prefix != self.INITIAL_MDNS_PREFIX:
            raise error.TestFail('Expected initial mDNS advertisements to have '
                                 'a prefix=%s' % self.INITIAL_MDNS_PREFIX)
        logging.info('Found initial records advertised by peerd.')
        # Now register services with the same name, and restart peerd.
        # 1) The old instance of peerd should withdraw its services from Avahi
        # 2) The new instance of peerd should re-register services with Avahi
        # 3) Avahi should notice that the name.service_type.domain tuple
        #    conflicts with existing records, and signal this to peerd.
        # 4) Peerd should pick a new prefix and try again.
        self.reset_peerd()
        self._zc_listener.register_service(
                self.INITIAL_MDNS_PREFIX,
                self.SERBUS_SERVICE_NAME,
                self.SERBUS_PROTOCOL,
                self.SERBUS_PORT,
                ['invalid=record'])
        self._peerd.expose_service(self.TEST_SERVICE_ID, self.TEST_SERVICE_INFO)
        run_until_predicate = lambda: self._found_expected_PTR_records(
                self.INITIAL_MDNS_PREFIX)
        success, duration = self._host.run_until(run_until_predicate,
                                                 self.TEST_TIMEOUT_SECONDS)
        if not success:
            raise error.TestFail('Timed out waiting for peerd to change the '
                                 'record prefix.')
