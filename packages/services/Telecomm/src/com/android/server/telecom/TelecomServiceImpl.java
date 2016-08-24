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

import static android.Manifest.permission.CALL_PHONE;
import static android.Manifest.permission.DUMP;
import static android.Manifest.permission.MODIFY_PHONE_STATE;
import static android.Manifest.permission.READ_PHONE_STATE;
import static android.Manifest.permission.READ_PRIVILEGED_PHONE_STATE;
import static android.Manifest.permission.REGISTER_SIM_SUBSCRIPTION;
import static android.Manifest.permission.WRITE_SECURE_SETTINGS;

import android.app.ActivityManager;
import android.app.AppOpsManager;
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.pm.ApplicationInfo;
import android.content.pm.PackageManager;
import android.content.res.Resources;
import android.net.Uri;
import android.os.Binder;
import android.os.Bundle;
import android.os.Process;
import android.os.UserHandle;
import android.os.UserManager;
import android.telecom.DefaultDialerManager;
import android.telecom.ParcelableCallAnalytics;
import android.telecom.PhoneAccount;
import android.telecom.PhoneAccountHandle;
import android.telecom.TelecomManager;
import android.telecom.VideoProfile;
import android.telephony.SubscriptionManager;
import android.telephony.TelephonyManager;
import android.util.EventLog;

// TODO: Needed for move to system service: import com.android.internal.R;
import com.android.internal.telecom.ITelecomService;
import com.android.internal.util.IndentingPrintWriter;
import com.android.server.telecom.components.UserCallIntentProcessorFactory;
import com.android.server.telecom.settings.BlockedNumbersActivity;

import java.io.FileDescriptor;
import java.io.PrintWriter;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collections;
import java.util.List;

/**
 * Implementation of the ITelecom interface.
 */
public class TelecomServiceImpl {
    public interface DefaultDialerManagerAdapter {
        String getDefaultDialerApplication(Context context);
        String getDefaultDialerApplication(Context context, int userId);
        boolean setDefaultDialerApplication(Context context, String packageName);
        boolean isDefaultOrSystemDialer(Context context, String packageName);
    }

    static class DefaultDialerManagerAdapterImpl implements DefaultDialerManagerAdapter {
        @Override
        public String getDefaultDialerApplication(Context context) {
            return DefaultDialerManager.getDefaultDialerApplication(context);
        }

        @Override
        public String getDefaultDialerApplication(Context context, int userId) {
            return DefaultDialerManager.getDefaultDialerApplication(context, userId);
        }

        @Override
        public boolean setDefaultDialerApplication(Context context, String packageName) {
            return DefaultDialerManager.setDefaultDialerApplication(context, packageName);
        }

        @Override
        public boolean isDefaultOrSystemDialer(Context context, String packageName) {
            return DefaultDialerManager.isDefaultOrSystemDialer(context, packageName);
        }
    }

    public interface SubscriptionManagerAdapter {
        int getDefaultVoiceSubId();
    }

    static class SubscriptionManagerAdapterImpl implements SubscriptionManagerAdapter {
        @Override
        public int getDefaultVoiceSubId() {
            return SubscriptionManager.getDefaultVoiceSubscriptionId();
        }
    }

    private static final String PERMISSION_PROCESS_PHONE_ACCOUNT_REGISTRATION =
            "android.permission.PROCESS_PHONE_ACCOUNT_REGISTRATION";
    private static final int DEFAULT_VIDEO_STATE = -1;

    private final ITelecomService.Stub mBinderImpl = new ITelecomService.Stub() {
        @Override
        public PhoneAccountHandle getDefaultOutgoingPhoneAccount(String uriScheme,
                String callingPackage) {
            try {
                Log.startSession("TSI.gDOPA");
                synchronized (mLock) {
                    if (!canReadPhoneState(callingPackage, "getDefaultOutgoingPhoneAccount")) {
                        return null;
                    }

                    final UserHandle callingUserHandle = Binder.getCallingUserHandle();
                    long token = Binder.clearCallingIdentity();
                    try {
                        return mPhoneAccountRegistrar
                                .getOutgoingPhoneAccountForScheme(uriScheme, callingUserHandle);
                    } catch (Exception e) {
                        Log.e(this, e, "getDefaultOutgoingPhoneAccount");
                        throw e;
                    } finally {
                        Binder.restoreCallingIdentity(token);
                    }
                }
            } finally {
                Log.endSession();
            }
        }

        @Override
        public PhoneAccountHandle getUserSelectedOutgoingPhoneAccount() {
            synchronized (mLock) {
                try {
                    Log.startSession("TSI.gUSOPA");
                    final UserHandle callingUserHandle = Binder.getCallingUserHandle();
                    return mPhoneAccountRegistrar.getUserSelectedOutgoingPhoneAccount(
                            callingUserHandle);
                } catch (Exception e) {
                    Log.e(this, e, "getUserSelectedOutgoingPhoneAccount");
                    throw e;
                } finally {
                    Log.endSession();
                }
            }
        }

        @Override
        public void setUserSelectedOutgoingPhoneAccount(PhoneAccountHandle accountHandle) {
            try {
                Log.startSession("TSI.sUSOPA");
                synchronized (mLock) {
                    enforceModifyPermission();
                    UserHandle callingUserHandle = Binder.getCallingUserHandle();
                    long token = Binder.clearCallingIdentity();
                    try {
                        mPhoneAccountRegistrar.setUserSelectedOutgoingPhoneAccount(
                                accountHandle, callingUserHandle);
                    } catch (Exception e) {
                        Log.e(this, e, "setUserSelectedOutgoingPhoneAccount");
                        throw e;
                    } finally {
                        Binder.restoreCallingIdentity(token);
                    }
                }
            } finally {
                Log.endSession();
            }
        }

        @Override
        public List<PhoneAccountHandle> getCallCapablePhoneAccounts(
                boolean includeDisabledAccounts, String callingPackage) {
            try {
                Log.startSession("TSI.gCCPA");
                if (!canReadPhoneState(callingPackage, "getDefaultOutgoingPhoneAccount")) {
                    return Collections.emptyList();
                }
                synchronized (mLock) {
                    final UserHandle callingUserHandle = Binder.getCallingUserHandle();
                    long token = Binder.clearCallingIdentity();
                    try {
                        return mPhoneAccountRegistrar.getCallCapablePhoneAccounts(null,
                                includeDisabledAccounts, callingUserHandle);
                    } catch (Exception e) {
                        Log.e(this, e, "getCallCapablePhoneAccounts");
                        throw e;
                    } finally {
                        Binder.restoreCallingIdentity(token);
                    }
                }
            } finally {
                Log.endSession();
            }
        }

        @Override
        public List<PhoneAccountHandle> getPhoneAccountsSupportingScheme(String uriScheme,
                String callingPackage) {
            try {
                Log.startSession("TSI.gPASS");
                synchronized (mLock) {
                    if (!canReadPhoneState(callingPackage, "getPhoneAccountsSupportingScheme")) {
                        return Collections.emptyList();
                    }
                    final UserHandle callingUserHandle = Binder.getCallingUserHandle();
                    long token = Binder.clearCallingIdentity();
                    try {
                        return mPhoneAccountRegistrar.getCallCapablePhoneAccounts(uriScheme, false,
                                callingUserHandle);
                    } catch (Exception e) {
                        Log.e(this, e, "getPhoneAccountsSupportingScheme %s", uriScheme);
                        throw e;
                    } finally {
                        Binder.restoreCallingIdentity(token);
                    }
                }
            } finally {
                Log.endSession();
            }
        }

        @Override
        public List<PhoneAccountHandle> getPhoneAccountsForPackage(String packageName) {
            synchronized (mLock) {
                final UserHandle callingUserHandle = Binder.getCallingUserHandle();
                long token = Binder.clearCallingIdentity();
                try {
                    Log.startSession("TSI.gPAFP");
                    return mPhoneAccountRegistrar.getPhoneAccountsForPackage(packageName,
                            callingUserHandle);
                } catch (Exception e) {
                    Log.e(this, e, "getPhoneAccountsForPackage %s", packageName);
                    throw e;
                } finally {
                    Binder.restoreCallingIdentity(token);
                    Log.endSession();
                }
            }
        }

        @Override
        public PhoneAccount getPhoneAccount(PhoneAccountHandle accountHandle) {
            synchronized (mLock) {
                final UserHandle callingUserHandle = Binder.getCallingUserHandle();
                long token = Binder.clearCallingIdentity();
                try {
                    Log.startSession("TSI.gPA");
                    // In ideal case, we should not resolve the handle across profiles. But given
                    // the fact that profile's call is handled by its parent user's in-call UI,
                    // parent user's in call UI need to be able to get phone account from the
                    // profile's phone account handle.
                    return mPhoneAccountRegistrar
                            .getPhoneAccount(accountHandle, callingUserHandle,
                            /* acrossProfiles */ true);
                } catch (Exception e) {
                    Log.e(this, e, "getPhoneAccount %s", accountHandle);
                    throw e;
                } finally {
                    Binder.restoreCallingIdentity(token);
                    Log.endSession();
                }
            }
        }

        @Override
        public int getAllPhoneAccountsCount() {
            synchronized (mLock) {
                try {
                    Log.startSession("TSI.gAPAC");
                    // This list is pre-filtered for the calling user.
                    return getAllPhoneAccounts().size();
                } catch (Exception e) {
                    Log.e(this, e, "getAllPhoneAccountsCount");
                    throw e;
                } finally {
                    Log.endSession();
                }
            }
        }

        @Override
        public List<PhoneAccount> getAllPhoneAccounts() {
            synchronized (mLock) {
                final UserHandle callingUserHandle = Binder.getCallingUserHandle();
                long token = Binder.clearCallingIdentity();
                try {
                    Log.startSession("TSI.gAPA");
                    return mPhoneAccountRegistrar.getAllPhoneAccounts(callingUserHandle);
                } catch (Exception e) {
                    Log.e(this, e, "getAllPhoneAccounts");
                    throw e;
                } finally {
                    Binder.restoreCallingIdentity(token);
                    Log.endSession();
                }
            }
        }

        @Override
        public List<PhoneAccountHandle> getAllPhoneAccountHandles() {
            synchronized (mLock) {
                final UserHandle callingUserHandle = Binder.getCallingUserHandle();
                long token = Binder.clearCallingIdentity();
                try {
                    Log.startSession("TSI.gAPAH");
                    return mPhoneAccountRegistrar.getAllPhoneAccountHandles(callingUserHandle);
                } catch (Exception e) {
                    Log.e(this, e, "getAllPhoneAccounts");
                    throw e;
                } finally {
                    Binder.restoreCallingIdentity(token);
                    Log.endSession();
                }
            }
        }

        @Override
        public PhoneAccountHandle getSimCallManager() {
            try {
                Log.startSession("TSI.gSCM");
                long token = Binder.clearCallingIdentity();
                int user;
                try {
                    user = ActivityManager.getCurrentUser();
                    return getSimCallManagerForUser(user);
                } finally {
                    Binder.restoreCallingIdentity(token);
                }
            } finally {
                Log.endSession();
            }
        }

        @Override
        public PhoneAccountHandle getSimCallManagerForUser(int user) {
            synchronized (mLock) {
                try {
                    Log.startSession("TSI.gSCMFU");
                    final int callingUid = Binder.getCallingUid();
                    long token = Binder.clearCallingIdentity();
                    try {
                        if (user != ActivityManager.getCurrentUser()) {
                            enforceCrossUserPermission(callingUid);
                        }
                        return mPhoneAccountRegistrar.getSimCallManager(UserHandle.of(user));
                    } finally {
                        Binder.restoreCallingIdentity(token);
                    }
                } catch (Exception e) {
                    Log.e(this, e, "getSimCallManager");
                    throw e;
                } finally {
                    Log.endSession();
                }
            }
        }

        @Override
        public void registerPhoneAccount(PhoneAccount account) {
            try {
                Log.startSession("TSI.rPA");
                synchronized (mLock) {
                    if (!mContext.getApplicationContext().getResources().getBoolean(
                            com.android.internal.R.bool.config_voice_capable)) {
                        Log.w(this,
                                "registerPhoneAccount not allowed on non-voice capable device.");
                        return;
                    }
                    try {
                        enforcePhoneAccountModificationForPackage(
                                account.getAccountHandle().getComponentName().getPackageName());
                        if (account.hasCapabilities(PhoneAccount.CAPABILITY_SIM_SUBSCRIPTION)) {
                            enforceRegisterSimSubscriptionPermission();
                        }
                        if (account.hasCapabilities(PhoneAccount.CAPABILITY_MULTI_USER)) {
                            enforceRegisterMultiUser();
                        }
                        enforceUserHandleMatchesCaller(account.getAccountHandle());
                        mPhoneAccountRegistrar.registerPhoneAccount(account);
                        // Broadcast an intent indicating the phone account which was registered.
                        long token = Binder.clearCallingIdentity();
                        try {
                            Intent intent = new Intent(
                                    TelecomManager.ACTION_PHONE_ACCOUNT_REGISTERED);
                            intent.putExtra(TelecomManager.EXTRA_PHONE_ACCOUNT_HANDLE,
                                    account.getAccountHandle());
                            Log.i(this, "Sending phone-account registered intent as user");
                            mContext.sendBroadcastAsUser(intent, UserHandle.ALL,
                                    PERMISSION_PROCESS_PHONE_ACCOUNT_REGISTRATION);
                        } finally {
                            Binder.restoreCallingIdentity(token);
                        }
                    } catch (Exception e) {
                        Log.e(this, e, "registerPhoneAccount %s", account);
                        throw e;
                    }
                }
            } finally {
                Log.endSession();
            }
        }

        @Override
        public void unregisterPhoneAccount(PhoneAccountHandle accountHandle) {
            synchronized (mLock) {
                try {
                    Log.startSession("TSI.uPA");
                    enforcePhoneAccountModificationForPackage(
                            accountHandle.getComponentName().getPackageName());
                    enforceUserHandleMatchesCaller(accountHandle);
                    mPhoneAccountRegistrar.unregisterPhoneAccount(accountHandle);

                    // Broadcast an intent indicating the phone account which was unregistered.
                    long token = Binder.clearCallingIdentity();
                    try {
                        Intent intent =
                                new Intent(TelecomManager.ACTION_PHONE_ACCOUNT_UNREGISTERED);
                        intent.putExtra(
                                TelecomManager.EXTRA_PHONE_ACCOUNT_HANDLE, accountHandle);
                        Log.i(this, "Sending phone-account unregistered intent as user");
                        mContext.sendBroadcastAsUser(intent, UserHandle.ALL,
                                PERMISSION_PROCESS_PHONE_ACCOUNT_REGISTRATION);
                    } finally {
                        Binder.restoreCallingIdentity(token);
                    }
                } catch (Exception e) {
                    Log.e(this, e, "unregisterPhoneAccount %s", accountHandle);
                    throw e;
                } finally {
                    Log.endSession();
                }
            }
        }

        @Override
        public void clearAccounts(String packageName) {
            synchronized (mLock) {
                try {
                    Log.startSession("TSI.cA");
                    enforcePhoneAccountModificationForPackage(packageName);
                    mPhoneAccountRegistrar
                            .clearAccounts(packageName, Binder.getCallingUserHandle());
                } catch (Exception e) {
                    Log.e(this, e, "clearAccounts %s", packageName);
                    throw e;
                } finally {
                    Log.endSession();
                }
            }
        }

        /**
         * @see android.telecom.TelecomManager#isVoiceMailNumber
         */
        @Override
        public boolean isVoiceMailNumber(PhoneAccountHandle accountHandle, String number,
                String callingPackage) {
            try {
                Log.startSession("TSI.iVMN");
                synchronized (mLock) {
                    if (!canReadPhoneState(callingPackage, "isVoiceMailNumber")) {
                        return false;
                    }
                    final UserHandle callingUserHandle = Binder.getCallingUserHandle();
                    if (!isPhoneAccountHandleVisibleToCallingUser(accountHandle,
                            callingUserHandle)) {
                        Log.d(this, "%s is not visible for the calling user [iVMN]", accountHandle);
                        return false;
                    }
                    long token = Binder.clearCallingIdentity();
                    try {
                        return mPhoneAccountRegistrar.isVoiceMailNumber(accountHandle, number);
                    } catch (Exception e) {
                        Log.e(this, e, "getSubscriptionIdForPhoneAccount");
                        throw e;
                    } finally {
                        Binder.restoreCallingIdentity(token);
                    }
                }
            } finally {
                Log.endSession();
            }
        }

        /**
         * @see android.telecom.TelecomManager#getVoiceMailNumber
         */
        @Override
        public String getVoiceMailNumber(PhoneAccountHandle accountHandle, String callingPackage) {
            try {
                Log.startSession("TSI.gVMN");
                synchronized (mLock) {
                    if (!canReadPhoneState(callingPackage, "getVoiceMailNumber")) {
                        return null;
                    }
                    try {
                        final UserHandle callingUserHandle = Binder.getCallingUserHandle();
                        if (!isPhoneAccountHandleVisibleToCallingUser(accountHandle,
                                callingUserHandle)) {
                            Log.d(this, "%s is not visible for the calling user [gVMN]",
                                    accountHandle);
                            return null;
                        }
                        int subId = mSubscriptionManagerAdapter.getDefaultVoiceSubId();
                        if (accountHandle != null) {
                            subId = mPhoneAccountRegistrar
                                    .getSubscriptionIdForPhoneAccount(accountHandle);
                        }
                        return getTelephonyManager().getVoiceMailNumber(subId);
                    } catch (Exception e) {
                        Log.e(this, e, "getSubscriptionIdForPhoneAccount");
                        throw e;
                    }
                }
            } finally {
                Log.endSession();
            }
        }

        /**
         * @see android.telecom.TelecomManager#getLine1Number
         */
        @Override
        public String getLine1Number(PhoneAccountHandle accountHandle, String callingPackage) {
            try {
                Log.startSession("getL1N");
                if (!canReadPhoneState(callingPackage, "getLine1Number")) {
                    return null;
                }

                synchronized (mLock) {
                    final UserHandle callingUserHandle = Binder.getCallingUserHandle();
                    if (!isPhoneAccountHandleVisibleToCallingUser(accountHandle,
                            callingUserHandle)) {
                        Log.d(this, "%s is not visible for the calling user [gL1N]", accountHandle);
                        return null;
                    }

                    long token = Binder.clearCallingIdentity();
                    try {
                        int subId = mPhoneAccountRegistrar.getSubscriptionIdForPhoneAccount(
                                accountHandle);
                        return getTelephonyManager().getLine1Number(subId);
                    } catch (Exception e) {
                        Log.e(this, e, "getSubscriptionIdForPhoneAccount");
                        throw e;
                    } finally {
                        Binder.restoreCallingIdentity(token);
                    }
                }
            } finally {
                Log.endSession();
            }
        }

        /**
         * @see android.telecom.TelecomManager#silenceRinger
         */
        @Override
        public void silenceRinger(String callingPackage) {
            try {
                Log.startSession("TSI.sR");
                synchronized (mLock) {
                    enforcePermissionOrPrivilegedDialer(MODIFY_PHONE_STATE, callingPackage);

                    long token = Binder.clearCallingIdentity();
                    try {
                        Log.i(this, "Silence Ringer requested by %s", callingPackage);
                        mCallsManager.getCallAudioManager().silenceRingers();
                        mCallsManager.getInCallController().silenceRinger();
                    } finally {
                        Binder.restoreCallingIdentity(token);
                    }
                }
            } finally {
                Log.endSession();
            }
        }

        /**
         * @see android.telecom.TelecomManager#getDefaultPhoneApp
         * @deprecated - Use {@link android.telecom.TelecomManager#getDefaultDialerPackage()}
         *         instead.
         */
        @Override
        public ComponentName getDefaultPhoneApp() {
            try {
                Log.startSession("TSI.gDPA");
                // No need to synchronize
                Resources resources = mContext.getResources();
                return new ComponentName(
                        resources.getString(R.string.ui_default_package),
                        resources.getString(R.string.dialer_default_class));
            } finally {
                Log.endSession();
            }
        }

        /**
         * @return the package name of the current user-selected default dialer. If no default
         *         has been selected, the package name of the system dialer is returned. If
         *         neither exists, then {@code null} is returned.
         * @see android.telecom.TelecomManager#getDefaultDialerPackage
         */
        @Override
        public String getDefaultDialerPackage() {
            try {
                Log.startSession("TSI.gDDP");
                final long token = Binder.clearCallingIdentity();
                try {
                    return mDefaultDialerManagerAdapter.getDefaultDialerApplication(mContext);
                } finally {
                    Binder.restoreCallingIdentity(token);
                }
            } finally {
                Log.endSession();
            }
        }

        /**
         * @see android.telecom.TelecomManager#getSystemDialerPackage
         */
        @Override
        public String getSystemDialerPackage() {
            try {
                Log.startSession("TSI.gSDP");
                return mContext.getResources().getString(R.string.ui_default_package);
            } finally {
                Log.endSession();
            }
        }

        /**
         * @see android.telecom.TelecomManager#isInCall
         */
        @Override
        public boolean isInCall(String callingPackage) {
            try {
                Log.startSession("TSI.iIC");
                if (!canReadPhoneState(callingPackage, "isInCall")) {
                    return false;
                }

                synchronized (mLock) {
                    final int callState = mCallsManager.getCallState();
                    return callState == TelephonyManager.CALL_STATE_OFFHOOK
                            || callState == TelephonyManager.CALL_STATE_RINGING;
                }
            } finally {
                Log.endSession();
            }
        }

        /**
         * @see android.telecom.TelecomManager#isRinging
         */
        @Override
        public boolean isRinging(String callingPackage) {
            try {
                Log.startSession("TSI.iR");
                if (!canReadPhoneState(callingPackage, "isRinging")) {
                    return false;
                }

                synchronized (mLock) {
                    // Note: We are explicitly checking the calls telecom is tracking rather than
                    // relying on mCallsManager#getCallState(). Since getCallState() relies on the
                    // current state as tracked by PhoneStateBroadcaster, any failure to properly
                    // track the current call state there could result in the wrong ringing state
                    // being reported by this API.
                    return mCallsManager.hasRingingCall();
                }
            } finally {
                Log.endSession();
            }
        }

        /**
         * @see TelecomManager#getCallState
         */
        @Override
        public int getCallState() {
            try {
                Log.startSession("TSI.getCallState");
                synchronized (mLock) {
                    return mCallsManager.getCallState();
                }
            } finally {
                Log.endSession();
            }
        }

        /**
         * @see android.telecom.TelecomManager#endCall
         */
        @Override
        public boolean endCall() {
            try {
                Log.startSession("TSI.eC");
                synchronized (mLock) {
                    enforceModifyPermission();

                    long token = Binder.clearCallingIdentity();
                    try {
                        return endCallInternal();
                    } finally {
                        Binder.restoreCallingIdentity(token);
                    }
                }
            } finally {
                Log.endSession();
            }
        }

        /**
         * @see android.telecom.TelecomManager#acceptRingingCall
         */
        @Override
        public void acceptRingingCall() {
            try {
                Log.startSession("TSI.aRC");
                synchronized (mLock) {
                    enforceModifyPermission();

                    long token = Binder.clearCallingIdentity();
                    try {
                        acceptRingingCallInternal(DEFAULT_VIDEO_STATE);
                    } finally {
                        Binder.restoreCallingIdentity(token);
                    }
                }
            } finally {
                Log.endSession();
            }
        }

        /**
         * @see android.telecom.TelecomManager#acceptRingingCall(int)
         *
         */
        @Override
        public void acceptRingingCallWithVideoState(int videoState) {
            try {
                Log.startSession("TSI.aRCWVS");
                synchronized (mLock) {
                    enforceModifyPermission();

                    long token = Binder.clearCallingIdentity();
                    try {
                        acceptRingingCallInternal(videoState);
                    } finally {
                        Binder.restoreCallingIdentity(token);
                    }
                }
            } finally {
                Log.endSession();
            }
        }

        /**
         * @see android.telecom.TelecomManager#showInCallScreen
         */
        @Override
        public void showInCallScreen(boolean showDialpad, String callingPackage) {
            try {
                Log.startSession("TSI.sICS");
                if (!canReadPhoneState(callingPackage, "showInCallScreen")) {
                    return;
                }

                synchronized (mLock) {

                    long token = Binder.clearCallingIdentity();
                    try {
                        mCallsManager.getInCallController().bringToForeground(showDialpad);
                    } finally {
                        Binder.restoreCallingIdentity(token);
                    }
                }
            } finally {
                Log.endSession();
            }
        }

        /**
         * @see android.telecom.TelecomManager#cancelMissedCallsNotification
         */
        @Override
        public void cancelMissedCallsNotification(String callingPackage) {
            try {
                Log.startSession("TSI.cMCN");
                synchronized (mLock) {
                    enforcePermissionOrPrivilegedDialer(MODIFY_PHONE_STATE, callingPackage);
                    UserHandle userHandle = Binder.getCallingUserHandle();
                    long token = Binder.clearCallingIdentity();
                    try {
                        mCallsManager.getMissedCallNotifier().clearMissedCalls(userHandle);
                    } finally {
                        Binder.restoreCallingIdentity(token);
                    }
                }
            } finally {
                Log.endSession();
            }
        }
        /**
         * @see android.telecom.TelecomManager#handleMmi
         */
        @Override
        public boolean handlePinMmi(String dialString, String callingPackage) {
            try {
                Log.startSession("TSI.hPM");
                synchronized (mLock) {
                    enforcePermissionOrPrivilegedDialer(MODIFY_PHONE_STATE, callingPackage);

                    // Switch identity so that TelephonyManager checks Telecom's permissions
                    // instead.
                    long token = Binder.clearCallingIdentity();
                    boolean retval = false;
                    try {
                        retval = getTelephonyManager().handlePinMmi(dialString);
                    } finally {
                        Binder.restoreCallingIdentity(token);
                    }

                    return retval;
                }
            }finally {
                Log.endSession();
            }
        }

        /**
         * @see android.telecom.TelecomManager#handleMmi
         */
        @Override
        public boolean handlePinMmiForPhoneAccount(PhoneAccountHandle accountHandle,
                String dialString, String callingPackage) {
            try {
                Log.startSession("TSI.hPMFPA");
                synchronized (mLock) {
                    enforcePermissionOrPrivilegedDialer(MODIFY_PHONE_STATE, callingPackage);

                    UserHandle callingUserHandle = Binder.getCallingUserHandle();
                    if (!isPhoneAccountHandleVisibleToCallingUser(accountHandle,
                            callingUserHandle)) {
                        Log.d(this, "%s is not visible for the calling user [hMMI]", accountHandle);
                        return false;
                    }

                    // Switch identity so that TelephonyManager checks Telecom's permissions
                    // instead.
                    long token = Binder.clearCallingIdentity();
                    boolean retval = false;
                    try {
                        int subId = mPhoneAccountRegistrar.getSubscriptionIdForPhoneAccount(
                                accountHandle);
                        retval = getTelephonyManager().handlePinMmiForSubscriber(subId, dialString);
                    } finally {
                        Binder.restoreCallingIdentity(token);
                    }
                    return retval;
                }
            }finally {
                Log.endSession();
            }
        }

        /**
         * @see android.telecom.TelecomManager#getAdnUriForPhoneAccount
         */
        @Override
        public Uri getAdnUriForPhoneAccount(PhoneAccountHandle accountHandle,
                String callingPackage) {
            try {
                Log.startSession("TSI.aAUFPA");
                synchronized (mLock) {
                    enforcePermissionOrPrivilegedDialer(MODIFY_PHONE_STATE, callingPackage);
                    if (!isPhoneAccountHandleVisibleToCallingUser(accountHandle,
                            Binder.getCallingUserHandle())) {
                        Log.d(this, "%s is not visible for the calling user [gA4PA]",
                                accountHandle);
                        return null;
                    }
                    // Switch identity so that TelephonyManager checks Telecom's permissions
                    // instead.
                    long token = Binder.clearCallingIdentity();
                    String retval = "content://icc/adn/";
                    try {
                        long subId = mPhoneAccountRegistrar
                                .getSubscriptionIdForPhoneAccount(accountHandle);
                        retval = retval + "subId/" + subId;
                    } finally {
                        Binder.restoreCallingIdentity(token);
                    }

                    return Uri.parse(retval);
                }
            } finally {
                Log.endSession();
            }
        }

        /**
         * @see android.telecom.TelecomManager#isTtySupported
         */
        @Override
        public boolean isTtySupported(String callingPackage) {
            try {
                Log.startSession("TSI.iTS");
                if (!canReadPhoneState(callingPackage, "hasVoiceMailNumber")) {
                    return false;
                }

                synchronized (mLock) {
                    return mCallsManager.isTtySupported();
                }
            } finally {
                Log.endSession();
            }
        }

        /**
         * @see android.telecom.TelecomManager#getCurrentTtyMode
         */
        @Override
        public int getCurrentTtyMode(String callingPackage) {
            try {
                Log.startSession("TSI.gCTM");
                if (!canReadPhoneState(callingPackage, "getCurrentTtyMode")) {
                    return TelecomManager.TTY_MODE_OFF;
                }

                synchronized (mLock) {
                    return mCallsManager.getCurrentTtyMode();
                }
            } finally {
                Log.endSession();
            }
        }

        /**
         * @see android.telecom.TelecomManager#addNewIncomingCall
         */
        @Override
        public void addNewIncomingCall(PhoneAccountHandle phoneAccountHandle, Bundle extras) {
            try {
                Log.startSession("TSI.aNIC");
                synchronized (mLock) {
                    Log.i(this, "Adding new incoming call with phoneAccountHandle %s",
                            phoneAccountHandle);
                    if (phoneAccountHandle != null &&
                            phoneAccountHandle.getComponentName() != null) {
                        // TODO(sail): Add unit tests for adding incoming calls from a SIM call
                        // manager.
                        if (isCallerSimCallManager() && TelephonyUtil.isPstnComponentName(
                                phoneAccountHandle.getComponentName())) {
                            Log.v(this, "Allowing call manager to add incoming call with PSTN" +
                                    " handle");
                        } else {
                            mAppOpsManager.checkPackage(
                                    Binder.getCallingUid(),
                                    phoneAccountHandle.getComponentName().getPackageName());
                            // Make sure it doesn't cross the UserHandle boundary
                            enforceUserHandleMatchesCaller(phoneAccountHandle);
                            enforcePhoneAccountIsRegisteredEnabled(phoneAccountHandle,
                                    Binder.getCallingUserHandle());
                        }
                        long token = Binder.clearCallingIdentity();
                        try {
                            Intent intent = new Intent(TelecomManager.ACTION_INCOMING_CALL);
                            intent.putExtra(TelecomManager.EXTRA_PHONE_ACCOUNT_HANDLE,
                                    phoneAccountHandle);
                            intent.putExtra(CallIntentProcessor.KEY_IS_INCOMING_CALL, true);
                            if (extras != null) {
                                extras.setDefusable(true);
                                intent.putExtra(TelecomManager.EXTRA_INCOMING_CALL_EXTRAS, extras);
                            }
                            mCallIntentProcessorAdapter.processIncomingCallIntent(
                                    mCallsManager, intent);
                        } finally {
                            Binder.restoreCallingIdentity(token);
                        }
                    } else {
                        Log.w(this, "Null phoneAccountHandle. Ignoring request to add new" +
                                " incoming call");
                    }
                }
            } finally {
                Log.endSession();
            }
        }

        /**
         * @see android.telecom.TelecomManager#addNewUnknownCall
         */
        @Override
        public void addNewUnknownCall(PhoneAccountHandle phoneAccountHandle, Bundle extras) {
            try {
                Log.startSession("TSI.aNUC");
                synchronized (mLock) {
                    if (phoneAccountHandle != null &&
                            phoneAccountHandle.getComponentName() != null) {
                        mAppOpsManager.checkPackage(
                                Binder.getCallingUid(),
                                phoneAccountHandle.getComponentName().getPackageName());

                        // Make sure it doesn't cross the UserHandle boundary
                        enforceUserHandleMatchesCaller(phoneAccountHandle);
                        enforcePhoneAccountIsRegisteredEnabled(phoneAccountHandle,
                                Binder.getCallingUserHandle());
                        long token = Binder.clearCallingIdentity();

                        try {
                            Intent intent = new Intent(TelecomManager.ACTION_NEW_UNKNOWN_CALL);
                            if (extras != null) {
                                extras.setDefusable(true);
                                intent.putExtras(extras);
                            }
                            intent.putExtra(CallIntentProcessor.KEY_IS_UNKNOWN_CALL, true);
                            intent.putExtra(TelecomManager.EXTRA_PHONE_ACCOUNT_HANDLE,
                                    phoneAccountHandle);
                            mCallIntentProcessorAdapter.processUnknownCallIntent(mCallsManager, intent);
                        } finally {
                            Binder.restoreCallingIdentity(token);
                        }
                    } else {
                        Log.i(this,
                                "Null phoneAccountHandle or not initiated by Telephony. " +
                                        "Ignoring request to add new unknown call.");
                    }
                }
            } finally {
                Log.endSession();
            }
        }

        /**
         * @see android.telecom.TelecomManager#placeCall
         */
        @Override
        public void placeCall(Uri handle, Bundle extras, String callingPackage) {
            try {
                Log.startSession("TSI.pC");
                enforceCallingPackage(callingPackage);
                if (!canCallPhone(callingPackage, "placeCall")) {
                    throw new SecurityException("Package " + callingPackage
                            + " is not allowed to place phone calls");
                }

                // Note: we can still get here for the default/system dialer, even if the Phone
                // permission is turned off. This is because the default/system dialer is always
                // allowed to attempt to place a call (regardless of permission state), in case
                // it turns out to be an emergency call. If the permission is denied and the
                // call is being made to a non-emergency number, the call will be denied later on
                // by {@link UserCallIntentProcessor}.

                final boolean hasCallAppOp = mAppOpsManager.noteOp(AppOpsManager.OP_CALL_PHONE,
                        Binder.getCallingUid(), callingPackage) == AppOpsManager.MODE_ALLOWED;

                final boolean hasCallPermission = mContext.checkCallingPermission(CALL_PHONE) ==
                        PackageManager.PERMISSION_GRANTED;

                synchronized (mLock) {
                    final UserHandle userHandle = Binder.getCallingUserHandle();
                    long token = Binder.clearCallingIdentity();
                    try {
                        final Intent intent = new Intent(Intent.ACTION_CALL, handle);
                        if (extras != null) {
                            extras.setDefusable(true);
                            intent.putExtras(extras);
                        }
                        mUserCallIntentProcessorFactory.create(mContext, userHandle)
                                .processIntent(
                                        intent, callingPackage, hasCallAppOp && hasCallPermission);
                    } finally {
                        Binder.restoreCallingIdentity(token);
                    }
                }
            } finally {
                Log.endSession();
            }
        }

        /**
         * @see android.telecom.TelecomManager#enablePhoneAccount
         */
        @Override
        public boolean enablePhoneAccount(PhoneAccountHandle accountHandle, boolean isEnabled) {
            try {
                Log.startSession("TSI.ePA");
                enforceModifyPermission();
                synchronized (mLock) {
                    long token = Binder.clearCallingIdentity();
                    try {
                        // enable/disable phone account
                        return mPhoneAccountRegistrar.enablePhoneAccount(accountHandle, isEnabled);
                    } finally {
                        Binder.restoreCallingIdentity(token);
                    }
                }
            } finally {
                Log.endSession();
            }
        }

        @Override
        public boolean setDefaultDialer(String packageName) {
            try {
                Log.startSession("TSI.sDD");
                enforcePermission(MODIFY_PHONE_STATE);
                enforcePermission(WRITE_SECURE_SETTINGS);
                synchronized (mLock) {
                    long token = Binder.clearCallingIdentity();
                    try {
                        final boolean result =
                                mDefaultDialerManagerAdapter
                                        .setDefaultDialerApplication(mContext, packageName);
                        if (result) {
                            final Intent intent =
                                    new Intent(TelecomManager.ACTION_DEFAULT_DIALER_CHANGED);
                            intent.putExtra(TelecomManager.EXTRA_CHANGE_DEFAULT_DIALER_PACKAGE_NAME,
                                    packageName);
                            mContext.sendBroadcastAsUser(intent,
                                    new UserHandle(ActivityManager.getCurrentUser()));
                        }
                        return result;
                    } finally {
                        Binder.restoreCallingIdentity(token);
                    }
                }
            } finally {
                Log.endSession();
            }
        }

        @Override
        public List<ParcelableCallAnalytics> dumpCallAnalytics() {
            try {
                Log.startSession("TSI.dCA");
                enforcePermission(DUMP);
                return Arrays.asList(Analytics.dumpToParcelableAnalytics());
            } finally {
                Log.endSession();
            }
        }

        /**
         * Dumps the current state of the TelecomService.  Used when generating problem reports.
         *
         * @param fd The file descriptor.
         * @param writer The print writer to dump the state to.
         * @param args Optional dump arguments.
         */
        @Override
        protected void dump(FileDescriptor fd, final PrintWriter writer, String[] args) {
            if (mContext.checkCallingOrSelfPermission(
                    android.Manifest.permission.DUMP)
                    != PackageManager.PERMISSION_GRANTED) {
                writer.println("Permission Denial: can't dump TelecomService " +
                        "from from pid=" + Binder.getCallingPid() + ", uid=" +
                        Binder.getCallingUid());
                return;
            }

            final IndentingPrintWriter pw = new IndentingPrintWriter(writer, "  ");
            if (mCallsManager != null) {
                pw.println("CallsManager: ");
                pw.increaseIndent();
                mCallsManager.dump(pw);
                pw.decreaseIndent();

                pw.println("PhoneAccountRegistrar: ");
                pw.increaseIndent();
                mPhoneAccountRegistrar.dump(pw);
                pw.decreaseIndent();

                pw.println("Analytics:");
                pw.increaseIndent();
                Analytics.dump(pw);
                pw.decreaseIndent();
            }

            Log.dumpCallEvents(pw);
        }

        /**
         * @see android.telecom.TelecomManager#createManageBlockedNumbersIntent
         */
        @Override
        public Intent createManageBlockedNumbersIntent() {
            return BlockedNumbersActivity.getIntentForStartingActivity();
        }
    };

    private Context mContext;
    private AppOpsManager mAppOpsManager;
    private UserManager mUserManager;
    private PackageManager mPackageManager;
    private CallsManager mCallsManager;
    private final PhoneAccountRegistrar mPhoneAccountRegistrar;
    private final CallIntentProcessor.Adapter mCallIntentProcessorAdapter;
    private final UserCallIntentProcessorFactory mUserCallIntentProcessorFactory;
    private final DefaultDialerManagerAdapter mDefaultDialerManagerAdapter;
    private final SubscriptionManagerAdapter mSubscriptionManagerAdapter;
    private final TelecomSystem.SyncRoot mLock;

    public TelecomServiceImpl(
            Context context,
            CallsManager callsManager,
            PhoneAccountRegistrar phoneAccountRegistrar,
            CallIntentProcessor.Adapter callIntentProcessorAdapter,
            UserCallIntentProcessorFactory userCallIntentProcessorFactory,
            DefaultDialerManagerAdapter defaultDialerManagerAdapter,
            SubscriptionManagerAdapter subscriptionManagerAdapter,
            TelecomSystem.SyncRoot lock) {
        mContext = context;
        mAppOpsManager = (AppOpsManager) mContext.getSystemService(Context.APP_OPS_SERVICE);

        mUserManager = (UserManager) mContext.getSystemService(Context.USER_SERVICE);
        mPackageManager = mContext.getPackageManager();

        mCallsManager = callsManager;
        mLock = lock;
        mPhoneAccountRegistrar = phoneAccountRegistrar;
        mUserCallIntentProcessorFactory = userCallIntentProcessorFactory;
        mDefaultDialerManagerAdapter = defaultDialerManagerAdapter;
        mCallIntentProcessorAdapter = callIntentProcessorAdapter;
        mSubscriptionManagerAdapter = subscriptionManagerAdapter;
    }

    public ITelecomService.Stub getBinder() {
        return mBinderImpl;
    }

    //
    // Supporting methods for the ITelecomService interface implementation.
    //

    private boolean isPhoneAccountHandleVisibleToCallingUser(
            PhoneAccountHandle phoneAccountUserHandle, UserHandle callingUser) {
        return mPhoneAccountRegistrar.getPhoneAccount(phoneAccountUserHandle, callingUser) != null;
    }

    private boolean isCallerSystemApp() {
        int uid = Binder.getCallingUid();
        String[] packages = mPackageManager.getPackagesForUid(uid);
        for (String packageName : packages) {
            if (isPackageSystemApp(packageName)) {
                return true;
            }
        }
        return false;
    }

    private boolean isPackageSystemApp(String packageName) {
        try {
            ApplicationInfo applicationInfo = mPackageManager.getApplicationInfo(packageName,
                    PackageManager.GET_META_DATA);
            if ((applicationInfo.flags & ApplicationInfo.FLAG_SYSTEM) != 0) {
                return true;
            }
        } catch (PackageManager.NameNotFoundException e) {
        }
        return false;
    }

    private void acceptRingingCallInternal(int videoState) {
        Call call = mCallsManager.getFirstCallWithState(CallState.RINGING);
        if (call != null) {
            if (videoState == DEFAULT_VIDEO_STATE || !isValidAcceptVideoState(videoState)) {
                videoState = call.getVideoState();
            }
            call.answer(videoState);
        }
    }

    private boolean endCallInternal() {
        // Always operate on the foreground call if one exists, otherwise get the first call in
        // priority order by call-state.
        Call call = mCallsManager.getForegroundCall();
        if (call == null) {
            call = mCallsManager.getFirstCallWithState(
                    CallState.ACTIVE,
                    CallState.DIALING,
                    CallState.RINGING,
                    CallState.ON_HOLD);
        }

        if (call != null) {
            if (call.getState() == CallState.RINGING) {
                call.reject(false /* rejectWithMessage */, null);
            } else {
                call.disconnect();
            }
            return true;
        }

        return false;
    }

    // Enforce that the PhoneAccountHandle being passed in is both registered to the current user
    // and enabled.
    private void enforcePhoneAccountIsRegisteredEnabled(PhoneAccountHandle phoneAccountHandle,
                                                        UserHandle callingUserHandle) {
        PhoneAccount phoneAccount = mPhoneAccountRegistrar.getPhoneAccount(phoneAccountHandle,
                callingUserHandle);
        if(phoneAccount == null) {
            EventLog.writeEvent(0x534e4554, "26864502", Binder.getCallingUid(), "R");
            throw new SecurityException("This PhoneAccountHandle is not registered for this user!");
        }
        if(!phoneAccount.isEnabled()) {
            EventLog.writeEvent(0x534e4554, "26864502", Binder.getCallingUid(), "E");
            throw new SecurityException("This PhoneAccountHandle is not enabled for this user!");
        }
    }

    private void enforcePhoneAccountModificationForPackage(String packageName) {
        // TODO: Use a new telecomm permission for this instead of reusing modify.

        int result = mContext.checkCallingOrSelfPermission(MODIFY_PHONE_STATE);

        // Callers with MODIFY_PHONE_STATE can use the PhoneAccount mechanism to implement
        // built-in behavior even when PhoneAccounts are not exposed as a third-part API. They
        // may also modify PhoneAccounts on behalf of any 'packageName'.

        if (result != PackageManager.PERMISSION_GRANTED) {
            // Other callers are only allowed to modify PhoneAccounts if the relevant system
            // feature is enabled ...
            enforceConnectionServiceFeature();
            // ... and the PhoneAccounts they refer to are for their own package.
            enforceCallingPackage(packageName);
        }
    }

    private void enforcePermissionOrPrivilegedDialer(String permission, String packageName) {
        if (!isPrivilegedDialerCalling(packageName)) {
            try {
                enforcePermission(permission);
            } catch (SecurityException e) {
                Log.e(this, e, "Caller must be the default or system dialer, or have the permission"
                        + " %s to perform this operation.", permission);
                throw e;
            }
        }
    }

    private void enforceCallingPackage(String packageName) {
        mAppOpsManager.checkPackage(Binder.getCallingUid(), packageName);
    }

    private void enforceConnectionServiceFeature() {
        enforceFeature(PackageManager.FEATURE_CONNECTION_SERVICE);
    }

    private void enforceRegisterSimSubscriptionPermission() {
        enforcePermission(REGISTER_SIM_SUBSCRIPTION);
    }

    private void enforceModifyPermission() {
        enforcePermission(MODIFY_PHONE_STATE);
    }

    private void enforcePermission(String permission) {
        mContext.enforceCallingOrSelfPermission(permission, null);
    }

    private void enforceRegisterMultiUser() {
        if (!isCallerSystemApp()) {
            throw new SecurityException("CAPABILITY_MULTI_USER is only available to system apps.");
        }
    }

    private void enforceUserHandleMatchesCaller(PhoneAccountHandle accountHandle) {
        if (!Binder.getCallingUserHandle().equals(accountHandle.getUserHandle())) {
            throw new SecurityException("Calling UserHandle does not match PhoneAccountHandle's");
        }
    }

    private void enforceCrossUserPermission(int callingUid) {
        if (callingUid != Process.SYSTEM_UID && callingUid != 0) {
            mContext.enforceCallingOrSelfPermission(
                    android.Manifest.permission.INTERACT_ACROSS_USERS_FULL, "Must be system or have"
                            + " INTERACT_ACROSS_USERS_FULL permission");
        }
    }

    private void enforceFeature(String feature) {
        PackageManager pm = mContext.getPackageManager();
        if (!pm.hasSystemFeature(feature)) {
            throw new UnsupportedOperationException(
                    "System does not support feature " + feature);
        }
    }

    private boolean canReadPhoneState(String callingPackage, String message) {
        // The system/default dialer can always read phone state - so that emergency calls will
        // still work.
        if (isPrivilegedDialerCalling(callingPackage)) {
            return true;
        }

        try {
            mContext.enforceCallingOrSelfPermission(READ_PRIVILEGED_PHONE_STATE, message);
            // SKIP checking run-time OP_READ_PHONE_STATE since caller or self has PRIVILEGED
            // permission
            return true;
        } catch (SecurityException e) {
            // Accessing phone state is gated by a special permission.
            mContext.enforceCallingOrSelfPermission(READ_PHONE_STATE, message);

            // Some apps that have the permission can be restricted via app ops.
            return mAppOpsManager.noteOp(AppOpsManager.OP_READ_PHONE_STATE,
                    Binder.getCallingUid(), callingPackage) == AppOpsManager.MODE_ALLOWED;
        }
    }

    private boolean canCallPhone(String callingPackage, String message) {
        // The system/default dialer can always read phone state - so that emergency calls will
        // still work.
        if (isPrivilegedDialerCalling(callingPackage)) {
            return true;
        }

        // Accessing phone state is gated by a special permission.
        mContext.enforceCallingOrSelfPermission(CALL_PHONE, message);

        // Some apps that have the permission can be restricted via app ops.
        return mAppOpsManager.noteOp(AppOpsManager.OP_CALL_PHONE,
                Binder.getCallingUid(), callingPackage) == AppOpsManager.MODE_ALLOWED;
    }

    private boolean isCallerSimCallManager() {
        long token = Binder.clearCallingIdentity();
        PhoneAccountHandle accountHandle = null;
        try {
             accountHandle = mPhoneAccountRegistrar.getSimCallManagerOfCurrentUser();
        } finally {
            Binder.restoreCallingIdentity(token);
        }

        if (accountHandle != null) {
            try {
                mAppOpsManager.checkPackage(
                        Binder.getCallingUid(), accountHandle.getComponentName().getPackageName());
                return true;
            } catch (SecurityException e) {
            }
        }
        return false;
    }

    private boolean isPrivilegedDialerCalling(String callingPackage) {
        mAppOpsManager.checkPackage(Binder.getCallingUid(), callingPackage);
        return mDefaultDialerManagerAdapter.isDefaultOrSystemDialer(mContext, callingPackage);
    }

    private TelephonyManager getTelephonyManager() {
        return (TelephonyManager) mContext.getSystemService(Context.TELEPHONY_SERVICE);
    }

    /**
     * Determines if a video state is valid for accepting an incoming call.
     * For the purpose of accepting a call, states {@link VideoProfile#STATE_AUDIO_ONLY}, and
     * any combination of {@link VideoProfile#STATE_RX_ENABLED} and
     * {@link VideoProfile#STATE_TX_ENABLED} are considered valid.
     *
     * @param videoState The video state.
     * @return {@code true} if the video state is valid, {@code false} otherwise.
     */
    private boolean isValidAcceptVideoState(int videoState) {
        // Given a video state input, turn off TX and RX so that we can determine if those were the
        // only bits set.
        int remainingState = videoState & ~VideoProfile.STATE_TX_ENABLED;
        remainingState = remainingState & ~VideoProfile.STATE_RX_ENABLED;

        // If only TX or RX were set (or neither), the video state is valid.
        return remainingState == 0;
    }
}
