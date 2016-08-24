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

package com.android.server.telecom;

import com.android.internal.annotations.VisibleForTesting;
import com.android.server.telecom.components.UserCallIntentProcessor;
import com.android.server.telecom.components.UserCallIntentProcessorFactory;
import com.android.server.telecom.ui.MissedCallNotifierImpl.MissedCallNotifierImplFactory;
import com.android.server.telecom.BluetoothPhoneServiceImpl.BluetoothPhoneServiceImplFactory;
import com.android.server.telecom.CallAudioManager.AudioServiceFactory;
import com.android.server.telecom.TelecomServiceImpl.DefaultDialerManagerAdapter;

import android.Manifest;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.net.Uri;
import android.os.UserHandle;

import java.io.FileNotFoundException;
import java.io.InputStream;

/**
 * Top-level Application class for Telecom.
 */
public final class TelecomSystem {

    /**
     * This interface is implemented by system-instantiated components (e.g., Services and
     * Activity-s) that wish to use the TelecomSystem but would like to be testable. Such a
     * component should implement the getTelecomSystem() method to return the global singleton,
     * and use its own method. Tests can subclass the component to return a non-singleton.
     *
     * A refactoring goal for Telecom is to limit use of the TelecomSystem singleton to those
     * system-instantiated components, and have all other parts of the system just take all their
     * dependencies as explicit arguments to their constructor or other methods.
     */
    public interface Component {
        TelecomSystem getTelecomSystem();
    }


    /**
     * Tagging interface for the object used for synchronizing multi-threaded operations in
     * the Telecom system.
     */
    public interface SyncRoot {
    }

    private static final IntentFilter USER_SWITCHED_FILTER =
            new IntentFilter(Intent.ACTION_USER_SWITCHED);

    private static final IntentFilter USER_STARTING_FILTER =
            new IntentFilter(Intent.ACTION_USER_STARTING);

    /** Intent filter for dialer secret codes. */
    private static final IntentFilter DIALER_SECRET_CODE_FILTER;

    /**
     * Initializes the dialer secret code intent filter.  Setup to handle the various secret codes
     * which can be dialed (e.g. in format *#*#code#*#*) to trigger various behavior in Telecom.
     */
    static {
        DIALER_SECRET_CODE_FILTER = new IntentFilter(
                "android.provider.Telephony.SECRET_CODE");
        DIALER_SECRET_CODE_FILTER.addDataScheme("android_secret_code");
        DIALER_SECRET_CODE_FILTER
                .addDataAuthority(DialerCodeReceiver.TELECOM_SECRET_CODE_DEBUG_ON, null);
        DIALER_SECRET_CODE_FILTER
                .addDataAuthority(DialerCodeReceiver.TELECOM_SECRET_CODE_DEBUG_OFF, null);
        DIALER_SECRET_CODE_FILTER
                .addDataAuthority(DialerCodeReceiver.TELECOM_SECRET_CODE_MARK, null);
    }

    private static TelecomSystem INSTANCE = null;

    private final SyncRoot mLock = new SyncRoot() { };
    private final MissedCallNotifier mMissedCallNotifier;
    private final PhoneAccountRegistrar mPhoneAccountRegistrar;
    private final CallsManager mCallsManager;
    private final RespondViaSmsManager mRespondViaSmsManager;
    private final Context mContext;
    private final BluetoothPhoneServiceImpl mBluetoothPhoneServiceImpl;
    private final CallIntentProcessor mCallIntentProcessor;
    private final TelecomBroadcastIntentProcessor mTelecomBroadcastIntentProcessor;
    private final TelecomServiceImpl mTelecomServiceImpl;
    private final ContactsAsyncHelper mContactsAsyncHelper;
    private final DialerCodeReceiver mDialerCodeReceiver;

    private final BroadcastReceiver mUserSwitchedReceiver = new BroadcastReceiver() {
        @Override
        public void onReceive(Context context, Intent intent) {
            Log.startSession("TSSwR.oR");
            try {
                int userHandleId = intent.getIntExtra(Intent.EXTRA_USER_HANDLE, 0);
                UserHandle currentUserHandle = new UserHandle(userHandleId);
                mPhoneAccountRegistrar.setCurrentUserHandle(currentUserHandle);
                mCallsManager.onUserSwitch(currentUserHandle);
            } finally {
                Log.endSession();
            }
        }
    };

    private final BroadcastReceiver mUserStartingReceiver = new BroadcastReceiver() {
        @Override
        public void onReceive(Context context, Intent intent) {
            Log.startSession("TSStR.oR");
            try {
                int userHandleId = intent.getIntExtra(Intent.EXTRA_USER_HANDLE, 0);
                UserHandle addingUserHandle = new UserHandle(userHandleId);
                mCallsManager.onUserStarting(addingUserHandle);
            } finally {
                Log.endSession();
            }
        }
    };

    public static TelecomSystem getInstance() {
        return INSTANCE;
    }

    public static void setInstance(TelecomSystem instance) {
        if (INSTANCE != null) {
            throw new RuntimeException("Attempt to set TelecomSystem.INSTANCE twice");
        }
        Log.i(TelecomSystem.class, "TelecomSystem.INSTANCE being set");
        INSTANCE = instance;
    }

    public TelecomSystem(
            Context context,
            MissedCallNotifierImplFactory missedCallNotifierImplFactory,
            CallerInfoAsyncQueryFactory callerInfoAsyncQueryFactory,
            HeadsetMediaButtonFactory headsetMediaButtonFactory,
            ProximitySensorManagerFactory proximitySensorManagerFactory,
            InCallWakeLockControllerFactory inCallWakeLockControllerFactory,
            AudioServiceFactory audioServiceFactory,
            BluetoothPhoneServiceImplFactory
                    bluetoothPhoneServiceImplFactory,
            Timeouts.Adapter timeoutsAdapter,
            AsyncRingtonePlayer asyncRingtonePlayer) {
        mContext = context.getApplicationContext();
        Log.setContext(mContext);
        Log.initMd5Sum();

        Log.startSession("TS.init");
        mPhoneAccountRegistrar = new PhoneAccountRegistrar(mContext);
        mContactsAsyncHelper = new ContactsAsyncHelper(
                new ContactsAsyncHelper.ContentResolverAdapter() {
                    @Override
                    public InputStream openInputStream(Context context, Uri uri)
                            throws FileNotFoundException {
                        return context.getContentResolver().openInputStream(uri);
                    }
                });
        BluetoothManager bluetoothManager = new BluetoothManager(mContext,
                new BluetoothAdapterProxy());
        WiredHeadsetManager wiredHeadsetManager = new WiredHeadsetManager(mContext);
        SystemStateProvider systemStateProvider = new SystemStateProvider(mContext);

        mMissedCallNotifier = missedCallNotifierImplFactory
                .makeMissedCallNotifierImpl(mContext, mPhoneAccountRegistrar);

        DefaultDialerManagerAdapter defaultDialerAdapter =
                new TelecomServiceImpl.DefaultDialerManagerAdapterImpl();

        mCallsManager = new CallsManager(
                mContext,
                mLock,
                mContactsAsyncHelper,
                callerInfoAsyncQueryFactory,
                mMissedCallNotifier,
                mPhoneAccountRegistrar,
                headsetMediaButtonFactory,
                proximitySensorManagerFactory,
                inCallWakeLockControllerFactory,
                audioServiceFactory,
                bluetoothManager,
                wiredHeadsetManager,
                systemStateProvider,
                defaultDialerAdapter,
                timeoutsAdapter,
                asyncRingtonePlayer);

        mRespondViaSmsManager = new RespondViaSmsManager(mCallsManager, mLock);
        mCallsManager.setRespondViaSmsManager(mRespondViaSmsManager);

        mContext.registerReceiver(mUserSwitchedReceiver, USER_SWITCHED_FILTER);
        mContext.registerReceiver(mUserStartingReceiver, USER_STARTING_FILTER);

        mBluetoothPhoneServiceImpl = bluetoothPhoneServiceImplFactory.makeBluetoothPhoneServiceImpl(
                mContext, mLock, mCallsManager, mPhoneAccountRegistrar);
        mCallIntentProcessor = new CallIntentProcessor(mContext, mCallsManager);
        mTelecomBroadcastIntentProcessor = new TelecomBroadcastIntentProcessor(
                mContext, mCallsManager);

        // Register the receiver for the dialer secret codes, used to enable extended logging.
        mDialerCodeReceiver = new DialerCodeReceiver(mCallsManager);
        mContext.registerReceiver(mDialerCodeReceiver, DIALER_SECRET_CODE_FILTER,
                Manifest.permission.CONTROL_INCALL_EXPERIENCE, null);

        mTelecomServiceImpl = new TelecomServiceImpl(
                mContext, mCallsManager, mPhoneAccountRegistrar,
                new CallIntentProcessor.AdapterImpl(),
                new UserCallIntentProcessorFactory() {
                    @Override
                    public UserCallIntentProcessor create(Context context, UserHandle userHandle) {
                        return new UserCallIntentProcessor(context, userHandle);
                    }
                },
                defaultDialerAdapter,
                new TelecomServiceImpl.SubscriptionManagerAdapterImpl(),
                mLock);
        Log.endSession();
    }

    @VisibleForTesting
    public PhoneAccountRegistrar getPhoneAccountRegistrar() {
        return mPhoneAccountRegistrar;
    }

    @VisibleForTesting
    public CallsManager getCallsManager() {
        return mCallsManager;
    }

    public BluetoothPhoneServiceImpl getBluetoothPhoneServiceImpl() {
        return mBluetoothPhoneServiceImpl;
    }

    public CallIntentProcessor getCallIntentProcessor() {
        return mCallIntentProcessor;
    }

    public TelecomBroadcastIntentProcessor getTelecomBroadcastIntentProcessor() {
        return mTelecomBroadcastIntentProcessor;
    }

    public TelecomServiceImpl getTelecomServiceImpl() {
        return mTelecomServiceImpl;
    }

    public Object getLock() {
        return mLock;
    }
}
