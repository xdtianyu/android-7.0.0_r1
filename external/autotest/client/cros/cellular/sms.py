#!/usr/bin/python
# Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import dbus, logging
from autotest_lib.client.common_lib import error

sample = {
    'pdu' :
      '07914140540510F0040B916171056429F500001190804181106904D4F29C0E',
    'parsed' :
      {'text' : 'Test',
       'number' : '+16175046925',
       'timestamp' : '110908141801-04',
       'smsc' : '+14044550010'
       }
    }

sample_multipart = {
    'pdu' :
      ['07912160130320F8440B916171056429F5000011909161037469A0050003920201A9'
       'E5391DF43683E6EF7619C47EBBCF207A194F0789EB74D03D4D47BFEB7450D89D0791'
       'D366737A5C67D3416374581E1ED3CBF23928ED1EB3EBE43219947683E8E832A85D9E'
       'CFC3E7B20B4445A7E72077B94C9E83E86F90B80C7ADBCB72101D5D06B1CBEE331D0D'
       'A2A3E5E539FACD2683CC6F39888E2E83D8EF71980D9ABFCDF47B585E06D1DF',
       '07912160130320F5440B916171056429F50000119091610384691505000392020241'
       'E437888E2E83E670769AEE02'],
    'parsed' :
      {'text' : 'Test of some long text but without any difficult characters'
       ' included in the message. This needs to be over the length threshold'
       ' for the local software to do the split.',
       'number' : '+16175046925',
       'timestamp' : '110919163047-04',
       'smsc' : '+12063130028'
       }
    }


class SmsStore(object):
    '''SMS content management - this maintains an internal model of the
    index->PDU mapping that the fakemodem program should be returning so
    that tests can add and remove individual PDUs and handles generating
    the correct set of responses, including the complete SMS list.
    '''

    def __init__(self, fakemodem):
        self.fakemodem = fakemodem
        self.smsdict = {}
        self.fakemodem.SetResponse('\+CMGR=', '', '+CMS ERROR: 321')
        self.fakemodem.SetResponse('\+CMGD=', '', '+CMS ERROR: 321')
        self._sms_regen_list()

    def sms_insert(self, index, pdu):
        '''Add a SMS to the fake modem's list.'''
        smsc_len = int(pdu[0:1], 16)
        mlen = len(pdu)/2 - smsc_len - 1

        self.fakemodem.RemoveResponse('\+CMGD=')
        self.fakemodem.RemoveResponse('\+CMGR=')
        self.fakemodem.SetResponse('\+CMGD=%d' % (index), '', '')
        self.fakemodem.SetResponse('\+CMGR=%d' % (index),
                                   '+CMGR: 1,,%d\r\n%s' % (mlen, pdu), '')
        self.fakemodem.SetResponse('\+CMGR=', '', '+CMS ERROR: 321')
        self.fakemodem.SetResponse('\+CMGD=', '', '+CMS ERROR: 321')

        self.smsdict[index] = pdu
        self._sms_regen_list()

    def sms_receive(self, index, pdu):
        '''Add a SMS to the fake modem's list, like sms_insert(), and generate
        an unsolicited new-sms message.'''
        self.sms_insert(index, pdu)
        self.fakemodem.SendUnsolicited('+CMTI: "ME",%d'%(index))

    def sms_remove(self, index):
        '''Remove a SMS from the fake modem's list'''
        self.fakemodem.RemoveResponse('\+CMGR=%d' % (index))
        self.fakemodem.RemoveResponse('\+CMGD=%d' % (index))
        del self.smsdict[index]
        self._sms_regen_list()

    def _sms_regen_list(self):
        response = ''
        keys = self.smsdict.keys()
        keys.sort()
        for i in keys:
            pdu = self.smsdict[i]
            smsc_len = int(pdu[0:1],16)
            mlen = len(pdu)/2 - smsc_len - 1
            response = response + '+CMGL: %d,1,,%d\r\n%s\r\n' % (i, mlen, pdu)
        self.fakemodem.SetResponse('\+CMGL=4', response, '')


class SmsTest(object):
    def __init__(self, gsmsms):
        self.gsmsms = gsmsms

    def compare(self, expected, got):
        '''Compare two SMS dictionaries, discounting the index number if
        not specified in the first.'''
        if expected == got:
            return True
        if 'index' in expected:
            return False
        if 'index' not in got:
            return False
        got = dict(got)
        del got['index']
        return expected == got

    def compare_list(self, expected_list, got_list):
        if len(expected_list) != len(got_list):
            return False
        # There must be a more Pythonic way to do this
        for (expected,got) in zip(expected_list, got_list):
            if self.compare(expected, got) == False:
                return False
        return True

    def test_get(self, index, expected):
        try:
            sms = self.gsmsms.Get(index)
        except dbus.DBusException, db:
            if expected is not None:
                raise
            return

        if expected is None:
            logging.info('Got %s' % sms)
            raise error.TestFail('SMS.Get(%d) succeeded unexpectedly' %
                                 index)
        if self.compare(expected, sms) == False:
            logging.info('Got %s, expected %s' % (sms, expected))
            raise error.TestFail('SMS.Get(%d) did not match expected values' %
                                 index)

    def test_delete(self, index, expected_success):
        try:
            self.gsmsms.Delete(index)
            if expected_success == False:
                raise error.TestFail('SMS.Delete(%d) succeeded unexpectedly' %
                                     index)
        except dbus.DBusException, db:
            if expected_success:
                raise

    def test_list(self, expected_list):
        sms_list = self.gsmsms.List()
        if self.compare_list(expected_list, sms_list) == False:
            logging.info('Got %s, expected %s' % (sms_list, expected_list))
            raise error.TestFail('SMS.List() did not match expected values')

    def test_has_none(self):
        '''Test that the SMS interface has no messages.'''
        self.test_list([])
        self.test_get(1, None)
        self.test_delete(1, False)
        self.test_delete(2, False)

    def test_has_one(self, parsed_sms):
        '''Test that the SMS interface has exactly one message at index 1
        As a side effect, deletes the message.'''
        self.test_list([parsed_sms])
        self.test_get(1, parsed_sms)
        self.test_get(2, None)
        self.test_delete(2, False)
        self.test_delete(1, True)
