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

package com.android.internal.telephony.gsm;

import android.content.Intent;
import android.os.HandlerThread;
import android.provider.Telephony;
import android.test.suitebuilder.annotation.SmallTest;

import com.android.internal.telephony.SmsStorageMonitor;
import com.android.internal.telephony.TelephonyTest;
import com.android.internal.telephony.TelephonyTestUtils;

import org.junit.After;
import org.junit.Before;
import org.junit.Test;
import org.mockito.ArgumentCaptor;
import org.mockito.Mock;

import static org.junit.Assert.assertTrue;
import static org.mockito.Mockito.anyBoolean;
import static org.mockito.Mockito.anyInt;
import static org.mockito.Mockito.doReturn;
import static org.mockito.Mockito.verify;

public class GsmCellBroadcastHandlerTest extends TelephonyTest {
    @Mock
    private SmsStorageMonitor mSmsStorageMonitor;
    @Mock
    private android.telephony.SmsMessage mSmsMessage;
    @Mock
    private SmsMessage mGsmSmsMessage;

    private GsmCellBroadcastHandler mGsmCellBroadcastHandler;


    private class GsmCellBroadcastHandlerTestHandler extends HandlerThread {

        private GsmCellBroadcastHandlerTestHandler(String name) {
            super(name);
        }

        @Override
        public void onLooperPrepared() {
            mGsmCellBroadcastHandler = GsmCellBroadcastHandler.makeGsmCellBroadcastHandler(
                    mContextFixture.getTestDouble(), mPhone);
            setReady(true);
        }
    }

    @Before
    public void setUp() throws Exception {

        super.setUp(getClass().getSimpleName());

        doReturn(true).when(mTelephonyManager).getSmsReceiveCapableForPhone(anyInt(), anyBoolean());
        doReturn(true).when(mSmsStorageMonitor).isStorageAvailable();

        new GsmCellBroadcastHandlerTestHandler(getClass().getSimpleName()).start();
        waitUntilReady();
    }

    @After
    public void tearDown() throws Exception {
        mGsmCellBroadcastHandler = null;
        super.tearDown();
    }

    @Test @SmallTest
    public void testBroadcastSms() {
        mSimulatedCommands.notifyGsmBroadcastSms(new byte[] {
                (byte)0xc0, //geographical scope
                (byte)0x01, //serial number
                (byte)0x01, //serial number
                (byte)0x01, //message identifier
                (byte)0x01, //message identifier
                (byte)0x01
        });
        TelephonyTestUtils.waitForMs(50);
        ArgumentCaptor<Intent> intentArgumentCaptor = ArgumentCaptor.forClass(Intent.class);
        verify(mContextFixture.getTestDouble()).sendBroadcast(intentArgumentCaptor.capture());
        assertTrue(intentArgumentCaptor.getValue().getAction().equals(
                Telephony.Sms.Intents.SMS_EMERGENCY_CB_RECEIVED_ACTION) ||
                intentArgumentCaptor.getValue().getAction().equals(
                        Telephony.Sms.Intents.SMS_CB_RECEIVED_ACTION));
    }
}
