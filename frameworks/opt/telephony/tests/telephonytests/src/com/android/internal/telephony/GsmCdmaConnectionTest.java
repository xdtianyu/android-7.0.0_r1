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

import android.os.Handler;
import android.os.Looper;
import android.telephony.PhoneNumberUtils;
import android.test.suitebuilder.annotation.SmallTest;
import android.test.suitebuilder.annotation.MediumTest;
import android.telephony.DisconnectCause;

import static org.junit.Assert.*;
import static org.mockito.Mockito.*;
import org.junit.After;
import org.junit.Before;
import org.junit.Test;
import org.mockito.Mock;
import static com.android.internal.telephony.TelephonyTestUtils.waitForMs;

public class GsmCdmaConnectionTest extends TelephonyTest {

    private GsmCdmaConnection connection;

    @Mock
    DriverCall mDC;

    @Before
    public void setUp() throws Exception {
        super.setUp(getClass().getSimpleName());
        replaceInstance(Handler.class, "mLooper", mCT, Looper.getMainLooper());

        mCT.mForegroundCall = new GsmCdmaCall(mCT);
        mCT.mBackgroundCall = new GsmCdmaCall(mCT);
        mCT.mRingingCall = new GsmCdmaCall(mCT);
    }

    @After
    public void tearDown() throws Exception {
        connection = null;
        super.tearDown();
    }

    @Test @SmallTest
    public void testFormatDialString(){
        connection = new GsmCdmaConnection(mPhone, String.format(
                "+1 (700).555-41NN%c1234", PhoneNumberUtils.PAUSE), mCT, null);
       /* case 1: If PAUSE/WAIT sequence at the end, ignore them */
        String formattedDialStr = connection.formatDialString(
                String.format("+1 (700).555-41NN1234%c", PhoneNumberUtils.PAUSE));
        assertEquals("+1 (700).555-41NN1234", formattedDialStr);

       /*  case 2: If consecutive PAUSE/WAIT sequence in the middle of the string,
        *  and if there is any WAIT in PAUSE/WAIT sequence, treat them like WAIT.*/
        formattedDialStr = connection.formatDialString("+1 (700).555-41NN,;1234");
        assertEquals("+1 (700).555-41NN;1234", formattedDialStr);
    }

    @Test @SmallTest
    public void testSanityGSM() {
        connection = new GsmCdmaConnection(mPhone, String.format(
                "+1 (700).555-41NN%c1234", PhoneNumberUtils.PAUSE), mCT, null);
        logd("Testing initial state of GsmCdmaConnection");
        assertEquals(GsmCdmaCall.State.IDLE, connection.getState());
        assertEquals(Connection.PostDialState.NOT_STARTED, connection.getPostDialState());
        assertEquals(DisconnectCause.NOT_DISCONNECTED, DisconnectCause.NOT_DISCONNECTED);
        assertEquals(0, connection.getDisconnectTime());
        assertEquals(0, connection.getHoldDurationMillis());
        assertEquals(PhoneConstants.PRESENTATION_ALLOWED, connection.getNumberPresentation());
        assertFalse(connection.isMultiparty());
        assertNotNull(connection.getRemainingPostDialString());
        assertEquals("+1 (700).555-41NN,1234", connection.getOrigDialString());
    }

    @Test @SmallTest
    public void testSanityCDMA() {
        doReturn(PhoneConstants.PHONE_TYPE_CDMA).when(mPhone).getPhoneType();
        connection = new GsmCdmaConnection(mPhone, String.format(
                "+1 (700).555-41NN%c1234", PhoneNumberUtils.PAUSE), mCT, null);
        logd("Testing initial state of GsmCdmaConnection");
        assertEquals(GsmCdmaCall.State.IDLE, connection.getState());
        assertEquals(Connection.PostDialState.NOT_STARTED, connection.getPostDialState());
        assertEquals(DisconnectCause.NOT_DISCONNECTED, DisconnectCause.NOT_DISCONNECTED);
        assertEquals(0, connection.getDisconnectTime());
        assertEquals(0, connection.getHoldDurationMillis());
        assertEquals(PhoneConstants.PRESENTATION_ALLOWED, connection.getNumberPresentation());
        assertFalse(connection.isMultiparty());
        assertNotNull(connection.getRemainingPostDialString());
        /* CDMA phone type dont have origDialString */
        assertNull(connection.getOrigDialString());
    }

    @Test @SmallTest
    public void testConnectionStateUpdate() {
        connection = new GsmCdmaConnection(mPhone, String.format(
                "+1 (700).555-41NN%c1234", PhoneNumberUtils.PAUSE), mCT, null);
        logd("Update the connection state from idle to active");
        mDC.state = DriverCall.State.ACTIVE;
        connection.update(mDC);
        assertEquals(GsmCdmaCall.State.ACTIVE, connection.getState());
        logd("update connection state from active to holding");
        mDC.state = DriverCall.State.HOLDING;
        connection.update(mDC);
        assertEquals(GsmCdmaCall.State.HOLDING, connection.getState());
        waitForMs(50);
        assertTrue(connection.getHoldDurationMillis() >= 50);
    }

    @Test @MediumTest
    public void testCDMAPostDialPause() {
        doReturn(PhoneConstants.PHONE_TYPE_CDMA).when(mPhone).getPhoneType();
        connection = new GsmCdmaConnection(mPhone, String.format(
                "+1 (700).555-41NN%c1234", PhoneNumberUtils.PAUSE), mCT, null);
        logd("Mock connection state from alerting to active ");
        mDC.state = DriverCall.State.ALERTING;
        connection.update(mDC);
        mDC.state = DriverCall.State.ACTIVE;
        connection.update(mDC);
        logd("process post dail sequence with pause");
        assertEquals(Connection.PostDialState.PAUSE, connection.getPostDialState());
        /* pause for 2000 ms + 50ms margin */
        waitForMs(GsmCdmaConnection.PAUSE_DELAY_MILLIS_CDMA + 100);
        assertEquals(Connection.PostDialState.COMPLETE, connection.getPostDialState());
    }

    @Test @MediumTest
    public void testGSMPostDialPause() {
        connection = new GsmCdmaConnection(mPhone, String.format(
                "+1 (700).555-41NN%c1234", PhoneNumberUtils.PAUSE), mCT, null);
        logd("Mock connection state from alerting to active ");
        mDC.state = DriverCall.State.ALERTING;
        connection.update(mDC);
        mDC.state = DriverCall.State.ACTIVE;
        connection.update(mDC);
        logd("process post dail sequence with pause");
        assertEquals(Connection.PostDialState.STARTED, connection.getPostDialState());
        /* pause for 2000 ms + 50ms margin */
        waitForMs(GsmCdmaConnection.PAUSE_DELAY_MILLIS_GSM + 100);
        assertEquals(Connection.PostDialState.COMPLETE, connection.getPostDialState());
    }


    @Test @SmallTest
    public void testPostDialWait() {
        doReturn(PhoneConstants.PHONE_TYPE_CDMA).when(mPhone).getPhoneType();
        connection = new GsmCdmaConnection(mPhone,
                String.format("+1 (700).555-41NN%c1234", PhoneNumberUtils.WAIT),mCT,null);
        logd("Mock connection state transition from alerting to active ");
        mDC.state = DriverCall.State.ALERTING;
        connection.update(mDC);
        mDC.state = DriverCall.State.ACTIVE;
        connection.update(mDC);
        logd("Process the post dial sequence with wait ");
        assertEquals(Connection.PostDialState.WAIT, connection.getPostDialState());
        connection.proceedAfterWaitChar();
        waitForMs(50);
        assertEquals(Connection.PostDialState.COMPLETE, connection.getPostDialState());
    }

    @Test @SmallTest
    public void testHangUpConnection() {
        connection = new GsmCdmaConnection(mPhone, String.format(
                "+1 (700).555-41NN%c1234", PhoneNumberUtils.PAUSE), mCT, null);
        mDC.state = DriverCall.State.ACTIVE;
        connection.update(mDC);
        logd("Hangup the connection locally");
        connection.onDisconnect(DisconnectCause.LOCAL);
        assertEquals(GsmCdmaCall.State.DISCONNECTED, connection.getState());
        assertEquals(DisconnectCause.LOCAL, connection.getDisconnectCause());
        assertTrue(connection.getDisconnectTime() <= System.currentTimeMillis());
    }
}
