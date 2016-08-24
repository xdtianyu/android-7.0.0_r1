/*
 * Copyright (C) 2015 The Android Open Source Project
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
 * limitations under the License
 */

package com.android.server.telecom.tests;

import static org.mockito.Matchers.any;
import static org.mockito.Matchers.anyInt;
import static org.mockito.Matchers.anyString;
import static org.mockito.Matchers.eq;
import static org.mockito.Matchers.isNull;
import static org.mockito.Mockito.never;
import static org.mockito.Mockito.timeout;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.verifyZeroInteractions;
import static org.mockito.Mockito.when;

import android.content.ContentResolver;
import android.content.Context;
import android.content.IContentProvider;
import android.media.AudioManager;
import android.net.Uri;
import android.os.Bundle;
import android.os.Handler;
import android.os.Looper;
import android.os.Process;
import android.provider.BlockedNumberContract;
import android.telecom.Call;
import android.telecom.CallAudioState;
import android.telecom.Connection;
import android.telecom.ConnectionRequest;
import android.telecom.DisconnectCause;
import android.telecom.ParcelableCall;
import android.telecom.ParcelableCallAnalytics;
import android.telecom.PhoneAccount;
import android.telecom.PhoneAccountHandle;
import android.telecom.TelecomManager;
import android.telecom.VideoProfile;
import android.test.suitebuilder.annotation.LargeTest;
import android.test.suitebuilder.annotation.MediumTest;
import android.test.suitebuilder.annotation.SmallTest;

import com.android.internal.telecom.IInCallAdapter;
import com.android.internal.telephony.CallerInfo;
import com.android.internal.util.IndentingPrintWriter;
import com.android.server.telecom.Analytics;
import com.android.server.telecom.Log;

import com.google.common.base.Predicate;

import org.mockito.invocation.InvocationOnMock;
import org.mockito.stubbing.Answer;

import java.io.StringWriter;
import java.util.List;
import java.util.Map;
import java.util.concurrent.BrokenBarrierException;
import java.util.concurrent.CountDownLatch;
import java.util.concurrent.CyclicBarrier;
import java.util.concurrent.TimeUnit;

import org.mockito.ArgumentCaptor;

import static org.mockito.Matchers.any;
import static org.mockito.Matchers.anyInt;
import static org.mockito.Matchers.eq;
import static org.mockito.Mockito.never;
import static org.mockito.Mockito.timeout;
import static org.mockito.Mockito.verify;

/**
 * Performs various basic call tests in Telecom.
 */
public class BasicCallTests extends TelecomSystemTest {
    private static final String TEST_BUNDLE_KEY = "android.telecom.extra.TEST";
    private static final String TEST_EVENT = "android.telecom.event.TEST";

    @LargeTest
    public void testSingleOutgoingCallLocalDisconnect() throws Exception {
        IdPair ids = startAndMakeActiveOutgoingCall("650-555-1212",
                mPhoneAccountA0.getAccountHandle(), mConnectionServiceFixtureA);

        mInCallServiceFixtureX.mInCallAdapter.disconnectCall(ids.mCallId);
        assertEquals(Call.STATE_ACTIVE, mInCallServiceFixtureX.getCall(ids.mCallId).getState());
        assertEquals(Call.STATE_ACTIVE, mInCallServiceFixtureY.getCall(ids.mCallId).getState());

        mConnectionServiceFixtureA.sendSetDisconnected(ids.mConnectionId, DisconnectCause.LOCAL);
        assertEquals(Call.STATE_DISCONNECTED,
                mInCallServiceFixtureX.getCall(ids.mCallId).getState());
        assertEquals(Call.STATE_DISCONNECTED,
                mInCallServiceFixtureY.getCall(ids.mCallId).getState());
        verifyNoBlockChecks();
    }

    @LargeTest
    public void testSingleOutgoingCallRemoteDisconnect() throws Exception {
        IdPair ids = startAndMakeActiveOutgoingCall("650-555-1212",
                mPhoneAccountA0.getAccountHandle(), mConnectionServiceFixtureA);

        mConnectionServiceFixtureA.sendSetDisconnected(ids.mConnectionId, DisconnectCause.LOCAL);
        assertEquals(Call.STATE_DISCONNECTED,
                mInCallServiceFixtureX.getCall(ids.mCallId).getState());
        assertEquals(Call.STATE_DISCONNECTED,
                mInCallServiceFixtureY.getCall(ids.mCallId).getState());
        verifyNoBlockChecks();
    }

    /**
     * Tests the {@link TelecomManager#acceptRingingCall()} API.  Tests simple case of an incoming
     * audio-only call.
     *
     * @throws Exception
     */
    @LargeTest
    public void testTelecomManagerAcceptRingingCall() throws Exception {
        IdPair ids = startIncomingPhoneCall("650-555-1212", mPhoneAccountA0.getAccountHandle(),
                mConnectionServiceFixtureA);

        assertEquals(Call.STATE_RINGING, mInCallServiceFixtureX.getCall(ids.mCallId).getState());
        assertEquals(Call.STATE_RINGING, mInCallServiceFixtureY.getCall(ids.mCallId).getState());

        // Use TelecomManager API to answer the ringing call.
        TelecomManager telecomManager = (TelecomManager) mComponentContextFixture.getTestDouble()
                .getApplicationContext().getSystemService(Context.TELECOM_SERVICE);
        telecomManager.acceptRingingCall();

        verify(mConnectionServiceFixtureA.getTestDouble(), timeout(TEST_TIMEOUT))
                .answer(ids.mCallId);
        mConnectionServiceFixtureA.sendSetActive(ids.mConnectionId);

        mInCallServiceFixtureX.mInCallAdapter.disconnectCall(ids.mCallId);
    }

    /**
     * Tests the {@link TelecomManager#acceptRingingCall()} API.  Tests simple case of an incoming
     * video call, which should be answered as video.
     *
     * @throws Exception
     */
    @LargeTest
    public void testTelecomManagerAcceptRingingVideoCall() throws Exception {
        IdPair ids = startIncomingPhoneCall("650-555-1212", mPhoneAccountA0.getAccountHandle(),
                VideoProfile.STATE_BIDIRECTIONAL, mConnectionServiceFixtureA);

        assertEquals(Call.STATE_RINGING, mInCallServiceFixtureX.getCall(ids.mCallId).getState());
        assertEquals(Call.STATE_RINGING, mInCallServiceFixtureY.getCall(ids.mCallId).getState());

        // Use TelecomManager API to answer the ringing call; the default expected behavior is to
        // answer using whatever video state the ringing call requests.
        TelecomManager telecomManager = (TelecomManager) mComponentContextFixture.getTestDouble()
                .getApplicationContext().getSystemService(Context.TELECOM_SERVICE);
        telecomManager.acceptRingingCall();

        // Answer video API should be called
        verify(mConnectionServiceFixtureA.getTestDouble(), timeout(TEST_TIMEOUT))
                .answerVideo(eq(ids.mCallId), eq(VideoProfile.STATE_BIDIRECTIONAL));
        mConnectionServiceFixtureA.sendSetActive(ids.mConnectionId);

        mInCallServiceFixtureX.mInCallAdapter.disconnectCall(ids.mCallId);
    }

    /**
     * Tests the {@link TelecomManager#acceptRingingCall(int)} API.  Tests answering a video call
     * as an audio call.
     *
     * @throws Exception
     */
    @LargeTest
    public void testTelecomManagerAcceptRingingVideoCallAsAudio() throws Exception {
        IdPair ids = startIncomingPhoneCall("650-555-1212", mPhoneAccountA0.getAccountHandle(),
                VideoProfile.STATE_BIDIRECTIONAL, mConnectionServiceFixtureA);

        assertEquals(Call.STATE_RINGING, mInCallServiceFixtureX.getCall(ids.mCallId).getState());
        assertEquals(Call.STATE_RINGING, mInCallServiceFixtureY.getCall(ids.mCallId).getState());

        // Use TelecomManager API to answer the ringing call.
        TelecomManager telecomManager = (TelecomManager) mComponentContextFixture.getTestDouble()
                .getApplicationContext().getSystemService(Context.TELECOM_SERVICE);
        telecomManager.acceptRingingCall(VideoProfile.STATE_AUDIO_ONLY);

        // The generic answer method on the ConnectionService is used to answer audio-only calls.
        verify(mConnectionServiceFixtureA.getTestDouble(), timeout(TEST_TIMEOUT))
                .answer(eq(ids.mCallId));
        mConnectionServiceFixtureA.sendSetActive(ids.mConnectionId);

        mInCallServiceFixtureX.mInCallAdapter.disconnectCall(ids.mCallId);
    }

    /**
     * Tests the {@link TelecomManager#acceptRingingCall()} API.  Tests simple case of an incoming
     * video call, where an attempt is made to answer with an invalid video state.
     *
     * @throws Exception
     */
    @LargeTest
    public void testTelecomManagerAcceptRingingInvalidVideoState() throws Exception {
        IdPair ids = startIncomingPhoneCall("650-555-1212", mPhoneAccountA0.getAccountHandle(),
                VideoProfile.STATE_BIDIRECTIONAL, mConnectionServiceFixtureA);

        assertEquals(Call.STATE_RINGING, mInCallServiceFixtureX.getCall(ids.mCallId).getState());
        assertEquals(Call.STATE_RINGING, mInCallServiceFixtureY.getCall(ids.mCallId).getState());

        // Use TelecomManager API to answer the ringing call; the default expected behavior is to
        // answer using whatever video state the ringing call requests.
        TelecomManager telecomManager = (TelecomManager) mComponentContextFixture.getTestDouble()
                .getApplicationContext().getSystemService(Context.TELECOM_SERVICE);
        telecomManager.acceptRingingCall(999 /* invalid videostate */);

        // Answer video API should be called
        verify(mConnectionServiceFixtureA.getTestDouble(), timeout(TEST_TIMEOUT))
                .answerVideo(eq(ids.mCallId), eq(VideoProfile.STATE_BIDIRECTIONAL));
        mConnectionServiceFixtureA.sendSetActive(ids.mConnectionId);
        mInCallServiceFixtureX.mInCallAdapter.disconnectCall(ids.mCallId);
    }

    @LargeTest
    public void testSingleIncomingCallLocalDisconnect() throws Exception {
        IdPair ids = startAndMakeActiveIncomingCall("650-555-1212",
                mPhoneAccountA0.getAccountHandle(), mConnectionServiceFixtureA);
        mInCallServiceFixtureX.mInCallAdapter.disconnectCall(ids.mCallId);
        assertEquals(Call.STATE_ACTIVE, mInCallServiceFixtureX.getCall(ids.mCallId).getState());
        assertEquals(Call.STATE_ACTIVE, mInCallServiceFixtureY.getCall(ids.mCallId).getState());

        mConnectionServiceFixtureA.sendSetDisconnected(ids.mConnectionId, DisconnectCause.LOCAL);
        assertEquals(Call.STATE_DISCONNECTED,
                mInCallServiceFixtureX.getCall(ids.mCallId).getState());
        assertEquals(Call.STATE_DISCONNECTED,
                mInCallServiceFixtureY.getCall(ids.mCallId).getState());
    }

    @LargeTest
    public void testSingleIncomingCallRemoteDisconnect() throws Exception {
        IdPair ids = startAndMakeActiveIncomingCall("650-555-1212",
                mPhoneAccountA0.getAccountHandle(), mConnectionServiceFixtureA);

        mConnectionServiceFixtureA.sendSetDisconnected(ids.mConnectionId, DisconnectCause.LOCAL);
        assertEquals(Call.STATE_DISCONNECTED,
                mInCallServiceFixtureX.getCall(ids.mCallId).getState());
        assertEquals(Call.STATE_DISCONNECTED,
                mInCallServiceFixtureY.getCall(ids.mCallId).getState());
    }

    @LargeTest
    public void testOutgoingCallAndSelectPhoneAccount() throws Exception {
        // Remove default PhoneAccount so that the Call moves into the correct
        // SELECT_PHONE_ACCOUNT state.
        mTelecomSystem.getPhoneAccountRegistrar().setUserSelectedOutgoingPhoneAccount(
                null, Process.myUserHandle());
        int startingNumConnections = mConnectionServiceFixtureA.mConnectionById.size();
        int startingNumCalls = mInCallServiceFixtureX.mCallById.size();
        String callId = startOutgoingPhoneCallWithNoPhoneAccount("650-555-1212",
                mConnectionServiceFixtureA);
        assertEquals(Call.STATE_SELECT_PHONE_ACCOUNT,
                mInCallServiceFixtureX.getCall(callId).getState());
        assertEquals(Call.STATE_SELECT_PHONE_ACCOUNT,
                mInCallServiceFixtureY.getCall(callId).getState());
        mInCallServiceFixtureX.mInCallAdapter.phoneAccountSelected(callId,
                mPhoneAccountA0.getAccountHandle(), false);

        IdPair ids = outgoingCallPhoneAccountSelected(mPhoneAccountA0.getAccountHandle(),
                startingNumConnections, startingNumCalls, mConnectionServiceFixtureA);

        mConnectionServiceFixtureA.sendSetDisconnected(ids.mConnectionId, DisconnectCause.LOCAL);
        assertEquals(Call.STATE_DISCONNECTED,
                mInCallServiceFixtureX.getCall(ids.mCallId).getState());
        assertEquals(Call.STATE_DISCONNECTED,
                mInCallServiceFixtureY.getCall(ids.mCallId).getState());
    }

    @LargeTest
    public void testIncomingCallFromContactWithSendToVoicemailIsRejected() throws Exception {
        Bundle extras = new Bundle();
        extras.putParcelable(
                TelecomManager.EXTRA_INCOMING_CALL_ADDRESS,
                Uri.fromParts(PhoneAccount.SCHEME_TEL, "650-555-1212", null));
        mTelecomSystem.getTelecomServiceImpl().getBinder()
                .addNewIncomingCall(mPhoneAccountA0.getAccountHandle(), extras);

        waitForHandlerAction(new Handler(Looper.getMainLooper()), TEST_TIMEOUT);
        verify(mConnectionServiceFixtureA.getTestDouble())
                .createConnection(any(PhoneAccountHandle.class), anyString(),
                        any(ConnectionRequest.class), eq(true), eq(false));

        assertEquals(1, mCallerInfoAsyncQueryFactoryFixture.mRequests.size());
        for (CallerInfoAsyncQueryFactoryFixture.Request request :
                mCallerInfoAsyncQueryFactoryFixture.mRequests) {
            CallerInfo sendToVoicemailCallerInfo = new CallerInfo();
            sendToVoicemailCallerInfo.shouldSendToVoicemail = true;
            request.replyWithCallerInfo(sendToVoicemailCallerInfo);
        }

        assertTrueWithTimeout(new Predicate<Void>() {
            @Override
            public boolean apply(Void aVoid) {
                return mConnectionServiceFixtureA.mConnectionService.rejectedCallIds.size() == 1;
            }
        });
        assertTrueWithTimeout(new Predicate<Void>() {
            @Override
            public boolean apply(Void aVoid) {
                return mMissedCallNotifier.missedCallsNotified.size() == 1;
            }
        });

        verify(mInCallServiceFixtureX.getTestDouble(), never())
                .setInCallAdapter(any(IInCallAdapter.class));
        verify(mInCallServiceFixtureY.getTestDouble(), never())
                .setInCallAdapter(any(IInCallAdapter.class));
    }

    @LargeTest
    public void testIncomingCallCallerInfoLookupTimesOutIsAllowed() throws Exception {
        Bundle extras = new Bundle();
        extras.putParcelable(
                TelecomManager.EXTRA_INCOMING_CALL_ADDRESS,
                Uri.fromParts(PhoneAccount.SCHEME_TEL, "650-555-1212", null));
        mTelecomSystem.getTelecomServiceImpl().getBinder()
                .addNewIncomingCall(mPhoneAccountA0.getAccountHandle(), extras);

        verify(mConnectionServiceFixtureA.getTestDouble())
                .createConnection(any(PhoneAccountHandle.class), anyString(),
                        any(ConnectionRequest.class), eq(true), eq(false));

        // Never reply to the caller info lookup.
        assertEquals(1, mCallerInfoAsyncQueryFactoryFixture.mRequests.size());

        verify(mInCallServiceFixtureX.getTestDouble(), timeout(TEST_TIMEOUT))
                .setInCallAdapter(any(IInCallAdapter.class));
        verify(mInCallServiceFixtureY.getTestDouble(), timeout(TEST_TIMEOUT))
                .setInCallAdapter(any(IInCallAdapter.class));

        assertEquals(0, mConnectionServiceFixtureA.mConnectionService.rejectedCallIds.size());
        assertEquals(0, mMissedCallNotifier.missedCallsNotified.size());

        assertTrueWithTimeout(new Predicate<Void>() {
            @Override
            public boolean apply(Void v) {
                return mInCallServiceFixtureX.mInCallAdapter != null;
            }
        });

        verify(mInCallServiceFixtureX.getTestDouble(), timeout(TEST_TIMEOUT))
                .addCall(any(ParcelableCall.class));
        verify(mInCallServiceFixtureY.getTestDouble(), timeout(TEST_TIMEOUT))
                .addCall(any(ParcelableCall.class));

        disconnectCall(mInCallServiceFixtureX.mLatestCallId,
                mConnectionServiceFixtureA.mLatestConnectionId);
    }

    @LargeTest
    public void testIncomingCallFromBlockedNumberIsRejected() throws Exception {
        String phoneNumber = "650-555-1212";
        blockNumber(phoneNumber);

        Bundle extras = new Bundle();
        extras.putParcelable(
                TelecomManager.EXTRA_INCOMING_CALL_ADDRESS,
                Uri.fromParts(PhoneAccount.SCHEME_TEL, phoneNumber, null));
        mTelecomSystem.getTelecomServiceImpl().getBinder()
                .addNewIncomingCall(mPhoneAccountA0.getAccountHandle(), extras);

        verify(mConnectionServiceFixtureA.getTestDouble())
                .createConnection(any(PhoneAccountHandle.class), anyString(),
                        any(ConnectionRequest.class), eq(true), eq(false));

        assertEquals(1, mCallerInfoAsyncQueryFactoryFixture.mRequests.size());
        for (CallerInfoAsyncQueryFactoryFixture.Request request :
                mCallerInfoAsyncQueryFactoryFixture.mRequests) {
            request.reply();
        }

        assertTrueWithTimeout(new Predicate<Void>() {
            @Override
            public boolean apply(Void aVoid) {
                return mConnectionServiceFixtureA.mConnectionService.rejectedCallIds.size() == 1;
            }
        });
        assertEquals(0, mMissedCallNotifier.missedCallsNotified.size());

        verify(mInCallServiceFixtureX.getTestDouble(), never())
                .setInCallAdapter(any(IInCallAdapter.class));
        verify(mInCallServiceFixtureY.getTestDouble(), never())
                .setInCallAdapter(any(IInCallAdapter.class));
    }

    @LargeTest
    public void testIncomingCallBlockCheckTimesoutIsAllowed() throws Exception {
        final CountDownLatch latch = new CountDownLatch(1);
        String phoneNumber = "650-555-1212";
        blockNumberWithAnswer(phoneNumber, new Answer<Bundle>() {
            @Override
            public Bundle answer(InvocationOnMock invocation) throws Throwable {
                latch.await(TEST_TIMEOUT * 2, TimeUnit.MILLISECONDS);
                Bundle bundle = new Bundle();
                bundle.putBoolean(BlockedNumberContract.RES_NUMBER_IS_BLOCKED, true);
                return bundle;
            }
        });

        IdPair ids = startAndMakeActiveIncomingCall(
                phoneNumber, mPhoneAccountA0.getAccountHandle(), mConnectionServiceFixtureA);
        latch.countDown();

        assertEquals(0, mConnectionServiceFixtureA.mConnectionService.rejectedCallIds.size());
        assertEquals(0, mMissedCallNotifier.missedCallsNotified.size());
        disconnectCall(ids.mCallId, ids.mConnectionId);
    }

    public void do_testDeadlockOnOutgoingCall() throws Exception {
        final IdPair ids = startOutgoingPhoneCall("650-555-1212",
                mPhoneAccountA0.getAccountHandle(), mConnectionServiceFixtureA,
                Process.myUserHandle());
        rapidFire(
                new Runnable() {
                    @Override
                    public void run() {
                        while (mCallerInfoAsyncQueryFactoryFixture.mRequests.size() > 0) {
                            mCallerInfoAsyncQueryFactoryFixture.mRequests.remove(0).reply();
                        }
                    }
                },
                new Runnable() {
                    @Override
                    public void run() {
                        try {
                            mConnectionServiceFixtureA.sendSetActive(ids.mConnectionId);
                        } catch (Exception e) {
                            Log.e(this, e, "");
                        }
                    }
                });
    }

    @MediumTest
    public void testDeadlockOnOutgoingCall() throws Exception {
        for (int i = 0; i < 100; i++) {
            BasicCallTests test = new BasicCallTests();
            test.setContext(getContext());
            test.setTestContext(getTestContext());
            test.setName(getName());
            test.setUp();
            test.do_testDeadlockOnOutgoingCall();
            test.tearDown();
        }
    }

    @LargeTest
    public void testIncomingThenOutgoingCalls() throws Exception {
        // TODO: We have to use the same PhoneAccount for both; see http://b/18461539
        IdPair incoming = startAndMakeActiveIncomingCall("650-555-2323",
                mPhoneAccountA0.getAccountHandle(), mConnectionServiceFixtureA);
        IdPair outgoing = startAndMakeActiveOutgoingCall("650-555-1212",
                mPhoneAccountA0.getAccountHandle(), mConnectionServiceFixtureA);

        mInCallServiceFixtureX.mInCallAdapter.disconnectCall(incoming.mCallId);
        mInCallServiceFixtureX.mInCallAdapter.disconnectCall(outgoing.mCallId);
    }

    @LargeTest
    public void testOutgoingThenIncomingCalls() throws Exception {
        // TODO: We have to use the same PhoneAccount for both; see http://b/18461539
        IdPair outgoing = startAndMakeActiveOutgoingCall("650-555-1212",
                mPhoneAccountA0.getAccountHandle(), mConnectionServiceFixtureA);
        IdPair incoming = startAndMakeActiveIncomingCall("650-555-2323",
                mPhoneAccountA0.getAccountHandle(), mConnectionServiceFixtureA);
        verify(mConnectionServiceFixtureA.getTestDouble())
                .hold(outgoing.mConnectionId);
        mConnectionServiceFixtureA.mConnectionById.get(outgoing.mConnectionId).state =
                Connection.STATE_HOLDING;
        mConnectionServiceFixtureA.sendSetOnHold(outgoing.mConnectionId);
        assertEquals(Call.STATE_HOLDING,
                mInCallServiceFixtureX.getCall(outgoing.mCallId).getState());
        assertEquals(Call.STATE_HOLDING,
                mInCallServiceFixtureY.getCall(outgoing.mCallId).getState());

        mInCallServiceFixtureX.mInCallAdapter.disconnectCall(incoming.mCallId);
        mInCallServiceFixtureX.mInCallAdapter.disconnectCall(outgoing.mCallId);
    }

    @LargeTest
    public void testAudioManagerOperations() throws Exception {
        AudioManager audioManager = (AudioManager) mComponentContextFixture.getTestDouble()
                .getApplicationContext().getSystemService(Context.AUDIO_SERVICE);

        IdPair outgoing = startAndMakeActiveOutgoingCall("650-555-1212",
                mPhoneAccountA0.getAccountHandle(), mConnectionServiceFixtureA);

        verify(audioManager, timeout(TEST_TIMEOUT)).requestAudioFocusForCall(anyInt(), anyInt());
        verify(audioManager, timeout(TEST_TIMEOUT).atLeastOnce())
                .setMode(AudioManager.MODE_IN_CALL);

        mInCallServiceFixtureX.mInCallAdapter.mute(true);
        verify(mAudioService, timeout(TEST_TIMEOUT))
                .setMicrophoneMute(eq(true), any(String.class), any(Integer.class));
        mInCallServiceFixtureX.mInCallAdapter.mute(false);
        verify(mAudioService, timeout(TEST_TIMEOUT))
                .setMicrophoneMute(eq(false), any(String.class), any(Integer.class));

        mInCallServiceFixtureX.mInCallAdapter.setAudioRoute(CallAudioState.ROUTE_SPEAKER);
        verify(audioManager, timeout(TEST_TIMEOUT))
                .setSpeakerphoneOn(true);
        mInCallServiceFixtureX.mInCallAdapter.setAudioRoute(CallAudioState.ROUTE_EARPIECE);
        verify(audioManager, timeout(TEST_TIMEOUT))
                .setSpeakerphoneOn(false);

        mConnectionServiceFixtureA.
                sendSetDisconnected(outgoing.mConnectionId, DisconnectCause.REMOTE);

        verify(audioManager, timeout(TEST_TIMEOUT))
                .abandonAudioFocusForCall();
        verify(audioManager, timeout(TEST_TIMEOUT).atLeastOnce())
                .setMode(AudioManager.MODE_NORMAL);
    }

    private void rapidFire(Runnable... tasks) {
        final CyclicBarrier barrier = new CyclicBarrier(tasks.length);
        final CountDownLatch latch = new CountDownLatch(tasks.length);
        for (int i = 0; i < tasks.length; i++) {
            final Runnable task = tasks[i];
            new Thread(new Runnable() {
                @Override
                public void run() {
                    try {
                        barrier.await();
                        task.run();
                    } catch (InterruptedException | BrokenBarrierException e){
                        Log.e(BasicCallTests.this, e, "Unexpectedly interrupted");
                    } finally {
                        latch.countDown();
                    }
                }
            }).start();
        }
        try {
            latch.await();
        } catch (InterruptedException e) {
            Log.e(BasicCallTests.this, e, "Unexpectedly interrupted");
        }
    }

    @MediumTest
    public void testBasicConferenceCall() throws Exception {
        makeConferenceCall();
    }

    @MediumTest
    public void testAddCallToConference1() throws Exception {
        ParcelableCall conferenceCall = makeConferenceCall();
        IdPair callId3 = startAndMakeActiveOutgoingCall("650-555-1214",
                mPhoneAccountA0.getAccountHandle(), mConnectionServiceFixtureA);
        // testAddCallToConference{1,2} differ in the order of arguments to InCallAdapter#conference
        mInCallServiceFixtureX.getInCallAdapter().conference(
                conferenceCall.getId(), callId3.mCallId);
        Thread.sleep(200);

        ParcelableCall call3 = mInCallServiceFixtureX.getCall(callId3.mCallId);
        ParcelableCall updatedConference = mInCallServiceFixtureX.getCall(conferenceCall.getId());
        assertEquals(conferenceCall.getId(), call3.getParentCallId());
        assertEquals(3, updatedConference.getChildCallIds().size());
        assertTrue(updatedConference.getChildCallIds().contains(callId3.mCallId));
    }

    @MediumTest
    public void testAddCallToConference2() throws Exception {
        ParcelableCall conferenceCall = makeConferenceCall();
        IdPair callId3 = startAndMakeActiveOutgoingCall("650-555-1214",
                mPhoneAccountA0.getAccountHandle(), mConnectionServiceFixtureA);
        mInCallServiceFixtureX.getInCallAdapter()
                .conference(callId3.mCallId, conferenceCall.getId());
        Thread.sleep(200);

        ParcelableCall call3 = mInCallServiceFixtureX.getCall(callId3.mCallId);
        ParcelableCall updatedConference = mInCallServiceFixtureX.getCall(conferenceCall.getId());
        assertEquals(conferenceCall.getId(), call3.getParentCallId());
        assertEquals(3, updatedConference.getChildCallIds().size());
        assertTrue(updatedConference.getChildCallIds().contains(callId3.mCallId));
    }

    /**
     * Tests the {@link Call#pullExternalCall()} API.  Verifies that if a call is not an external
     * call, no pull call request is made to the connection service.
     *
     * @throws Exception
     */
    @MediumTest
    public void testPullNonExternalCall() throws Exception {
        // TODO: Revisit this unit test once telecom support for filtering external calls from
        // InCall services is implemented.
        IdPair ids = startAndMakeActiveIncomingCall("650-555-1212",
                mPhoneAccountA0.getAccountHandle(), mConnectionServiceFixtureA);
        assertEquals(Call.STATE_ACTIVE, mInCallServiceFixtureX.getCall(ids.mCallId).getState());

        // Attempt to pull the call and verify the API call makes it through
        mInCallServiceFixtureX.mInCallAdapter.pullExternalCall(ids.mCallId);
        verify(mConnectionServiceFixtureA.getTestDouble(), timeout(TEST_TIMEOUT).never())
                .pullExternalCall(ids.mCallId);
    }

    /**
     * Tests the {@link Connection#sendConnectionEvent(String)} API.
     *
     * @throws Exception
     */
    @MediumTest
    public void testSendConnectionEventNull() throws Exception {
        IdPair ids = startAndMakeActiveIncomingCall("650-555-1212",
                mPhoneAccountA0.getAccountHandle(), mConnectionServiceFixtureA);
        assertEquals(Call.STATE_ACTIVE, mInCallServiceFixtureX.getCall(ids.mCallId).getState());
        mConnectionServiceFixtureA.sendConnectionEvent(ids.mConnectionId, TEST_EVENT, null);
        verify(mInCallServiceFixtureX.getTestDouble(), timeout(TEST_TIMEOUT))
                .onConnectionEvent(ids.mCallId, TEST_EVENT, null);
    }

    /**
     * Tests the {@link Connection#sendConnectionEvent(String)} API.
     *
     * @throws Exception
     */
    @MediumTest
    public void testSendConnectionEventNotNull() throws Exception {
        IdPair ids = startAndMakeActiveIncomingCall("650-555-1212",
                mPhoneAccountA0.getAccountHandle(), mConnectionServiceFixtureA);
        assertEquals(Call.STATE_ACTIVE, mInCallServiceFixtureX.getCall(ids.mCallId).getState());

        Bundle testBundle = new Bundle();
        testBundle.putString(TEST_BUNDLE_KEY, "TEST");

        ArgumentCaptor<Bundle> bundleArgumentCaptor = ArgumentCaptor.forClass(Bundle.class);
        mConnectionServiceFixtureA.sendConnectionEvent(ids.mConnectionId, TEST_EVENT, testBundle);
        verify(mInCallServiceFixtureX.getTestDouble(), timeout(TEST_TIMEOUT))
                .onConnectionEvent(eq(ids.mCallId), eq(TEST_EVENT), bundleArgumentCaptor.capture());
        assert (bundleArgumentCaptor.getValue().containsKey(TEST_BUNDLE_KEY));
    }

    /**
     * Tests the {@link Call#sendCallEvent(String, Bundle)} API.
     *
     * @throws Exception
     */
    @MediumTest
    public void testSendCallEventNull() throws Exception {
        IdPair ids = startAndMakeActiveIncomingCall("650-555-1212",
                mPhoneAccountA0.getAccountHandle(), mConnectionServiceFixtureA);
        assertEquals(Call.STATE_ACTIVE, mInCallServiceFixtureX.getCall(ids.mCallId).getState());

        mInCallServiceFixtureX.mInCallAdapter.sendCallEvent(ids.mCallId, TEST_EVENT, null);
        verify(mConnectionServiceFixtureA.getTestDouble(), timeout(TEST_TIMEOUT))
                .sendCallEvent(ids.mCallId, TEST_EVENT, null);
    }

    /**
     * Tests the {@link Call#sendCallEvent(String, Bundle)} API.
     *
     * @throws Exception
     */
    @MediumTest
    public void testSendCallEventNonNull() throws Exception {
        IdPair ids = startAndMakeActiveIncomingCall("650-555-1212",
                mPhoneAccountA0.getAccountHandle(), mConnectionServiceFixtureA);
        assertEquals(Call.STATE_ACTIVE, mInCallServiceFixtureX.getCall(ids.mCallId).getState());

        Bundle testBundle = new Bundle();
        testBundle.putString(TEST_BUNDLE_KEY, "TEST");

        ArgumentCaptor<Bundle> bundleArgumentCaptor = ArgumentCaptor.forClass(Bundle.class);
        mInCallServiceFixtureX.mInCallAdapter.sendCallEvent(ids.mCallId, TEST_EVENT,
                testBundle);
        verify(mConnectionServiceFixtureA.getTestDouble(), timeout(TEST_TIMEOUT))
                .sendCallEvent(eq(ids.mCallId), eq(TEST_EVENT),
                        bundleArgumentCaptor.capture());
        assert (bundleArgumentCaptor.getValue().containsKey(TEST_BUNDLE_KEY));
    }

    @MediumTest
    public void testAnalyticsSingleCall() throws Exception {
        IdPair testCall = startAndMakeActiveIncomingCall(
                "650-555-1212",
                mPhoneAccountA0.getAccountHandle(),
                mConnectionServiceFixtureA);
        Map<String, Analytics.CallInfoImpl> analyticsMap = Analytics.cloneData();

        assertTrue(analyticsMap.containsKey(testCall.mCallId));

        Analytics.CallInfoImpl callAnalytics = analyticsMap.get(testCall.mCallId);
        assertTrue(callAnalytics.startTime > 0);
        assertEquals(0, callAnalytics.endTime);
        assertEquals(Analytics.INCOMING_DIRECTION, callAnalytics.callDirection);
        assertFalse(callAnalytics.isInterrupted);
        assertNull(callAnalytics.callTerminationReason);
        assertEquals(mConnectionServiceComponentNameA.flattenToShortString(),
                callAnalytics.connectionService);

        mConnectionServiceFixtureA.
                sendSetDisconnected(testCall.mConnectionId, DisconnectCause.ERROR);

        analyticsMap = Analytics.cloneData();
        callAnalytics = analyticsMap.get(testCall.mCallId);
        assertTrue(callAnalytics.endTime > 0);
        assertNotNull(callAnalytics.callTerminationReason);
        assertEquals(DisconnectCause.ERROR, callAnalytics.callTerminationReason.getCode());

        StringWriter sr = new StringWriter();
        IndentingPrintWriter ip = new IndentingPrintWriter(sr, "    ");
        Analytics.dump(ip);
        String dumpResult = sr.toString();
        String[] expectedFields = {"startTime", "endTime", "direction", "isAdditionalCall",
                "isInterrupted", "callTechnologies", "callTerminationReason", "connectionService"};
        for (String field : expectedFields) {
            assertTrue(dumpResult.contains(field));
        }
    }

    @SmallTest
    public void testAnalyticsDumping() throws Exception {
        Analytics.reset();
        IdPair testCall = startAndMakeActiveIncomingCall(
                "650-555-1212",
                mPhoneAccountA0.getAccountHandle(),
                mConnectionServiceFixtureA);

        mConnectionServiceFixtureA.
                sendSetDisconnected(testCall.mConnectionId, DisconnectCause.ERROR);
        Analytics.CallInfoImpl expectedAnalytics = Analytics.cloneData().get(testCall.mCallId);

        TelecomManager tm = (TelecomManager) mSpyContext.getSystemService(Context.TELECOM_SERVICE);
        List<ParcelableCallAnalytics> analyticsList = tm.dumpAnalytics();

        assertEquals(1, analyticsList.size());
        ParcelableCallAnalytics pCA = analyticsList.get(0);

        assertTrue(Math.abs(expectedAnalytics.startTime - pCA.getStartTimeMillis()) <
                ParcelableCallAnalytics.MILLIS_IN_5_MINUTES);
        assertEquals(0, pCA.getStartTimeMillis() % ParcelableCallAnalytics.MILLIS_IN_5_MINUTES);
        assertTrue(Math.abs((expectedAnalytics.endTime - expectedAnalytics.startTime) -
                pCA.getCallDurationMillis()) < ParcelableCallAnalytics.MILLIS_IN_1_SECOND);
        assertEquals(0, pCA.getCallDurationMillis() % ParcelableCallAnalytics.MILLIS_IN_1_SECOND);

        assertEquals(expectedAnalytics.callDirection, pCA.getCallType());
        assertEquals(expectedAnalytics.isAdditionalCall, pCA.isAdditionalCall());
        assertEquals(expectedAnalytics.isInterrupted, pCA.isInterrupted());
        assertEquals(expectedAnalytics.callTechnologies, pCA.getCallTechnologies());
        assertEquals(expectedAnalytics.callTerminationReason.getCode(),
                pCA.getCallTerminationCode());
        assertEquals(expectedAnalytics.connectionService, pCA.getConnectionService());
    }

    @MediumTest
    public void testAnalyticsTwoCalls() throws Exception {
        IdPair testCall1 = startAndMakeActiveIncomingCall(
                "650-555-1212",
                mPhoneAccountA0.getAccountHandle(),
                mConnectionServiceFixtureA);
        IdPair testCall2 = startAndMakeActiveOutgoingCall(
                "650-555-1213",
                mPhoneAccountA0.getAccountHandle(),
                mConnectionServiceFixtureA);

        Map<String, Analytics.CallInfoImpl> analyticsMap = Analytics.cloneData();
        assertTrue(analyticsMap.containsKey(testCall1.mCallId));
        assertTrue(analyticsMap.containsKey(testCall2.mCallId));

        Analytics.CallInfoImpl callAnalytics1 = analyticsMap.get(testCall1.mCallId);
        Analytics.CallInfoImpl callAnalytics2 = analyticsMap.get(testCall2.mCallId);
        assertTrue(callAnalytics1.startTime > 0);
        assertTrue(callAnalytics2.startTime > 0);
        assertEquals(0, callAnalytics1.endTime);
        assertEquals(0, callAnalytics2.endTime);

        assertEquals(Analytics.INCOMING_DIRECTION, callAnalytics1.callDirection);
        assertEquals(Analytics.OUTGOING_DIRECTION, callAnalytics2.callDirection);

        assertTrue(callAnalytics1.isInterrupted);
        assertTrue(callAnalytics2.isAdditionalCall);

        assertNull(callAnalytics1.callTerminationReason);
        assertNull(callAnalytics2.callTerminationReason);

        assertEquals(mConnectionServiceComponentNameA.flattenToShortString(),
                callAnalytics1.connectionService);
        assertEquals(mConnectionServiceComponentNameA.flattenToShortString(),
                callAnalytics1.connectionService);

        mConnectionServiceFixtureA.
                sendSetDisconnected(testCall2.mConnectionId, DisconnectCause.REMOTE);
        mConnectionServiceFixtureA.
                sendSetDisconnected(testCall1.mConnectionId, DisconnectCause.ERROR);

        analyticsMap = Analytics.cloneData();
        callAnalytics1 = analyticsMap.get(testCall1.mCallId);
        callAnalytics2 = analyticsMap.get(testCall2.mCallId);
        assertTrue(callAnalytics1.endTime > 0);
        assertTrue(callAnalytics2.endTime > 0);
        assertNotNull(callAnalytics1.callTerminationReason);
        assertNotNull(callAnalytics2.callTerminationReason);
        assertEquals(DisconnectCause.ERROR, callAnalytics1.callTerminationReason.getCode());
        assertEquals(DisconnectCause.REMOTE, callAnalytics2.callTerminationReason.getCode());
    }

    private void blockNumber(String phoneNumber) throws Exception {
        blockNumberWithAnswer(phoneNumber, new Answer<Bundle>() {
            @Override
            public Bundle answer(InvocationOnMock invocation) throws Throwable {
                Bundle bundle = new Bundle();
                bundle.putBoolean(BlockedNumberContract.RES_NUMBER_IS_BLOCKED, true);
                return bundle;
            }
        });
    }

    private void blockNumberWithAnswer(String phoneNumber, Answer answer) throws Exception {
        when(getBlockedNumberProvider().call(
                anyString(),
                eq(BlockedNumberContract.SystemContract.METHOD_SHOULD_SYSTEM_BLOCK_NUMBER),
                eq(phoneNumber),
                isNull(Bundle.class))).thenAnswer(answer);
    }

    private void verifyNoBlockChecks() {
        verifyZeroInteractions(getBlockedNumberProvider());
    }

    private IContentProvider getBlockedNumberProvider() {
        return mSpyContext.getContentResolver().acquireProvider(BlockedNumberContract.AUTHORITY);
    }

    private void disconnectCall(String callId, String connectionId) throws Exception {
        mConnectionServiceFixtureA.sendSetDisconnected(connectionId, DisconnectCause.LOCAL);
        assertEquals(Call.STATE_DISCONNECTED, mInCallServiceFixtureX.getCall(callId).getState());
        assertEquals(Call.STATE_DISCONNECTED, mInCallServiceFixtureY.getCall(callId).getState());
    }

    /**
     * Tests the {@link Call#pullExternalCall()} API.  Ensures that an external call which is
     * pullable can be pulled.
     *
     * @throws Exception
     */
    @LargeTest
    public void testPullExternalCall() throws Exception {
        // TODO: Revisit this unit test once telecom support for filtering external calls from
        // InCall services is implemented.
        mConnectionServiceFixtureA.mConnectionServiceDelegate.mCapabilities =
                Connection.CAPABILITY_CAN_PULL_CALL;
        mConnectionServiceFixtureA.mConnectionServiceDelegate.mProperties =
                Connection.PROPERTY_IS_EXTERNAL_CALL;

        IdPair ids = startAndMakeActiveIncomingCall("650-555-1212",
                mPhoneAccountA0.getAccountHandle(), mConnectionServiceFixtureA);
        assertEquals(Call.STATE_ACTIVE, mInCallServiceFixtureX.getCall(ids.mCallId).getState());

        // Attempt to pull the call and verify the API call makes it through
        mInCallServiceFixtureX.mInCallAdapter.pullExternalCall(ids.mCallId);
        verify(mConnectionServiceFixtureA.getTestDouble(), timeout(TEST_TIMEOUT))
                .pullExternalCall(ids.mCallId);
    }

    /**
     * Tests the {@link Call#pullExternalCall()} API.  Verifies that if an external call is not
     * marked as pullable that the connection service does not get an API call to pull the external
     * call.
     *
     * @throws Exception
     */
    @LargeTest
    public void testPullNonPullableExternalCall() throws Exception {
        // TODO: Revisit this unit test once telecom support for filtering external calls from
        // InCall services is implemented.
        mConnectionServiceFixtureA.mConnectionServiceDelegate.mProperties =
                Connection.PROPERTY_IS_EXTERNAL_CALL;

        IdPair ids = startAndMakeActiveIncomingCall("650-555-1212",
                mPhoneAccountA0.getAccountHandle(), mConnectionServiceFixtureA);
        assertEquals(Call.STATE_ACTIVE, mInCallServiceFixtureX.getCall(ids.mCallId).getState());

        // Attempt to pull the call and verify the API call makes it through
        mInCallServiceFixtureX.mInCallAdapter.pullExternalCall(ids.mCallId);
        verify(mConnectionServiceFixtureA.getTestDouble(), timeout(TEST_TIMEOUT).never())
                .pullExternalCall(ids.mCallId);
    }
}
