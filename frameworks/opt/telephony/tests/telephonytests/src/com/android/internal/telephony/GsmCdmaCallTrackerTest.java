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

import android.os.HandlerThread;
import android.os.SystemProperties;
import android.telephony.DisconnectCause;
import android.telephony.PhoneNumberUtils;
import android.telephony.ServiceState;
import android.test.suitebuilder.annotation.SmallTest;

import com.android.internal.telephony.test.SimulatedCommandsVerifier;
import android.os.Message;
import org.junit.After;
import org.junit.Assert;
import org.junit.Before;
import org.junit.Test;
import org.mockito.Mock;
import org.mockito.ArgumentCaptor;
import android.os.Handler;

import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertNotNull;
import static org.junit.Assert.assertTrue;
import static org.mockito.Mockito.doReturn;
import static org.junit.Assert.assertEquals;
import static org.mockito.Mockito.*;
import static com.android.internal.telephony.TelephonyTestUtils.waitForMs;


public class GsmCdmaCallTrackerTest extends TelephonyTest {
    private static final int VOICE_CALL_STARTED_EVENT = 0;
    private static final int VOICE_CALL_ENDED_EVENT = 1;
    private String mDialString = PhoneNumberUtils.stripSeparators("+17005554141");
    /* Handler class initiated at the HandlerThread */
    private GsmCdmaCallTracker mCTUT;
    @Mock
    GsmCdmaCall mCall;
    @Mock
    private Handler mHandler;

    private class GsmCdmaCTHandlerThread extends HandlerThread {

        private GsmCdmaCTHandlerThread(String name) {
            super(name);
        }
        @Override
        public void onLooperPrepared() {
            mCTUT = new GsmCdmaCallTracker(mPhone);
            setReady(true);
        }
    }

    @Before
    public void setUp() throws Exception {
        super.setUp(this.getClass().getSimpleName());
        mSimulatedCommands.setRadioPower(true, null);
        mPhone.mCi = this.mSimulatedCommands;
        mContextFixture.putStringArrayResource(com.android.internal.R.array.dial_string_replace,
                new String[]{});

        new GsmCdmaCTHandlerThread(TAG).start();

        waitUntilReady();
        logd("GsmCdmaCallTracker initiated, waiting for Power on");
        /* Make sure radio state is power on before dial.
         * When radio state changed from off to on, CallTracker
         * will poll result from RIL. Avoid dialing triggered at the same*/
        waitForMs(100);
    }

    @After
    public void tearDown() throws Exception {
        mCTUT = null;
        super.tearDown();
    }

    @Test
    @SmallTest
    public void testMOCallDial() {
        doReturn(ServiceState.STATE_IN_SERVICE).when(mServiceState).getState();
        assertEquals(PhoneConstants.State.IDLE, mCTUT.getState());
        assertEquals(GsmCdmaCall.State.IDLE, mCTUT.mForegroundCall.getState());
        assertEquals(GsmCdmaCall.State.IDLE, mCTUT.mBackgroundCall.getState());
        assertEquals(0, mCTUT.mForegroundCall.getConnections().size());
        try {
            mCTUT.dial(mDialString);
        } catch(Exception ex) {
            ex.printStackTrace();
            Assert.fail("unexpected exception thrown"+ex.getMessage()+ex.getStackTrace());
        }

        assertEquals(PhoneConstants.State.OFFHOOK, mCTUT.getState());
        assertEquals(GsmCdmaCall.State.DIALING, mCTUT.mForegroundCall.getState());
        assertEquals(1, mCTUT.mForegroundCall.getConnections().size());
        /* verify the command is sent out to RIL */
        verify(mSimulatedCommandsVerifier).dial(eq(PhoneNumberUtils.
                        extractNetworkPortionAlt(mDialString)), anyInt(),
                eq((UUSInfo) null),
                isA(Message.class));
    }

    @Test
    @SmallTest
    public void testMOCallPickUp() {
        testMOCallDial();
        logd("Waiting for POLL CALL response from RIL");
        TelephonyTestUtils.waitForMs(50);
        logd("Pick Up MO call, expecting call state change event ");
        mSimulatedCommands.progressConnectingToActive();
        waitForMs(100);
        assertEquals(GsmCdmaCall.State.ACTIVE, mCTUT.mForegroundCall.getState());
        assertEquals(GsmCdmaCall.State.IDLE, mCTUT.mBackgroundCall.getState());
    }

    @Test
    @SmallTest
    public void testMOCallHangup() {
        testMOCallDial();
        logd("Waiting for POLL CALL response from RIL ");
        TelephonyTestUtils.waitForMs(50);
        assertEquals(GsmCdmaCall.State.DIALING, mCTUT.mForegroundCall.getState());
        assertEquals(PhoneConstants.State.OFFHOOK, mCTUT.getState());
        assertEquals(1, mCTUT.mForegroundCall.getConnections().size());
        logd("Hang up MO call after MO call established ");
        try {
            mCTUT.hangup(mCTUT.mForegroundCall);
        } catch(Exception ex) {
            ex.printStackTrace();
            Assert.fail("unexpected exception thrown" + ex.getMessage());
        }
        waitForMs(200);
        assertEquals(GsmCdmaCall.State.IDLE, mCTUT.mForegroundCall.getState());
        assertEquals(0, mCTUT.mForegroundCall.getConnections().size());
        assertEquals(PhoneConstants.State.IDLE, mCTUT.getState());
    }

    @Test
    @SmallTest
    public void testMOCallDialPickUpHangup() {
        testMOCallPickUp();
        assertEquals(GsmCdmaCall.State.ACTIVE, mCTUT.mForegroundCall.getState());
        assertEquals(PhoneConstants.State.OFFHOOK, mCTUT.getState());
        assertEquals(1, mCTUT.mForegroundCall.getConnections().size());
         /* get the reference of the connection before reject */
        Connection mConnection = mCTUT.mForegroundCall.getConnections().get(0);
        assertEquals(DisconnectCause.NOT_DISCONNECTED, mConnection.getDisconnectCause());
        logd("hang up MO call after pickup");
        try {
            mCTUT.hangup(mCTUT.mForegroundCall);
        } catch(Exception ex) {
            ex.printStackTrace();
            Assert.fail("unexpected exception thrown" + ex.getMessage());
        }
        /* request send to RIL still in disconnecting state */
        waitForMs(200);
        assertEquals(GsmCdmaCall.State.IDLE, mCTUT.mForegroundCall.getState());
        assertEquals(0, mCTUT.mForegroundCall.getConnections().size());
        assertEquals(PhoneConstants.State.IDLE, mCTUT.getState());
        assertEquals(DisconnectCause.LOCAL, mConnection.getDisconnectCause());

    }

    @Test
    @SmallTest
    public void testMOCallPendingHangUp() {
        testMOCallDial();
        logd("MO call hangup before established[ getting result from RIL ]");
        /* poll call result from RIL, find that there is a pendingMO call,
         * Didn't do anything for hangup, clear during handle poll result */
        try {
            mCTUT.hangup(mCTUT.mForegroundCall);
        } catch(Exception ex) {
            ex.printStackTrace();
            Assert.fail("unexpected exception thrown" + ex.getMessage());
        }
        waitForMs(200);
        assertEquals(GsmCdmaCall.State.IDLE, mCTUT.mForegroundCall.getState());
        assertEquals(0, mCTUT.mForegroundCall.getConnections().size());
        assertEquals(PhoneConstants.State.IDLE, mCTUT.getState());
    }

    @Test
    @SmallTest
    public void testMOCallSwitch() {
        testMOCallPickUp();
        logd("MO call picked up, initiating a new MO call");
        assertEquals(GsmCdmaCall.State.ACTIVE, mCTUT.mForegroundCall.getState());
        assertEquals(GsmCdmaCall.State.IDLE, mCTUT.mBackgroundCall.getState());
        assertEquals(1, mCTUT.mForegroundCall.getConnections().size());
        assertEquals(0, mCTUT.mBackgroundCall.getConnections().size());

        String mDialString = PhoneNumberUtils.stripSeparators("+17005554142");
        try {
            mCTUT.dial(mDialString);
        } catch(Exception ex) {
            ex.printStackTrace();
            Assert.fail("unexpected exception thrown" + ex.getMessage());
        }
        waitForMs(200);
        assertEquals(GsmCdmaCall.State.DIALING, mCTUT.mForegroundCall.getState());
        assertEquals(GsmCdmaCall.State.HOLDING, mCTUT.mBackgroundCall.getState());
        assertEquals(1, mCTUT.mForegroundCall.getConnections().size());
        assertEquals(1, mCTUT.mBackgroundCall.getConnections().size());

    }

    @Test
    @SmallTest
    public void testMTCallRinging() {
        /* Mock there is a MT call mRinging call and try to accept this MT call */
        /* if we got a active state followed by another MT call-> move to background call */
        assertEquals(PhoneConstants.State.IDLE, mCTUT.getState());
        assertEquals(0, mCTUT.mRingingCall.getConnections().size());
        assertEquals(GsmCdmaCall.State.IDLE, mCTUT.mForegroundCall.getState());
        String mDialString = PhoneNumberUtils.stripSeparators("+17005554141");
        logd("MT call Ringing");
        mSimulatedCommands.triggerRing(mDialString);
        waitForMs(50);
        assertEquals(PhoneConstants.State.RINGING, mCTUT.getState());
        assertEquals(1, mCTUT.mRingingCall.getConnections().size());
    }

    @Test
    @SmallTest
    public void testMTCallAccept() {
        testMTCallRinging();
        assertEquals(mCTUT.mForegroundCall.getConnections().size(),0);
        logd("accept the MT call");
        try{
            mCTUT.acceptCall();
        } catch(Exception ex) {
            ex.printStackTrace();
            Assert.fail("unexpected exception thrown" + ex.getMessage());
        }
        verify(mSimulatedCommandsVerifier).acceptCall(isA(Message.class));
        /* send to the RIL */
        TelephonyTestUtils.waitForMs(50);
        assertEquals(PhoneConstants.State.OFFHOOK, mCTUT.getState());
        assertEquals(GsmCdmaCall.State.ACTIVE, mCTUT.mForegroundCall.getState());
        assertEquals(1, mCTUT.mForegroundCall.getConnections().size());
        assertEquals(0, mCTUT.mRingingCall.getConnections().size());
    }

    @Test
    @SmallTest
    public void testMTCallReject() {
        testMTCallRinging();
        logd("MT call ringing and rejected ");
        /* get the reference of the connection before reject */
        Connection mConnection = mCTUT.mRingingCall.getConnections().get(0);
        assertNotNull(mConnection);
        assertEquals(DisconnectCause.NOT_DISCONNECTED, mConnection.getDisconnectCause());
        try {
            mCTUT.rejectCall();
        } catch(Exception ex) {
            ex.printStackTrace();
            Assert.fail("unexpected exception thrown" + ex.getMessage());
        }
        waitForMs(50);
        assertEquals(PhoneConstants.State.IDLE, mCTUT.getState());
        assertEquals(GsmCdmaCall.State.IDLE, mCTUT.mForegroundCall.getState());
        assertEquals(0, mCTUT.mForegroundCall.getConnections().size());
        /* ? why rejectCall didnt -> hang up locally to set the cause to LOCAL? */
        assertEquals(DisconnectCause.INCOMING_MISSED, mConnection.getDisconnectCause());

    }

    @Test
    @SmallTest
    public void testMOCallSwitchHangupForeGround() {
        testMOCallSwitch();
        logd("Hang up the foreground MO call while dialing ");
        try {
            mCTUT.hangup(mCTUT.mForegroundCall);
        } catch(Exception ex) {
            ex.printStackTrace();
            Assert.fail("unexpected exception thrown" + ex.getMessage());
        }
        waitForMs(200);
        logd(" Foreground Call is IDLE and BackGround Call is still HOLDING ");
        /* if we want to hang up foreground call which is alerting state, hangup all */
        assertEquals(GsmCdmaCall.State.IDLE, mCTUT.mForegroundCall.getState());
        assertEquals(GsmCdmaCall.State.HOLDING, mCTUT.mBackgroundCall.getState());
    }

    @Test
    @SmallTest
    public void testMOCallPickUpHangUpResumeBackGround() {
        testMOCallSwitch();
        logd("Pick up the new MO Call");
        try{
            mSimulatedCommands.progressConnectingToActive();
        } catch(Exception ex) {
            ex.printStackTrace();
            Assert.fail("unexpected exception thrown" + ex.getMessage());
        }

        waitForMs(200);
        assertEquals(GsmCdmaCall.State.ACTIVE, mCTUT.mForegroundCall.getState());
        assertEquals(GsmCdmaCall.State.HOLDING, mCTUT.mBackgroundCall.getState());

        logd("Hang up the new MO Call");
        try {
            mCTUT.hangup(mCTUT.mForegroundCall);
        } catch(Exception ex) {
            ex.printStackTrace();
            Assert.fail("unexpected exception thrown" + ex.getMessage());
        }

        waitForMs(200);
        logd(" BackGround Call switch to ForeGround Call ");
        assertEquals(GsmCdmaCall.State.ACTIVE, mCTUT.mForegroundCall.getState());
        assertEquals(GsmCdmaCall.State.IDLE, mCTUT.mBackgroundCall.getState());
    }

    @Test @SmallTest
    public void testVoiceCallStartListener(){
        logd("register for voice call started event");
        mCTUT.registerForVoiceCallStarted(mHandler, VOICE_CALL_STARTED_EVENT, null);
        logd("voice call started");
        testMOCallPickUp();
        ArgumentCaptor<Message> mCaptorMessage = ArgumentCaptor.forClass(Message.class);
        ArgumentCaptor<Long> mCaptorLong = ArgumentCaptor.forClass(Long.class);
        verify(mHandler,times(1)).sendMessageAtTime(mCaptorMessage.capture(), mCaptorLong.capture());
        assertEquals(VOICE_CALL_STARTED_EVENT, mCaptorMessage.getValue().what);

    }

    @Test @SmallTest
    public void testVoiceCallEndedListener(){
        logd("register for voice call ended event");
        mCTUT.registerForVoiceCallEnded(mHandler, VOICE_CALL_ENDED_EVENT, null);
        ArgumentCaptor<Message> mCaptorMessage = ArgumentCaptor.forClass(Message.class);
        ArgumentCaptor<Long> mCaptorLong = ArgumentCaptor.forClass(Long.class);
        testMOCallHangup();
        verify(mHandler,times(1)).sendMessageAtTime(mCaptorMessage.capture(), mCaptorLong.capture());
        assertEquals(VOICE_CALL_ENDED_EVENT, mCaptorMessage.getValue().what);
    }

    @Test @SmallTest
    public void testUpdatePhoneType() {
        // verify getCurrentCalls is called on init
        verify(mSimulatedCommandsVerifier).getCurrentCalls(any(Message.class));

        // update phone type
        mCTUT.updatePhoneType();

        // verify getCurrentCalls is called on updating phone type
        verify(mSimulatedCommandsVerifier, times(2)).getCurrentCalls(any(Message.class));
    }
}

