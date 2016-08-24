/*
 * Copyright (C) 2011 The Android Open Source Project
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

package com.android.cellbroadcastreceiver;

import android.app.KeyguardManager;
import android.app.Notification;
import android.app.NotificationManager;
import android.app.PendingIntent;
import android.app.Service;
import android.app.ActivityManagerNative;
import android.content.Context;
import android.content.Intent;
import android.content.SharedPreferences;
import android.os.Bundle;
import android.os.IBinder;
import android.os.RemoteException;
import android.os.UserHandle;
import android.preference.PreferenceManager;
import android.provider.Telephony;
import android.telephony.CellBroadcastMessage;
import android.telephony.SmsCbCmasInfo;
import android.telephony.SmsCbEtwsInfo;
import android.telephony.SmsCbLocation;
import android.telephony.SmsCbMessage;
import android.telephony.SubscriptionManager;
import android.util.Log;

import java.util.ArrayList;
import java.util.HashSet;
import java.util.Locale;

/**
 * This service manages the display and animation of broadcast messages.
 * Emergency messages display with a flashing animated exclamation mark icon,
 * and an alert tone is played when the alert is first shown to the user
 * (but not when the user views a previously received broadcast).
 */
public class CellBroadcastAlertService extends Service {
    private static final String TAG = "CellBroadcastAlertService";

    /** Intent action to display alert dialog/notification, after verifying the alert is new. */
    static final String SHOW_NEW_ALERT_ACTION = "cellbroadcastreceiver.SHOW_NEW_ALERT";

    /** Use the same notification ID for non-emergency alerts. */
    static final int NOTIFICATION_ID = 1;

    /** Sticky broadcast for latest area info broadcast received. */
    static final String CB_AREA_INFO_RECEIVED_ACTION =
            "android.cellbroadcastreceiver.CB_AREA_INFO_RECEIVED";

    /**
     *  Container for service category, serial number, location, body hash code, and ETWS primary/
     *  secondary information for duplication detection.
     */
    private static final class MessageServiceCategoryAndScope {
        private final int mServiceCategory;
        private final int mSerialNumber;
        private final SmsCbLocation mLocation;
        private final int mBodyHash;
        private final boolean mIsEtwsPrimary;

        MessageServiceCategoryAndScope(int serviceCategory, int serialNumber,
                SmsCbLocation location, int bodyHash, boolean isEtwsPrimary) {
            mServiceCategory = serviceCategory;
            mSerialNumber = serialNumber;
            mLocation = location;
            mBodyHash = bodyHash;
            mIsEtwsPrimary = isEtwsPrimary;
        }

        @Override
        public int hashCode() {
            return mLocation.hashCode() + 5 * mServiceCategory + 7 * mSerialNumber + 13 * mBodyHash
                    + 17 * Boolean.hashCode(mIsEtwsPrimary);
        }

        @Override
        public boolean equals(Object o) {
            if (o == this) {
                return true;
            }
            if (o instanceof MessageServiceCategoryAndScope) {
                MessageServiceCategoryAndScope other = (MessageServiceCategoryAndScope) o;
                return (mServiceCategory == other.mServiceCategory &&
                        mSerialNumber == other.mSerialNumber &&
                        mLocation.equals(other.mLocation) &&
                        mBodyHash == other.mBodyHash &&
                        mIsEtwsPrimary == other.mIsEtwsPrimary);
            }
            return false;
        }

        @Override
        public String toString() {
            return "{mServiceCategory: " + mServiceCategory + " serial number: " + mSerialNumber +
                    " location: " + mLocation.toString() + " body hash: " + mBodyHash +
                    " mIsEtwsPrimary: " + mIsEtwsPrimary + "}";
        }
    }

    /** Cache of received message IDs, for duplicate message detection. */
    private static final HashSet<MessageServiceCategoryAndScope> sCmasIdSet =
            new HashSet<MessageServiceCategoryAndScope>(8);

    /** Maximum number of message IDs to save before removing the oldest message ID. */
    private static final int MAX_MESSAGE_ID_SIZE = 65535;

    /** List of message IDs received, for removing oldest ID when max message IDs are received. */
    private static final ArrayList<MessageServiceCategoryAndScope> sCmasIdList =
            new ArrayList<MessageServiceCategoryAndScope>(8);

    /** Index of message ID to replace with new message ID when max message IDs are received. */
    private static int sCmasIdListIndex = 0;

    @Override
    public int onStartCommand(Intent intent, int flags, int startId) {
        String action = intent.getAction();
        if (Telephony.Sms.Intents.SMS_EMERGENCY_CB_RECEIVED_ACTION.equals(action) ||
                Telephony.Sms.Intents.SMS_CB_RECEIVED_ACTION.equals(action)) {
            handleCellBroadcastIntent(intent);
        } else if (SHOW_NEW_ALERT_ACTION.equals(action)) {
            try {
                if (UserHandle.myUserId() ==
                        ActivityManagerNative.getDefault().getCurrentUser().id) {
                    showNewAlert(intent);
                } else {
                    Log.d(TAG,"Not active user, ignore the alert display");
                }
            } catch (RemoteException e) {
                e.printStackTrace();
            }
        } else {
            Log.e(TAG, "Unrecognized intent action: " + action);
        }
        return START_NOT_STICKY;
    }

    private void handleCellBroadcastIntent(Intent intent) {
        Bundle extras = intent.getExtras();
        if (extras == null) {
            Log.e(TAG, "received SMS_CB_RECEIVED_ACTION with no extras!");
            return;
        }

        SmsCbMessage message = (SmsCbMessage) extras.get("message");

        if (message == null) {
            Log.e(TAG, "received SMS_CB_RECEIVED_ACTION with no message extra");
            return;
        }

        final CellBroadcastMessage cbm = new CellBroadcastMessage(message);

        if (!isMessageEnabledByUser(cbm)) {
            Log.d(TAG, "ignoring alert of type " + cbm.getServiceCategory() +
                    " by user preference");
            return;
        }

        // If this is an ETWS message, then we want to include the body message to be a factor for
        // duplication detection. We found that some Japanese carriers send ETWS messages
        // with the same serial number, therefore the subsequent messages were all ignored.
        // In the other hand, US carriers have the requirement that only serial number, location,
        // and category should be used for duplicate detection.
        int hashCode = message.isEtwsMessage() ? message.getMessageBody().hashCode() : 0;

        // If this is an ETWS message, we need to include primary/secondary message information to
        // be a factor for duplication detection as well. Per 3GPP TS 23.041 section 8.2,
        // duplicate message detection shall be performed independently for primary and secondary
        // notifications.
        boolean isEtwsPrimary = false;
        if (message.isEtwsMessage()) {
            SmsCbEtwsInfo etwsInfo = message.getEtwsWarningInfo();
            if (etwsInfo != null) {
                isEtwsPrimary = etwsInfo.isPrimary();
            } else {
                Log.w(TAG, "ETWS info is not available.");
            }
        }

        // Check for duplicate message IDs according to CMAS carrier requirements. Message IDs
        // are stored in volatile memory. If the maximum of 65535 messages is reached, the
        // message ID of the oldest message is deleted from the list.
        MessageServiceCategoryAndScope newCmasId = new MessageServiceCategoryAndScope(
                message.getServiceCategory(), message.getSerialNumber(), message.getLocation(),
                hashCode, isEtwsPrimary);

        Log.d(TAG, "message ID = " + newCmasId);

        // Add the new message ID to the list. It's okay if this is a duplicate message ID,
        // because the list is only used for removing old message IDs from the hash set.
        if (sCmasIdList.size() < MAX_MESSAGE_ID_SIZE) {
            sCmasIdList.add(newCmasId);
        } else {
            // Get oldest message ID from the list and replace with the new message ID.
            MessageServiceCategoryAndScope oldestCmasId = sCmasIdList.get(sCmasIdListIndex);
            sCmasIdList.set(sCmasIdListIndex, newCmasId);
            Log.d(TAG, "message ID limit reached, removing oldest message ID " + oldestCmasId);
            // Remove oldest message ID from the set.
            sCmasIdSet.remove(oldestCmasId);
            if (++sCmasIdListIndex >= MAX_MESSAGE_ID_SIZE) {
                sCmasIdListIndex = 0;
            }
        }
        // Set.add() returns false if message ID has already been added
        if (!sCmasIdSet.add(newCmasId)) {
            Log.d(TAG, "ignoring duplicate alert with " + newCmasId);
            return;
        }

        final Intent alertIntent = new Intent(SHOW_NEW_ALERT_ACTION);
        alertIntent.setClass(this, CellBroadcastAlertService.class);
        alertIntent.putExtra("message", cbm);

        // write to database on a background thread
        new CellBroadcastContentProvider.AsyncCellBroadcastTask(getContentResolver())
                .execute(new CellBroadcastContentProvider.CellBroadcastOperation() {
                    @Override
                    public boolean execute(CellBroadcastContentProvider provider) {
                        if (provider.insertNewBroadcast(cbm)) {
                            // new message, show the alert or notification on UI thread
                            startService(alertIntent);
                            return true;
                        } else {
                            return false;
                        }
                    }
                });
    }

    private void showNewAlert(Intent intent) {
        Bundle extras = intent.getExtras();
        if (extras == null) {
            Log.e(TAG, "received SHOW_NEW_ALERT_ACTION with no extras!");
            return;
        }

        CellBroadcastMessage cbm = (CellBroadcastMessage) intent.getParcelableExtra("message");

        if (cbm == null) {
            Log.e(TAG, "received SHOW_NEW_ALERT_ACTION with no message extra");
            return;
        }

        if (CellBroadcastConfigService.isEmergencyAlertMessage(cbm)) {
            // start alert sound / vibration / TTS and display full-screen alert
            openEmergencyAlertNotification(cbm);
        } else {
            // add notification to the bar
            addToNotificationBar(cbm);
        }
    }

    /**
     * Filter out broadcasts on the test channels that the user has not enabled,
     * and types of notifications that the user is not interested in receiving.
     * This allows us to enable an entire range of message identifiers in the
     * radio and not have to explicitly disable the message identifiers for
     * test broadcasts. In the unlikely event that the default shared preference
     * values were not initialized in CellBroadcastReceiverApp, the second parameter
     * to the getBoolean() calls match the default values in res/xml/preferences.xml.
     *
     * @param message the message to check
     * @return true if the user has enabled this message type; false otherwise
     */
    private boolean isMessageEnabledByUser(CellBroadcastMessage message) {

        // Check if all emergency alerts are disabled.
        boolean emergencyAlertEnabled = PreferenceManager.getDefaultSharedPreferences(this).
                getBoolean(CellBroadcastSettings.KEY_ENABLE_EMERGENCY_ALERTS, true);

        // Check if ETWS/CMAS test message is forced to disabled on the device.
        boolean forceDisableEtwsCmasTest =
                CellBroadcastSettings.isEtwsCmasTestMessageForcedDisabled(this);

        if (message.isEtwsTestMessage()) {
            return emergencyAlertEnabled &&
                    !forceDisableEtwsCmasTest &&
                    PreferenceManager.getDefaultSharedPreferences(this)
                    .getBoolean(CellBroadcastSettings.KEY_ENABLE_ETWS_TEST_ALERTS, false);
        }

        if (message.isEtwsMessage()) {
            // ETWS messages.
            // Turn on/off emergency notifications is the only way to turn on/off ETWS messages.
            return emergencyAlertEnabled;

        }

        if (message.isCmasMessage()) {
            switch (message.getCmasMessageClass()) {
                case SmsCbCmasInfo.CMAS_CLASS_EXTREME_THREAT:
                    return emergencyAlertEnabled &&
                            PreferenceManager.getDefaultSharedPreferences(this).getBoolean(
                            CellBroadcastSettings.KEY_ENABLE_CMAS_EXTREME_THREAT_ALERTS, true);

                case SmsCbCmasInfo.CMAS_CLASS_SEVERE_THREAT:
                    return emergencyAlertEnabled &&
                            PreferenceManager.getDefaultSharedPreferences(this).getBoolean(
                            CellBroadcastSettings.KEY_ENABLE_CMAS_SEVERE_THREAT_ALERTS, true);

                case SmsCbCmasInfo.CMAS_CLASS_CHILD_ABDUCTION_EMERGENCY:
                    return emergencyAlertEnabled &&
                            PreferenceManager.getDefaultSharedPreferences(this)
                            .getBoolean(CellBroadcastSettings.KEY_ENABLE_CMAS_AMBER_ALERTS, true);

                case SmsCbCmasInfo.CMAS_CLASS_REQUIRED_MONTHLY_TEST:
                case SmsCbCmasInfo.CMAS_CLASS_CMAS_EXERCISE:
                case SmsCbCmasInfo.CMAS_CLASS_OPERATOR_DEFINED_USE:
                    return emergencyAlertEnabled &&
                            !forceDisableEtwsCmasTest &&
                            PreferenceManager.getDefaultSharedPreferences(this)
                                    .getBoolean(CellBroadcastSettings.KEY_ENABLE_CMAS_TEST_ALERTS,
                                            false);
                default:
                    return true;    // presidential-level CMAS alerts are always enabled
            }
        }

        if (message.getServiceCategory() == 50) {
            // save latest area info broadcast for Settings display and send as broadcast
            CellBroadcastReceiverApp.setLatestAreaInfo(message);
            Intent intent = new Intent(CB_AREA_INFO_RECEIVED_ACTION);
            intent.putExtra("message", message);
            // Send broadcast twice, once for apps that have PRIVILEGED permission and once
            // for those that have the runtime one
            sendBroadcastAsUser(intent, UserHandle.ALL,
                    android.Manifest.permission.READ_PRIVILEGED_PHONE_STATE);
            sendBroadcastAsUser(intent, UserHandle.ALL,
                    android.Manifest.permission.READ_PHONE_STATE);
            return false;   // area info broadcasts are displayed in Settings status screen
        }

        return true;    // other broadcast messages are always enabled
    }

    /**
     * Display a full-screen alert message for emergency alerts.
     * @param message the alert to display
     */
    private void openEmergencyAlertNotification(CellBroadcastMessage message) {
        // Acquire a CPU wake lock until the alert dialog and audio start playing.
        CellBroadcastAlertWakeLock.acquireScreenCpuWakeLock(this);

        // Close dialogs and window shade
        Intent closeDialogs = new Intent(Intent.ACTION_CLOSE_SYSTEM_DIALOGS);
        sendBroadcast(closeDialogs);

        // start audio/vibration/speech service for emergency alerts
        Intent audioIntent = new Intent(this, CellBroadcastAlertAudio.class);
        audioIntent.setAction(CellBroadcastAlertAudio.ACTION_START_ALERT_AUDIO);
        SharedPreferences prefs = PreferenceManager.getDefaultSharedPreferences(this);

        int duration;   // alert audio duration in ms
        if (message.isCmasMessage()) {
            // CMAS requirement: duration of the audio attention signal is 10.5 seconds.
            duration = 10500;
        } else {
            duration = Integer.parseInt(prefs.getString(
                    CellBroadcastSettings.KEY_ALERT_SOUND_DURATION,
                    CellBroadcastSettings.ALERT_SOUND_DEFAULT_DURATION)) * 1000;
        }
        audioIntent.putExtra(CellBroadcastAlertAudio.ALERT_AUDIO_DURATION_EXTRA, duration);

        if (message.isEtwsMessage()) {
            // For ETWS, always vibrate, even in silent mode.
            audioIntent.putExtra(CellBroadcastAlertAudio.ALERT_AUDIO_VIBRATE_EXTRA, true);
            audioIntent.putExtra(CellBroadcastAlertAudio.ALERT_AUDIO_ETWS_VIBRATE_EXTRA, true);
        } else {
            // For other alerts, vibration can be disabled in app settings.
            audioIntent.putExtra(CellBroadcastAlertAudio.ALERT_AUDIO_VIBRATE_EXTRA,
                    prefs.getBoolean(CellBroadcastSettings.KEY_ENABLE_ALERT_VIBRATE, true));
        }

        String messageBody = message.getMessageBody();

        if (prefs.getBoolean(CellBroadcastSettings.KEY_ENABLE_ALERT_SPEECH, true)) {
            audioIntent.putExtra(CellBroadcastAlertAudio.ALERT_AUDIO_MESSAGE_BODY, messageBody);

            String preferredLanguage = message.getLanguageCode();
            String defaultLanguage = null;
            if (message.isEtwsMessage()) {
                // Only do TTS for ETWS secondary message.
                // There is no text in ETWS primary message. When we construct the ETWS primary
                // message, we hardcode "ETWS" as the body hence we don't want to speak that out here.
                
                // Also in many cases we see the secondary message comes few milliseconds after
                // the primary one. If we play TTS for the primary one, It will be overwritten by
                // the secondary one immediately anyway.
                if (!message.getEtwsWarningInfo().isPrimary()) {
                    // Since only Japanese carriers are using ETWS, if there is no language specified
                    // in the ETWS message, we'll use Japanese as the default language.
                    defaultLanguage = "ja";
                }
            } else {
                // If there is no language specified in the CMAS message, use device's
                // default language.
                defaultLanguage = Locale.getDefault().getLanguage();
            }

            Log.d(TAG, "Preferred language = " + preferredLanguage +
                    ", Default language = " + defaultLanguage);
            audioIntent.putExtra(CellBroadcastAlertAudio.ALERT_AUDIO_MESSAGE_PREFERRED_LANGUAGE,
                    preferredLanguage);
            audioIntent.putExtra(CellBroadcastAlertAudio.ALERT_AUDIO_MESSAGE_DEFAULT_LANGUAGE,
                    defaultLanguage);
        }
        startService(audioIntent);

        // Decide which activity to start based on the state of the keyguard.
        Class c = CellBroadcastAlertDialog.class;
        KeyguardManager km = (KeyguardManager) getSystemService(Context.KEYGUARD_SERVICE);
        if (km.inKeyguardRestrictedInputMode()) {
            // Use the full screen activity for security.
            c = CellBroadcastAlertFullScreen.class;
        }

        ArrayList<CellBroadcastMessage> messageList = new ArrayList<CellBroadcastMessage>(1);
        messageList.add(message);

        Intent alertDialogIntent = createDisplayMessageIntent(this, c, messageList);
        alertDialogIntent.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
        startActivity(alertDialogIntent);
    }

    /**
     * Add the new alert to the notification bar (non-emergency alerts), or launch a
     * high-priority immediate intent for emergency alerts.
     * @param message the alert to display
     */
    private void addToNotificationBar(CellBroadcastMessage message) {
        int channelTitleId = CellBroadcastResources.getDialogTitleResource(message);
        CharSequence channelName = getText(channelTitleId);
        String messageBody = message.getMessageBody();

        // Pass the list of unread non-emergency CellBroadcastMessages
        ArrayList<CellBroadcastMessage> messageList = CellBroadcastReceiverApp
                .addNewMessageToList(message);

        // Create intent to show the new messages when user selects the notification.
        Intent intent = createDisplayMessageIntent(this, CellBroadcastAlertDialog.class,
                messageList);
        intent.putExtra(CellBroadcastAlertFullScreen.FROM_NOTIFICATION_EXTRA, true);

        PendingIntent pi = PendingIntent.getActivity(this, NOTIFICATION_ID, intent,
                PendingIntent.FLAG_ONE_SHOT | PendingIntent.FLAG_UPDATE_CURRENT);

        // use default sound/vibration/lights for non-emergency broadcasts
        Notification.Builder builder = new Notification.Builder(this)
                .setSmallIcon(R.drawable.ic_notify_alert)
                .setTicker(channelName)
                .setWhen(System.currentTimeMillis())
                .setContentIntent(pi)
                .setCategory(Notification.CATEGORY_SYSTEM)
                .setPriority(Notification.PRIORITY_HIGH)
                .setColor(getResources().getColor(R.color.notification_color))
                .setVisibility(Notification.VISIBILITY_PUBLIC)
                .setDefaults(Notification.DEFAULT_ALL);

        builder.setDefaults(Notification.DEFAULT_ALL);

        // increment unread alert count (decremented when user dismisses alert dialog)
        int unreadCount = messageList.size();
        if (unreadCount > 1) {
            // use generic count of unread broadcasts if more than one unread
            builder.setContentTitle(getString(R.string.notification_multiple_title));
            builder.setContentText(getString(R.string.notification_multiple, unreadCount));
        } else {
            builder.setContentTitle(channelName).setContentText(messageBody);
        }

        NotificationManager notificationManager =
            (NotificationManager)getSystemService(Context.NOTIFICATION_SERVICE);

        notificationManager.notify(NOTIFICATION_ID, builder.build());
    }

    static Intent createDisplayMessageIntent(Context context, Class intentClass,
            ArrayList<CellBroadcastMessage> messageList) {
        // Trigger the list activity to fire up a dialog that shows the received messages
        Intent intent = new Intent(context, intentClass);
        intent.putParcelableArrayListExtra(CellBroadcastMessage.SMS_CB_MESSAGE_EXTRA, messageList);
        return intent;
    }

    @Override
    public IBinder onBind(Intent intent) {
        return null;    // clients can't bind to this service
    }
}
