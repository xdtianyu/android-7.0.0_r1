# Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from autotest_lib.client.common_lib import error
from autotest_lib.client.common_lib.cros.network import xmlrpc_datatypes
from autotest_lib.client.common_lib.cros.network import xmlrpc_security_types
from autotest_lib.server.cros.network import hostap_config
from autotest_lib.server.cros.network import wifi_cell_test_base


class ProfileRemovingContext(object):
    """Creates and pushes a profile that is guaranteed to be removed."""

    @property
    def profile_name(self):
        """@return string: name of profile created and pushed."""
        return self._profile_name


    def __init__(self, wifi_client, profile_name='always_removed'):
        self._wifi_client = wifi_client
        self._profile_name = profile_name


    def __enter__(self):
        if not all([self._wifi_client.shill.create_profile(self.profile_name),
                    self._wifi_client.shill.push_profile(self.profile_name)]):
            raise error.TestFail('Failed to create/push profile %s' %
                                 self.profile_name)
        return self


    def __exit__(self, exc_type, exc_value, traceback):
        # Ignore pop errors in case the test popped it on its own
        self._wifi_client.shill.pop_profile(self.profile_name)
        if not self._wifi_client.shill.remove_profile(self.profile_name):
            raise error.TestFail('Failed to remove profile %s.' %
                                 self.profile_name)


class network_WiFi_ProfileBasic(wifi_cell_test_base.WiFiCellTestBase):
    """Tests that credentials are stored in profiles."""

    version = 1

    CHANNEL_NUMBER = 1
    STATE_TRANSITION_TIMEOUT_SECONDS = 20


    def _assert_state_transition(self, ssid, states):
        """Raise an error if a WiFi service doesn't transition to |states|.

        @param ssid: string ssid of service.
        @param states: list of string states to wait for.

        """
        result = self.context.client.wait_for_service_states(
                ssid, states,
                timeout_seconds=self.STATE_TRANSITION_TIMEOUT_SECONDS)

        success, state, duration_seconds = result
        if not success:
            raise error.TestFail('Timed out waiting for states: %r in %f '
                                 'seconds.  Ended in %s' %
                                 (states, duration_seconds, state))


    def run_once(self):
        """Body of the test."""
        self.context.client.shill.clean_profiles()
        wep_config = xmlrpc_security_types.WEPConfig(
                wep_keys=['abcde', 'fghij', 'klmno', 'pqrst'])
        ap_config0 = hostap_config.HostapConfig(
                channel=self.CHANNEL_NUMBER, security_config=wep_config)
        ap_config1 = hostap_config.HostapConfig(
                channel=self.CHANNEL_NUMBER, security_config=wep_config)
        with ProfileRemovingContext(self.context.client,
                                    profile_name='bottom') as bottom:
            self.context.configure(ap_config0)
            client_config0 = xmlrpc_datatypes.AssociationParameters(
                    security_config=ap_config0.security_config,
                    ssid=self.context.router.get_ssid())
            self.context.assert_connect_wifi(client_config0)
            self.context.assert_ping_from_dut(ap_num=0)
            # Check that popping a profile causes a loss of credentials and a
            # disconnect.
            if not self.context.client.shill.pop_profile(bottom.profile_name):
                raise error.TestFail('Failed to pop profile %s.' %
                                      bottom.profile_name)

            self._assert_state_transition(client_config0.ssid, ['idle'])
            # Check that pushing a profile causes credentials to reappear.
            if not self.context.client.shill.push_profile(bottom.profile_name):
                raise error.TestFail('Failed to push profile %s.' %
                                      bottom.profile_name)

            self._assert_state_transition(client_config0.ssid,
                                          ['ready', 'portal', 'online'])

            # Explicitly disconnect from the AP.
            self.context.client.shill.disconnect(client_config0.ssid)
            self._assert_state_transition(client_config0.ssid, ['idle'])

            with ProfileRemovingContext(self.context.client,
                                        profile_name='top') as top:
                # Changes to the profile stack should clear the "explicitly
                # disconnected" flag on all services.  This should cause shill
                # to re-connect to the AP.
                self._assert_state_transition(client_config0.ssid,
                                              ['ready', 'portal', 'online'])

                self.context.configure(ap_config1, multi_interface=True)
                client_config1 = xmlrpc_datatypes.AssociationParameters(
                        security_config=ap_config1.security_config,
                        ssid=self.context.router.get_ssid(instance=1))
                self.context.assert_connect_wifi(client_config1)
                self.context.assert_ping_from_dut(ap_num=1)
                # Check that deleting an entry also causes a disconnect and
                # autoconect to a previously remembered service.
                if not self.context.client.shill.delete_entries_for_ssid(
                        client_config1.ssid):
                    raise error.TestFail('Failed to delete profile entry for '
                                         '%s' % client_config1.ssid)

                self._assert_state_transition(client_config1.ssid, ['idle'])
                self._assert_state_transition(client_config0.ssid,
                                              ['ready', 'portal', 'online'])
                # Verify that the same sort of thing happens when we pop
                # a profile on top of another one.
                self.context.assert_connect_wifi(client_config1)
                self.context.assert_ping_from_dut(ap_num=1)
                if not self.context.client.shill.pop_profile(top.profile_name):
                    raise error.TestFail('Failed to pop profile %s.' %
                                          top.profile_name)
                self._assert_state_transition(client_config1.ssid, ['idle'])
                self._assert_state_transition(client_config0.ssid,
                                              ['ready', 'portal', 'online'])

                # Re-push the top profile.
                if not self.context.client.shill.push_profile(top.profile_name):
                    raise error.TestFail('Failed to push profile %s.' %
                                          top.profile_name)

                # Explicitly disconnect from the AP.
                self.context.client.shill.disconnect(client_config0.ssid)
                self._assert_state_transition(client_config0.ssid, ['idle'])

                # Verify that popping a profile -- even one which does not
                # affect the service profile -- returns explicitly disconnected
                # services back into the pool of connectable services.
                if not self.context.client.shill.pop_profile(top.profile_name):
                    raise error.TestFail('Failed to pop profile %s.' %
                                          top.profile_name)

                # A change to the profile stack should have caused us to
                # reconnect to the service, since the "explicitly disconnected"
                # flag will be removed.
                self._assert_state_transition(client_config0.ssid,
                                              ['ready', 'portal', 'online'])
