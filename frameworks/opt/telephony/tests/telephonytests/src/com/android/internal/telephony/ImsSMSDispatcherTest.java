/*
 * Copyright (C) 2016 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package com.android.internal.telephony;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertNull;
import static org.junit.Assert.assertTrue;
import static org.mockito.Matchers.any;
import static org.mockito.Matchers.anyString;
import static org.mockito.Matchers.isNull;
import static org.mockito.Mockito.anyInt;
import static org.mockito.Mockito.doReturn;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.eq;
import static org.mockito.Mockito.times;

import android.os.HandlerThread;
import android.os.Message;
import android.test.suitebuilder.annotation.SmallTest;

import static com.android.internal.telephony.TelephonyTestUtils.waitForMs;

import org.junit.After;
import org.junit.Before;
import org.junit.Test;
import org.mockito.ArgumentCaptor;
import org.mockito.Mock;

public class ImsSMSDispatcherTest extends TelephonyTest {
    @Mock
    private SMSDispatcher.SmsTracker mTracker;

    private ImsSMSDispatcher mImsSmsDispatcher;

    private class ImsSmsDispatcherTestHandler extends HandlerThread {

        private ImsSmsDispatcherTestHandler(String name) {
            super(name);
        }

        @Override
        public void onLooperPrepared() {
            mImsSmsDispatcher = new ImsSMSDispatcher(mPhone, mSmsStorageMonitor,
                    mSmsUsageMonitor);
            //Initial state of RIL is power on, need to wait util RADIO_ON msg get handled
            waitForMs(200);
            setReady(true);
        }
    }

    @Before
    public void setUp() throws Exception {
        super.setUp(getClass().getSimpleName());
        setupMockPackagePermissionChecks();

        new ImsSmsDispatcherTestHandler(getClass().getSimpleName()).start();
        waitUntilReady();
    }

    @After
    public void tearDown() throws Exception {
        mImsSmsDispatcher = null;
        super.tearDown();
    }

    @Test @SmallTest
    public void testSmsHandleStateUpdate() throws Exception {
        assertEquals(SmsConstants.FORMAT_UNKNOWN, mImsSmsDispatcher.getImsSmsFormat());
        //Mock ImsNetWorkStateChange with GSM phone type
        switchImsSmsFormat(PhoneConstants.PHONE_TYPE_GSM);
        assertEquals(SmsConstants.FORMAT_3GPP, mImsSmsDispatcher.getImsSmsFormat());
        assertTrue(mImsSmsDispatcher.isIms());

        //Mock ImsNetWorkStateChange with Cdma Phone type
        switchImsSmsFormat(PhoneConstants.PHONE_TYPE_CDMA);
        assertEquals(SmsConstants.FORMAT_3GPP2, mImsSmsDispatcher.getImsSmsFormat());
        assertTrue(mImsSmsDispatcher.isIms());
    }

    @Test @SmallTest
    public void testSendImsGmsTest() throws Exception {
        switchImsSmsFormat(PhoneConstants.PHONE_TYPE_GSM);
        mImsSmsDispatcher.sendText("111"/* desAddr*/, "222" /*scAddr*/, TAG,
                null, null, null, null, false);
        verify(mSimulatedCommandsVerifier).sendImsGsmSms(eq("038122f2"),
                eq("0100038111f1000014c9f67cda9c12d37378983e4697e5d4f29c0e"), eq(0), eq(0),
                any(Message.class));
    }

    @Test @SmallTest
    public void testSendImsGmsTestWithOutDesAddr() throws Exception {
        switchImsSmsFormat(PhoneConstants.PHONE_TYPE_GSM);
        mImsSmsDispatcher.sendText(null, "222" /*scAddr*/, TAG,
                null, null, null, null, false);
        verify(mSimulatedCommandsVerifier, times(0)).sendImsGsmSms(anyString(), anyString(),
                anyInt(), anyInt(), any(Message.class));
    }

    @Test @SmallTest
    public void testSendImsCdmaTest() throws Exception {
        switchImsSmsFormat(PhoneConstants.PHONE_TYPE_CDMA);
        mImsSmsDispatcher.sendText("111"/* desAddr*/, "222" /*scAddr*/, TAG,
                null, null, null, null, false);
        verify(mSimulatedCommandsVerifier).sendImsCdmaSms((byte[])any(), eq(0), eq(0),
                any(Message.class));
    }

    @Test @SmallTest
    public void testSendRetrySmsCdmaTest() throws Exception {
        // newFormat will be based on voice technology
        ArgumentCaptor<byte[]> captor = ArgumentCaptor.forClass(byte[].class);
        switchImsSmsFormat(PhoneConstants.PHONE_TYPE_CDMA);
        replaceInstance(SMSDispatcher.SmsTracker.class, "mFormat", mTracker,
                SmsConstants.FORMAT_3GPP2);
        doReturn(PhoneConstants.PHONE_TYPE_CDMA).when(mPhone).getPhoneType();
        mImsSmsDispatcher.sendRetrySms(mTracker);
        verify(mSimulatedCommandsVerifier).sendImsCdmaSms(captor.capture(), eq(0), eq(0),
                any(Message.class));
        assertEquals(1, captor.getAllValues().size());
        assertNull(captor.getAllValues().get(0));
    }

    @Test @SmallTest
    public void testSendRetrySmsGsmTest() throws Exception {
        // newFormat will be based on voice technology will be GSM if phone type is not CDMA
        switchImsSmsFormat(PhoneConstants.PHONE_TYPE_GSM);
        replaceInstance(SMSDispatcher.SmsTracker.class, "mFormat", mTracker,
                SmsConstants.FORMAT_3GPP);
        mImsSmsDispatcher.sendRetrySms(mTracker);
        verify(mSimulatedCommandsVerifier).sendImsGsmSms((String)isNull(), (String)isNull(), eq(0),
                eq(0), any(Message.class));
    }

    private void switchImsSmsFormat(int phoneType) {
        mSimulatedCommands.setImsRegistrationState(new int[]{1, phoneType});
        mSimulatedCommands.notifyImsNetworkStateChanged();
        /* wait for async msg get handled */
        waitForMs(200);
    }
}

