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

import android.content.ContentResolver;
import android.content.Context;
import android.os.Handler;
import android.os.HandlerThread;
import android.os.Message;
import android.test.suitebuilder.annotation.MediumTest;

import com.android.internal.os.SomeArgs;
import com.android.server.telecom.Runnable;
import com.android.server.telecom.Session;
import com.android.server.telecom.SystemLoggingContainer;
import com.android.server.telecom.Log;

import org.junit.Assert;
import org.mockito.MockitoAnnotations;

import java.lang.ref.WeakReference;
import java.util.ArrayList;
import java.util.Random;
import java.util.concurrent.atomic.AtomicInteger;

import static org.mockito.Matchers.eq;
import static org.mockito.Mockito.verify;

/**
 * Unit tests for Telecom's Logging system.
 */
public class LogTest extends TelecomTestCase{

    /**
     * This helper class captures the logs that are sent to Log and stores them in an array to be
     * verified by LogTest.
     */
    private class TestLoggingContainer extends SystemLoggingContainer {
        public ArrayList<String> receivedStrings;

        public TestLoggingContainer() {
            receivedStrings = new ArrayList<>(100);
        }

        @Override
        public synchronized void v(String msgTag, String msg) {
            if (msgTag.equals(LogTest.TESTING_TAG)) {
                synchronized (this) {
                    receivedStrings.add(processMessage(msg));
                }
            }
        }

        @Override
        public synchronized void i(String msgTag, String msg) {
            if (msgTag.equals(LogTest.TESTING_TAG)) {
                synchronized (this) {
                    receivedStrings.add(processMessage(msg));
                }
            }
        }

        public boolean didReceiveMessage(int timeoutMs, String msg) {
            String matchedString = null;
            // Wait for timeout to expire before checking received messages
            if (timeoutMs > 0) {
                try {
                    Thread.sleep(timeoutMs);
                } catch (InterruptedException e) {
                    Log.w(LogTest.TESTING_TAG, "TestLoggingContainer: Thread Interrupted!");
                }
            }
            synchronized (this) {
                for (String receivedString : receivedStrings) {
                    if (receivedString.contains(msg)) {
                        matchedString = receivedString;
                        break;
                    }
                }
                if (matchedString != null) {
                    receivedStrings.remove(matchedString);
                    return true;
                }
            }
            android.util.Log.i(TESTING_TAG, "Did not receive message: " + msg);
            return false;
        }

        public boolean isMessagesEmpty() {
            boolean isEmpty = receivedStrings.isEmpty();
            if (!isEmpty) {
                printMessagesThatAreLeft();
            }
            return isEmpty;
        }

        public synchronized void printMessagesThatAreLeft() {
            android.util.Log.i(TESTING_TAG, "Remaining Messages in Log Queue:");
            for (String receivedString : receivedStrings) {
                android.util.Log.i(TESTING_TAG, "\t- " + receivedString);
            }
        }

        // Remove Unnecessary parts of message string before processing
        private String processMessage(String msg) {
            if(msg.contains(Session.CREATE_SUBSESSION)) {
                return clipMsg(Session.CREATE_SUBSESSION, msg);
            }
            if(msg.contains(Session.CONTINUE_SUBSESSION)) {
                return clipMsg(Session.CONTINUE_SUBSESSION, msg);
            }
            if (msg.contains(Session.END_SUBSESSION)) {
                return clipMsg(Session.END_SUBSESSION, msg);
            }
            if (msg.contains(Session.END_SESSION)) {
                return clipMsg(Session.END_SESSION, msg);
            }
            return msg;
        }

        private String clipMsg(String id, String msg) {
                int clipStartIndex = msg.indexOf(id) + id.length();
                int clipEndIndex = msg.lastIndexOf(":");
                return msg.substring(0, clipStartIndex) + msg.substring(clipEndIndex, msg.length());
        }
    }

    public static final int TEST_THREAD_COUNT = 150;
    public static final int TEST_SLEEP_TIME_MS = 50;
    // Should be larger than TEST_SLEEP_TIME_MS!
    public static final int TEST_VERIFY_TIMEOUT_MS = 100;
    public static final String TEST_ENTER_METHOD1 = "TEM1";
    public static final String TEST_ENTER_METHOD2 = "TEM2";
    public static final String TEST_ENTER_METHOD3 = "TEM3";
    public static final String TEST_ENTER_METHOD4 = "TEM4";
    public static final String TEST_CLASS_NAME = "LogTest";

    private static final int EVENT_START_TEST_SLEEPY_METHOD = 0;
    private static final int EVENT_START_TEST_SLEEPY_MULTIPLE_METHOD = 1;
    private static final int EVENT_LAST_MESSAGE = 2;

    private static final long RANDOM_NUMBER_SEED = 6191991;

    Random rng = new Random(RANDOM_NUMBER_SEED);

    private Handler mSleepyHandler = new Handler(
            new HandlerThread("sleepyThread"){{start();}}.getLooper()) {
        @Override
        public void handleMessage(Message msg) {
            SomeArgs args = (SomeArgs) msg.obj;
            Session subsession = (Session) args.arg1;
            Log.continueSession(subsession, "lTSH.hM");
            switch (msg.what) {
                case EVENT_START_TEST_SLEEPY_METHOD:
                    sleepyMethod(TEST_SLEEP_TIME_MS);
                    break;
            }
            Log.endSession();
        }
    };

    private boolean isHandlerCompleteWithEvents;
    private Handler mSleepyMultipleHandler = new Handler(
            new HandlerThread("sleepyMultipleThread"){{start();}}.getLooper()){
        @Override
        public void handleMessage(Message msg) {
            switch (msg.what) {
                case EVENT_START_TEST_SLEEPY_MULTIPLE_METHOD:
                    SomeArgs args = (SomeArgs) msg.obj;
                    Session subsession = (Session) args.arg1;
                    Log.continueSession(subsession, "lTSCH.hM");
                    sleepyMultipleMethod();
                    Log.endSession();
                    break;
                case EVENT_LAST_MESSAGE:
                    isHandlerCompleteWithEvents = true;
                    break;
            }
        }
    };

    private AtomicInteger mCompleteCount;
    class LogTestRunnable implements java.lang.Runnable {
        private String mshortMethodName;
        public LogTestRunnable(String shortMethodName) {
            mshortMethodName = shortMethodName;
        }

        public void run() {
            Log.startSession(mshortMethodName);
            sleepyCallerMethod(TEST_SLEEP_TIME_MS);
            Log.endSession();
            mCompleteCount.incrementAndGet();
        }
    }

    TestLoggingContainer mTestSystemLogger;

    @Override
    public void setUp() throws Exception {
        super.setUp();
        MockitoAnnotations.initMocks(this);
        mTestSystemLogger = new TestLoggingContainer();
        Log.setLoggingContainer(mTestSystemLogger);
        Log.setIsExtendedLoggingEnabled(true);
        Log.restartSessionCounter();
        Log.sCleanStaleSessions = null;
        Log.sSessionMapper.clear();
        Log.setContext(mComponentContextFixture.getTestDouble().getApplicationContext());
        Log.sSessionCleanupTimeoutMs = new Log.ISessionCleanupTimeoutMs() {
            @Override
            // Set to the default value of Timeouts.getStaleSessionCleanupTimeoutMillis without
            // needing to query.
            public long get() {
                return Log.DEFAULT_SESSION_TIMEOUT_MS;
            }
        };
    }

    @Override
    public void tearDown() throws Exception {
        mTestSystemLogger = null;
        Log.setLoggingContainer(new SystemLoggingContainer());
        super.tearDown();
    }

    @MediumTest
    public void testSingleThreadSession() throws Exception {
        String sessionName = "LT.sTS";
        Log.startSession(sessionName);
        sleepyMethod(TEST_SLEEP_TIME_MS);
        Log.endSession();

        verifyEventResult(Session.START_SESSION, sessionName, "", 0, TEST_VERIFY_TIMEOUT_MS);
        verifyMethodCall("", sessionName, 0, "", TEST_ENTER_METHOD1, TEST_VERIFY_TIMEOUT_MS);
        verifyEventResult(Session.END_SUBSESSION, sessionName, "", 0, TEST_VERIFY_TIMEOUT_MS);
        verifyEndEventResult(sessionName, "", 0, TEST_VERIFY_TIMEOUT_MS);

        assertEquals(Log.sSessionMapper.size(), 0);
        assertEquals(true, mTestSystemLogger.isMessagesEmpty());
    }

    @MediumTest
    public void testSingleHandlerThreadSession() throws Exception {
        String sessionName = "LT.tSHTS";
        Log.startSession(sessionName);
        Session subsession = Log.createSubsession();
        SomeArgs args = SomeArgs.obtain();
        args.arg1 = subsession;
        mSleepyHandler.obtainMessage(EVENT_START_TEST_SLEEPY_METHOD, args).sendToTarget();
        Log.endSession();

        verifyEventResult(Session.START_SESSION, sessionName, "", 0, TEST_VERIFY_TIMEOUT_MS);
        verifyEventResult(Session.CREATE_SUBSESSION, sessionName, "", 0, TEST_VERIFY_TIMEOUT_MS);
        verifyContinueEventResult(sessionName, "lTSH.hM", "_0", 0, TEST_VERIFY_TIMEOUT_MS);
        verifyEventResult(Session.END_SUBSESSION, sessionName, "", 0, TEST_VERIFY_TIMEOUT_MS);
        verifyMethodCall(sessionName, "lTSH.hM", 0, "_0", TEST_ENTER_METHOD1,
                TEST_VERIFY_TIMEOUT_MS);
        verifyEventResult(Session.END_SUBSESSION, sessionName + "->lTSH.hM", "_0", 0,
                TEST_VERIFY_TIMEOUT_MS);
        verifyEndEventResult(sessionName, "", 0, TEST_VERIFY_TIMEOUT_MS);

        assertEquals(Log.sSessionMapper.size(), 0);
        assertEquals(true, mTestSystemLogger.isMessagesEmpty());
    }

    @MediumTest
    public void testSpawnMultipleThreadSessions() throws Exception {
        final String sessionName = "LT.lTR";
        mCompleteCount = new AtomicInteger(0);
        for (int i = 0; i < TEST_THREAD_COUNT; i++) {
            Thread.sleep(10);
            new Thread(new LogTestRunnable(sessionName)).start();
        }

        // Poll until all of the threads have completed
        while (mCompleteCount.get() < TEST_THREAD_COUNT) {
            Thread.sleep(1000);
        }

        // Loop through verification separately to spawn threads fast so there is possible overlap
        // (verifyEventResult(...) delays)
        for (int i = 0; i < TEST_THREAD_COUNT; i++) {
            verifyEventResult(Session.START_SESSION, sessionName, "", i, 0);
            verifyMethodCall("", sessionName, i, "", TEST_ENTER_METHOD2, 0);
            verifyMethodCall("", sessionName, i, "", TEST_ENTER_METHOD1, 0);
            verifyEventResult(Session.END_SUBSESSION, sessionName, "", i, 0);
            verifyEndEventResult(sessionName, "", i, 0);
        }

        assertEquals(Log.sSessionMapper.size(), 0);
        assertEquals(true, mTestSystemLogger.isMessagesEmpty());
    }

    @MediumTest
    public void testSpawnMultipleThreadMultipleHandlerSession() throws Exception {
        String sessionName = "LT.tSMTMHS";
        Log.startSession(sessionName);
        Session subsession = Log.createSubsession();
        SomeArgs args = SomeArgs.obtain();
        args.arg1 = subsession;
        mSleepyMultipleHandler.obtainMessage(EVENT_START_TEST_SLEEPY_MULTIPLE_METHOD,
                args).sendToTarget();
        Log.endSession();

        verifyEventResult(Session.START_SESSION, sessionName, "", 0, TEST_VERIFY_TIMEOUT_MS);
        verifyEventResult(Session.END_SUBSESSION, sessionName, "", 0, TEST_VERIFY_TIMEOUT_MS);
        verifyEventResult(Session.CREATE_SUBSESSION, sessionName, "", 0, TEST_VERIFY_TIMEOUT_MS);
        verifyContinueEventResult(sessionName, "lTSCH.hM", "_0", 0, TEST_VERIFY_TIMEOUT_MS);
        verifyMethodCall(sessionName, "lTSCH.hM", 0, "_0", TEST_ENTER_METHOD3,
                TEST_VERIFY_TIMEOUT_MS);
        verifyEventResult(Session.END_SUBSESSION, sessionName + "->lTSCH.hM", "_0", 0,
                TEST_VERIFY_TIMEOUT_MS);
        verifyEventResult(Session.CREATE_SUBSESSION, sessionName + "->lTSCH.hM", "_0", 0,
                TEST_VERIFY_TIMEOUT_MS);
        verifyContinueEventResult(sessionName + "->" + "lTSCH.hM", "lTSH.hM", "_0_0", 0,
                TEST_VERIFY_TIMEOUT_MS);
        verifyMethodCall(sessionName + "->lTSCH.hM", "lTSH.hM", 0, "_0_0", TEST_ENTER_METHOD1,
                TEST_VERIFY_TIMEOUT_MS);
        verifyEventResult(Session.END_SUBSESSION, sessionName + "->lTSCH.hM->lTSH.hM", "_0_0", 0,
                TEST_VERIFY_TIMEOUT_MS);
        verifyEndEventResult(sessionName, "", 0, TEST_VERIFY_TIMEOUT_MS);

        assertEquals(Log.sSessionMapper.size(), 0);
        assertEquals(true, mTestSystemLogger.isMessagesEmpty());
    }

    @MediumTest
    public void testSpawnMultipleThreadMultipleHandlerSessions() throws Exception {
        String sessionName = "LT.tSMTMHSs";
        isHandlerCompleteWithEvents = false;
        for (int i = 0; i < TEST_THREAD_COUNT; i++) {
            Log.startSession(sessionName);
            Session subsession = Log.createSubsession();
            SomeArgs args = SomeArgs.obtain();
            args.arg1 = subsession;
            mSleepyMultipleHandler.obtainMessage(EVENT_START_TEST_SLEEPY_MULTIPLE_METHOD,
                    args).sendToTarget();
            Log.endSession();
        }
        // Send a message that denotes the last message that is sent. We poll until this message
        // is processed in order to verify the results without waiting an arbitrary amount of time
        // (that can change per device).
        mSleepyMultipleHandler.obtainMessage(EVENT_LAST_MESSAGE).sendToTarget();

        while (!isHandlerCompleteWithEvents) {
            Thread.sleep(1000);
        }

        for (int i = 0; i < TEST_THREAD_COUNT; i++) {
            verifyEventResult(Session.START_SESSION, sessionName, "", i, 0);
            verifyEventResult(Session.END_SUBSESSION, sessionName, "", i, 0);
            verifyEventResult(Session.CREATE_SUBSESSION, sessionName, "", i, 0);
            verifyContinueEventResult(sessionName, "lTSCH.hM", "_0", i, 0);
            verifyMethodCall(sessionName, "lTSCH.hM", i, "_0", TEST_ENTER_METHOD3, 0);
            verifyEventResult(Session.END_SUBSESSION, sessionName + "->lTSCH.hM", "_0", i, 0);
            verifyEventResult(Session.CREATE_SUBSESSION, sessionName + "->lTSCH.hM", "_0", i, 0);
            verifyContinueEventResult(sessionName + "->" + "lTSCH.hM", "lTSH.hM", "_0_0", i, 0);
            verifyMethodCall(sessionName + "->lTSCH.hM", "lTSH.hM", i, "_0_0", TEST_ENTER_METHOD1,
                    0);
            verifyEventResult(Session.END_SUBSESSION, sessionName + "->lTSCH.hM->lTSH.hM", "_0_0",
                    i, 0);
            verifyEndEventResult(sessionName, "", i, 0);
        }

        assertEquals(Log.sSessionMapper.size(), 0);
        assertEquals(true, mTestSystemLogger.isMessagesEmpty());
    }

    @MediumTest
    public void testCancelSubsession() throws Exception {
        String sessionName = "LT.tCS";
        Log.startSession(sessionName);
        Session subsession = Log.createSubsession();
        Log.cancelSubsession(subsession);
        Log.endSession();

        verifyEventResult(Session.START_SESSION, sessionName, "", 0, 0);
        verifyEventResult(Session.CREATE_SUBSESSION, sessionName, "", 0, 0);
        verifyEventResult(Session.END_SUBSESSION, sessionName, "", 0, 0);
        verifyEndEventResult(sessionName, "", 0, 0);

        assertEquals(Log.sSessionMapper.size(), 0);
        assertEquals(true, mTestSystemLogger.isMessagesEmpty());
    }

    @MediumTest
    public void testInternalExternalCallToMethod() throws Exception {
        String sessionName = "LT.tIECTM";
        Log.startSession(sessionName);
        internalExternalMethod();
        Log.endSession();

        verifyEventResult(Session.START_SESSION, sessionName, "", 0, 0);
        verifyEventResult(Session.CREATE_SUBSESSION, sessionName, "", 0, 0);
        verifyContinueEventResult(sessionName, "", "", 0, 0);
        verifyEventResult(Session.END_SUBSESSION, sessionName, "", 0, 0);
        verifyMethodCall("", sessionName, 0, "", TEST_ENTER_METHOD4, 0);
        verifyEventResult(Session.END_SUBSESSION, sessionName, "", 0, 0);
        verifyEndEventResult(sessionName, "", 0, 0);

        assertEquals(Log.sSessionMapper.size(), 0);
        assertEquals(true, mTestSystemLogger.isMessagesEmpty());
    }

    @MediumTest
    public void testGarbageCollectionWithTimeout() throws Exception {
        String sessionName = "LT.tGCWT";

        // Don't end session (Oops!)
        Log.startSession(sessionName);
        internalDanglingMethod();
        Log.sSessionCleanupHandler.postDelayed(new java.lang.Runnable() {
            @Override
            public void run() {
                android.util.Log.i(TESTING_TAG, "Running Test SessionCleanupHandler method.");
                Log.cleanupStaleSessions(1000);
            }
        }, 1000);

        verifyEventResult(Session.START_SESSION, sessionName, "", 0, 0);
        verifyEventResult(Session.CREATE_SUBSESSION, sessionName, "", 0, 0);
        verifyContinueEventResult(sessionName, "", "", 0, 0);
        verifyMethodCall("", sessionName, 0, "", TEST_ENTER_METHOD4, 0);

        // Verify the session is still active in sSessionMapper
        assertEquals(Log.sSessionMapper.size(), 1);
        assertEquals(true, mTestSystemLogger.isMessagesEmpty());

        // Keep a weak reference to the object to check if it eventually gets garbage collected.
        int threadId = Log.getCallingThreadId();
        WeakReference<Session> sessionRef = new WeakReference<>(
                Log.sSessionMapper.get(threadId));

        Thread.sleep(1100);
        assertEquals(0, Log.sSessionMapper.size());
        // "Suggest" that the GC collects the now isolated Session and subsession and wait for it
        // to occur. "System.gc()" was previously used, but it does not always perform GC, so the
        // internal method is now called.
        Runtime.getRuntime().gc();
        Thread.sleep(1000);
        assertEquals(null, sessionRef.get());
    }

    private void verifyMethodCall(String parentSessionName, String methodName, int sessionId,
            String subsession, String shortMethodName, int timeoutMs) {
        if (!parentSessionName.isEmpty()){
            parentSessionName += "->";
        }
        boolean isMessageReceived = mTestSystemLogger.didReceiveMessage(timeoutMs,
                buildExpectedResult(parentSessionName + methodName, sessionId, subsession,
                        shortMethodName));

        assertEquals(true, isMessageReceived);
    }

    private String buildExpectedSession(String shortMethodName, int sessionId) {
        return shortMethodName + "@" + Log.getBase64Encoding(sessionId);
    }

    private String buildExpectedResult(String shortMethodName, int sessionId,
            String subsessionId, String logText) {
        return TEST_CLASS_NAME + ": " +  logText + ": " +
                buildExpectedSession(shortMethodName, sessionId) + subsessionId;
    }

    private void verifyContinueEventResult(String shortOldMethodName, String shortNewMethodName,
                String subsession, int sessionId, int timeoutMs) {
        String expectedSession = buildExpectedSession(shortNewMethodName, sessionId);
        if(!shortNewMethodName.isEmpty()) {
            shortOldMethodName += "->";
        }
        boolean isMessageReceived = mTestSystemLogger.didReceiveMessage(timeoutMs,
                Session.CONTINUE_SUBSESSION + ": " + shortOldMethodName + expectedSession +
                        subsession);
        assertEquals(true, isMessageReceived);
    }

    private void verifyEventResult(String event, String shortMethodName,  String subsession,
            int sessionId, int timeoutMs) {
        String expectedSession = buildExpectedSession(shortMethodName, sessionId);
        boolean isMessageReceived = mTestSystemLogger.didReceiveMessage(timeoutMs,event + ": "  +
                expectedSession + subsession);
        assertEquals(true, isMessageReceived);
    }

    private void verifyEndEventResult(String shortMethodName, String subsession, int sessionId,
            int timeoutMs) {
        String expectedSession = buildExpectedSession(shortMethodName, sessionId);
        boolean isMessageReceived = mTestSystemLogger.didReceiveMessage(timeoutMs,
                Session.END_SESSION + ": " + expectedSession + subsession);
        assertEquals(true, isMessageReceived);
    }

    private void internalExternalMethod() {
        Log.startSession("LT.iEM");
        Log.i(TEST_CLASS_NAME, TEST_ENTER_METHOD4);
        Log.endSession();
    }

    private void internalDanglingMethod() {
        Log.startSession("LT.iEM");
        Log.i(TEST_CLASS_NAME, TEST_ENTER_METHOD4);
    }

    private void sleepyCallerMethod(int timeToSleepMs) {
        Log.i(TEST_CLASS_NAME, TEST_ENTER_METHOD2);
        try {
            Thread.sleep(timeToSleepMs);
            sleepyMethod(rng.nextInt(TEST_SLEEP_TIME_MS));
        } catch (InterruptedException e) {
            // This should not happen
            Assert.fail("Thread sleep interrupted: " + e.getMessage());
        }

    }

    private void sleepyMultipleMethod() {
        Log.i(TEST_CLASS_NAME, TEST_ENTER_METHOD3);
        Session subsession = Log.createSubsession();
        SomeArgs args = SomeArgs.obtain();
        args.arg1 = subsession;
        mSleepyHandler.obtainMessage(EVENT_START_TEST_SLEEPY_METHOD, args).sendToTarget();
        try {
            Thread.sleep(TEST_SLEEP_TIME_MS);
        } catch (InterruptedException e) {
            // This should not happen
            Assert.fail("Thread sleep interrupted: " + e.getMessage());
        }
    }

    private void sleepyMethod(int timeToSleepMs) {
        Log.i(TEST_CLASS_NAME, TEST_ENTER_METHOD1);
        try {
            Thread.sleep(timeToSleepMs);
        } catch (InterruptedException e) {
            // This should not happen
            Assert.fail("Thread sleep interrupted: " + e.getMessage());
        }
    }
}