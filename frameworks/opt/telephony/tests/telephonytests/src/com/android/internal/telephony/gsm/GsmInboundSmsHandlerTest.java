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

import android.content.BroadcastReceiver;
import android.content.ContentValues;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.database.Cursor;
import android.database.MatrixCursor;
import android.net.Uri;
import android.os.AsyncResult;
import android.os.Bundle;
import android.os.Handler;
import android.os.HandlerThread;
import android.os.UserManager;
import android.os.RemoteException;
import android.os.UserHandle;
import android.provider.Telephony;
import android.test.mock.MockContentProvider;
import android.test.mock.MockContentResolver;
import android.test.suitebuilder.annotation.MediumTest;

import com.android.internal.telephony.InboundSmsHandler;
import com.android.internal.telephony.InboundSmsTracker;
import com.android.internal.telephony.SmsBroadcastUndelivered;
import com.android.internal.telephony.SmsHeader;
import com.android.internal.telephony.SmsStorageMonitor;
import com.android.internal.telephony.TelephonyTest;
import com.android.internal.telephony.cdma.CdmaInboundSmsHandler;
import com.android.internal.util.HexDump;
import com.android.internal.util.IState;
import com.android.internal.util.StateMachine;

import static com.android.internal.telephony.TelephonyTestUtils.waitForMs;
import static org.junit.Assert.*;
import static org.mockito.Mockito.*;

import org.junit.After;
import org.junit.Before;
import org.junit.Test;
import org.mockito.ArgumentCaptor;
import org.mockito.Mock;

import java.lang.reflect.Method;
import java.util.ArrayList;
import java.util.List;

public class GsmInboundSmsHandlerTest extends TelephonyTest {
    @Mock
    private SmsStorageMonitor mSmsStorageMonitor;
    @Mock
    private android.telephony.SmsMessage mSmsMessage;
    @Mock
    private SmsMessage mGsmSmsMessage;
    @Mock
    private SmsHeader mSmsHeader;
    @Mock
    private InboundSmsTracker mInboundSmsTrackerPart1;
    @Mock
    private InboundSmsTracker mInboundSmsTrackerPart2;
    @Mock
    private CdmaInboundSmsHandler mCdmaInboundSmsHandler;

    private GsmInboundSmsHandler mGsmInboundSmsHandler;

    private FakeSmsContentProvider mContentProvider;
    private static final Uri sRawUri = Uri.withAppendedPath(Telephony.Sms.CONTENT_URI, "raw");
    private static final Uri sRawUriPermanentDelete =
            Uri.withAppendedPath(Telephony.Sms.CONTENT_URI, "raw/permanentDelete");

    private ContentValues mInboundSmsTrackerCV = new ContentValues();
    // For multi-part SMS
    private ContentValues mInboundSmsTrackerCVPart1;
    private ContentValues mInboundSmsTrackerCVPart2;
    private String mMessageBody = "This is the message body of a single-part message";
    private String mMessageBodyPart1 = "This is the first part of a multi-part message";
    private String mMessageBodyPart2 = "This is the second part of a multi-part message";

    byte[] mSmsPdu = new byte[]{(byte)0xFF, (byte)0xFF, (byte)0xFF};

    public static class FakeSmsContentProvider extends MockContentProvider {
        private String[] mRawColumns = {"_id",
                "date",
                "reference_number",
                "count",
                "sequence",
                "destination_port",
                "address",
                "sub_id",
                "pdu",
                "deleted",
                "message_body"};
        private List<ArrayList<Object>> mListOfRows = new ArrayList<ArrayList<Object>>();
        private int mNumRows = 0;

        private int getColumnIndex(String columnName) {
            int i = 0;
            for (String s : mRawColumns) {
                if (s.equals(columnName)) {
                    break;
                }
                i++;
            }
            return i;
        }

        @Override
        public int delete(Uri uri, String selection, String[] selectionArgs) {
            int count = 0;
            if (mNumRows > 0) {
                // parse selection and selectionArgs
                SelectionParams selectionParams = new SelectionParams();
                selectionParams.parseSelectionParams(selection, selectionArgs);

                List<Integer> deleteRows = new ArrayList<Integer>();
                int i = -1;
                for (ArrayList<Object> row : mListOfRows) {
                    i++;
                    // filter based on selection parameters if needed
                    if (selection != null) {
                        if (!selectionParams.isMatch(row)) {
                            continue;
                        }
                    }
                    if (uri.compareTo(sRawUri) == 0) {
                        row.set(getColumnIndex("deleted"), "1");
                    } else {
                        // save index for removal
                        deleteRows.add(i);
                    }
                    count++;
                }

                if (uri.compareTo(sRawUriPermanentDelete) == 0) {
                    for (i = deleteRows.size() - 1; i >= 0; i--) {
                        mListOfRows.remove(i);
                    }
                }
            }
            return count;
        }

        @Override
        public Uri insert(Uri uri, ContentValues values) {
            Uri newUri = null;
            if (uri.compareTo(sRawUri) == 0) {
                if (values != null) {
                    mListOfRows.add(convertRawCVtoArrayList(values));
                    mNumRows++;
                    newUri = Uri.withAppendedPath(uri, "" + mNumRows);
                }
            }
            logd("insert called, new numRows: " + mNumRows);
            return newUri;
        }

        private ArrayList<Object> convertRawCVtoArrayList(ContentValues values) {
            ArrayList<Object> newRow = new ArrayList<>();
            for (String key : mRawColumns) {
                if (values.containsKey(key)) {
                    newRow.add(values.getAsString(key));
                } else if (key.equals("_id")) {
                    newRow.add(mNumRows + 1);
                } else if (key.equals("deleted")) {
                    newRow.add("0");
                } else {
                    newRow.add(null);
                }
            }
            return newRow;
        }

        private class SelectionParams {
            String[] paramName = null;
            String[] paramValue = null;

            private void parseSelectionParams(String selection, String[] selectionArgs) {
                if (selection != null) {
                    selection = selection.toLowerCase();
                    String[] selectionParams = selection.toLowerCase().split("and");
                    int i = 0;
                    int j = 0;
                    paramName = new String[selectionParams.length];
                    paramValue = new String[selectionParams.length];
                    for (String param : selectionParams) {
                        String[] paramWithArg = param.split("=");
                        paramName[i] = paramWithArg[0].trim();
                        if (param.contains("?")) {
                            paramValue[i] = selectionArgs[j];
                            j++;
                        } else {
                            paramValue[i] = paramWithArg[1].trim();
                        }
                        //logd(paramName[i] + " = " + paramValue[i]);
                        i++;
                    }
                }
            }

            private boolean isMatch(ArrayList<Object> row) {
                for (int i = 0; i < paramName.length; i++) {
                    int columnIndex = 0;
                    for (String columnName : mRawColumns) {
                        if (columnName.equals(paramName[i])) {
                            if ((paramValue[i] == null && row.get(columnIndex) != null) ||
                                    (paramValue[i] != null &&
                                            !paramValue[i].equals(row.get(columnIndex)))) {
                                logd("Not a match due to " + columnName + ": " + paramValue[i] +
                                        ", " + row.get(columnIndex));
                                return false;
                            } else {
                                // move on to next param
                                break;
                            }
                        }
                        columnIndex++;
                    }
                }
                return true;
            }
        }

        @Override
        public Cursor query(Uri uri, String[] projection, String selection, String[] selectionArgs,
                            String sortOrder) {
            logd("query called for: " + selection);
            MatrixCursor cursor = new MatrixCursor(projection);
            if (mNumRows > 0) {
                // parse selection and selectionArgs
                SelectionParams selectionParams = new SelectionParams();
                selectionParams.parseSelectionParams(selection, selectionArgs);

                for (ArrayList<Object> row : mListOfRows) {
                    ArrayList<Object> retRow = new ArrayList<>();
                    // filter based on selection parameters if needed
                    if (selection != null) {
                        if (!selectionParams.isMatch(row)) {
                            continue;
                        }
                    }

                    for (String columnName : projection) {
                        int columnIndex = getColumnIndex(columnName);
                        retRow.add(row.get(columnIndex));
                    }
                    cursor.addRow(retRow);
                }
            }
            if (cursor != null) {
                logd("returning rows: " + cursor.getCount());
            }
            return cursor;
        }

        @Override
        public Bundle call(String method, String request, Bundle args) {
            return null;
        }
    }

    private class GsmInboundSmsHandlerTestHandler extends HandlerThread {

        private GsmInboundSmsHandlerTestHandler(String name) {
            super(name);
        }

        @Override
        public void onLooperPrepared() {
            mGsmInboundSmsHandler = GsmInboundSmsHandler.makeInboundSmsHandler(mContext,
                    mSmsStorageMonitor, mPhone);
            setReady(true);
        }
    }

    private IState getCurrentState() {
        try {
            Method method = StateMachine.class.getDeclaredMethod("getCurrentState");
            method.setAccessible(true);
            return (IState) method.invoke(mGsmInboundSmsHandler);
        } catch (Exception e) {
            fail(e.toString());
            return null;
        }
    }

    @Before
    public void setUp() throws Exception {
        super.setUp("GsmInboundSmsHandlerTest");

        doReturn(true).when(mTelephonyManager).getSmsReceiveCapableForPhone(anyInt(), anyBoolean());
        doReturn(true).when(mSmsStorageMonitor).isStorageAvailable();

        UserManager userManager = (UserManager)mContext.getSystemService(Context.USER_SERVICE);
        doReturn(true).when(userManager).isUserUnlocked();

        try {
            doReturn(new int[]{UserHandle.USER_SYSTEM}).when(mIActivityManager).getRunningUserIds();
        } catch (RemoteException re) {
            fail("Unexpected RemoteException: " + re.getStackTrace());
        }

        mSmsMessage.mWrappedSmsMessage = mGsmSmsMessage;
        mInboundSmsTrackerCV.put("destination_port", 1 << 16);
        mInboundSmsTrackerCV.put("pdu", HexDump.toHexString(mSmsPdu));
        mInboundSmsTrackerCV.put("address", "1234567890");
        mInboundSmsTrackerCV.put("reference_number", 1);
        mInboundSmsTrackerCV.put("sequence", 1);
        mInboundSmsTrackerCV.put("count", 1);
        mInboundSmsTrackerCV.put("date", System.currentTimeMillis());
        mInboundSmsTrackerCV.put("message_body", mMessageBody);

        doReturn(1).when(mInboundSmsTracker).getMessageCount();
        doReturn(1).when(mInboundSmsTracker).getReferenceNumber();
        doReturn("1234567890").when(mInboundSmsTracker).getAddress();
        doReturn(1).when(mInboundSmsTracker).getSequenceNumber();
        doReturn(1).when(mInboundSmsTracker).getIndexOffset();
        doReturn(-1).when(mInboundSmsTracker).getDestPort();
        doReturn(mMessageBody).when(mInboundSmsTracker).getMessageBody();
        doReturn(mSmsPdu).when(mInboundSmsTracker).getPdu();
        doReturn(mInboundSmsTrackerCV.get("date")).when(mInboundSmsTracker).getTimestamp();
        doReturn(mInboundSmsTrackerCV).when(mInboundSmsTracker).getContentValues();

        mContentProvider = new FakeSmsContentProvider();
        ((MockContentResolver)mContext.getContentResolver()).addProvider(
                Telephony.Sms.CONTENT_URI.getAuthority(), mContentProvider);

        new GsmInboundSmsHandlerTestHandler(TAG).start();
        waitUntilReady();
    }

    @After
    public void tearDown() throws Exception {
        // wait for wakelock to be released; timeout at 10s
        int i = 0;
        while (mGsmInboundSmsHandler.getWakeLock().isHeld() && i < 100) {
            waitForMs(100);
            i++;
        }
        assertFalse(mGsmInboundSmsHandler.getWakeLock().isHeld());
        mGsmInboundSmsHandler = null;
        super.tearDown();
    }

    private void transitionFromStartupToIdle() {
        // verify initially in StartupState
        assertEquals("StartupState", getCurrentState().getName());

        // trigger transition to IdleState
        mGsmInboundSmsHandler.sendMessage(InboundSmsHandler.EVENT_START_ACCEPTING_SMS);
        waitForMs(50);

        assertEquals("IdleState", getCurrentState().getName());
    }

    private void verifySmsIntentBroadcasts(int numPastBroadcasts) {
        ArgumentCaptor<Intent> intentArgumentCaptor = ArgumentCaptor.forClass(Intent.class);
        verify(mContext, times(1 + numPastBroadcasts)).sendBroadcast(
                intentArgumentCaptor.capture());
        assertEquals(Telephony.Sms.Intents.SMS_DELIVER_ACTION,
                intentArgumentCaptor.getAllValues().get(numPastBroadcasts).getAction());
        assertEquals("WaitingState", getCurrentState().getName());

        mContextFixture.sendBroadcastToOrderedBroadcastReceivers();

        intentArgumentCaptor = ArgumentCaptor.forClass(Intent.class);
        verify(mContext, times(2 + numPastBroadcasts)).sendBroadcast(
                intentArgumentCaptor.capture());
        assertEquals(Telephony.Sms.Intents.SMS_RECEIVED_ACTION,
                intentArgumentCaptor.getAllValues().get(numPastBroadcasts + 1).getAction());
        assertEquals("WaitingState", getCurrentState().getName());

        mContextFixture.sendBroadcastToOrderedBroadcastReceivers();
        waitForMs(50);

        assertEquals("IdleState", getCurrentState().getName());
    }

    @Test
    @MediumTest
    public void testNewSms() {
        transitionFromStartupToIdle();

        // send new SMS to state machine and verify that triggers SMS_DELIVER_ACTION
        mGsmInboundSmsHandler.sendMessage(InboundSmsHandler.EVENT_NEW_SMS,
                new AsyncResult(null, mSmsMessage, null));
        waitForMs(100);

        verifySmsIntentBroadcasts(0);

        // send same SMS again, verify no broadcasts are sent
        mGsmInboundSmsHandler.sendMessage(InboundSmsHandler.EVENT_NEW_SMS,
                new AsyncResult(null, mSmsMessage, null));
        waitForMs(100);

        verify(mContext, times(2)).sendBroadcast(any(Intent.class));
        assertEquals("IdleState", getCurrentState().getName());
    }

    @Test
    @MediumTest
    public void testNewSmsFromBlockedNumber_noBroadcastsSent() {
        String blockedNumber = "123456789";
        doReturn(blockedNumber).when(mInboundSmsTracker).getAddress();
        mFakeBlockedNumberContentProvider.mBlockedNumbers.add(blockedNumber);

        transitionFromStartupToIdle();

        mGsmInboundSmsHandler.sendMessage(InboundSmsHandler.EVENT_NEW_SMS,
                new AsyncResult(null, mSmsMessage, null));
        waitForMs(100);

        verify(mContext, never()).sendBroadcast(any(Intent.class));
        assertEquals("IdleState", getCurrentState().getName());
    }

    private void verifyDataSmsIntentBroadcasts(int numPastBroadcasts) {
        ArgumentCaptor<Intent> intentArgumentCaptor = ArgumentCaptor.forClass(Intent.class);
        verify(mContext, times(1 + numPastBroadcasts)).sendBroadcast(
                intentArgumentCaptor.capture());
        assertEquals(Telephony.Sms.Intents.DATA_SMS_RECEIVED_ACTION,
                intentArgumentCaptor.getAllValues().get(numPastBroadcasts).getAction());
        assertEquals("WaitingState", getCurrentState().getName());

        mContextFixture.sendBroadcastToOrderedBroadcastReceivers();
        waitForMs(50);

        assertEquals("IdleState", getCurrentState().getName());
    }

    @Test
    @MediumTest
    public void testBroadcastSms() {
        transitionFromStartupToIdle();

        doReturn(0).when(mInboundSmsTracker).getDestPort();
        mGsmInboundSmsHandler.sendMessage(InboundSmsHandler.EVENT_BROADCAST_SMS,
                mInboundSmsTracker);
        waitForMs(100);

        verifyDataSmsIntentBroadcasts(0);

        // send same data sms again, and since it's not text sms it should be broadcast again
        mGsmInboundSmsHandler.sendMessage(InboundSmsHandler.EVENT_BROADCAST_SMS,
                mInboundSmsTracker);
        waitForMs(100);

        verifyDataSmsIntentBroadcasts(1);
    }

    @Test
    @MediumTest
    public void testInjectSms() {
        transitionFromStartupToIdle();

        mGsmInboundSmsHandler.sendMessage(InboundSmsHandler.EVENT_INJECT_SMS, new AsyncResult(null,
                mSmsMessage, null));
        waitForMs(100);

        verifySmsIntentBroadcasts(0);

        // inject same SMS again, verify no broadcasts are sent
        mGsmInboundSmsHandler.sendMessage(InboundSmsHandler.EVENT_INJECT_SMS, new AsyncResult(null,
                mSmsMessage, null));
        waitForMs(100);

        verify(mContext, times(2)).sendBroadcast(any(Intent.class));
        assertEquals("IdleState", getCurrentState().getName());
    }

    private void prepareMultiPartSms() {
        // Part 1
        mInboundSmsTrackerCVPart1 = new ContentValues();
        mInboundSmsTrackerCVPart1.put("destination_port", 1 << 16);
        mInboundSmsTrackerCVPart1.put("pdu", HexDump.toHexString(mSmsPdu));
        mInboundSmsTrackerCVPart1.put("address", "1234567890");
        mInboundSmsTrackerCVPart1.put("reference_number", 1);
        mInboundSmsTrackerCVPart1.put("sequence", 1);
        mInboundSmsTrackerCVPart1.put("count", 2);
        mInboundSmsTrackerCVPart1.put("date", System.currentTimeMillis());
        mInboundSmsTrackerCVPart1.put("message_body", mMessageBodyPart1);

        doReturn(2).when(mInboundSmsTrackerPart1).getMessageCount();
        doReturn(1).when(mInboundSmsTrackerPart1).getReferenceNumber();
        doReturn("1234567890").when(mInboundSmsTrackerPart1).getAddress();
        doReturn(1).when(mInboundSmsTrackerPart1).getSequenceNumber();
        doReturn(1).when(mInboundSmsTrackerPart1).getIndexOffset();
        doReturn(-1).when(mInboundSmsTrackerPart1).getDestPort();
        doReturn(mMessageBodyPart1).when(mInboundSmsTrackerPart1).getMessageBody();
        doReturn(mSmsPdu).when(mInboundSmsTrackerPart1).getPdu();
        doReturn(mInboundSmsTrackerCVPart1.get("date")).when(mInboundSmsTrackerPart1).
                getTimestamp();
        doReturn(mInboundSmsTrackerCVPart1).when(mInboundSmsTrackerPart1).getContentValues();

        // Part 2
        mInboundSmsTrackerCVPart2 = new ContentValues();
        mInboundSmsTrackerCVPart2.put("destination_port", 1 << 16);
        mInboundSmsTrackerCVPart2.put("pdu", HexDump.toHexString(mSmsPdu));
        mInboundSmsTrackerCVPart2.put("address", "1234567890");
        mInboundSmsTrackerCVPart2.put("reference_number", 1);
        mInboundSmsTrackerCVPart2.put("sequence", 2);
        mInboundSmsTrackerCVPart2.put("count", 2);
        mInboundSmsTrackerCVPart2.put("date", System.currentTimeMillis());
        mInboundSmsTrackerCVPart2.put("message_body", mMessageBodyPart2);

        doReturn(2).when(mInboundSmsTrackerPart2).getMessageCount();
        doReturn(1).when(mInboundSmsTrackerPart2).getReferenceNumber();
        doReturn("1234567890").when(mInboundSmsTrackerPart2).getAddress();
        doReturn(2).when(mInboundSmsTrackerPart2).getSequenceNumber();
        doReturn(1).when(mInboundSmsTrackerPart2).getIndexOffset();
        doReturn(-1).when(mInboundSmsTrackerPart2).getDestPort();
        doReturn(mMessageBodyPart2).when(mInboundSmsTrackerPart2).getMessageBody();
        doReturn(mSmsPdu).when(mInboundSmsTrackerPart2).getPdu();
        doReturn(mInboundSmsTrackerCVPart2.get("date")).when(mInboundSmsTrackerPart2).
                getTimestamp();
        doReturn(mInboundSmsTrackerCVPart2).when(mInboundSmsTrackerPart2).getContentValues();
    }

    @Test
    @MediumTest
    public void testMultiPartSms() {
        transitionFromStartupToIdle();

        // prepare SMS part 1 and part 2
        prepareMultiPartSms();

        mSmsHeader.concatRef = new SmsHeader.ConcatRef();
        doReturn(mSmsHeader).when(mGsmSmsMessage).getUserDataHeader();

        doReturn(mInboundSmsTrackerPart1).when(mTelephonyComponentFactory)
                .makeInboundSmsTracker(any(byte[].class), anyLong(), anyInt(), anyBoolean(),
                        anyString(), anyInt(), anyInt(), anyInt(), anyBoolean(), anyString());
        mGsmInboundSmsHandler.sendMessage(InboundSmsHandler.EVENT_NEW_SMS, new AsyncResult(null,
                mSmsMessage, null));
        waitForMs(100);

        // State machine should go back to idle and wait for second part
        assertEquals("IdleState", getCurrentState().getName());

        doReturn(mInboundSmsTrackerPart2).when(mTelephonyComponentFactory)
                .makeInboundSmsTracker(any(byte[].class), anyLong(), anyInt(), anyBoolean(),
                        anyString(), anyInt(), anyInt(), anyInt(), anyBoolean(), anyString());
        mGsmInboundSmsHandler.sendMessage(InboundSmsHandler.EVENT_NEW_SMS, new AsyncResult(null,
                mSmsMessage, null));
        waitForMs(100);

        // verify broadcast intents
        verifySmsIntentBroadcasts(0);

        // if an additional copy of one of the segments above is received, it should not be kept in
        // the db and should not be combined with any subsequent messages received from the same
        // sender

        // additional copy of part 2 of message
        doReturn(mInboundSmsTrackerPart2).when(mTelephonyComponentFactory)
                .makeInboundSmsTracker(any(byte[].class), anyLong(), anyInt(), anyBoolean(),
                        anyString(), anyInt(), anyInt(), anyInt(), anyBoolean(), anyString());
        mGsmInboundSmsHandler.sendMessage(InboundSmsHandler.EVENT_NEW_SMS, new AsyncResult(null,
                mSmsMessage, null));
        waitForMs(100);

        // verify no additional broadcasts sent
        verify(mContext, times(2)).sendBroadcast(any(Intent.class));

        // part 1 of new sms recieved from same sender with same parameters, just different
        // timestamps, should not be combined with the additional part 2 received above

        // call prepareMultiPartSms() to update timestamps
        prepareMultiPartSms();

        // part 1 of new sms
        doReturn(mInboundSmsTrackerPart1).when(mTelephonyComponentFactory)
                .makeInboundSmsTracker(any(byte[].class), anyLong(), anyInt(), anyBoolean(),
                        anyString(), anyInt(), anyInt(), anyInt(), anyBoolean(), anyString());
        mGsmInboundSmsHandler.sendMessage(InboundSmsHandler.EVENT_NEW_SMS, new AsyncResult(null,
                mSmsMessage, null));
        waitForMs(100);

        // verify no additional broadcasts sent
        verify(mContext, times(2)).sendBroadcast(any(Intent.class));

        assertEquals("IdleState", getCurrentState().getName());
    }

    @Test
    @MediumTest
    public void testMultiPartIncompleteSms() {
        /**
         * Test scenario: 2 messages are received with same address, ref number, count, and
         * seqNumber, with count = 2 and seqNumber = 1. We should not try to merge these.
         */
        transitionFromStartupToIdle();

        // prepare SMS part 1 and part 2
        prepareMultiPartSms();
        // change seqNumber in part 2 to 1
        mInboundSmsTrackerCVPart2.put("sequence", 1);

        mSmsHeader.concatRef = new SmsHeader.ConcatRef();
        doReturn(mSmsHeader).when(mGsmSmsMessage).getUserDataHeader();

        doReturn(mInboundSmsTrackerPart1).when(mTelephonyComponentFactory)
                .makeInboundSmsTracker(any(byte[].class), anyLong(), anyInt(), anyBoolean(),
                        anyString(), anyInt(), anyInt(), anyInt(), anyBoolean(), anyString());
        mGsmInboundSmsHandler.sendMessage(InboundSmsHandler.EVENT_NEW_SMS, new AsyncResult(null,
                mSmsMessage, null));
        waitForMs(100);

        // State machine should go back to idle and wait for second part
        assertEquals("IdleState", getCurrentState().getName());

        doReturn(mInboundSmsTrackerPart2).when(mTelephonyComponentFactory)
                .makeInboundSmsTracker(any(byte[].class), anyLong(), anyInt(), anyBoolean(),
                        anyString(), anyInt(), anyInt(), anyInt(), anyBoolean(), anyString());
        mGsmInboundSmsHandler.sendMessage(InboundSmsHandler.EVENT_NEW_SMS, new AsyncResult(null,
                mSmsMessage, null));
        waitForMs(100);

        // verify no broadcasts sent
        verify(mContext, never()).sendBroadcast(any(Intent.class));
        // State machine should go back to idle
        assertEquals("IdleState", getCurrentState().getName());
    }

    @Test
    @MediumTest
    public void testMultipartSmsFromBlockedNumber_noBroadcastsSent() {
        mFakeBlockedNumberContentProvider.mBlockedNumbers.add("1234567890");

        transitionFromStartupToIdle();

        // prepare SMS part 1 and part 2
        prepareMultiPartSms();

        mSmsHeader.concatRef = new SmsHeader.ConcatRef();
        doReturn(mSmsHeader).when(mGsmSmsMessage).getUserDataHeader();
        doReturn(mInboundSmsTrackerPart1).when(mTelephonyComponentFactory)
                .makeInboundSmsTracker(any(byte[].class), anyLong(), anyInt(), anyBoolean(),
                        anyString(), anyInt(), anyInt(), anyInt(), anyBoolean(), anyString());

        mGsmInboundSmsHandler.sendMessage(InboundSmsHandler.EVENT_NEW_SMS, new AsyncResult(null,
                mSmsMessage, null));
        waitForMs(100);

        // State machine should go back to idle and wait for second part
        assertEquals("IdleState", getCurrentState().getName());

        doReturn(mInboundSmsTrackerPart2).when(mTelephonyComponentFactory)
                .makeInboundSmsTracker(any(byte[].class), anyLong(), anyInt(), anyBoolean(),
                        anyString(), anyInt(), anyInt(), anyInt(), anyBoolean(), anyString());
        mGsmInboundSmsHandler.sendMessage(InboundSmsHandler.EVENT_NEW_SMS, new AsyncResult(null,
                mSmsMessage, null));
        waitForMs(100);

        verify(mContext, never()).sendBroadcast(any(Intent.class));
        assertEquals("IdleState", getCurrentState().getName());
    }

    @Test
    @MediumTest
    public void testBroadcastUndeliveredUserLocked() throws Exception {
        replaceInstance(SmsBroadcastUndelivered.class, "instance", null, null);
        doReturn(0).when(mInboundSmsTracker).getDestPort();

        // add a fake entry to db
        ContentValues rawSms = new ContentValues();
        mContentProvider.insert(sRawUri, rawSms);

        // make it a single-part message
        doReturn(1).when(mInboundSmsTracker).getMessageCount();

        // user locked
        UserManager userManager = (UserManager)mContext.getSystemService(Context.USER_SERVICE);
        doReturn(false).when(userManager).isUserUnlocked();

        SmsBroadcastUndelivered.initialize(mContext, mGsmInboundSmsHandler, mCdmaInboundSmsHandler);

        // verify that a broadcast receiver is registered for current user (user == null) based on
        // implementation in ContextFixture
        verify(mContext).registerReceiverAsUser(any(BroadcastReceiver.class), eq((UserHandle)null),
                any(IntentFilter.class), eq((String)null), eq((Handler)null));

        waitForMs(100);

        // verify no broadcasts sent because due to !isUserUnlocked
        verify(mContext, never()).sendBroadcast(any(Intent.class));

        // when user unlocks the device, the message in db should be broadcast
        doReturn(true).when(userManager).isUserUnlocked();
        mContext.sendBroadcast(new Intent(Intent.ACTION_USER_UNLOCKED));
        waitForMs(100);

        verifyDataSmsIntentBroadcasts(1);
    }

    @Test
    @MediumTest
    public void testBroadcastUndeliveredUserUnlocked() throws Exception {
        replaceInstance(SmsBroadcastUndelivered.class, "instance", null, null);
        doReturn(0).when(mInboundSmsTracker).getDestPort();

        // add a fake entry to db
        ContentValues rawSms = new ContentValues();
        mContentProvider.insert(sRawUri, rawSms);

        // make it a single-part message
        doReturn(1).when(mInboundSmsTracker).getMessageCount();

        SmsBroadcastUndelivered.initialize(mContext, mGsmInboundSmsHandler, mCdmaInboundSmsHandler);
        waitForMs(100);

        // user is unlocked; intent should be broadcast right away
        verifyDataSmsIntentBroadcasts(0);
    }

    @Test
    @MediumTest
    public void testBroadcastUndeliveredDeleted() throws Exception {
        replaceInstance(SmsBroadcastUndelivered.class, "instance", null, null);
        SmsBroadcastUndelivered.initialize(mContext, mGsmInboundSmsHandler, mCdmaInboundSmsHandler);
        doReturn(0).when(mInboundSmsTracker).getDestPort();

        //add a fake entry to db
        ContentValues rawSms = new ContentValues();
        rawSms.put("deleted", 1);
        mContentProvider.insert(sRawUri, rawSms);

        //make it a single-part message
        doReturn(1).when(mInboundSmsTracker).getMessageCount();

        //when user unlocks the device, broadcast should not be sent for new message
        mContext.sendBroadcast(new Intent(Intent.ACTION_USER_UNLOCKED));
        waitForMs(100);

        verify(mContext, times(1)).sendBroadcast(any(Intent.class));
        assertEquals("IdleState", getCurrentState().getName());

    }

    @Test
    @MediumTest
    public void testBroadcastUndeliveredMultiPart() throws Exception {
        replaceInstance(SmsBroadcastUndelivered.class, "instance", null, null);

        // prepare SMS part 1 and part 2
        prepareMultiPartSms();

        //add the 2 SMS parts to db
        mContentProvider.insert(sRawUri, mInboundSmsTrackerCVPart1);
        mContentProvider.insert(sRawUri, mInboundSmsTrackerCVPart2);

        //return InboundSmsTracker objects corresponding to the 2 parts
        doReturn(mInboundSmsTrackerPart1).doReturn(mInboundSmsTrackerPart2).
                when(mTelephonyComponentFactory).makeInboundSmsTracker(any(Cursor.class),
                anyBoolean());

        SmsBroadcastUndelivered.initialize(mContext, mGsmInboundSmsHandler, mCdmaInboundSmsHandler);
        waitForMs(100);

        verifySmsIntentBroadcasts(0);
    }
}
