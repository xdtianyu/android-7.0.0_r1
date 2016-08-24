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

import android.Manifest;
import android.content.ContentProvider;
import android.content.ContentValues;
import android.content.Context;
import android.content.Intent;
import android.content.pm.UserInfo;
import android.net.Uri;
import android.os.AsyncResult;
import android.os.HandlerThread;
import android.os.Message;
import android.provider.Telephony;
import android.telephony.CarrierConfigManager;
import android.telephony.SubscriptionInfo;
import android.telephony.SubscriptionManager;
import android.test.mock.MockContentProvider;
import android.test.mock.MockContentResolver;
import android.test.suitebuilder.annotation.SmallTest;

import com.android.internal.telephony.uicc.IccCardProxy;
import com.android.internal.telephony.uicc.IccFileHandler;
import com.android.internal.telephony.uicc.IccRecords;
import com.android.internal.telephony.uicc.IccUtils;

import org.junit.After;
import org.junit.Assert;
import org.junit.Before;
import org.junit.Test;
import org.mockito.ArgumentCaptor;
import org.mockito.Mock;
import org.mockito.invocation.InvocationOnMock;
import org.mockito.stubbing.Answer;

import java.util.Arrays;
import java.util.HashMap;

import static com.android.internal.telephony.TelephonyTestUtils.waitForMs;
import static org.junit.Assert.assertEquals;
import static org.mockito.Mockito.any;
import static org.mockito.Mockito.anyBoolean;
import static org.mockito.Mockito.anyInt;
import static org.mockito.Mockito.anyString;
import static org.mockito.Mockito.doAnswer;
import static org.mockito.Mockito.doReturn;
import static org.mockito.Mockito.eq;
import static org.mockito.Mockito.times;
import static org.mockito.Mockito.verify;

public class SubscriptionInfoUpdaterTest extends TelephonyTest {

    private static final int FAKE_SUB_ID = 1;
    private static final String FAKE_PLMN = "123456";


    private IccRecords mIccRecord;
    @Mock
    private UserInfo mUserInfo;
    @Mock
    private SubscriptionInfo mSubInfo;
    @Mock
    private ContentProvider mContentProvider;
    @Mock
    private HashMap<String, Object> mSubscriptionContent;
    @Mock
    private IccFileHandler mIccFileHandler;

    /*Custom ContentProvider */
    private class FakeSubscriptionContentProvider extends MockContentProvider {
        @Override
        public int update(Uri uri, ContentValues values, String selection, String[] selectionArgs) {
            for (String key : values.keySet()) {
                mSubscriptionContent.put(key, values.get(key));
            }
            return 1;
        }
    }

    private class SubscriptionInfoUpdaterHandlerThread extends HandlerThread {

        private SubscriptionInfoUpdaterHandlerThread(String name) {
            super(name);
        }

        @Override
        public void onLooperPrepared() {
            new SubscriptionInfoUpdater(mContext, new Phone[]{mPhone},
                    new CommandsInterface[]{mSimulatedCommands});
            setReady(true);
        }
    }

    @Before
    public void setUp() throws Exception {
        super.setUp(this.getClass().getSimpleName());

        replaceInstance(SubscriptionInfoUpdater.class, "mIccId", null,
                new String[SubscriptionInfoUpdater.STATUS_SIM1_INSERTED]);
        replaceInstance(SubscriptionInfoUpdater.class, "mInsertSimState", null,
                new int[SubscriptionInfoUpdater.STATUS_SIM1_INSERTED]);
        replaceInstance(SubscriptionInfoUpdater.class, "mContext", null, null);
        replaceInstance(SubscriptionInfoUpdater.class, "PROJECT_SIM_NUM", null,
                SubscriptionInfoUpdater.STATUS_SIM1_INSERTED);

        doReturn(SubscriptionInfoUpdater.STATUS_SIM1_INSERTED)
                .when(mTelephonyManager).getSimCount();
        doReturn(SubscriptionInfoUpdater.STATUS_SIM1_INSERTED)
                .when(mTelephonyManager).getPhoneCount();

        doReturn(mUserInfo).when(mIActivityManager).getCurrentUser();
        doReturn(new int[]{FAKE_SUB_ID}).when(mSubscriptionController).getSubId(0);
        mContentProvider = new FakeSubscriptionContentProvider();
        ((MockContentResolver) mContext.getContentResolver()).addProvider(
                SubscriptionManager.CONTENT_URI.getAuthority(),
                mContentProvider);
        mIccRecord = mIccCardProxy.getIccRecords();

        new SubscriptionInfoUpdaterHandlerThread(TAG).start();
        waitUntilReady();
    }

    @After
    public void tearDown() throws Exception {
        super.tearDown();
    }

    @Test
    @SmallTest
    public void testSimAbsent() {
        doReturn(Arrays.asList(mSubInfo)).when(mSubscriptionController)
                .getSubInfoUsingSlotIdWithCheck(eq(0), anyBoolean(), anyString());
        Intent mIntent = new Intent(TelephonyIntents.ACTION_SIM_STATE_CHANGED);
        mIntent.putExtra(IccCardConstants.INTENT_KEY_ICC_STATE,
                IccCardConstants.INTENT_VALUE_ICC_ABSENT);
        mIntent.putExtra(PhoneConstants.PHONE_KEY, 0);

        mContext.sendBroadcast(mIntent);

        waitForMs(100);
        verify(mSubscriptionContent).put(eq(SubscriptionManager.SIM_SLOT_INDEX),
                eq(SubscriptionManager.INVALID_SIM_SLOT_INDEX));

        CarrierConfigManager mConfigManager = (CarrierConfigManager)
                mContext.getSystemService(Context.CARRIER_CONFIG_SERVICE);
        verify(mConfigManager).updateConfigForPhoneId(eq(0),
                eq(IccCardConstants.INTENT_VALUE_ICC_ABSENT));
        verify(mSubscriptionController, times(1)).notifySubscriptionInfoChanged();
    }

    @Test
    @SmallTest
    public void testSimUnknown() {
        Intent mIntent = new Intent(TelephonyIntents.ACTION_SIM_STATE_CHANGED);
        mIntent.putExtra(IccCardConstants.INTENT_KEY_ICC_STATE,
                IccCardConstants.INTENT_VALUE_ICC_UNKNOWN);
        mIntent.putExtra(PhoneConstants.PHONE_KEY, FAKE_SUB_ID);

        mContext.sendBroadcast(mIntent);

        waitForMs(100);
        verify(mSubscriptionContent, times(0)).put(anyString(), any());
        CarrierConfigManager mConfigManager = (CarrierConfigManager)
                mContext.getSystemService(Context.CARRIER_CONFIG_SERVICE);
        verify(mConfigManager).updateConfigForPhoneId(eq(1),
                eq(IccCardConstants.INTENT_VALUE_ICC_UNKNOWN));
        verify(mSubscriptionController, times(0)).notifySubscriptionInfoChanged();
    }

    @Test
    @SmallTest
    public void testSimError() {
        Intent mIntent = new Intent(TelephonyIntents.ACTION_SIM_STATE_CHANGED);
        mIntent.putExtra(IccCardConstants.INTENT_KEY_ICC_STATE,
                IccCardConstants.INTENT_VALUE_ICC_CARD_IO_ERROR);
        mIntent.putExtra(PhoneConstants.PHONE_KEY, 2);

        mContext.sendBroadcast(mIntent);
        waitForMs(100);
        verify(mSubscriptionContent, times(0)).put(anyString(), any());
        CarrierConfigManager mConfigManager = (CarrierConfigManager)
                mContext.getSystemService(Context.CARRIER_CONFIG_SERVICE);
        verify(mConfigManager).updateConfigForPhoneId(eq(2),
                eq(IccCardConstants.INTENT_VALUE_ICC_CARD_IO_ERROR));
        verify(mSubscriptionController, times(0)).notifySubscriptionInfoChanged();
    }

    @Test
    @SmallTest
    public void testWrongSimState() {
        Intent mIntent = new Intent(TelephonyIntents.ACTION_SIM_STATE_CHANGED);
        mIntent.putExtra(IccCardConstants.INTENT_KEY_ICC_STATE,
                IccCardConstants.INTENT_VALUE_ICC_IMSI);
        mIntent.putExtra(PhoneConstants.PHONE_KEY, 2);

        mContext.sendBroadcast(mIntent);
        waitForMs(100);
        verify(mSubscriptionContent, times(0)).put(anyString(), any());
        CarrierConfigManager mConfigManager = (CarrierConfigManager)
                mContext.getSystemService(Context.CARRIER_CONFIG_SERVICE);
        verify(mConfigManager, times(0)).updateConfigForPhoneId(eq(2),
                eq(IccCardConstants.INTENT_VALUE_ICC_IMSI));
        verify(mSubscriptionController, times(0)).notifySubscriptionInfoChanged();
    }

    @Test
    @SmallTest
    public void testSimLoaded() {
        /* mock new sim got loaded and there is no sim loaded before */
        doReturn(null).when(mSubscriptionController)
                .getSubInfoUsingSlotIdWithCheck(eq(0), anyBoolean(), anyString());
        doReturn("89012604200000000000").when(mIccRecord).getIccId();
        doReturn(FAKE_PLMN).when(mTelephonyManager).getSimOperatorNumericForPhone(0);
        Intent intentInternalSimStateChanged =
                new Intent(IccCardProxy.ACTION_INTERNAL_SIM_STATE_CHANGED);
        intentInternalSimStateChanged.putExtra(IccCardConstants.INTENT_KEY_ICC_STATE,
                IccCardConstants.INTENT_VALUE_ICC_LOADED);
        intentInternalSimStateChanged.putExtra(PhoneConstants.PHONE_KEY, 0);

        mContext.sendBroadcast(intentInternalSimStateChanged);
        waitForMs(100);

        // verify SIM_STATE_CHANGED broadcast. It should be broadcast twice, once for
        // READ_PHONE_STATE and once for READ_PRIVILEGED_PHONE_STATE
        /* todo: cannot verify as intent is sent using ActivityManagerNative.broadcastStickyIntent()
         * uncomment code below when that is fixed
         */
        /* ArgumentCaptor<Intent> intentArgumentCaptor = ArgumentCaptor.forClass(Intent.class);
        ArgumentCaptor<String> stringArgumentCaptor = ArgumentCaptor.forClass(String.class);
        verify(mContext, times(2)).sendBroadcast(intentArgumentCaptor.capture(),
                stringArgumentCaptor.capture());
        assertEquals(TelephonyIntents.ACTION_SIM_STATE_CHANGED,
                intentArgumentCaptor.getAllValues().get(0).getAction());
        assertEquals(Manifest.permission.READ_PHONE_STATE,
                stringArgumentCaptor.getAllValues().get(0));
        assertEquals(TelephonyIntents.ACTION_SIM_STATE_CHANGED,
                intentArgumentCaptor.getAllValues().get(1).getAction());
        assertEquals(Manifest.permission.READ_PRIVILEGED_PHONE_STATE,
                stringArgumentCaptor.getAllValues().get(1)); */

        SubscriptionManager mSubscriptionManager = SubscriptionManager.from(mContext);
        verify(mTelephonyManager).getSimOperatorNumericForPhone(0);
        verify(mSubscriptionManager, times(1)).addSubscriptionInfoRecord(
                eq("89012604200000000000"), eq(0));
        verify(mSubscriptionController, times(1)).notifySubscriptionInfoChanged();
        verify(mSubscriptionController, times(1)).setMccMnc(FAKE_PLMN, FAKE_SUB_ID);
        CarrierConfigManager mConfigManager = (CarrierConfigManager)
                mContext.getSystemService(Context.CARRIER_CONFIG_SERVICE);
        verify(mConfigManager, times(1)).updateConfigForPhoneId(eq(0),
                eq(IccCardConstants.INTENT_VALUE_ICC_LOADED));

        // ACTION_USER_UNLOCKED should trigger another SIM_STATE_CHANGED
        Intent intentSimStateChanged = new Intent(Intent.ACTION_USER_UNLOCKED);
        mContext.sendBroadcast(intentSimStateChanged);
        waitForMs(100);

        // verify SIM_STATE_CHANGED broadcast
        /* todo: cannot verify as intent is sent using ActivityManagerNative.broadcastStickyIntent()
         * uncomment code below when that is fixed
         */
        /* verify(mContext, times(4)).sendBroadcast(intentArgumentCaptor.capture(),
                stringArgumentCaptor.capture());
        assertEquals(TelephonyIntents.ACTION_SIM_STATE_CHANGED,
                intentArgumentCaptor.getAllValues().get(2).getAction());
        assertEquals(Manifest.permission.READ_PHONE_STATE,
                stringArgumentCaptor.getAllValues().get(2));
        assertEquals(TelephonyIntents.ACTION_SIM_STATE_CHANGED,
                intentArgumentCaptor.getAllValues().get(3).getAction());
        assertEquals(Manifest.permission.READ_PRIVILEGED_PHONE_STATE,
                stringArgumentCaptor.getAllValues().get(3)); */
    }

    @Test
    @SmallTest
    public void testSimLoadedEmptyOperatorNumeric() {
        /* mock new sim got loaded and there is no sim loaded before */
        doReturn(null).when(mSubscriptionController)
                .getSubInfoUsingSlotIdWithCheck(eq(0), anyBoolean(), anyString());
        doReturn("89012604200000000000").when(mIccRecord).getIccId();
        // operator numeric is empty
        doReturn("").when(mTelephonyManager).getSimOperatorNumericForPhone(0);
        Intent mIntent = new Intent(IccCardProxy.ACTION_INTERNAL_SIM_STATE_CHANGED);
        mIntent.putExtra(IccCardConstants.INTENT_KEY_ICC_STATE,
                IccCardConstants.INTENT_VALUE_ICC_LOADED);
        mIntent.putExtra(PhoneConstants.PHONE_KEY, 0);

        mContext.sendBroadcast(mIntent);
        waitForMs(100);
        SubscriptionManager mSubscriptionManager = SubscriptionManager.from(mContext);
        verify(mTelephonyManager).getSimOperatorNumericForPhone(0);
        verify(mSubscriptionManager, times(1)).addSubscriptionInfoRecord(
                eq("89012604200000000000"), eq(0));
        verify(mSubscriptionController, times(1)).notifySubscriptionInfoChanged();
        verify(mSubscriptionController, times(0)).setMccMnc(anyString(), anyInt());
        CarrierConfigManager mConfigManager = (CarrierConfigManager)
                mContext.getSystemService(Context.CARRIER_CONFIG_SERVICE);
        verify(mConfigManager, times(1)).updateConfigForPhoneId(eq(0),
                eq(IccCardConstants.INTENT_VALUE_ICC_LOADED));
    }

    @Test
    @SmallTest
    public void testSimLockedWithOutIccId() {
        /* mock no IccId Info present and try to query IccId
         after IccId query, update subscriptionDB */
        doReturn(mIccFileHandler).when(mIccCardProxy).getIccFileHandler();
        doAnswer(new Answer<Void>() {
            @Override
            public Void answer(InvocationOnMock invocation) throws Throwable {
                Message msg = (Message) invocation.getArguments()[1];
                AsyncResult.forMessage(msg).result = IccUtils
                        .hexStringToBytes("89012604200000000000");
                msg.sendToTarget();
                return null;
            }
        }).when(mIccFileHandler).loadEFTransparent(anyInt(), any(Message.class));

        doReturn(Arrays.asList(mSubInfo)).when(mSubscriptionController)
                .getSubInfoUsingSlotIdWithCheck(eq(0), anyBoolean(), anyString());

        Intent mIntent = new Intent(IccCardProxy.ACTION_INTERNAL_SIM_STATE_CHANGED);
        mIntent.putExtra(IccCardConstants.INTENT_KEY_ICC_STATE,
                IccCardConstants.INTENT_VALUE_ICC_LOCKED);
        mIntent.putExtra(IccCardConstants.INTENT_KEY_LOCKED_REASON, "TESTING");
        mIntent.putExtra(PhoneConstants.PHONE_KEY, 0);

        mContext.sendBroadcast(mIntent);
        waitForMs(100);

        /* old IccId != new queried IccId */
        verify(mSubscriptionContent).put(eq(SubscriptionManager.SIM_SLOT_INDEX),
                eq(SubscriptionManager.INVALID_SIM_SLOT_INDEX));
        SubscriptionManager mSubscriptionManager = SubscriptionManager.from(mContext);
        verify(mSubscriptionManager, times(1)).addSubscriptionInfoRecord(
                eq("98106240020000000000"), eq(0));

        verify(mSubscriptionController, times(1)).notifySubscriptionInfoChanged();
        CarrierConfigManager mConfigManager = (CarrierConfigManager)
                mContext.getSystemService(Context.CARRIER_CONFIG_SERVICE);
        verify(mConfigManager, times(1)).updateConfigForPhoneId(eq(0),
                eq(IccCardConstants.INTENT_VALUE_ICC_LOCKED));
    }

    @Test
    @SmallTest
    public void testSimLockWIthIccId() {
        /* no need for IccId query */
        try {
            replaceInstance(SubscriptionInfoUpdater.class, "mIccId", null,
                    new String[]{"89012604200000000000"});
        } catch (Exception ex) {
            Assert.fail("unexpected exception thrown" + ex.getMessage());
        }
        doReturn(mIccFileHandler).when(mIccCardProxy).getIccFileHandler();

        Intent mIntent = new Intent(IccCardProxy.ACTION_INTERNAL_SIM_STATE_CHANGED);
        mIntent.putExtra(IccCardConstants.INTENT_KEY_ICC_STATE,
                IccCardConstants.INTENT_VALUE_ICC_LOCKED);
        mIntent.putExtra(IccCardConstants.INTENT_KEY_LOCKED_REASON, "TESTING");
        mIntent.putExtra(PhoneConstants.PHONE_KEY, 0);

        mContext.sendBroadcast(mIntent);
        waitForMs(100);

        verify(mIccFileHandler, times(0)).loadEFTransparent(anyInt(), any(Message.class));
        SubscriptionManager mSubscriptionManager = SubscriptionManager.from(mContext);
        verify(mSubscriptionManager, times(0)).addSubscriptionInfoRecord(
                anyString(), eq(0));
        verify(mSubscriptionController, times(0)).notifySubscriptionInfoChanged();
        CarrierConfigManager mConfigManager = (CarrierConfigManager)
                mContext.getSystemService(Context.CARRIER_CONFIG_SERVICE);
        /* broadcast is done */
        verify(mConfigManager, times(1)).updateConfigForPhoneId(eq(0),
                eq(IccCardConstants.INTENT_VALUE_ICC_LOCKED));
    }

}