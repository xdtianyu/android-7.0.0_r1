/*
 * Copyright (C) 2006 The Android Open Source Project
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

package com.android.phone;

import android.app.Activity;
import android.app.KeyguardManager;
import android.app.ProgressDialog;
import android.content.BroadcastReceiver;
import android.content.ContentResolver;
import android.content.Context;
import android.content.ContextWrapper;
import android.content.Intent;
import android.content.IntentFilter;
import android.media.AudioManager;
import android.net.ConnectivityManager;
import android.net.Uri;
import android.os.AsyncResult;
import android.os.Bundle;
import android.os.Handler;
import android.os.Message;
import android.os.PersistableBundle;
import android.os.PowerManager;
import android.os.ServiceManager;
import android.os.SystemClock;
import android.os.SystemProperties;
import android.os.SystemService;
import android.os.UpdateLock;
import android.os.UserManager;
import android.preference.PreferenceManager;
import android.provider.Settings;
import android.telephony.CarrierConfigManager;
import android.telephony.ServiceState;
import android.telephony.SubscriptionManager;
import android.util.Log;
import android.widget.Toast;

import com.android.internal.telephony.Call;
import com.android.internal.telephony.CallManager;
import com.android.internal.telephony.IccCardConstants;
import com.android.internal.telephony.MmiCode;
import com.android.internal.telephony.Phone;
import com.android.internal.telephony.PhoneConstants;
import com.android.internal.telephony.PhoneFactory;
import com.android.internal.telephony.TelephonyCapabilities;
import com.android.internal.telephony.TelephonyIntents;
import com.android.phone.common.CallLogAsync;
import com.android.phone.settings.SettingsConstants;
import com.android.server.sip.SipService;
import com.android.services.telephony.activation.SimActivationManager;
import com.android.services.telephony.sip.SipUtil;

/**
 * Global state for the telephony subsystem when running in the primary
 * phone process.
 */
public class PhoneGlobals extends ContextWrapper {
    public static final String LOG_TAG = "PhoneApp";

    /**
     * Phone app-wide debug level:
     *   0 - no debug logging
     *   1 - normal debug logging if ro.debuggable is set (which is true in
     *       "eng" and "userdebug" builds but not "user" builds)
     *   2 - ultra-verbose debug logging
     *
     * Most individual classes in the phone app have a local DBG constant,
     * typically set to
     *   (PhoneApp.DBG_LEVEL >= 1) && (SystemProperties.getInt("ro.debuggable", 0) == 1)
     * or else
     *   (PhoneApp.DBG_LEVEL >= 2)
     * depending on the desired verbosity.
     *
     * ***** DO NOT SUBMIT WITH DBG_LEVEL > 0 *************
     */
    public static final int DBG_LEVEL = 0;

    private static final boolean DBG =
            (PhoneGlobals.DBG_LEVEL >= 1) && (SystemProperties.getInt("ro.debuggable", 0) == 1);
    private static final boolean VDBG = (PhoneGlobals.DBG_LEVEL >= 2);

    // Message codes; see mHandler below.
    private static final int EVENT_SIM_NETWORK_LOCKED = 3;
    private static final int EVENT_SIM_STATE_CHANGED = 8;
    private static final int EVENT_DATA_ROAMING_DISCONNECTED = 10;
    private static final int EVENT_DATA_ROAMING_OK = 11;
    private static final int EVENT_UNSOL_CDMA_INFO_RECORD = 12;
    private static final int EVENT_RESTART_SIP = 13;

    // The MMI codes are also used by the InCallScreen.
    public static final int MMI_INITIATE = 51;
    public static final int MMI_COMPLETE = 52;
    public static final int MMI_CANCEL = 53;
    // Don't use message codes larger than 99 here; those are reserved for
    // the individual Activities of the Phone UI.

    public static final int AIRPLANE_ON = 1;
    public static final int AIRPLANE_OFF = 0;

    /**
     * Allowable values for the wake lock code.
     *   SLEEP means the device can be put to sleep.
     *   PARTIAL means wake the processor, but we display can be kept off.
     *   FULL means wake both the processor and the display.
     */
    public enum WakeState {
        SLEEP,
        PARTIAL,
        FULL
    }

    private static PhoneGlobals sMe;

    // A few important fields we expose to the rest of the package
    // directly (rather than thru set/get methods) for efficiency.
    CallController callController;
    CallManager mCM;
    CallNotifier notifier;
    CallerInfoCache callerInfoCache;
    NotificationMgr notificationMgr;
    public PhoneInterfaceManager phoneMgr;
    public SimActivationManager simActivationManager;
    CarrierConfigLoader configLoader;

    private CallGatewayManager callGatewayManager;
    private Phone phoneInEcm;

    static boolean sVoiceCapable = true;

    // TODO: Remove, no longer used.
    CdmaPhoneCallState cdmaPhoneCallState;

    // The currently-active PUK entry activity and progress dialog.
    // Normally, these are the Emergency Dialer and the subsequent
    // progress dialog.  null if there is are no such objects in
    // the foreground.
    private Activity mPUKEntryActivity;
    private ProgressDialog mPUKEntryProgressDialog;

    private boolean mDataDisconnectedDueToRoaming = false;

    private WakeState mWakeState = WakeState.SLEEP;

    private PowerManager mPowerManager;
    private PowerManager.WakeLock mWakeLock;
    private PowerManager.WakeLock mPartialWakeLock;
    private KeyguardManager mKeyguardManager;

    private UpdateLock mUpdateLock;

    // Broadcast receiver for various intent broadcasts (see onCreate())
    private final BroadcastReceiver mReceiver = new PhoneAppBroadcastReceiver();

    /**
     * The singleton OtaUtils instance used for OTASP calls.
     *
     * The OtaUtils instance is created lazily the first time we need to
     * make an OTASP call, regardless of whether it's an interactive or
     * non-interactive OTASP call.
     */
    public OtaUtils otaUtils;

    // Following are the CDMA OTA information Objects used during OTA Call.
    // cdmaOtaProvisionData object store static OTA information that needs
    // to be maintained even during Slider open/close scenarios.
    // cdmaOtaConfigData object stores configuration info to control visiblity
    // of each OTA Screens.
    // cdmaOtaScreenState object store OTA Screen State information.
    public OtaUtils.CdmaOtaProvisionData cdmaOtaProvisionData;
    public OtaUtils.CdmaOtaConfigData cdmaOtaConfigData;
    public OtaUtils.CdmaOtaScreenState cdmaOtaScreenState;
    public OtaUtils.CdmaOtaInCallScreenUiState cdmaOtaInCallScreenUiState;

    Handler mHandler = new Handler() {
        @Override
        public void handleMessage(Message msg) {
            PhoneConstants.State phoneState;
            switch (msg.what) {
                // TODO: This event should be handled by the lock screen, just
                // like the "SIM missing" and "Sim locked" cases (bug 1804111).
                case EVENT_SIM_NETWORK_LOCKED:
                    if (getCarrierConfig().getBoolean(
                            CarrierConfigManager.KEY_IGNORE_SIM_NETWORK_LOCKED_EVENTS_BOOL)) {
                        // Some products don't have the concept of a "SIM network lock"
                        Log.i(LOG_TAG, "Ignoring EVENT_SIM_NETWORK_LOCKED event; "
                              + "not showing 'SIM network unlock' PIN entry screen");
                    } else {
                        // Normal case: show the "SIM network unlock" PIN entry screen.
                        // The user won't be able to do anything else until
                        // they enter a valid SIM network PIN.
                        Log.i(LOG_TAG, "show sim depersonal panel");
                        IccNetworkDepersonalizationPanel.showDialog();
                    }
                    break;

                case EVENT_DATA_ROAMING_DISCONNECTED:
                    notificationMgr.showDataDisconnectedRoaming();
                    break;

                case EVENT_DATA_ROAMING_OK:
                    notificationMgr.hideDataDisconnectedRoaming();
                    break;

                case MMI_COMPLETE:
                    onMMIComplete((AsyncResult) msg.obj);
                    break;

                case MMI_CANCEL:
                    PhoneUtils.cancelMmiCode(mCM.getFgPhone());
                    break;

                case EVENT_SIM_STATE_CHANGED:
                    // Marks the event where the SIM goes into ready state.
                    // Right now, this is only used for the PUK-unlocking
                    // process.
                    if (msg.obj.equals(IccCardConstants.INTENT_VALUE_ICC_READY)) {
                        // when the right event is triggered and there
                        // are UI objects in the foreground, we close
                        // them to display the lock panel.
                        if (mPUKEntryActivity != null) {
                            mPUKEntryActivity.finish();
                            mPUKEntryActivity = null;
                        }
                        if (mPUKEntryProgressDialog != null) {
                            mPUKEntryProgressDialog.dismiss();
                            mPUKEntryProgressDialog = null;
                        }
                    }
                    break;

                case EVENT_UNSOL_CDMA_INFO_RECORD:
                    //TODO: handle message here;
                    break;
                case EVENT_RESTART_SIP:
                    // This should only run if the Phone process crashed and was restarted. We do
                    // not want this running if the device is still in the FBE encrypted state.
                    // This is the same procedure that is triggered in the SipBroadcastReceiver
                    // upon BOOT_COMPLETED.
                    UserManager userManager = UserManager.get(sMe);
                    if (userManager != null && userManager.isUserUnlocked()) {
                        SipUtil.startSipService();
                    }
                    break;
            }
        }
    };

    public PhoneGlobals(Context context) {
        super(context);
        sMe = this;
    }

    public void onCreate() {
        if (VDBG) Log.v(LOG_TAG, "onCreate()...");

        ContentResolver resolver = getContentResolver();

        // Cache the "voice capable" flag.
        // This flag currently comes from a resource (which is
        // overrideable on a per-product basis):
        sVoiceCapable =
                getResources().getBoolean(com.android.internal.R.bool.config_voice_capable);
        // ...but this might eventually become a PackageManager "system
        // feature" instead, in which case we'd do something like:
        // sVoiceCapable =
        //   getPackageManager().hasSystemFeature(PackageManager.FEATURE_TELEPHONY_VOICE_CALLS);

        if (mCM == null) {
            // Initialize the telephony framework
            PhoneFactory.makeDefaultPhones(this);

            // Start TelephonyDebugService After the default phone is created.
            Intent intent = new Intent(this, TelephonyDebugService.class);
            startService(intent);

            mCM = CallManager.getInstance();
            for (Phone phone : PhoneFactory.getPhones()) {
                mCM.registerPhone(phone);
            }

            // Create the NotificationMgr singleton, which is used to display
            // status bar icons and control other status bar behavior.
            notificationMgr = NotificationMgr.init(this);

            // If PhoneGlobals has crashed and is being restarted, then restart.
            mHandler.sendEmptyMessage(EVENT_RESTART_SIP);

            // Create an instance of CdmaPhoneCallState and initialize it to IDLE
            cdmaPhoneCallState = new CdmaPhoneCallState();
            cdmaPhoneCallState.CdmaPhoneCallStateInit();

            // before registering for phone state changes
            mPowerManager = (PowerManager) getSystemService(Context.POWER_SERVICE);
            mWakeLock = mPowerManager.newWakeLock(PowerManager.FULL_WAKE_LOCK, LOG_TAG);
            // lock used to keep the processor awake, when we don't care for the display.
            mPartialWakeLock = mPowerManager.newWakeLock(PowerManager.PARTIAL_WAKE_LOCK
                    | PowerManager.ON_AFTER_RELEASE, LOG_TAG);

            mKeyguardManager = (KeyguardManager) getSystemService(Context.KEYGUARD_SERVICE);

            // Get UpdateLock to suppress system-update related events (e.g. dialog show-up)
            // during phone calls.
            mUpdateLock = new UpdateLock("phone");

            if (DBG) Log.d(LOG_TAG, "onCreate: mUpdateLock: " + mUpdateLock);

            CallLogger callLogger = new CallLogger(this, new CallLogAsync());

            callGatewayManager = CallGatewayManager.getInstance();

            // Create the CallController singleton, which is the interface
            // to the telephony layer for user-initiated telephony functionality
            // (like making outgoing calls.)
            callController = CallController.init(this, callLogger, callGatewayManager);

            // Create the CallerInfoCache singleton, which remembers custom ring tone and
            // send-to-voicemail settings.
            //
            // The asynchronous caching will start just after this call.
            callerInfoCache = CallerInfoCache.init(this);

            phoneMgr = PhoneInterfaceManager.init(this, PhoneFactory.getDefaultPhone());

            configLoader = CarrierConfigLoader.init(this);

            // Create the CallNotifer singleton, which handles
            // asynchronous events from the telephony layer (like
            // launching the incoming-call UI when an incoming call comes
            // in.)
            notifier = CallNotifier.init(this);

            PhoneUtils.registerIccStatus(mHandler, EVENT_SIM_NETWORK_LOCKED);

            // register for MMI/USSD
            mCM.registerForMmiComplete(mHandler, MMI_COMPLETE, null);

            // register connection tracking to PhoneUtils
            PhoneUtils.initializeConnectionHandler(mCM);

            // Register for misc other intent broadcasts.
            IntentFilter intentFilter =
                    new IntentFilter(Intent.ACTION_AIRPLANE_MODE_CHANGED);
            intentFilter.addAction(TelephonyIntents.ACTION_ANY_DATA_CONNECTION_STATE_CHANGED);
            intentFilter.addAction(TelephonyIntents.ACTION_SIM_STATE_CHANGED);
            intentFilter.addAction(TelephonyIntents.ACTION_RADIO_TECHNOLOGY_CHANGED);
            intentFilter.addAction(TelephonyIntents.ACTION_SERVICE_STATE_CHANGED);
            intentFilter.addAction(TelephonyIntents.ACTION_EMERGENCY_CALLBACK_MODE_CHANGED);
            registerReceiver(mReceiver, intentFilter);

            //set the default values for the preferences in the phone.
            PreferenceManager.setDefaultValues(this, R.xml.network_setting, false);

            PreferenceManager.setDefaultValues(this, R.xml.call_feature_setting, false);

            // Make sure the audio mode (along with some
            // audio-mode-related state of our own) is initialized
            // correctly, given the current state of the phone.
            PhoneUtils.setAudioMode(mCM);
        }

        cdmaOtaProvisionData = new OtaUtils.CdmaOtaProvisionData();
        cdmaOtaConfigData = new OtaUtils.CdmaOtaConfigData();
        cdmaOtaScreenState = new OtaUtils.CdmaOtaScreenState();
        cdmaOtaInCallScreenUiState = new OtaUtils.CdmaOtaInCallScreenUiState();

        simActivationManager = new SimActivationManager();

        // XXX pre-load the SimProvider so that it's ready
        resolver.getType(Uri.parse("content://icc/adn"));

        // TODO: Register for Cdma Information Records
        // phone.registerCdmaInformationRecord(mHandler, EVENT_UNSOL_CDMA_INFO_RECORD, null);

        // Read HAC settings and configure audio hardware
        if (getResources().getBoolean(R.bool.hac_enabled)) {
            int hac = android.provider.Settings.System.getInt(
                    getContentResolver(),
                    android.provider.Settings.System.HEARING_AID,
                    0);
            AudioManager audioManager = (AudioManager) getSystemService(Context.AUDIO_SERVICE);
            audioManager.setParameter(SettingsConstants.HAC_KEY,
                    hac == SettingsConstants.HAC_ENABLED
                            ? SettingsConstants.HAC_VAL_ON : SettingsConstants.HAC_VAL_OFF);
        }
    }

    /**
     * Returns the singleton instance of the PhoneApp.
     */
    public static PhoneGlobals getInstance() {
        if (sMe == null) {
            throw new IllegalStateException("No PhoneGlobals here!");
        }
        return sMe;
    }

    /**
     * Returns the singleton instance of the PhoneApp if running as the
     * primary user, otherwise null.
     */
    static PhoneGlobals getInstanceIfPrimary() {
        return sMe;
    }

    /**
     * Returns the default phone.
     *
     * WARNING: This method should be used carefully, now that there may be multiple phones.
     */
    public static Phone getPhone() {
        return PhoneFactory.getDefaultPhone();
    }

    public static Phone getPhone(int subId) {
        return PhoneFactory.getPhone(SubscriptionManager.getPhoneId(subId));
    }

    /* package */ CallManager getCallManager() {
        return mCM;
    }

    public PersistableBundle getCarrierConfig() {
        return getCarrierConfigForSubId(SubscriptionManager.getDefaultSubscriptionId());
    }

    public PersistableBundle getCarrierConfigForSubId(int subId) {
        return configLoader.getConfigForSubId(subId);
    }

    /**
     * Handles OTASP-related events from the telephony layer.
     *
     * While an OTASP call is active, the CallNotifier forwards
     * OTASP-related telephony events to this method.
     */
    void handleOtaspEvent(Message msg) {
        if (DBG) Log.d(LOG_TAG, "handleOtaspEvent(message " + msg + ")...");

        if (otaUtils == null) {
            // We shouldn't be getting OTASP events without ever
            // having started the OTASP call in the first place!
            Log.w(LOG_TAG, "handleOtaEvents: got an event but otaUtils is null! "
                  + "message = " + msg);
            return;
        }

        otaUtils.onOtaProvisionStatusChanged((AsyncResult) msg.obj);
    }

    /**
     * Similarly, handle the disconnect event of an OTASP call
     * by forwarding it to the OtaUtils instance.
     */
    /* package */ void handleOtaspDisconnect() {
        if (DBG) Log.d(LOG_TAG, "handleOtaspDisconnect()...");

        if (otaUtils == null) {
            // We shouldn't be getting OTASP events without ever
            // having started the OTASP call in the first place!
            Log.w(LOG_TAG, "handleOtaspDisconnect: otaUtils is null!");
            return;
        }

        otaUtils.onOtaspDisconnect();
    }

    /**
     * Sets the activity responsible for un-PUK-blocking the device
     * so that we may close it when we receive a positive result.
     * mPUKEntryActivity is also used to indicate to the device that
     * we are trying to un-PUK-lock the phone. In other words, iff
     * it is NOT null, then we are trying to unlock and waiting for
     * the SIM to move to READY state.
     *
     * @param activity is the activity to close when PUK has
     * finished unlocking. Can be set to null to indicate the unlock
     * or SIM READYing process is over.
     */
    void setPukEntryActivity(Activity activity) {
        mPUKEntryActivity = activity;
    }

    Activity getPUKEntryActivity() {
        return mPUKEntryActivity;
    }

    /**
     * Sets the dialog responsible for notifying the user of un-PUK-
     * blocking - SIM READYing progress, so that we may dismiss it
     * when we receive a positive result.
     *
     * @param dialog indicates the progress dialog informing the user
     * of the state of the device.  Dismissed upon completion of
     * READYing process
     */
    void setPukEntryProgressDialog(ProgressDialog dialog) {
        mPUKEntryProgressDialog = dialog;
    }

    /**
     * Controls whether or not the screen is allowed to sleep.
     *
     * Once sleep is allowed (WakeState is SLEEP), it will rely on the
     * settings for the poke lock to determine when to timeout and let
     * the device sleep {@link PhoneGlobals#setScreenTimeout}.
     *
     * @param ws tells the device to how to wake.
     */
    /* package */ void requestWakeState(WakeState ws) {
        if (VDBG) Log.d(LOG_TAG, "requestWakeState(" + ws + ")...");
        synchronized (this) {
            if (mWakeState != ws) {
                switch (ws) {
                    case PARTIAL:
                        // acquire the processor wake lock, and release the FULL
                        // lock if it is being held.
                        mPartialWakeLock.acquire();
                        if (mWakeLock.isHeld()) {
                            mWakeLock.release();
                        }
                        break;
                    case FULL:
                        // acquire the full wake lock, and release the PARTIAL
                        // lock if it is being held.
                        mWakeLock.acquire();
                        if (mPartialWakeLock.isHeld()) {
                            mPartialWakeLock.release();
                        }
                        break;
                    case SLEEP:
                    default:
                        // release both the PARTIAL and FULL locks.
                        if (mWakeLock.isHeld()) {
                            mWakeLock.release();
                        }
                        if (mPartialWakeLock.isHeld()) {
                            mPartialWakeLock.release();
                        }
                        break;
                }
                mWakeState = ws;
            }
        }
    }

    /**
     * If we are not currently keeping the screen on, then poke the power
     * manager to wake up the screen for the user activity timeout duration.
     */
    /* package */ void wakeUpScreen() {
        synchronized (this) {
            if (mWakeState == WakeState.SLEEP) {
                if (DBG) Log.d(LOG_TAG, "pulse screen lock");
                mPowerManager.wakeUp(SystemClock.uptimeMillis(), "android.phone:WAKE");
            }
        }
    }

    /**
     * Sets the wake state and screen timeout based on the current state
     * of the phone, and the current state of the in-call UI.
     *
     * This method is a "UI Policy" wrapper around
     * {@link PhoneGlobals#requestWakeState} and {@link PhoneGlobals#setScreenTimeout}.
     *
     * It's safe to call this method regardless of the state of the Phone
     * (e.g. whether or not it's idle), and regardless of the state of the
     * Phone UI (e.g. whether or not the InCallScreen is active.)
     */
    /* package */ void updateWakeState() {
        PhoneConstants.State state = mCM.getState();

        // True if the speakerphone is in use.  (If so, we *always* use
        // the default timeout.  Since the user is obviously not holding
        // the phone up to his/her face, we don't need to worry about
        // false touches, and thus don't need to turn the screen off so
        // aggressively.)
        // Note that we need to make a fresh call to this method any
        // time the speaker state changes.  (That happens in
        // PhoneUtils.turnOnSpeaker().)
        boolean isSpeakerInUse = (state == PhoneConstants.State.OFFHOOK) && PhoneUtils.isSpeakerOn(this);

        // TODO (bug 1440854): The screen timeout *might* also need to
        // depend on the bluetooth state, but this isn't as clear-cut as
        // the speaker state (since while using BT it's common for the
        // user to put the phone straight into a pocket, in which case the
        // timeout should probably still be short.)

        // Decide whether to force the screen on or not.
        //
        // Force the screen to be on if the phone is ringing or dialing,
        // or if we're displaying the "Call ended" UI for a connection in
        // the "disconnected" state.
        // However, if the phone is disconnected while the user is in the
        // middle of selecting a quick response message, we should not force
        // the screen to be on.
        //
        boolean isRinging = (state == PhoneConstants.State.RINGING);
        boolean isDialing = (mCM.getFgPhone().getForegroundCall().getState() == Call.State.DIALING);
        boolean keepScreenOn = isRinging || isDialing;
        // keepScreenOn == true means we'll hold a full wake lock:
        requestWakeState(keepScreenOn ? WakeState.FULL : WakeState.SLEEP);
    }

    KeyguardManager getKeyguardManager() {
        return mKeyguardManager;
    }

    private void onMMIComplete(AsyncResult r) {
        if (VDBG) Log.d(LOG_TAG, "onMMIComplete()...");
        MmiCode mmiCode = (MmiCode) r.result;
        PhoneUtils.displayMMIComplete(mmiCode.getPhone(), getInstance(), mmiCode, null, null);
    }

    private void initForNewRadioTechnology(int phoneId) {
        if (DBG) Log.d(LOG_TAG, "initForNewRadioTechnology...");

        final Phone phone = PhoneFactory.getPhone(phoneId);
        if (phone == null || !TelephonyCapabilities.supportsOtasp(phone)) {
            // Clean up OTA for non-CDMA since it is only valid for CDMA.
            clearOtaState();
        }

        notifier.updateCallNotifierRegistrationsAfterRadioTechnologyChange();
    }

    private void handleAirplaneModeChange(int newMode) {
        if (newMode == AIRPLANE_ON) {
            // If we are trying to turn off the radio, make sure there are no active
            // emergency calls.  If there are, switch airplane mode back to off.
            if (PhoneUtils.isInEmergencyCall(mCM)) {
                // Switch airplane mode back to off.
                ConnectivityManager.from(this).setAirplaneMode(false);
                Toast.makeText(this, R.string.radio_off_during_emergency_call, Toast.LENGTH_LONG)
                        .show();
                Log.i(LOG_TAG, "Ignoring airplane mode: emergency call. Turning airplane off");
            } else {
                Log.i(LOG_TAG, "Turning radio off - airplane");
                PhoneUtils.setRadioPower(false);
            }
        } else {
            Log.i(LOG_TAG, "Turning radio on - airplane");
            PhoneUtils.setRadioPower(true);
        }
    }

    /**
     * Receiver for misc intent broadcasts the Phone app cares about.
     */
    private class PhoneAppBroadcastReceiver extends BroadcastReceiver {
        @Override
        public void onReceive(Context context, Intent intent) {
            String action = intent.getAction();
            if (action.equals(Intent.ACTION_AIRPLANE_MODE_CHANGED)) {
                int airplaneMode = Settings.Global.getInt(getContentResolver(),
                        Settings.Global.AIRPLANE_MODE_ON, AIRPLANE_OFF);
                // Treat any non-OFF values as ON.
                if (airplaneMode != AIRPLANE_OFF) {
                    airplaneMode = AIRPLANE_ON;
                }
                handleAirplaneModeChange(airplaneMode);
            } else if (action.equals(TelephonyIntents.ACTION_ANY_DATA_CONNECTION_STATE_CHANGED)) {
                int subId = intent.getIntExtra(PhoneConstants.SUBSCRIPTION_KEY,
                        SubscriptionManager.INVALID_SUBSCRIPTION_ID);
                int phoneId = SubscriptionManager.getPhoneId(subId);
                String state = intent.getStringExtra(PhoneConstants.STATE_KEY);
                if (VDBG) {
                    Log.d(LOG_TAG, "mReceiver: ACTION_ANY_DATA_CONNECTION_STATE_CHANGED");
                    Log.d(LOG_TAG, "- state: " + state);
                    Log.d(LOG_TAG, "- reason: "
                    + intent.getStringExtra(PhoneConstants.STATE_CHANGE_REASON_KEY));
                    Log.d(LOG_TAG, "- subId: " + subId);
                    Log.d(LOG_TAG, "- phoneId: " + phoneId);
                }
                Phone phone = SubscriptionManager.isValidPhoneId(phoneId) ?
                        PhoneFactory.getPhone(phoneId) : PhoneFactory.getDefaultPhone();

                // The "data disconnected due to roaming" notification is shown
                // if (a) you have the "data roaming" feature turned off, and
                // (b) you just lost data connectivity because you're roaming.
                boolean disconnectedDueToRoaming =
                        !phone.getDataRoamingEnabled()
                        && PhoneConstants.DataState.DISCONNECTED.equals(state)
                        && Phone.REASON_ROAMING_ON.equals(
                            intent.getStringExtra(PhoneConstants.STATE_CHANGE_REASON_KEY));
                if (mDataDisconnectedDueToRoaming != disconnectedDueToRoaming) {
                    mDataDisconnectedDueToRoaming = disconnectedDueToRoaming;
                    mHandler.sendEmptyMessage(disconnectedDueToRoaming
                            ? EVENT_DATA_ROAMING_DISCONNECTED : EVENT_DATA_ROAMING_OK);
                }
            } else if ((action.equals(TelephonyIntents.ACTION_SIM_STATE_CHANGED)) &&
                    (mPUKEntryActivity != null)) {
                // if an attempt to un-PUK-lock the device was made, while we're
                // receiving this state change notification, notify the handler.
                // NOTE: This is ONLY triggered if an attempt to un-PUK-lock has
                // been attempted.
                mHandler.sendMessage(mHandler.obtainMessage(EVENT_SIM_STATE_CHANGED,
                        intent.getStringExtra(IccCardConstants.INTENT_KEY_ICC_STATE)));
            } else if (action.equals(TelephonyIntents.ACTION_RADIO_TECHNOLOGY_CHANGED)) {
                String newPhone = intent.getStringExtra(PhoneConstants.PHONE_NAME_KEY);
                int phoneId = intent.getIntExtra(PhoneConstants.PHONE_KEY,
                        SubscriptionManager.INVALID_PHONE_INDEX);
                Log.d(LOG_TAG, "Radio technology switched. Now " + newPhone + " (" + phoneId
                        + ") is active.");
                initForNewRadioTechnology(phoneId);
            } else if (action.equals(TelephonyIntents.ACTION_SERVICE_STATE_CHANGED)) {
                handleServiceStateChanged(intent);
            } else if (action.equals(TelephonyIntents.ACTION_EMERGENCY_CALLBACK_MODE_CHANGED)) {
                int phoneId = intent.getIntExtra(PhoneConstants.PHONE_KEY, 0);
                phoneInEcm = getPhone(phoneId);
                Log.d(LOG_TAG, "Emergency Callback Mode. phoneId:" + phoneId);
                if (phoneInEcm != null) {
                    if (TelephonyCapabilities.supportsEcm(phoneInEcm)) {
                        Log.d(LOG_TAG, "Emergency Callback Mode arrived in PhoneApp.");
                        // Start Emergency Callback Mode service
                        if (intent.getBooleanExtra("phoneinECMState", false)) {
                            context.startService(new Intent(context,
                                    EmergencyCallbackModeService.class));
                        } else {
                            phoneInEcm = null;
                        }
                    } else {
                        // It doesn't make sense to get ACTION_EMERGENCY_CALLBACK_MODE_CHANGED
                        // on a device that doesn't support ECM in the first place.
                        Log.e(LOG_TAG, "Got ACTION_EMERGENCY_CALLBACK_MODE_CHANGED, but "
                                + "ECM isn't supported for phone: " + phoneInEcm.getPhoneName());
                        phoneInEcm = null;
                    }
                } else {
                    Log.w(LOG_TAG, "phoneInEcm is null.");
                }
            }
        }
    }

    private void handleServiceStateChanged(Intent intent) {
        /**
         * This used to handle updating EriTextWidgetProvider this routine
         * and and listening for ACTION_SERVICE_STATE_CHANGED intents could
         * be removed. But leaving just in case it might be needed in the near
         * future.
         */

        // If service just returned, start sending out the queued messages
        Bundle extras = intent.getExtras();
        if (extras != null) {
            ServiceState ss = ServiceState.newFromBundle(extras);
            if (ss != null) {
                int state = ss.getState();
                notificationMgr.updateNetworkSelection(state);
            }
        }
    }

    // it is safe to call clearOtaState() even if the InCallScreen isn't active
    public void clearOtaState() {
        if (DBG) Log.d(LOG_TAG, "- clearOtaState ...");
        if (otaUtils != null) {
            otaUtils.cleanOtaScreen(true);
            if (DBG) Log.d(LOG_TAG, "  - clearOtaState clears OTA screen");
        }
    }

    // it is safe to call dismissOtaDialogs() even if the InCallScreen isn't active
    public void dismissOtaDialogs() {
        if (DBG) Log.d(LOG_TAG, "- dismissOtaDialogs ...");
        if (otaUtils != null) {
            otaUtils.dismissAllOtaDialogs();
            if (DBG) Log.d(LOG_TAG, "  - dismissOtaDialogs clears OTA dialogs");
        }
    }

    public Phone getPhoneInEcm() {
        return phoneInEcm;
    }

    /**
     * Triggers a refresh of the message waiting (voicemail) indicator.
     *
     * @param subId the subscription id we should refresh the notification for.
     */
    public void refreshMwiIndicator(int subId) {
        notificationMgr.refreshMwi(subId);
    }

    /**
     * Dismisses the message waiting (voicemail) indicator.
     *
     * @param subId the subscription id we should dismiss the notification for.
     */
    public void clearMwiIndicator(int subId) {
        notificationMgr.updateMwi(subId, false);
    }
}
