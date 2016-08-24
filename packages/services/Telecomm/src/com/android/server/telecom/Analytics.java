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

package com.android.server.telecom;

import android.os.Parcelable;
import android.telecom.ParcelableCallAnalytics;
import android.telecom.DisconnectCause;

import com.android.internal.annotations.VisibleForTesting;
import com.android.internal.telephony.CallerInfo;
import com.android.internal.util.IndentingPrintWriter;

import java.util.HashMap;
import java.util.Map;

/**
 * A class that collects and stores data on how calls are being made, in order to
 * aggregate these into useful statistics.
 */
public class Analytics {
   public static class CallInfo {
        void setCallStartTime(long startTime) {
        }

        void setCallEndTime(long endTime) {
        }

        void setCallIsAdditional(boolean isAdditional) {
        }

        void setCallIsInterrupted(boolean isInterrupted) {
        }

        void setCallDisconnectCause(DisconnectCause disconnectCause) {
        }

        void addCallTechnology(int callTechnology) {
        }

        void setCreatedFromExistingConnection(boolean createdFromExistingConnection) {
        }

        void setCallConnectionService(String connectionServiceName) {
        }
    }

    /**
     * A class that holds data associated with a call.
     */
    @VisibleForTesting
    public static class CallInfoImpl extends CallInfo {
        public String callId;
        public long startTime;  // start time in milliseconds since the epoch. 0 if not yet set.
        public long endTime;  // end time in milliseconds since the epoch. 0 if not yet set.
        public int callDirection;  // one of UNKNOWN_DIRECTION, INCOMING_DIRECTION,
                                   // or OUTGOING_DIRECTION.
        public boolean isAdditionalCall = false;  // true if the call came in while another call was
                                                  // in progress or if the user dialed this call
                                                  // while in the middle of another call.
        public boolean isInterrupted = false;  // true if the call was interrupted by an incoming
                                               // or outgoing call.
        public int callTechnologies;  // bitmask denoting which technologies a call used.

        // true if the Telecom Call object was created from an existing connection via
        // CallsManager#createCallForExistingConnection, for example, by ImsConference.
        public boolean createdFromExistingConnection = false;

        public DisconnectCause callTerminationReason;
        public String connectionService;
        public boolean isEmergency = false;

        CallInfoImpl(String callId, int callDirection) {
            this.callId = callId;
            startTime = 0;
            endTime = 0;
            this.callDirection = callDirection;
            callTechnologies = 0;
            connectionService = "";
        }

        CallInfoImpl(CallInfoImpl other) {
            this.callId = other.callId;
            this.startTime = other.startTime;
            this.endTime = other.endTime;
            this.callDirection = other.callDirection;
            this.isAdditionalCall = other.isAdditionalCall;
            this.isInterrupted = other.isInterrupted;
            this.callTechnologies = other.callTechnologies;
            this.createdFromExistingConnection = other.createdFromExistingConnection;
            this.connectionService = other.connectionService;
            this.isEmergency = other.isEmergency;

            if (other.callTerminationReason != null) {
                this.callTerminationReason = new DisconnectCause(
                        other.callTerminationReason.getCode(),
                        other.callTerminationReason.getLabel(),
                        other.callTerminationReason.getDescription(),
                        other.callTerminationReason.getReason(),
                        other.callTerminationReason.getTone());
            } else {
                this.callTerminationReason = null;
            }
        }

        @Override
        public void setCallStartTime(long startTime) {
            Log.d(TAG, "setting startTime for call " + callId + " to " + startTime);
            this.startTime = startTime;
        }

        @Override
        public void setCallEndTime(long endTime) {
            Log.d(TAG, "setting endTime for call " + callId + " to " + endTime);
            this.endTime = endTime;
        }

        @Override
        public void setCallIsAdditional(boolean isAdditional) {
            Log.d(TAG, "setting isAdditional for call " + callId + " to " + isAdditional);
            this.isAdditionalCall = isAdditional;
        }

        @Override
        public void setCallIsInterrupted(boolean isInterrupted) {
            Log.d(TAG, "setting isInterrupted for call " + callId + " to " + isInterrupted);
            this.isInterrupted = isInterrupted;
        }

        @Override
        public void addCallTechnology(int callTechnology) {
            Log.d(TAG, "adding callTechnology for call " + callId + ": " + callTechnology);
            this.callTechnologies |= callTechnology;
        }

        @Override
        public void setCallDisconnectCause(DisconnectCause disconnectCause) {
            Log.d(TAG, "setting disconnectCause for call " + callId + " to " + disconnectCause);
            this.callTerminationReason = disconnectCause;
        }

        @Override
        public void setCreatedFromExistingConnection(boolean createdFromExistingConnection) {
            Log.d(TAG, "setting createdFromExistingConnection for call " + callId + " to "
                    + createdFromExistingConnection);
            this.createdFromExistingConnection = createdFromExistingConnection;
        }

        @Override
        public void setCallConnectionService(String connectionServiceName) {
            Log.d(TAG, "setting connection service for call " + callId + ": "
                    + connectionServiceName);
            this.connectionService = connectionServiceName;
        }

        @Override
        public String toString() {
            return "{\n"
                    + "    startTime: " + startTime + '\n'
                    + "    endTime: " + endTime + '\n'
                    + "    direction: " + getCallDirectionString() + '\n'
                    + "    isAdditionalCall: " + isAdditionalCall + '\n'
                    + "    isInterrupted: " + isInterrupted + '\n'
                    + "    callTechnologies: " + getCallTechnologiesAsString() + '\n'
                    + "    callTerminationReason: " + getCallDisconnectReasonString() + '\n'
                    + "    connectionService: " + connectionService + '\n'
                    + "}\n";
        }

        public ParcelableCallAnalytics toParcelableAnalytics() {
            // Rounds up to the nearest second.
            long callDuration = (endTime == 0 || startTime == 0) ? 0 : endTime - startTime;
            callDuration += (callDuration % MILLIS_IN_1_SECOND == 0) ?
                    0 : (MILLIS_IN_1_SECOND - callDuration % MILLIS_IN_1_SECOND);
            return new ParcelableCallAnalytics(
                    // rounds down to nearest 5 minute mark
                    startTime - startTime % ParcelableCallAnalytics.MILLIS_IN_5_MINUTES,
                    callDuration,
                    callDirection,
                    isAdditionalCall,
                    isInterrupted,
                    callTechnologies,
                    callTerminationReason == null ?
                            ParcelableCallAnalytics.STILL_CONNECTED :
                            callTerminationReason.getCode(),
                    isEmergency,
                    connectionService,
                    createdFromExistingConnection);
        }

        private String getCallDirectionString() {
            switch (callDirection) {
                case UNKNOWN_DIRECTION:
                    return "UNKNOWN";
                case INCOMING_DIRECTION:
                    return "INCOMING";
                case OUTGOING_DIRECTION:
                    return "OUTGOING";
                default:
                    return "UNKNOWN";
            }
        }

        private String getCallTechnologiesAsString() {
            StringBuilder s = new StringBuilder();
            s.append('[');
            if ((callTechnologies & CDMA_PHONE) != 0) s.append("CDMA ");
            if ((callTechnologies & GSM_PHONE) != 0) s.append("GSM ");
            if ((callTechnologies & SIP_PHONE) != 0) s.append("SIP ");
            if ((callTechnologies & IMS_PHONE) != 0) s.append("IMS ");
            if ((callTechnologies & THIRD_PARTY_PHONE) != 0) s.append("THIRD_PARTY ");
            s.append(']');
            return s.toString();
        }

        private String getCallDisconnectReasonString() {
            if (callTerminationReason != null) {
                return callTerminationReason.toString();
            } else {
                return "NOT SET";
            }
        }
    }
    public static final String TAG = "TelecomAnalytics";

    // Constants for call direction
    public static final int UNKNOWN_DIRECTION = ParcelableCallAnalytics.CALLTYPE_UNKNOWN;
    public static final int INCOMING_DIRECTION = ParcelableCallAnalytics.CALLTYPE_INCOMING;
    public static final int OUTGOING_DIRECTION = ParcelableCallAnalytics.CALLTYPE_OUTGOING;

    // Constants for call technology
    public static final int CDMA_PHONE = ParcelableCallAnalytics.CDMA_PHONE;
    public static final int GSM_PHONE = ParcelableCallAnalytics.GSM_PHONE;
    public static final int IMS_PHONE = ParcelableCallAnalytics.IMS_PHONE;
    public static final int SIP_PHONE = ParcelableCallAnalytics.SIP_PHONE;
    public static final int THIRD_PARTY_PHONE = ParcelableCallAnalytics.THIRD_PARTY_PHONE;

    public static final long MILLIS_IN_1_SECOND = ParcelableCallAnalytics.MILLIS_IN_1_SECOND;

    private static final Object sLock = new Object(); // Coarse lock for all of analytics
    private static final Map<String, CallInfoImpl> sCallIdToInfo = new HashMap<>();

    public static CallInfo initiateCallAnalytics(String callId, int direction) {
        Log.d(TAG, "Starting analytics for call " + callId);
        CallInfoImpl callInfo = new CallInfoImpl(callId, direction);
        synchronized (sLock) {
            sCallIdToInfo.put(callId, callInfo);
        }
        return callInfo;
    }

    public static ParcelableCallAnalytics[] dumpToParcelableAnalytics() {
        ParcelableCallAnalytics[] result;
        synchronized (sLock) {
            result = new ParcelableCallAnalytics[sCallIdToInfo.size()];
            int idx = 0;
            for (CallInfoImpl entry : sCallIdToInfo.values()) {
                result[idx] = entry.toParcelableAnalytics();
                idx++;
            }
            sCallIdToInfo.clear();
        }
        return result;
    }

    public static void dump(IndentingPrintWriter writer) {
        synchronized (sLock) {
            for (Map.Entry<String, CallInfoImpl> entry : sCallIdToInfo.entrySet()) {
                writer.printf("Call %s: ", entry.getKey());
                writer.println(entry.getValue().toString());
            }
        }
    }

    public static void reset() {
        synchronized (sLock) {
            sCallIdToInfo.clear();
        }
    }

    /**
     * Returns a deep copy of callIdToInfo that's safe to read/write without synchronization
     */
    public static Map<String, CallInfoImpl> cloneData() {
        synchronized (sLock) {
            Map<String, CallInfoImpl> result = new HashMap<>(sCallIdToInfo.size());
            for (Map.Entry<String, CallInfoImpl> entry : sCallIdToInfo.entrySet()) {
                result.put(entry.getKey(), new CallInfoImpl(entry.getValue()));
            }
            return result;
        }
    }
}
