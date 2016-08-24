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

import static org.junit.Assert.fail;
import static org.mockito.Matchers.anyBoolean;
import static org.mockito.Matchers.anyLong;
import static org.mockito.Matchers.anyString;
import static org.mockito.Mockito.any;
import static org.mockito.Mockito.anyInt;
import static org.mockito.Mockito.doAnswer;
import static org.mockito.Mockito.doReturn;
import static org.mockito.Mockito.eq;

import android.app.ActivityManagerNative;
import android.app.IActivityManager;
import android.content.Context;
import android.content.IIntentSender;
import android.content.Intent;
import android.content.SharedPreferences;
import android.content.pm.PackageInfo;
import android.content.pm.PackageManager;
import android.database.Cursor;
import android.os.Bundle;
import android.os.Handler;
import android.os.IBinder;
import android.os.IDeviceIdleController;
import android.os.RegistrantList;
import android.os.ServiceManager;
import android.provider.BlockedNumberContract;
import android.telephony.ServiceState;
import android.telephony.TelephonyManager;
import android.test.mock.MockContentProvider;
import android.test.mock.MockContentResolver;
import android.util.Log;
import android.util.Singleton;
import android.util.SparseArray;

import com.android.ims.ImsCall;
import com.android.ims.ImsCallProfile;
import com.android.ims.ImsEcbm;
import com.android.ims.ImsManager;
import com.android.internal.telephony.cdma.CdmaSubscriptionSourceManager;
import com.android.internal.telephony.cdma.EriManager;
import com.android.internal.telephony.dataconnection.DcTracker;
import com.android.internal.telephony.imsphone.ImsPhone;
import com.android.internal.telephony.imsphone.ImsPhoneCallTracker;
import com.android.internal.telephony.mocks.TelephonyRegistryMock;
import com.android.internal.telephony.test.SimulatedCommands;
import com.android.internal.telephony.test.SimulatedCommandsVerifier;
import com.android.internal.telephony.uicc.IccCardProxy;
import com.android.internal.telephony.uicc.IccRecords;
import com.android.internal.telephony.uicc.IsimUiccRecords;
import com.android.internal.telephony.uicc.RuimRecords;
import com.android.internal.telephony.uicc.SIMRecords;
import com.android.internal.telephony.uicc.UiccCardApplication;
import com.android.internal.telephony.uicc.UiccController;

import org.mockito.Mock;
import org.mockito.MockitoAnnotations;
import org.mockito.invocation.InvocationOnMock;
import org.mockito.stubbing.Answer;

import java.lang.reflect.Field;
import java.util.HashMap;
import java.util.HashSet;
import java.util.Iterator;
import java.util.LinkedList;
import java.util.Set;

public abstract class TelephonyTest {
    protected static String TAG;

    @Mock
    protected GsmCdmaPhone mPhone;
    @Mock
    protected ImsPhone mImsPhone;
    @Mock
    protected ServiceStateTracker mSST;
    @Mock
    protected GsmCdmaCallTracker mCT;
    @Mock
    protected ImsPhoneCallTracker mImsCT;
    @Mock
    protected UiccController mUiccController;
    @Mock
    protected IccCardProxy mIccCardProxy;
    @Mock
    protected CallManager mCallManager;
    @Mock
    protected PhoneNotifier mNotifier;
    @Mock
    protected TelephonyComponentFactory mTelephonyComponentFactory;
    @Mock
    protected CdmaSubscriptionSourceManager mCdmaSSM;
    @Mock
    protected RegistrantList mRegistrantList;
    @Mock
    protected IccPhoneBookInterfaceManager mIccPhoneBookIntManager;
    @Mock
    protected ImsManager mImsManager;
    @Mock
    protected DcTracker mDcTracker;
    @Mock
    protected GsmCdmaCall mGsmCdmaCall;
    @Mock
    protected ImsCall mImsCall;
    @Mock
    protected ImsCallProfile mImsCallProfile;
    @Mock
    protected ImsEcbm mImsEcbm;
    @Mock
    protected SubscriptionController mSubscriptionController;
    @Mock
    protected ServiceState mServiceState;
    @Mock
    protected SimulatedCommandsVerifier mSimulatedCommandsVerifier;
    @Mock
    protected IDeviceIdleController mIDeviceIdleController;
    @Mock
    protected InboundSmsHandler mInboundSmsHandler;
    @Mock
    protected WspTypeDecoder mWspTypeDecoder;
    @Mock
    protected UiccCardApplication mUiccCardApplication3gpp;
    @Mock
    protected UiccCardApplication mUiccCardApplication3gpp2;
    @Mock
    protected UiccCardApplication mUiccCardApplicationIms;
    @Mock
    protected SIMRecords mSimRecords;
    @Mock
    protected RuimRecords mRuimRecords;
    @Mock
    protected IsimUiccRecords mIsimUiccRecords;
    @Mock
    protected ProxyController mProxyController;
    @Mock
    protected Singleton<IActivityManager> mIActivityManagerSingleton;
    @Mock
    protected IActivityManager mIActivityManager;
    @Mock
    protected InboundSmsTracker mInboundSmsTracker;
    @Mock
    protected IIntentSender mIIntentSender;
    @Mock
    protected IBinder mIBinder;
    @Mock
    protected SmsStorageMonitor mSmsStorageMonitor;
    @Mock
    protected SmsUsageMonitor mSmsUsageMonitor;
    @Mock
    protected PackageInfo mPackageInfo;
    @Mock
    protected EriManager mEriManager;
    @Mock
    protected IBinder mConnMetLoggerBinder;

    protected TelephonyManager mTelephonyManager;
    protected SimulatedCommands mSimulatedCommands;
    protected ContextFixture mContextFixture;
    protected Context mContext;
    protected FakeBlockedNumberContentProvider mFakeBlockedNumberContentProvider;
    private Object mLock = new Object();
    private boolean mReady;
    protected HashMap<String, IBinder> mServiceManagerMockedServices = new HashMap<>();


    protected HashMap<Integer, ImsManager> mImsManagerInstances = new HashMap<>();
    private HashMap<InstanceKey, Object> mOldInstances = new HashMap<InstanceKey, Object>();

    private LinkedList<InstanceKey> mInstanceKeys = new LinkedList<InstanceKey>();

    private class InstanceKey {
        public final Class mClass;
        public final String mInstName;
        public final Object mObj;
        InstanceKey(final Class c, final String instName, final Object obj) {
            mClass = c;
            mInstName = instName;
            mObj = obj;
        }

        @Override
        public int hashCode() {
            return (mClass.getName().hashCode() * 31 + mInstName.hashCode()) * 31;
        }

        @Override
        public boolean equals(Object obj) {
            if (obj == null || obj.getClass() != getClass()) {
                return false;
            }

            InstanceKey other = (InstanceKey) obj;
            return (other.mClass == mClass && other.mInstName.equals(mInstName)
                    && other.mObj == mObj);
        }
    }

    protected void waitUntilReady() {
        while (true) {
            synchronized (mLock) {
                if (mReady) {
                    break;
                }
            }
        }
    }

    protected void setReady(boolean ready) {
        synchronized (mLock) {
            mReady = ready;
        }
    }

    protected synchronized void replaceInstance(final Class c, final String instanceName,
                                                final Object obj, final Object newValue)
            throws Exception {
        Field field = c.getDeclaredField(instanceName);
        field.setAccessible(true);

        InstanceKey key = new InstanceKey(c, instanceName, obj);
        if (!mOldInstances.containsKey(key)) {
            mOldInstances.put(key, field.get(obj));
            mInstanceKeys.add(key);
        }
        field.set(obj, newValue);
    }

    protected synchronized void restoreInstance(final Class c, final String instanceName,
                                                final Object obj) throws Exception {
        InstanceKey key = new InstanceKey(c, instanceName, obj);
        if (mOldInstances.containsKey(key)) {
            Field field = c.getDeclaredField(instanceName);
            field.setAccessible(true);
            field.set(obj, mOldInstances.get(key));
            mOldInstances.remove(key);
            mInstanceKeys.remove(key);
        }
    }

    protected synchronized void restoreInstances() throws Exception {
        Iterator<InstanceKey> it = mInstanceKeys.descendingIterator();

        while (it.hasNext()) {
            InstanceKey key = it.next();
            Field field = key.mClass.getDeclaredField(key.mInstName);
            field.setAccessible(true);
            field.set(key.mObj, mOldInstances.get(key));
        }

        mInstanceKeys.clear();
        mOldInstances.clear();
    }

    protected void setUp(String tag) throws Exception {
        TAG = tag;
        MockitoAnnotations.initMocks(this);

        //Use reflection to mock singletons
        replaceInstance(CallManager.class, "INSTANCE", null, mCallManager);
        replaceInstance(TelephonyComponentFactory.class, "sInstance", null,
                mTelephonyComponentFactory);
        replaceInstance(UiccController.class, "mInstance", null, mUiccController);
        replaceInstance(CdmaSubscriptionSourceManager.class, "sInstance", null, mCdmaSSM);
        replaceInstance(ImsManager.class, "sImsManagerInstances", null, mImsManagerInstances);
        replaceInstance(SubscriptionController.class, "sInstance", null, mSubscriptionController);
        replaceInstance(ProxyController.class, "sProxyController", null, mProxyController);
        replaceInstance(ActivityManagerNative.class, "gDefault", null, mIActivityManagerSingleton);
        replaceInstance(CdmaSubscriptionSourceManager.class,
                "mCdmaSubscriptionSourceChangedRegistrants", mCdmaSSM, mRegistrantList);
        replaceInstance(SimulatedCommandsVerifier.class, "sInstance", null,
                mSimulatedCommandsVerifier);
        replaceInstance(Singleton.class, "mInstance", mIActivityManagerSingleton,
                mIActivityManager);
        replaceInstance(ServiceManager.class, "sCache", null, mServiceManagerMockedServices);

        mSimulatedCommands = new SimulatedCommands();
        mContextFixture = new ContextFixture();
        mContext = mContextFixture.getTestDouble();
        mFakeBlockedNumberContentProvider = new FakeBlockedNumberContentProvider();
        ((MockContentResolver)mContext.getContentResolver()).addProvider(
                BlockedNumberContract.AUTHORITY, mFakeBlockedNumberContentProvider);
        mPhone.mCi = mSimulatedCommands;
        mCT.mCi = mSimulatedCommands;
        mTelephonyManager = (TelephonyManager) mContext.getSystemService(Context.TELEPHONY_SERVICE);

        replaceInstance(TelephonyManager.class, "sInstance", null,
                mContext.getSystemService(Context.TELEPHONY_SERVICE));

        //mTelephonyComponentFactory
        doReturn(mSST).when(mTelephonyComponentFactory)
                .makeServiceStateTracker(any(GsmCdmaPhone.class), any(CommandsInterface.class));
        doReturn(mIccCardProxy).when(mTelephonyComponentFactory)
                .makeIccCardProxy(any(Context.class), any(CommandsInterface.class), anyInt());
        doReturn(mCT).when(mTelephonyComponentFactory)
                .makeGsmCdmaCallTracker(any(GsmCdmaPhone.class));
        doReturn(mIccPhoneBookIntManager).when(mTelephonyComponentFactory)
                .makeIccPhoneBookInterfaceManager(any(Phone.class));
        doReturn(mDcTracker).when(mTelephonyComponentFactory)
                .makeDcTracker(any(Phone.class));
        doReturn(mWspTypeDecoder).when(mTelephonyComponentFactory)
                .makeWspTypeDecoder(any(byte[].class));
        doReturn(mInboundSmsTracker).when(mTelephonyComponentFactory)
                .makeInboundSmsTracker(any(byte[].class), anyLong(), anyInt(), anyBoolean(),
                        anyBoolean(), anyString(), anyString());
        doReturn(mInboundSmsTracker).when(mTelephonyComponentFactory)
                .makeInboundSmsTracker(any(byte[].class), anyLong(), anyInt(), anyBoolean(),
                        anyString(), anyInt(), anyInt(), anyInt(), anyBoolean(), anyString());
        doReturn(mInboundSmsTracker).when(mTelephonyComponentFactory)
                .makeInboundSmsTracker(any(Cursor.class), anyBoolean());
        doReturn(mImsCT).when(mTelephonyComponentFactory)
                .makeImsPhoneCallTracker(any(ImsPhone.class));
        doReturn(mCdmaSSM).when(mTelephonyComponentFactory)
                .getCdmaSubscriptionSourceManagerInstance(any(Context.class),
                        any(CommandsInterface.class), any(Handler.class),
                        anyInt(), any(Object.class));
        doReturn(mIDeviceIdleController).when(mTelephonyComponentFactory)
                .getIDeviceIdleController();

        //mPhone
        doReturn(mContext).when(mPhone).getContext();
        doReturn(mContext).when(mImsPhone).getContext();
        doReturn(true).when(mPhone).getUnitTestMode();
        doReturn(mIccCardProxy).when(mPhone).getIccCard();
        doReturn(mServiceState).when(mPhone).getServiceState();
        doReturn(mServiceState).when(mImsPhone).getServiceState();
        doReturn(true).when(mPhone).isPhoneTypeGsm();
        doReturn(PhoneConstants.PHONE_TYPE_GSM).when(mPhone).getPhoneType();
        doReturn(mCT).when(mPhone).getCallTracker();
        doReturn(mSST).when(mPhone).getServiceStateTracker();
        mPhone.mEriManager = mEriManager;

        //mUiccController
        doReturn(mUiccCardApplication3gpp).when(mUiccController).getUiccCardApplication(anyInt(),
                eq(UiccController.APP_FAM_3GPP));
        doReturn(mUiccCardApplication3gpp2).when(mUiccController).getUiccCardApplication(anyInt(),
                eq(UiccController.APP_FAM_3GPP2));
        doReturn(mUiccCardApplicationIms).when(mUiccController).getUiccCardApplication(anyInt(),
                eq(UiccController.APP_FAM_IMS));

        doAnswer(new Answer<IccRecords>() {
            public IccRecords answer(InvocationOnMock invocation) {
                switch ((Integer) invocation.getArguments()[1]) {
                    case UiccController.APP_FAM_3GPP:
                        return mSimRecords;
                    case UiccController.APP_FAM_3GPP2:
                        return mRuimRecords;
                    case UiccController.APP_FAM_IMS:
                        return mIsimUiccRecords;
                    default:
                        logd("Unrecognized family " + invocation.getArguments()[1]);
                        return null;
                }
            }
        }).when(mUiccController).getIccRecords(anyInt(), anyInt());

        //UiccCardApplication
        doReturn(mSimRecords).when(mUiccCardApplication3gpp).getIccRecords();
        doReturn(mRuimRecords).when(mUiccCardApplication3gpp2).getIccRecords();
        doReturn(mIsimUiccRecords).when(mUiccCardApplicationIms).getIccRecords();

        //mIccCardProxy
        doReturn(mSimRecords).when(mIccCardProxy).getIccRecords();
        doAnswer(new Answer<IccRecords>() {
            public IccRecords answer(InvocationOnMock invocation) {
                return (mPhone.isPhoneTypeGsm()) ? mSimRecords : mRuimRecords;
            }
        }).when(mIccCardProxy).getIccRecords();

        //SMS
        doReturn(true).when(mSmsStorageMonitor).isStorageAvailable();
        doReturn(true).when(mSmsUsageMonitor).check(anyString(), anyInt());
        doReturn(true).when(mTelephonyManager).getSmsReceiveCapableForPhone(anyInt(), anyBoolean());
        doReturn(true).when(mTelephonyManager).getSmsSendCapableForPhone(
                anyInt(), anyBoolean());

        //Misc
        doReturn(ServiceState.RIL_RADIO_TECHNOLOGY_UMTS).when(mServiceState).
                getRilDataRadioTechnology();
        doReturn(mPhone).when(mCT).getPhone();
        mImsManagerInstances.put(mPhone.getPhoneId(), null);
        doReturn(mImsEcbm).when(mImsManager).getEcbmInterface(anyInt());
        doReturn(mPhone).when(mInboundSmsHandler).getPhone();
        doReturn(mImsCallProfile).when(mImsCall).getCallProfile();
        doReturn(mIBinder).when(mIIntentSender).asBinder();
        doReturn(mIIntentSender).when(mIActivityManager).getIntentSender(anyInt(),
                anyString(), any(IBinder.class), anyString(), anyInt(), any(Intent[].class),
                any(String[].class), anyInt(), any(Bundle.class), anyInt());
        mSST.mSS = mServiceState;
        mServiceManagerMockedServices.put("connectivity_metrics_logger", mConnMetLoggerBinder);

        setReady(false);
    }

    protected void tearDown() throws Exception {

        SharedPreferences sharedPreferences = mContext.getSharedPreferences((String) null, 0);
        sharedPreferences.edit().clear().commit();

        restoreInstances();
    }

    protected static void logd(String s) {
        Log.d(TAG, s);
    }

    public static class FakeBlockedNumberContentProvider extends MockContentProvider {
        public Set<String> mBlockedNumbers = new HashSet<>();
        public int mNumEmergencyContactNotifications = 0;

        @Override
        public Bundle call(String method, String arg, Bundle extras) {
            switch (method) {
                case BlockedNumberContract.SystemContract.METHOD_SHOULD_SYSTEM_BLOCK_NUMBER:
                    Bundle bundle = new Bundle();
                    bundle.putBoolean(BlockedNumberContract.RES_NUMBER_IS_BLOCKED,
                            mBlockedNumbers.contains(arg));
                    return bundle;
                case BlockedNumberContract.SystemContract.METHOD_NOTIFY_EMERGENCY_CONTACT:
                    mNumEmergencyContactNotifications++;
                    return new Bundle();
                default:
                    fail("Method not expected: " + method);
            }
            return null;
        }
    }

    protected void setupMockPackagePermissionChecks() throws Exception {
        PackageManager mockPackageManager = mContext.getPackageManager();
        doReturn(new String[]{TAG}).when(mockPackageManager).getPackagesForUid(anyInt());
        doReturn(mPackageInfo).when(mockPackageManager).getPackageInfo(eq(TAG), anyInt());
    }
}
