/*
 * Copyright 2014, The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package com.android.server.telecom;

import android.content.Context;
import android.net.Uri;
import android.os.Handler;
import android.os.Looper;
import android.os.AsyncTask;
import android.telecom.PhoneAccount;
import android.telephony.PhoneNumberUtils;
import android.text.TextUtils;
import android.util.Base64;

import com.android.internal.annotations.VisibleForTesting;
import com.android.internal.util.IndentingPrintWriter;

import java.nio.ByteBuffer;
import java.security.MessageDigest;
import java.security.NoSuchAlgorithmException;
import java.text.DateFormat;
import java.text.SimpleDateFormat;
import java.util.Arrays;
import java.util.Date;
import java.util.HashMap;
import java.util.IllegalFormatException;
import java.util.Iterator;
import java.util.LinkedList;
import java.util.List;
import java.util.Locale;
import java.util.Map;
import java.util.concurrent.ConcurrentHashMap;
import java.util.concurrent.LinkedBlockingQueue;

/**
 * Manages logging for the entire module.
 */
@VisibleForTesting
public class Log {

    /**
     * Stores the various events associated with {@link Call}s. Also stores all request-response
     * pairs amongst the events.
     */
    public final static class Events {
        public static final String CREATED = "CREATED";
        public static final String DESTROYED = "DESTROYED";
        public static final String SET_NEW = "SET_NEW";
        public static final String SET_CONNECTING = "SET_CONNECTING";
        public static final String SET_DIALING = "SET_DIALING";
        public static final String SET_ACTIVE = "SET_ACTIVE";
        public static final String SET_HOLD = "SET_HOLD";
        public static final String SET_RINGING = "SET_RINGING";
        public static final String SET_DISCONNECTED = "SET_DISCONNECTED";
        public static final String SET_DISCONNECTING = "SET_DISCONNECTING";
        public static final String SET_SELECT_PHONE_ACCOUNT = "SET_SELECT_PHONE_ACCOUNT";
        public static final String REQUEST_HOLD = "REQUEST_HOLD";
        public static final String REQUEST_UNHOLD = "REQUEST_UNHOLD";
        public static final String REQUEST_DISCONNECT = "REQUEST_DISCONNECT";
        public static final String REQUEST_ACCEPT = "REQUEST_ACCEPT";
        public static final String REQUEST_REJECT = "REQUEST_REJECT";
        public static final String START_DTMF = "START_DTMF";
        public static final String STOP_DTMF = "STOP_DTMF";
        public static final String START_RINGER = "START_RINGER";
        public static final String STOP_RINGER = "STOP_RINGER";
        public static final String SKIP_RINGING = "SKIP_RINGING";
        public static final String START_CALL_WAITING_TONE = "START_CALL_WAITING_TONE";
        public static final String STOP_CALL_WAITING_TONE = "STOP_CALL_WAITING_TONE";
        public static final String START_CONNECTION = "START_CONNECTION";
        public static final String BIND_CS = "BIND_CS";
        public static final String CS_BOUND = "CS_BOUND";
        public static final String CONFERENCE_WITH = "CONF_WITH";
        public static final String SPLIT_CONFERENCE = "CONF_SPLIT";
        public static final String SWAP = "SWAP";
        public static final String ADD_CHILD = "ADD_CHILD";
        public static final String REMOVE_CHILD = "REMOVE_CHILD";
        public static final String SET_PARENT = "SET_PARENT";
        public static final String MUTE = "MUTE";
        public static final String AUDIO_ROUTE = "AUDIO_ROUTE";
        public static final String ERROR_LOG = "ERROR";
        public static final String USER_LOG_MARK = "USER_LOG_MARK";
        public static final String SILENCE = "SILENCE";
        public static final String BIND_SCREENING = "BIND_SCREENING";
        public static final String SCREENING_BOUND = "SCREENING_BOUND";
        public static final String SCREENING_SENT = "SCREENING_SENT";
        public static final String SCREENING_COMPLETED = "SCREENING_COMPLETED";
        public static final String BLOCK_CHECK_INITIATED = "BLOCK_CHECK_INITIATED";
        public static final String BLOCK_CHECK_FINISHED = "BLOCK_CHECK_FINISHED";
        public static final String DIRECT_TO_VM_INITIATED = "DIRECT_TO_VM_INITIATED";
        public static final String DIRECT_TO_VM_FINISHED = "DIRECT_TO_VM_FINISHED";
        public static final String FILTERING_INITIATED = "FILTERING_INITIATED";
        public static final String FILTERING_COMPLETED = "FILTERING_COMPLETED";
        public static final String FILTERING_TIMED_OUT = "FILTERING_TIMED_OUT";
        public static final String REMOTELY_HELD = "REMOTELY_HELD";
        public static final String REMOTELY_UNHELD = "REMOTELY_UNHELD";
        public static final String PULL = "PULL";
        public static final String INFO = "INFO";

        /**
         * Maps from a request to a response.  The same event could be listed as the
         * response for multiple requests (e.g. REQUEST_ACCEPT and REQUEST_UNHOLD both map to the
         * SET_ACTIVE response). This map is used to print out the amount of time it takes between
         * a request and a response.
         */
        public static final Map<String, String> requestResponsePairs =
                new HashMap<String, String>() {{
                    put(REQUEST_ACCEPT, SET_ACTIVE);
                    put(REQUEST_REJECT, SET_DISCONNECTED);
                    put(REQUEST_DISCONNECT, SET_DISCONNECTED);
                    put(REQUEST_HOLD, SET_HOLD);
                    put(REQUEST_UNHOLD, SET_ACTIVE);
                    put(START_CONNECTION, SET_DIALING);
                    put(BIND_CS, CS_BOUND);
                    put(SCREENING_SENT, SCREENING_COMPLETED);
                    put(BLOCK_CHECK_INITIATED, BLOCK_CHECK_FINISHED);
                    put(DIRECT_TO_VM_INITIATED, DIRECT_TO_VM_FINISHED);
                    put(FILTERING_INITIATED, FILTERING_COMPLETED);
                }};
    }

    public static class CallEvent {
        public String eventId;
        public String sessionId;
        public long time;
        public Object data;

        public CallEvent(String eventId, String sessionId, long time, Object data) {
            this.eventId = eventId;
            this.sessionId = sessionId;
            this.time = time;
            this.data = data;
        }
    }

    public static class CallEventRecord {
        private static final DateFormat sLongDateFormat = new SimpleDateFormat(
                "yyyy-MM-dd HH:mm:ss.SSS");
        private static final DateFormat sDateFormat = new SimpleDateFormat("HH:mm:ss.SSS");
        private static int sNextId = 1;
        private final List<CallEvent> mEvents = new LinkedList<>();
        private final Call mCall;

        public CallEventRecord(Call call) {
            mCall = call;
        }

        public Call getCall() {
            return mCall;
        }

        public void addEvent(String event, String sessionId, Object data) {
            mEvents.add(new CallEvent(event, sessionId, System.currentTimeMillis(), data));
            Log.i("Event", "Call %s: %s, %s", mCall.getId(), event, data);
        }

        public void dump(IndentingPrintWriter pw) {
            Map<String, CallEvent> pendingResponses = new HashMap<>();

            pw.print("Call ");
            pw.print(mCall.getId());
            pw.print(" [");
            pw.print(sLongDateFormat.format(new Date(mCall.getCreationTimeMillis())));
            pw.print("]");
            pw.println(mCall.isIncoming() ? "(MT - incoming)" : "(MO - outgoing)");

            pw.increaseIndent();
            pw.println("To address: " + piiHandle(mCall.getHandle()));

            for (CallEvent event : mEvents) {

                // We print out events in chronological order. During that process we look at each
                // event and see if it maps to a request on the Request-Response pairs map. If it
                // does, then we effectively start 'listening' for the response. We do that by
                // storing the response event ID in {@code pendingResponses}. When we find the
                // response in a later iteration of the loop, we grab the original request and
                // calculate the time it took to get a response.
                if (Events.requestResponsePairs.containsKey(event.eventId)) {
                    // This event expects a response, so add that response to the maps
                    // of pending events.
                    String pendingResponse = Events.requestResponsePairs.get(event.eventId);
                    pendingResponses.put(pendingResponse, event);
                }

                pw.print(sDateFormat.format(new Date(event.time)));
                pw.print(" - ");
                pw.print(event.eventId);
                if (event.data != null) {
                    pw.print(" (");
                    Object data = event.data;

                    if (data instanceof Call) {
                        // If the data is another call, then change the data to the call's CallEvent
                        // ID instead.
                        CallEventRecord record = mCallEventRecordMap.get(data);
                        if (record != null) {
                            data = "Call " + record.mCall.getId();
                        }
                    }

                    pw.print(data);
                    pw.print(")");
                }

                // If this event is a response event that we've been waiting for, calculate the time
                // it took for the response to complete and print that out as well.
                CallEvent requestEvent = pendingResponses.remove(event.eventId);
                if (requestEvent != null) {
                    pw.print(", time since ");
                    pw.print(requestEvent.eventId);
                    pw.print(": ");
                    pw.print(event.time - requestEvent.time);
                    pw.print(" ms");
                }
                pw.print(":");
                pw.print(event.sessionId);
                pw.println();
            }
            pw.decreaseIndent();
        }
    }

    public static final int MAX_CALLS_TO_CACHE = 5;  // Arbitrarily chosen.
    public static final int MAX_CALLS_TO_CACHE_DEBUG = 20;  // Arbitrarily chosen.
    private static final long EXTENDED_LOGGING_DURATION_MILLIS = 60000 * 30; // 30 minutes

    // Don't check in with this true!
    private static final boolean LOG_DBG = false;

    // Currently using 3 letters, So don't exceed 64^3
    private static final long SESSION_ID_ROLLOVER_THRESHOLD = 262144;

    // Generic tag for all In Call logging
    @VisibleForTesting
    public static String TAG = "Telecom";
    public static String LOGGING_TAG = "Logging";

    public static final boolean FORCE_LOGGING = false; /* STOP SHIP if true */
    public static final boolean SYSTRACE_DEBUG = false; /* STOP SHIP if true */
    public static final boolean DEBUG = isLoggable(android.util.Log.DEBUG);
    public static final boolean INFO = isLoggable(android.util.Log.INFO);
    public static final boolean VERBOSE = isLoggable(android.util.Log.VERBOSE);
    public static final boolean WARN = isLoggable(android.util.Log.WARN);
    public static final boolean ERROR = isLoggable(android.util.Log.ERROR);

    private static final Map<Call, CallEventRecord> mCallEventRecordMap = new HashMap<>();
    private static LinkedBlockingQueue<CallEventRecord> mCallEventRecords =
            new LinkedBlockingQueue<CallEventRecord>(MAX_CALLS_TO_CACHE);

    private static Context mContext = null;
    // Synchronized in all method calls
    private static int sCodeEntryCounter = 0;
    @VisibleForTesting
    public static ConcurrentHashMap<Integer, Session> sSessionMapper = new ConcurrentHashMap<>(100);
    @VisibleForTesting
    public static Handler sSessionCleanupHandler = new Handler(Looper.getMainLooper());
    @VisibleForTesting
    public static java.lang.Runnable sCleanStaleSessions = new java.lang.Runnable() {
        @Override
        public void run() {
            cleanupStaleSessions(getSessionCleanupTimeoutMs());
        }
    };

    // Set the logging container to be the system's. This will only change when being mocked
    // during testing.
    private static SystemLoggingContainer systemLogger = new SystemLoggingContainer();

    /**
     * Tracks whether user-activated extended logging is enabled.
     */
    private static boolean mIsUserExtendedLoggingEnabled = false;

    /**
     * The time when user-activated extended logging should be ended.  Used to determine when
     * extended logging should automatically be disabled.
     */
    private static long mUserExtendedLoggingStopTime = 0;

    private Log() {
    }

    public static void setContext(Context context) {
        mContext = context;
    }

    /**
     * Enable or disable extended telecom logging.
     *
     * @param isExtendedLoggingEnabled {@code true} if extended logging should be enabled,
     *          {@code false} if it should be disabled.
     */
    public static void setIsExtendedLoggingEnabled(boolean isExtendedLoggingEnabled) {
        // If the state hasn't changed, bail early.
        if (mIsUserExtendedLoggingEnabled == isExtendedLoggingEnabled) {
            return;
        }

        // Resize the event queue.
        int newSize = isExtendedLoggingEnabled ? MAX_CALLS_TO_CACHE_DEBUG : MAX_CALLS_TO_CACHE;
        LinkedBlockingQueue<CallEventRecord> oldEventLog = mCallEventRecords;
        mCallEventRecords = new LinkedBlockingQueue<CallEventRecord>(newSize);
        mCallEventRecordMap.clear();

        // Copy the existing queue into the new one.
        for (CallEventRecord event : oldEventLog) {
            addCallEventRecord(event);
        }

        mIsUserExtendedLoggingEnabled = isExtendedLoggingEnabled;
        if (mIsUserExtendedLoggingEnabled) {
            mUserExtendedLoggingStopTime = System.currentTimeMillis()
                    + EXTENDED_LOGGING_DURATION_MILLIS;
        } else {
            mUserExtendedLoggingStopTime = 0;
        }
    }

    public static final long DEFAULT_SESSION_TIMEOUT_MS = 30000L; // 30 seconds
    private static MessageDigest sMessageDigest;

    public static void initMd5Sum() {
        new AsyncTask<Void, Void, Void>() {
            @Override
            public Void doInBackground(Void... args) {
                MessageDigest md;
                try {
                    md = MessageDigest.getInstance("SHA-1");
                } catch (NoSuchAlgorithmException e) {
                    md = null;
                }
                sMessageDigest = md;
                return null;
            }
        }.execute();
    }

    @VisibleForTesting
    public static void setTag(String tag) {
        TAG = tag;
    }

    @VisibleForTesting
    public static void setLoggingContainer(SystemLoggingContainer logger) {
        systemLogger = logger;
    }

    // Overridden in LogTest to skip query to ContentProvider
    public interface ISessionCleanupTimeoutMs {
        long get();
    }
    @VisibleForTesting
    public static ISessionCleanupTimeoutMs sSessionCleanupTimeoutMs =
            new ISessionCleanupTimeoutMs() {
                @Override
                public long get() {
                    // mContext will be null if Log is called from another process
                    // (UserCallActivity, for example). For these cases, use the default value.
                    if(mContext == null) {
                        return DEFAULT_SESSION_TIMEOUT_MS;
                    }
                    return Timeouts.getStaleSessionCleanupTimeoutMillis(
                            mContext.getContentResolver());
                }
            };

    private static long getSessionCleanupTimeoutMs() {
        return sSessionCleanupTimeoutMs.get();
    }

    private static synchronized void resetStaleSessionTimer() {
        sSessionCleanupHandler.removeCallbacksAndMessages(null);
        // Will be null in Log Testing
        if (sCleanStaleSessions != null) {
            sSessionCleanupHandler.postDelayed(sCleanStaleSessions, getSessionCleanupTimeoutMs());
        }
    }

    /**
     * Call at an entry point to the Telecom code to track the session. This code must be
     * accompanied by a Log.endSession().
     */
    public static synchronized void startSession(String shortMethodName) {
        startSession(shortMethodName, null);
    }
    public static synchronized void startSession(String shortMethodName,
            String callerIdentification) {
        resetStaleSessionTimer();
        int threadId = getCallingThreadId();
        Session activeSession = sSessionMapper.get(threadId);
        // We have called startSession within an active session that has not ended... Register this
        // session as a subsession.
        if (activeSession != null) {
            Session childSession = createSubsession(true);
            continueSession(childSession, shortMethodName);
            return;
        }
        Session newSession = new Session(getNextSessionID(), shortMethodName,
                System.currentTimeMillis(), threadId, false, callerIdentification);
        sSessionMapper.put(threadId, newSession);

        Log.v(LOGGING_TAG, Session.START_SESSION);
    }


    /**
     * Notifies the logging system that a subsession will be run at a later point and
     * allocates the resources. Returns a session object that must be used in
     * Log.continueSession(...) to start the subsession.
     */
    public static Session createSubsession() {
        return createSubsession(false);
    }

    private static synchronized Session createSubsession(boolean isStartedFromActiveSession) {
        int threadId = getCallingThreadId();
        Session threadSession = sSessionMapper.get(threadId);
        if (threadSession == null) {
            Log.d(LOGGING_TAG, "Log.createSubsession was called with no session active.");
            return null;
        }
        // Start execution time of the session will be overwritten in continueSession(...).
        Session newSubsession = new Session(threadSession.getNextChildId(),
                threadSession.getShortMethodName(), System.currentTimeMillis(), threadId,
                isStartedFromActiveSession, null);
        threadSession.addChild(newSubsession);
        newSubsession.setParentSession(threadSession);

        if(!isStartedFromActiveSession) {
            Log.v(LOGGING_TAG, Session.CREATE_SUBSESSION + " " + newSubsession.toString());
        } else {
            Log.v(LOGGING_TAG, Session.CREATE_SUBSESSION + " (Invisible subsession)");
        }
        return newSubsession;
    }

    /**
     * Cancels a subsession that had Log.createSubsession() called on it, but will never have
     * Log.continueSession(...) called on it due to an error. Allows the subsession to be cleaned
     * gracefully instead of being removed by the sSessionCleanupHandler forcefully later.
     */
    public static synchronized void cancelSubsession(Session subsession) {
        if (subsession == null) {
            return;
        }

        subsession.markSessionCompleted(0);
        endParentSessions(subsession);
    }

    /**
     * Starts the subsession that was created in Log.CreateSubsession. The Log.endSession() method
     * must be called at the end of this method. The full session will complete when all
     * subsessions are completed.
     */
    public static synchronized void continueSession(Session subsession, String shortMethodName) {
        if (subsession == null) {
            return;
        }
        resetStaleSessionTimer();
        String callingMethodName = subsession.getShortMethodName();
        subsession.setShortMethodName(callingMethodName + "->" + shortMethodName);
        subsession.setExecutionStartTimeMs(System.currentTimeMillis());
        Session parentSession = subsession.getParentSession();
        if (parentSession == null) {
            Log.d(LOGGING_TAG, "Log.continueSession was called with no session active for " +
                    "method %s.", shortMethodName);
            return;
        }

        sSessionMapper.put(getCallingThreadId(), subsession);
        if(!subsession.isStartedFromActiveSession()) {
            Log.v(LOGGING_TAG, Session.CONTINUE_SUBSESSION);
        } else {
            Log.v(LOGGING_TAG, Session.CONTINUE_SUBSESSION + " (Invisible Subsession) with " +
                    "Method " + shortMethodName);
        }
    }

    public static void checkIsThreadLogged() {
        int threadId = getCallingThreadId();
        Session threadSession = sSessionMapper.get(threadId);
        if (threadSession == null) {
            android.util.Log.e(LOGGING_TAG, "Logging Thread Check Failed!", new Exception());
        }
    }

    /**
     * Ends the current session/subsession. Must be called after a Log.startSession(...) and
     * Log.continueSession(...) call.
     */
    public static synchronized void endSession() {
        int threadId = getCallingThreadId();
        Session completedSession = sSessionMapper.get(threadId);
        if (completedSession == null) {
            Log.w(LOGGING_TAG, "Log.endSession was called with no session active.");
            return;
        }

        completedSession.markSessionCompleted(System.currentTimeMillis());
        if(!completedSession.isStartedFromActiveSession()) {
            Log.v(LOGGING_TAG, Session.END_SUBSESSION + " (dur: " +
                    completedSession.getLocalExecutionTime() + " mS)");
        } else {
            Log.v(LOGGING_TAG, Session.END_SUBSESSION + " (Invisible Subsession) (dur: " +
                    completedSession.getLocalExecutionTime() + " mS)");
        }
        // Remove after completed so that reference still exists for logging the end events
        Session parentSession = completedSession.getParentSession();
        sSessionMapper.remove(threadId);
        endParentSessions(completedSession);
        // If this subsession was started from a parent session using Log.startSession, return the
        // ThreadID back to the parent after completion.
        if (parentSession != null && !parentSession.isSessionCompleted() &&
                completedSession.isStartedFromActiveSession()) {
            sSessionMapper.put(threadId, parentSession);
        }
    }

    // Recursively deletes all complete parent sessions of the current subsession if it is a leaf.
    private static void endParentSessions(Session subsession) {
        // Session is not completed or not currently a leaf, so we can not remove because a child is
        // still running
        if (!subsession.isSessionCompleted() || subsession.getChildSessions().size() != 0) {
            return;
        }

        Session parentSession = subsession.getParentSession();
        if (parentSession != null) {
            subsession.setParentSession(null);
            parentSession.removeChild(subsession);
            endParentSessions(parentSession);
        } else {
            // All of the subsessions have been completed and it is time to report on the full
            // running time of the session.
            long fullSessionTimeMs =
                    System.currentTimeMillis() - subsession.getExecutionStartTimeMilliseconds();
            Log.v(LOGGING_TAG, Session.END_SESSION + " (dur: " + fullSessionTimeMs + " ms): " +
                    subsession.toString());
        }
    }

    private synchronized static String getNextSessionID() {
        Integer nextId = sCodeEntryCounter++;
        if (nextId >= SESSION_ID_ROLLOVER_THRESHOLD) {
            restartSessionCounter();
            nextId = sCodeEntryCounter++;
        }
        return getBase64Encoding(nextId);
    }

    @VisibleForTesting
    public synchronized static void restartSessionCounter() {
        sCodeEntryCounter = 0;
    }

    @VisibleForTesting
    public static String getBase64Encoding(int number) {
        byte[] idByteArray = ByteBuffer.allocate(4).putInt(number).array();
        idByteArray = Arrays.copyOfRange(idByteArray, 2, 4);
        return Base64.encodeToString(idByteArray, Base64.NO_WRAP | Base64.NO_PADDING);
    }

    public static int getCallingThreadId() {
        return android.os.Process.myTid();
    }

    public static void event(Call call, String event) {
        event(call, event, null);
    }

    public static void event(Call call, String event, Object data) {
        Session currentSession = sSessionMapper.get(getCallingThreadId());
        String currentSessionID = currentSession != null ? currentSession.toString() : "";

        if (call == null) {
            Log.i(TAG, "Non-call EVENT: %s, %s", event, data);
            return;
        }
        synchronized (mCallEventRecords) {
            if (!mCallEventRecordMap.containsKey(call)) {
                CallEventRecord newRecord = new CallEventRecord(call);
                addCallEventRecord(newRecord);
            }

            CallEventRecord record = mCallEventRecordMap.get(call);
            record.addEvent(event, currentSessionID, data);
        }
    }

    @VisibleForTesting
    public static synchronized void cleanupStaleSessions(long timeoutMs) {
        String logMessage = "Stale Sessions Cleaned:\n";
        boolean isSessionsStale = false;
        long currentTimeMs = System.currentTimeMillis();
        // Remove references that are in the Session Mapper (causing GC to occur) on
        // sessions that are lasting longer than LOGGING_SESSION_TIMEOUT_MS.
        // If this occurs, then there is most likely a Session active that never had
        // Log.endSession called on it.
        for (Iterator<ConcurrentHashMap.Entry<Integer, Session>> it =
             sSessionMapper.entrySet().iterator(); it.hasNext(); ) {
            ConcurrentHashMap.Entry<Integer, Session> entry = it.next();
            Session session = entry.getValue();
            if (currentTimeMs - session.getExecutionStartTimeMilliseconds() > timeoutMs) {
                it.remove();
                logMessage += session.printFullSessionTree() + "\n";
                isSessionsStale = true;
            }
        }
        if (isSessionsStale) {
            Log.w(LOGGING_TAG, logMessage);
        } else {
            Log.v(LOGGING_TAG, "No stale logging sessions needed to be cleaned...");
        }
    }

    private static void addCallEventRecord(CallEventRecord newRecord) {
        Call call = newRecord.getCall();

        // First remove the oldest entry if no new ones exist.
        if (mCallEventRecords.remainingCapacity() == 0) {
            CallEventRecord record = mCallEventRecords.poll();
            if (record != null) {
                mCallEventRecordMap.remove(record.getCall());
            }
        }

        // Now add a new entry
        mCallEventRecords.add(newRecord);
        mCallEventRecordMap.put(call, newRecord);
    }

    /**
     * If user enabled extended logging is enabled and the time limit has passed, disables the
     * extended logging.
     */
    private static void maybeDisableLogging() {
        if (!mIsUserExtendedLoggingEnabled) {
            return;
        }

        if (mUserExtendedLoggingStopTime < System.currentTimeMillis()) {
            mUserExtendedLoggingStopTime = 0;
            mIsUserExtendedLoggingEnabled = false;
        }
    }

    public static boolean isLoggable(int level) {
        return FORCE_LOGGING || android.util.Log.isLoggable(TAG, level);
    }

    public static void d(String prefix, String format, Object... args) {
        if (mIsUserExtendedLoggingEnabled) {
            maybeDisableLogging();
            systemLogger.i(TAG, buildMessage(prefix, format, args));
        } else if (DEBUG) {
            systemLogger.d(TAG, buildMessage(prefix, format, args));
        }
    }

    public static void d(Object objectPrefix, String format, Object... args) {
        if (mIsUserExtendedLoggingEnabled) {
            maybeDisableLogging();
            systemLogger.i(TAG, buildMessage(getPrefixFromObject(objectPrefix), format, args));
        } else if (DEBUG) {
            systemLogger.d(TAG, buildMessage(getPrefixFromObject(objectPrefix), format, args));
        }
    }

    public static void i(String prefix, String format, Object... args) {
        if (INFO) {
            systemLogger.i(TAG, buildMessage(prefix, format, args));
        }
    }

    public static void i(Object objectPrefix, String format, Object... args) {
        if (INFO) {
            systemLogger.i(TAG, buildMessage(getPrefixFromObject(objectPrefix), format, args));
        }
    }

    public static void v(String prefix, String format, Object... args) {
        if (mIsUserExtendedLoggingEnabled) {
            maybeDisableLogging();
            systemLogger.i(TAG, buildMessage(prefix, format, args));
        } else if (VERBOSE) {
            systemLogger.v(TAG, buildMessage(prefix, format, args));
        }
    }

    public static void v(Object objectPrefix, String format, Object... args) {
        if (mIsUserExtendedLoggingEnabled) {
            maybeDisableLogging();
            systemLogger.i(TAG, buildMessage(getPrefixFromObject(objectPrefix), format, args));
        } else if (VERBOSE) {
            systemLogger.v(TAG, buildMessage(getPrefixFromObject(objectPrefix), format, args));
        }
    }

    public static void w(String prefix, String format, Object... args) {
        if (WARN) {
            systemLogger.w(TAG, buildMessage(prefix, format, args));
        }
    }

    public static void w(Object objectPrefix, String format, Object... args) {
        if (WARN) {
            systemLogger.w(TAG, buildMessage(getPrefixFromObject(objectPrefix), format, args));
        }
    }

    public static void e(String prefix, Throwable tr, String format, Object... args) {
        if (ERROR) {
            systemLogger.e(TAG, buildMessage(prefix, format, args), tr);
        }
    }

    public static void e(Object objectPrefix, Throwable tr, String format, Object... args) {
        if (ERROR) {
            systemLogger.e(TAG, buildMessage(getPrefixFromObject(objectPrefix), format, args),
                    tr);
        }
    }

    public static void wtf(String prefix, Throwable tr, String format, Object... args) {
        systemLogger.wtf(TAG, buildMessage(prefix, format, args), tr);
    }

    public static void wtf(Object objectPrefix, Throwable tr, String format, Object... args) {
        systemLogger.wtf(TAG, buildMessage(getPrefixFromObject(objectPrefix), format, args),
                tr);
    }

    public static void wtf(String prefix, String format, Object... args) {
        String msg = buildMessage(prefix, format, args);
        systemLogger.wtf(TAG, msg, new IllegalStateException(msg));
    }

    public static void wtf(Object objectPrefix, String format, Object... args) {
        String msg = buildMessage(getPrefixFromObject(objectPrefix), format, args);
        systemLogger.wtf(TAG, msg, new IllegalStateException(msg));
    }

    public static String piiHandle(Object pii) {
        if (pii == null || VERBOSE) {
            return String.valueOf(pii);
        }

        StringBuilder sb = new StringBuilder();
        if (pii instanceof Uri) {
            Uri uri = (Uri) pii;
            String scheme = uri.getScheme();

            if (!TextUtils.isEmpty(scheme)) {
                sb.append(scheme).append(":");
            }

            String textToObfuscate = uri.getSchemeSpecificPart();
            if (PhoneAccount.SCHEME_TEL.equals(scheme)) {
                for (int i = 0; i < textToObfuscate.length(); i++) {
                    char c = textToObfuscate.charAt(i);
                    sb.append(PhoneNumberUtils.isDialable(c) ? "*" : c);
                }
            } else if (PhoneAccount.SCHEME_SIP.equals(scheme)) {
                for (int i = 0; i < textToObfuscate.length(); i++) {
                    char c = textToObfuscate.charAt(i);
                    if (c != '@' && c != '.') {
                        c = '*';
                    }
                    sb.append(c);
                }
            } else {
                sb.append(pii(pii));
            }
        }

        return sb.toString();
    }

    /**
     * Redact personally identifiable information for production users.
     * If we are running in verbose mode, return the original string, otherwise
     * return a SHA-1 hash of the input string.
     */
    public static String pii(Object pii) {
        if (pii == null || VERBOSE) {
            return String.valueOf(pii);
        }
        return "[" + secureHash(String.valueOf(pii).getBytes()) + "]";
    }

    public static void dumpCallEvents(IndentingPrintWriter pw) {
        pw.println("Historical Calls:");
        pw.increaseIndent();
        for (CallEventRecord callEventRecord : mCallEventRecords) {
            callEventRecord.dump(pw);
        }
        pw.decreaseIndent();
    }

    private static String secureHash(byte[] input) {
        if (sMessageDigest != null) {
            sMessageDigest.reset();
            sMessageDigest.update(input);
            byte[] result = sMessageDigest.digest();
            return encodeHex(result);
        } else {
            return "Uninitialized SHA1";
        }
    }

    private static String encodeHex(byte[] bytes) {
        StringBuffer hex = new StringBuffer(bytes.length * 2);

        for (int i = 0; i < bytes.length; i++) {
            int byteIntValue = bytes[i] & 0xff;
            if (byteIntValue < 0x10) {
                hex.append("0");
            }
            hex.append(Integer.toString(byteIntValue, 16));
        }

        return hex.toString();
    }

    private static String getPrefixFromObject(Object obj) {
        return obj == null ? "<null>" : obj.getClass().getSimpleName();
    }

    private static String buildMessage(String prefix, String format, Object... args) {
        if (LOG_DBG) {
            checkIsThreadLogged();
        }
        // Incorporate thread ID and calling method into prefix
        String sessionPostfix = "";
        Session currentSession = sSessionMapper.get(getCallingThreadId());
        if (currentSession != null) {
            sessionPostfix = ": " + currentSession.toString();
        }

        String msg;
        try {
            msg = (args == null || args.length == 0) ? format
                    : String.format(Locale.US, format, args);
        } catch (IllegalFormatException ife) {
            e("Log", ife, "IllegalFormatException: formatString='%s' numArgs=%d", format,
                    args.length);
            msg = format + " (An error occurred while formatting the message.)";
        }
        return String.format(Locale.US, "%s: %s%s", prefix, msg, sessionPostfix);
    }
}
