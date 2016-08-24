/*
 * Copyright (C) 2014 The Android Open Source Project
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

package com.android.services.telephony;

import com.android.phone.PhoneUtils;

import android.os.Handler;
import android.telecom.Connection;
import android.telecom.DisconnectCause;
import android.telecom.PhoneAccountHandle;

import java.util.ArrayList;
import java.util.List;

/**
 * Manages CDMA conference calls. CDMA conference calls are much more limited than GSM conference
 * calls. Two main points of difference:
 * 1) Users cannot manage individual calls within a conference
 * 2) Whether a conference call starts off as a conference or as two distinct calls is a matter of
 *    physical location (some antennas are different than others). Worst still, there's no
 *    indication given to us as to what state they are in.
 *
 * To make life easier on the user we do the following: Whenever there exist 2 or more calls, we
 * say that we are in a conference call with {@link Connection#PROPERTY_GENERIC_CONFERENCE}.
 * Generic indicates that this is a simple conference that doesn't support conference management.
 * The conference call will also support "MERGE" to begin with and stop supporting it the first time
 * we are asked to actually execute a merge. I emphasize when "we are asked" because we get no
 * indication whether the merge succeeds from CDMA, we just assume it does. Thats the best we
 * can do. Also, we do not kill a conference call once it is created unless all underlying
 * connections also go away.
 *
 * Outgoing CDMA calls made while another call exists would normally trigger a conference to be
 * created. To avoid this and make it seem like there is a "dialing" state, we fake it and prevent
 * the conference from being created for 3 seconds. This is a more pleasant experience for the user.
 */
final class CdmaConferenceController {
    private final Connection.Listener mConnectionListener = new Connection.Listener() {
                @Override
                public void onStateChanged(Connection c, int state) {
                    recalculateConference();
                }

                @Override
                public void onDisconnected(Connection c, DisconnectCause disconnectCause) {
                    recalculateConference();
                }

                @Override
                public void onDestroyed(Connection c) {
                    remove((CdmaConnection) c);
                }
            };

    private static final int ADD_OUTGOING_CONNECTION_DELAY_MILLIS = 6000;

    /** The known CDMA connections. */
    private final List<CdmaConnection> mCdmaConnections = new ArrayList<>();

    /**
     * Newly added connections.  We keep track of newly added outgoing connections because we do not
     * create a conference until a second outgoing call has existing for
     * {@link #ADD_OUTGOING_CONNECTION_DELAY_MILLIS} milliseconds.  This allows the UI to show the
     * call as "Dialing" for a certain amount of seconds.
     */
    private final List<CdmaConnection> mPendingOutgoingConnections = new ArrayList<>();

    private final TelephonyConnectionService mConnectionService;

    private final Handler mHandler = new Handler();

    public CdmaConferenceController(TelephonyConnectionService connectionService) {
        mConnectionService = connectionService;
    }

    /** The CDMA conference connection object. */
    private CdmaConference mConference;

    void add(final CdmaConnection connection) {
        if (!mCdmaConnections.isEmpty() && connection.isOutgoing()) {
            // There already exists a connection, so this will probably result in a conference once
            // it is added. For outgoing connections which are added while another connection
            // exists, we mark them as "dialing" for a set amount of time to give the user time to
            // see their new call as "Dialing" before it turns into a conference call.
            // During that time, we also mark the other calls as "held" or else it can cause issues
            // due to having an ACTIVE and a DIALING call simultaneously.
            connection.forceAsDialing(true);
            final List<CdmaConnection> connectionsToReset =
                    new ArrayList<>(mCdmaConnections.size());
            for (CdmaConnection current : mCdmaConnections) {
                if (current.setHoldingForConference()) {
                    connectionsToReset.add(current);
                }
            }
            mHandler.postDelayed(new Runnable() {
                @Override
                public void run() {
                    connection.forceAsDialing(false);
                    addInternal(connection);
                    for (CdmaConnection current : connectionsToReset) {
                        current.resetStateForConference();
                    }
                }
            }, ADD_OUTGOING_CONNECTION_DELAY_MILLIS);
        } else {
            // This is the first connection, or it is incoming, so let it flow through.
            addInternal(connection);
        }
    }

    private void addInternal(CdmaConnection connection) {
        mCdmaConnections.add(connection);
        connection.addConnectionListener(mConnectionListener);
        recalculateConference();
    }

    private void remove(CdmaConnection connection) {
        connection.removeConnectionListener(mConnectionListener);
        mCdmaConnections.remove(connection);
        recalculateConference();
    }

    private void recalculateConference() {
        List<CdmaConnection> conferenceConnections = new ArrayList<>(mCdmaConnections.size());
        for (CdmaConnection connection : mCdmaConnections) {
            // We do not include call-waiting calls in conferences.
            if (!connection.isCallWaiting() &&
                    connection.getState() != Connection.STATE_DISCONNECTED) {
                conferenceConnections.add(connection);
            }
        }

        Log.d(this, "recalculating conference calls %d", conferenceConnections.size());
        if (conferenceConnections.size() >= 2) {
            boolean isNewlyCreated = false;

            CdmaConnection newConnection = mCdmaConnections.get(mCdmaConnections.size() - 1);

            // There are two or more CDMA connections. Do the following:
            // 1) Create a new conference connection if it doesn't exist.
            if (mConference == null) {
                Log.i(this, "Creating new Cdma conference call");
                PhoneAccountHandle phoneAccountHandle =
                        PhoneUtils.makePstnPhoneAccountHandle(newConnection.getPhone());
                mConference = new CdmaConference(phoneAccountHandle);
                isNewlyCreated = true;
            }

            if (newConnection.isOutgoing()) {
                // Only an outgoing call can be merged with an ongoing call.
                mConference.updateCapabilities(Connection.CAPABILITY_MERGE_CONFERENCE);
            } else {
                // If the most recently added connection was an incoming call, enable
                // swap instead of merge.
                mConference.updateCapabilities(Connection.CAPABILITY_SWAP_CONFERENCE);
            }

            // 2) Add any new connections to the conference
            List<Connection> existingChildConnections =
                    new ArrayList<>(mConference.getConnections());
            for (CdmaConnection connection : conferenceConnections) {
                if (!existingChildConnections.contains(connection)) {
                    Log.i(this, "Adding connection to conference call: %s", connection);
                    mConference.addConnection(connection);
                }
                existingChildConnections.remove(connection);
            }

            // 3) Remove any lingering old/disconnected/destroyed connections
            for (Connection oldConnection : existingChildConnections) {
                mConference.removeConnection(oldConnection);
                Log.i(this, "Removing connection from conference call: %s", oldConnection);
            }

            // 4) Add the conference to the connection service if it is new.
            if (isNewlyCreated) {
                Log.d(this, "Adding the conference call");
                mConnectionService.addConference(mConference);
            }
        } else if (conferenceConnections.isEmpty()) {
            // There are no more connection so if we still have a conference, lets remove it.
            if (mConference != null) {
                Log.i(this, "Destroying the CDMA conference connection.");
                mConference.destroy();
                mConference = null;
            }
        }
    }
}
