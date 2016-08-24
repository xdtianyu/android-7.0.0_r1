#!/usr/bin/python3.4
#
#   Copyright 2016 - The Android Open Source Project
#
#   Licensed under the Apache License, Version 2.0 (the "License");
#   you may not use this file except in compliance with the License.
#   You may obtain a copy of the License at
#
#       http://www.apache.org/licenses/LICENSE-2.0
#
#   Unless required by applicable law or agreed to in writing, software
#   distributed under the License is distributed on an "AS IS" BASIS,
#   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#   See the License for the specific language governing permissions and
#   limitations under the License.

import queue

import acts.base_test as base_test
import acts.controllers.android_device as android_device
import acts.test_utils.wifi.wifi_test_utils as wutils

from acts import asserts

ON_IDENTITY_CHANGED = "WifiNanOnIdentityChanged"
ON_MATCH = "WifiNanSessionOnMatch"
ON_MESSAGE_RX = "WifiNanSessionOnMessageReceived"
ON_MESSAGE_TX_FAIL = "WifiNanSessionOnMessageSendFail"
ON_MESSAGE_TX_OK = "WifiNanSessionOnMessageSendSuccess"

class WifiNanManagerTest(base_test.BaseTestClass):
    msg_id = 10

    def __init__(self, controllers):
        base_test.BaseTestClass.__init__(self, controllers)
        self.publisher = self.android_devices[0]
        self.subscriber = self.android_devices[1]
        self.tests = (
            "test_nan_base_test",
            )

    def setup_class(self):
        required_params = (
            "config_request1",
            "config_request2",
            "publish_data",
            "publish_settings",
            "subscribe_data",
            "subscribe_settings"
        )
        self.unpack_userparams(required_params)

    def setup_test(self):
        assert wutils.wifi_toggle_state(self.publisher, True)
        assert wutils.wifi_toggle_state(self.subscriber, True)

    def teardown_class(self):
        assert wutils.wifi_toggle_state(self.publisher, False)
        assert wutils.wifi_toggle_state(self.subscriber, False)

    def reliable_tx(self, device, peer, msg):
        num_tries = 0
        max_num_tries = 10
        events_regex = '%s|%s' % (ON_MESSAGE_TX_FAIL, ON_MESSAGE_TX_OK)
        self.msg_id = self.msg_id + 1

        while True:
            try:
                num_tries += 1
                device.droid.wifiNanSendMessage(peer, msg, self.msg_id)
                events = device.ed.pop_events(events_regex, 30)
                for event in events:
                    self.log.info('%s: %s' % (event['name'], event['data']))
                    if event['data']['messageId'] != self.msg_id:
                        continue
                    if event['name'] == ON_MESSAGE_TX_OK:
                        return True
                    if num_tries == max_num_tries:
                        self.log.info("Max number of retries reached")
                        return False
            except queue.Empty:
                self.log.info('Timed out while waiting for %s' % events_regex)
                return False

    def test_nan_base_test(self):
        """Perform NAN configuration, discovery, and message exchange.

        Configuration: 2 devices, one acting as Publisher (P) and the
        other as Subscriber (S)

        Logical steps:
          * P & S configure NAN
          * P & S wait for NAN configuration confirmation
          * P starts publishing
          * S starts subscribing
          * S waits for a match (discovery) notification
          * S sends a message to P, confirming that sent successfully
          * P waits for a message and confirms that received (uncorrupted)
          * P sends a message to S, confirming that sent successfully
          * S waits for a message and confirms that received (uncorrupted)
        """
        self.publisher.droid.wifiNanEnable(self.config_request1)
        self.subscriber.droid.wifiNanEnable(self.config_request2)

        sub2pub_msg = "How are you doing?"
        pub2sub_msg = "Doing ok - thanks!"

        try:
            event = self.publisher.ed.pop_event(ON_IDENTITY_CHANGED, 30)
            self.log.info('%s: %s' % (ON_IDENTITY_CHANGED, event['data']))
        except queue.Empty:
            asserts.fail('Timed out while waiting for %s on Publisher' %
                      ON_IDENTITY_CHANGED)
        self.log.debug(event)

        try:
            event = self.subscriber.ed.pop_event(ON_IDENTITY_CHANGED, 30)
            self.log.info('%s: %s' % (ON_IDENTITY_CHANGED, event['data']))
        except queue.Empty:
            asserts.fail('Timed out while waiting for %s on Subscriber' %
                      ON_IDENTITY_CHANGED)
        self.log.debug(event)

        self.publisher.droid.wifiNanPublish(self.publish_data,
                                            self.publish_settings, 0)
        self.subscriber.droid.wifiNanSubscribe(self.subscribe_data,
                                               self.subscribe_settings, 0)

        try:
            event = self.subscriber.ed.pop_event(ON_MATCH, 30)
            self.log.info('%s: %s' % (ON_MATCH, event['data']))
        except queue.Empty:
            asserts.fail('Timed out while waiting for %s on Subscriber' % ON_MATCH)
        self.log.debug(event)

        asserts.assert_true(self.reliable_tx(self.subscriber,
                                          event['data']['peerId'],
                                          sub2pub_msg),
                         "Failed to transmit from subscriber")

        try:
            event = self.publisher.ed.pop_event(ON_MESSAGE_RX, 10)
            self.log.info('%s: %s' % (ON_MESSAGE_RX, event['data']))
            asserts.assert_true(event['data']['messageAsString'] == sub2pub_msg,
                             "Subscriber -> publisher message corrupted")
        except queue.Empty:
            asserts.fail('Timed out while waiting for %s on publisher' %
                      ON_MESSAGE_RX)

        asserts.assert_true(self.reliable_tx(self.publisher,
                                          event['data']['peerId'],
                                          pub2sub_msg),
                         "Failed to transmit from publisher")

        try:
            event = self.subscriber.ed.pop_event(ON_MESSAGE_RX, 10)
            self.log.info('%s: %s' % (ON_MESSAGE_RX, event['data']))
            asserts.assert_true(event['data']['messageAsString'] == pub2sub_msg,
                             "Publisher -> subscriber message corrupted")
        except queue.Empty:
            asserts.fail('Timed out while waiting for %s on subscriber' %
                      ON_MESSAGE_RX)
