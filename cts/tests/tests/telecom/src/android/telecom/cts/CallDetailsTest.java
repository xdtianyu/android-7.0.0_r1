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

package android.telecom.cts;

import static android.telecom.cts.TestUtils.*;

import static org.hamcrest.CoreMatchers.is;
import static org.junit.Assert.assertThat;

import android.graphics.drawable.Icon;
import android.os.Bundle;
import android.net.Uri;
import android.telecom.Call;
import android.telecom.Connection;
import android.telecom.ConnectionRequest;
import android.telecom.DisconnectCause;
import android.telecom.GatewayInfo;
import android.telecom.PhoneAccount;
import android.telecom.PhoneAccountHandle;
import android.telecom.StatusHints;
import android.telecom.TelecomManager;
import android.util.Log;

import java.util.Arrays;
import java.util.List;

/**
 * Suites of tests that verifies the various Call details.
 */
public class CallDetailsTest extends BaseTelecomTestWithMockServices {

    /**
     * {@link Connection#PROPERTY_HIGH_DEF_AUDIO} is @hide, so define it here for now.
     */
    public static final int PROPERTY_HIGH_DEF_AUDIO = 1<<2;

    /**
     * {@link Connection#PROPERTY_WIFI} is @hide, so define it here for now.
     */
    public static final int PROPERTY_WIFI = 1<<3;
    public static final int CONNECTION_PROPERTIES =  PROPERTY_HIGH_DEF_AUDIO | PROPERTY_WIFI;
    public static final int CONNECTION_CAPABILITIES =
            Connection.CAPABILITY_HOLD | Connection.CAPABILITY_MUTE;
    public static final int CALL_CAPABILITIES =
            Call.Details.CAPABILITY_HOLD | Call.Details.CAPABILITY_MUTE;
    public static final int CALL_PROPERTIES =
            Call.Details.PROPERTY_HIGH_DEF_AUDIO | Call.Details.PROPERTY_WIFI;
    public static final String CALLER_DISPLAY_NAME = "CTS test";
    public static final int CALLER_DISPLAY_NAME_PRESENTATION = TelecomManager.PRESENTATION_ALLOWED;
    public static final String TEST_SUBJECT = "test";
    public static final String TEST_CHILD_NUMBER = "650-555-1212";
    public static final String TEST_FORWARDED_NUMBER = "650-555-1212";
    public static final String TEST_EXTRA_KEY = "com.test.extra.TEST";
    public static final String TEST_EXTRA_KEY2 = "com.test.extra.TEST2";
    public static final String TEST_EXTRA_KEY3 = "com.test.extra.TEST3";
    public static final int TEST_EXTRA_VALUE = 10;
    public static final String TEST_EVENT = "com.test.event.TEST";

    private StatusHints mStatusHints;
    private Bundle mExtras = new Bundle();

    private MockInCallService mInCallService;
    private Call mCall;
    private MockConnection mConnection;

    @Override
    protected void setUp() throws Exception {
        super.setUp();
        if (mShouldTestTelecom) {
            PhoneAccount account = setupConnectionService(
                    new MockConnectionService() {
                        @Override
                        public Connection onCreateOutgoingConnection(
                                PhoneAccountHandle connectionManagerPhoneAccount,
                                ConnectionRequest request) {
                            Connection connection = super.onCreateOutgoingConnection(
                                    connectionManagerPhoneAccount,
                                    request);
                            mConnection = (MockConnection) connection;
                            // Modify the connection object created with local values.
                            connection.setConnectionCapabilities(CONNECTION_CAPABILITIES);
                            connection.setCallerDisplayName(
                                    CALLER_DISPLAY_NAME,
                                    CALLER_DISPLAY_NAME_PRESENTATION);
                            connection.setExtras(mExtras);
                            mStatusHints = new StatusHints(
                                    "CTS test",
                                    Icon.createWithResource(
                                            getInstrumentation().getContext(),
                                            R.drawable.ic_phone_24dp),
                                            null);
                            connection.setStatusHints(mStatusHints);
                            lock.release();
                            return connection;
                        }
                    }, FLAG_REGISTER | FLAG_ENABLE);

            /** Place a call as a part of the setup before we test the various
             *  Call details.
             */
            placeAndVerifyCall();
            verifyConnectionForOutgoingCall();

            mInCallService = mInCallCallbacks.getService();
            mCall = mInCallService.getLastCall();

            assertCallState(mCall, Call.STATE_DIALING);
        }
    }

    /**
     * Tests whether the getAccountHandle() getter returns the correct object.
     */
    public void testAccountHandle() {
        if (!mShouldTestTelecom) {
            return;
        }

        assertThat(mCall.getDetails().getAccountHandle(), is(PhoneAccountHandle.class));
        assertEquals(TEST_PHONE_ACCOUNT_HANDLE, mCall.getDetails().getAccountHandle());
    }

    /**
     * Tests whether the getCallCapabilities() getter returns the correct object.
     */
    public void testCallCapabilities() {
        if (!mShouldTestTelecom) {
            return;
        }

        assertThat(mCall.getDetails().getCallCapabilities(), is(Integer.class));
        assertEquals(CALL_CAPABILITIES, mCall.getDetails().getCallCapabilities());
        assertTrue(mCall.getDetails().can(Call.Details.CAPABILITY_HOLD));
        assertTrue(mCall.getDetails().can(Call.Details.CAPABILITY_MUTE));
        assertFalse(mCall.getDetails().can(Call.Details.CAPABILITY_MANAGE_CONFERENCE));
        assertFalse(mCall.getDetails().can(Call.Details.CAPABILITY_RESPOND_VIA_TEXT));
    }

    /**
     * Tests propagation of the local video capabilities from telephony through to in-call.
     */
    public void testCallLocalVideoCapability() {
        if (!mShouldTestTelecom) {
            return;
        }

        // Note: Local support for video is disabled when a call is in dialing state.
        mConnection.setConnectionCapabilities(
                Connection.CAPABILITY_SUPPORTS_VT_LOCAL_BIDIRECTIONAL);
        assertCallCapabilities(mCall, 0);

        mConnection.setConnectionCapabilities(Connection.CAPABILITY_SUPPORTS_VT_LOCAL_RX);
        assertCallCapabilities(mCall, 0);

        mConnection.setConnectionCapabilities(Connection.CAPABILITY_SUPPORTS_VT_LOCAL_TX);
        assertCallCapabilities(mCall, 0);

        mConnection.setConnectionCapabilities(
                Connection.CAPABILITY_SUPPORTS_VT_REMOTE_BIDIRECTIONAL);
        assertCallCapabilities(mCall, 0);

        mConnection.setConnectionCapabilities(Connection.CAPABILITY_SUPPORTS_VT_REMOTE_RX);
        assertCallCapabilities(mCall, 0);

        mConnection.setConnectionCapabilities(Connection.CAPABILITY_SUPPORTS_VT_REMOTE_TX);
        assertCallCapabilities(mCall, 0);

        // Set call active; we expect the capabilities to make it through now.
        mConnection.setActive();

        mConnection.setConnectionCapabilities(
                Connection.CAPABILITY_SUPPORTS_VT_LOCAL_BIDIRECTIONAL);
        assertCallCapabilities(mCall, Call.Details.CAPABILITY_SUPPORTS_VT_LOCAL_BIDIRECTIONAL);

        mConnection.setConnectionCapabilities(Connection.CAPABILITY_SUPPORTS_VT_LOCAL_RX);
        assertCallCapabilities(mCall, Call.Details.CAPABILITY_SUPPORTS_VT_LOCAL_RX);

        mConnection.setConnectionCapabilities(Connection.CAPABILITY_SUPPORTS_VT_LOCAL_TX);
        assertCallCapabilities(mCall, Call.Details.CAPABILITY_SUPPORTS_VT_LOCAL_TX);

        mConnection.setConnectionCapabilities(
                Connection.CAPABILITY_SUPPORTS_VT_REMOTE_BIDIRECTIONAL);
        assertCallCapabilities(mCall, Call.Details.CAPABILITY_SUPPORTS_VT_REMOTE_BIDIRECTIONAL);

        mConnection.setConnectionCapabilities(Connection.CAPABILITY_SUPPORTS_VT_REMOTE_RX);
        assertCallCapabilities(mCall, Call.Details.CAPABILITY_SUPPORTS_VT_REMOTE_RX);

        mConnection.setConnectionCapabilities(Connection.CAPABILITY_SUPPORTS_VT_REMOTE_TX);
        assertCallCapabilities(mCall, Call.Details.CAPABILITY_SUPPORTS_VT_REMOTE_TX);
    }

    /**
     * Tests passing call capabilities from Connections to Calls.
     */
    public void testCallCapabilityPropagation() {
        if (!mShouldTestTelecom) {
            return;
        }

        mConnection.setConnectionCapabilities(Connection.CAPABILITY_CAN_PAUSE_VIDEO);
        assertCallCapabilities(mCall, Call.Details.CAPABILITY_CAN_PAUSE_VIDEO);

        mConnection.setConnectionCapabilities(Connection.CAPABILITY_HOLD);
        assertCallCapabilities(mCall, Call.Details.CAPABILITY_HOLD);

        mConnection.setConnectionCapabilities(Connection.CAPABILITY_MANAGE_CONFERENCE);
        assertCallCapabilities(mCall, Call.Details.CAPABILITY_MANAGE_CONFERENCE);

        mConnection.setConnectionCapabilities(Connection.CAPABILITY_MERGE_CONFERENCE);
        assertCallCapabilities(mCall, Call.Details.CAPABILITY_MERGE_CONFERENCE);

        mConnection.setConnectionCapabilities(Connection.CAPABILITY_MUTE);
        assertCallCapabilities(mCall, Call.Details.CAPABILITY_MUTE);

        mConnection.setConnectionCapabilities(Connection.CAPABILITY_RESPOND_VIA_TEXT);
        assertCallCapabilities(mCall, Call.Details.CAPABILITY_RESPOND_VIA_TEXT);

        mConnection.setConnectionCapabilities(Connection.CAPABILITY_SEPARATE_FROM_CONFERENCE);
        assertCallCapabilities(mCall, Call.Details.CAPABILITY_SEPARATE_FROM_CONFERENCE);

        mConnection.setConnectionCapabilities(Connection.CAPABILITY_SUPPORT_HOLD);
        assertCallCapabilities(mCall, Call.Details.CAPABILITY_SUPPORT_HOLD);

        mConnection.setConnectionCapabilities(Connection.CAPABILITY_SWAP_CONFERENCE);
        assertCallCapabilities(mCall, Call.Details.CAPABILITY_SWAP_CONFERENCE);
    }

    /**
     * Tests whether the getCallerDisplayName() getter returns the correct object.
     */
    public void testCallerDisplayName() {
        if (!mShouldTestTelecom) {
            return;
        }

        assertThat(mCall.getDetails().getCallerDisplayName(), is(String.class));
        assertEquals(CALLER_DISPLAY_NAME, mCall.getDetails().getCallerDisplayName());
    }

    /**
     * Tests whether the getCallerDisplayNamePresentation() getter returns the correct object.
     */
    public void testCallerDisplayNamePresentation() {
        if (!mShouldTestTelecom) {
            return;
        }

        assertThat(mCall.getDetails().getCallerDisplayNamePresentation(), is(Integer.class));
        assertEquals(CALLER_DISPLAY_NAME_PRESENTATION, mCall.getDetails().getCallerDisplayNamePresentation());
    }

    /**
     * Tests whether the getCallProperties() getter returns the correct object.
     */
    public void testCallProperties() {
        if (!mShouldTestTelecom) {
            return;
        }

        assertThat(mCall.getDetails().getCallProperties(), is(Integer.class));

        // No public call properties at the moment, so ensure we have 0 as a return.
        assertEquals(0, mCall.getDetails().getCallProperties());
    }

    /**
     * Tests whether the getConnectTimeMillis() getter returns the correct object.
     */
    public void testConnectTimeMillis() {
        if (!mShouldTestTelecom) {
            return;
        }

        assertThat(mCall.getDetails().getConnectTimeMillis(), is(Long.class));
    }

    /**
     * Tests whether the getDisconnectCause() getter returns the correct object.
     */
    public void testDisconnectCause() {
        if (!mShouldTestTelecom) {
            return;
        }

        assertThat(mCall.getDetails().getDisconnectCause(), is(DisconnectCause.class));
    }

    /**
     * Tests whether the getExtras() getter returns the correct object.
     */
    public void testExtras() {
        if (!mShouldTestTelecom) {
            return;
        }

        if (mCall.getDetails().getExtras() != null) {
            assertThat(mCall.getDetails().getExtras(), is(Bundle.class));
        }
    }

    /**
     * Tests whether the getIntentExtras() getter returns the correct object.
     */
    public void testIntentExtras() {
        if (!mShouldTestTelecom) {
            return;
        }

        assertThat(mCall.getDetails().getIntentExtras(), is(Bundle.class));
    }

    /**
     * Tests whether the getGatewayInfo() getter returns the correct object.
     */
    public void testGatewayInfo() {
        if (!mShouldTestTelecom) {
            return;
        }

        if (mCall.getDetails().getGatewayInfo() != null) {
            assertThat(mCall.getDetails().getGatewayInfo(), is(GatewayInfo.class));
        }
    }

    /**
     * Tests whether the getHandle() getter returns the correct object.
     */
    public void testHandle() {
        if (!mShouldTestTelecom) {
            return;
        }

        assertThat(mCall.getDetails().getHandle(), is(Uri.class));
        assertEquals(getTestNumber(), mCall.getDetails().getHandle());
    }

    /**
     * Tests whether the getHandlePresentation() getter returns the correct object.
     */
    public void testHandlePresentation() {
        if (!mShouldTestTelecom) {
            return;
        }

        assertThat(mCall.getDetails().getHandlePresentation(), is(Integer.class));
        assertEquals(MockConnectionService.CONNECTION_PRESENTATION, mCall.getDetails().getHandlePresentation());
    }

    /**
     * Tests whether the getStatusHints() getter returns the correct object.
     */
    public void testStatusHints() {
        if (!mShouldTestTelecom) {
            return;
        }

        assertThat(mCall.getDetails().getStatusHints(), is(StatusHints.class));
        assertEquals(mStatusHints.getLabel(), mCall.getDetails().getStatusHints().getLabel());
        assertEquals(
                mStatusHints.getIcon().toString(),
                mCall.getDetails().getStatusHints().getIcon().toString());
        assertEquals(mStatusHints.getExtras(), mCall.getDetails().getStatusHints().getExtras());
    }

    /**
     * Tests whether the getVideoState() getter returns the correct object.
     */
    public void testVideoState() {
        if (!mShouldTestTelecom) {
            return;
        }

        assertThat(mCall.getDetails().getVideoState(), is(Integer.class));
    }

    /**
     * Tests communication of {@link Connection#setExtras(Bundle)} through to
     * {@link Call.Details#getExtras()}.
     */
    public void testExtrasPropagation() {
        if (!mShouldTestTelecom) {
            return;
        }

        Bundle exampleExtras = new Bundle();
        exampleExtras.putString(Connection.EXTRA_CALL_SUBJECT, TEST_SUBJECT);
        exampleExtras.putString(Connection.EXTRA_CHILD_ADDRESS, TEST_CHILD_NUMBER);
        exampleExtras.putString(Connection.EXTRA_LAST_FORWARDED_NUMBER, TEST_FORWARDED_NUMBER);
        exampleExtras.putInt(TEST_EXTRA_KEY, TEST_EXTRA_VALUE);
        mConnection.setExtras(exampleExtras);

        // Make sure we got back a bundle with the call subject key set.
        assertCallExtras(mCall, Connection.EXTRA_CALL_SUBJECT);

        Bundle callExtras = mCall.getDetails().getExtras();
        assertEquals(TEST_SUBJECT, callExtras.getString(Connection.EXTRA_CALL_SUBJECT));
        assertEquals(TEST_CHILD_NUMBER, callExtras.getString(Connection.EXTRA_CHILD_ADDRESS));
        assertEquals(TEST_FORWARDED_NUMBER,
                callExtras.getString(Connection.EXTRA_LAST_FORWARDED_NUMBER));
        assertEquals(TEST_EXTRA_VALUE, callExtras.getInt(TEST_EXTRA_KEY));
    }

    /**
     * Asserts that a call's extras contain a specified key.
     *
     * @param call The call.
     * @param expectedKey The expected extras key.
     */
    private void assertCallExtras(final Call call, final String expectedKey) {
        waitUntilConditionIsTrueOrTimeout(
                new Condition() {
                    @Override
                    public Object expected() {
                        return expectedKey;
                    }

                    @Override
                    public Object actual() {
                        return call.getDetails().getExtras().containsKey(expectedKey) ? expectedKey
                                : "";
                    }
                },
                TestUtils.WAIT_FOR_STATE_CHANGE_TIMEOUT_MS,
                "Call should have extras key " + expectedKey
        );
    }
}
