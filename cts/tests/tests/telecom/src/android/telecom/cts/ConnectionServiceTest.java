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
 * limitations under the License.
 */

package android.telecom.cts;

import static android.telecom.cts.TestUtils.*;

import android.telecom.Call;
import android.telecom.Connection;
import android.telecom.ConnectionService;
import android.util.Log;

import java.util.Collection;
import java.util.concurrent.TimeUnit;

/**
 * Test some additional {@link ConnectionService} APIs not already covered by other tests.
 */
public class ConnectionServiceTest extends BaseTelecomTestWithMockServices {

    @Override
    protected void setUp() throws Exception {
        super.setUp();
        mContext = getInstrumentation().getContext();
        if (mShouldTestTelecom) {
            setupConnectionService(null, FLAG_REGISTER | FLAG_ENABLE);
        }
    }

    public void testAddExistingConnection() {
        if (!mShouldTestTelecom) {
            return;
        }

        placeAndVerifyCall();
        verifyConnectionForOutgoingCall();

        // Add second connection (add existing connection)
        final MockConnection connection = new MockConnection();
        connection.setOnHold();
        CtsConnectionService.addExistingConnectionToTelecom(TEST_PHONE_ACCOUNT_HANDLE, connection);
        assertNumCalls(mInCallCallbacks.getService(), 2);
        mInCallCallbacks.lock.drainPermits();
        final Call call = mInCallCallbacks.getService().getLastCall();
        assertCallState(call, Call.STATE_HOLDING);
    }

    public void testGetAllConnections() {
        if (!mShouldTestTelecom) {
            return;
        }

        // Add first connection (outgoing call)
        placeAndVerifyCall();
        final Connection connection1 = verifyConnectionForOutgoingCall();

        Collection<Connection> connections = CtsConnectionService.getAllConnectionsFromTelecom();
        assertEquals(1, connections.size());
        assertTrue(connections.contains(connection1));
        // Need to move this to active since we reject the 3rd incoming call below if this is in
        // dialing state (b/23428950).
        connection1.setActive();
        assertCallState(mInCallCallbacks.getService().getLastCall(), Call.STATE_ACTIVE);

        // Add second connection (add existing connection)
        final Connection connection2 = new MockConnection();
        connection2.setActive();
        CtsConnectionService.addExistingConnectionToTelecom(TEST_PHONE_ACCOUNT_HANDLE, connection2);
        assertNumCalls(mInCallCallbacks.getService(), 2);
        mInCallCallbacks.lock.drainPermits();
        connections = CtsConnectionService.getAllConnectionsFromTelecom();
        assertEquals(2, connections.size());
        assertTrue(connections.contains(connection2));

        // Add third connection (incoming call)
        addAndVerifyNewIncomingCall(createTestNumber(), null);
        final Connection connection3 = verifyConnectionForIncomingCall();
        connections = CtsConnectionService.getAllConnectionsFromTelecom();
        assertEquals(3, connections.size());
        assertTrue(connections.contains(connection3));
    }
}
