package com.android.phone.settings;

import android.content.Context;
import android.media.RingtoneManager;
import android.net.Uri;
import android.os.Handler;
import android.os.Message;
import android.preference.Preference;
import android.preference.RingtonePreference;
import android.util.AttributeSet;

import com.android.internal.telephony.Phone;
import com.android.phone.common.util.SettingsUtil;

/**
 * Looks up the voicemail ringtone's name asynchronously and updates the preference's summary when
 * it is created or updated.
 */
public class VoicemailRingtonePreference extends RingtonePreference {
    private static final int MSG_UPDATE_VOICEMAIL_RINGTONE_SUMMARY = 1;

    private Runnable mVoicemailRingtoneLookupRunnable;
    private Handler mVoicemailRingtoneLookupComplete;

    private Phone mPhone;

    public VoicemailRingtonePreference(Context context, AttributeSet attrs) {
        super(context, attrs);

        mVoicemailRingtoneLookupComplete = new Handler() {
            @Override
            public void handleMessage(Message msg) {
                switch (msg.what) {
                    case MSG_UPDATE_VOICEMAIL_RINGTONE_SUMMARY:
                        setSummary((CharSequence) msg.obj);
                        break;
                }
            }
        };
    }

    public void init(Phone phone) {
        mPhone = phone;

        // Requesting the ringtone will trigger migration if necessary.
        VoicemailNotificationSettingsUtil.getRingtoneUri(phone);

        final Preference preference = this;
        final String preferenceKey =
                VoicemailNotificationSettingsUtil.getVoicemailRingtoneSharedPrefsKey(mPhone);
        mVoicemailRingtoneLookupRunnable = new Runnable() {
            @Override
            public void run() {
                SettingsUtil.updateRingtoneName(
                        preference.getContext(),
                        mVoicemailRingtoneLookupComplete,
                        RingtoneManager.TYPE_NOTIFICATION,
                        preferenceKey,
                        MSG_UPDATE_VOICEMAIL_RINGTONE_SUMMARY);
            }
        };

        updateRingtoneName();
    }

    @Override
    protected Uri onRestoreRingtone() {
        return VoicemailNotificationSettingsUtil.getRingtoneUri(mPhone);
    }

    @Override
    protected void onSaveRingtone(Uri ringtoneUri) {
        // Don't call superclass method because it uses the pref key as the SharedPreferences key.
        // Delegate to the voicemail notification utility to save the ringtone instead.
        VoicemailNotificationSettingsUtil.setRingtoneUri(mPhone, ringtoneUri);

        updateRingtoneName();
    }

    private void updateRingtoneName() {
        new Thread(mVoicemailRingtoneLookupRunnable).start();
    }
}
