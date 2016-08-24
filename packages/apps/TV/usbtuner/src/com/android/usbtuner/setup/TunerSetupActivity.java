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
 * limitations under the License.
 */

package com.android.usbtuner.setup;

import android.app.Fragment;
import android.app.FragmentManager;
import android.app.Notification;
import android.app.NotificationManager;
import android.app.PendingIntent;
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.res.Resources;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.media.tv.TvContract;
import android.os.Bundle;
import android.support.v4.app.NotificationCompat;
import android.view.KeyEvent;

import com.android.tv.common.TvCommonConstants;
import com.android.tv.common.TvCommonUtils;
import com.android.tv.common.ui.setup.SetupActivity;
import com.android.tv.common.ui.setup.SetupFragment;
import com.android.tv.common.ui.setup.SetupMultiPaneFragment;
import com.android.usbtuner.R;
import com.android.usbtuner.UsbTunerPreferences;
import com.android.usbtuner.tvinput.UsbTunerTvInputService;

/**
 * An activity that serves USB tuner setup process.
 */
public class TunerSetupActivity extends SetupActivity {
    // For the recommendation card
    private static final String TV_ACTIVITY_CLASS_NAME = "com.android.tv.TvActivity";
    private static final String NOTIFY_TAG = "UsbTunerSetup";
    private static final int NOTIFY_ID = 1000;
    private static final String TAG_DRAWABLE = "drawable";
    private static final String TAG_ICON = "ic_launcher_s";

    private static final int CHANNEL_MAP_SCAN_FILE[] = {
            R.raw.ut_us_atsc_center_frequencies_8vsb,
            R.raw.ut_us_cable_standard_center_frequencies_qam256,
            R.raw.ut_us_all,
            R.raw.ut_kr_atsc_center_frequencies_8vsb,
            R.raw.ut_kr_cable_standard_center_frequencies_qam256,
            R.raw.ut_kr_all,
            R.raw.ut_kr_dev_cj_cable_center_frequencies_qam256};

    private ScanFragment mLastScanFragment;

    @Override
    protected Fragment onCreateInitialFragment() {
        SetupFragment fragment = new WelcomeFragment();
        fragment.setShortDistance(SetupFragment.FRAGMENT_EXIT_TRANSITION
                | SetupFragment.FRAGMENT_REENTER_TRANSITION);
        return fragment;
    }

    @Override
    protected void executeAction(String category, int actionId) {
        switch (category) {
            case WelcomeFragment.ACTION_CATEGORY:
                switch (actionId) {
                    case SetupMultiPaneFragment.ACTION_DONE:
                        // If the scan was performed, then the result should be OK.
                        setResult(mLastScanFragment == null ? RESULT_CANCELED : RESULT_OK);
                        finish();
                        break;
                    default: {
                        SetupFragment fragment = new ConnectionTypeFragment();
                        fragment.setShortDistance(SetupFragment.FRAGMENT_ENTER_TRANSITION
                                | SetupFragment.FRAGMENT_RETURN_TRANSITION);
                        showFragment(fragment, true);
                        break;
                    }
                }
                break;
            case ConnectionTypeFragment.ACTION_CATEGORY:
                mLastScanFragment = new ScanFragment();
                Bundle args = new Bundle();
                args.putInt(ScanFragment.EXTRA_FOR_CHANNEL_SCAN_FILE, CHANNEL_MAP_SCAN_FILE[actionId]);
                mLastScanFragment.setArguments(args);
                showFragment(mLastScanFragment, true);
                break;
            case ScanFragment.ACTION_CATEGORY:
                switch (actionId) {
                    case ScanFragment.ACTION_CANCEL:
                        getFragmentManager().popBackStack();
                        break;
                    case ScanFragment.ACTION_FINISH:
                        SetupFragment fragment = new ScanResultFragment();
                        fragment.setShortDistance(SetupFragment.FRAGMENT_EXIT_TRANSITION
                                | SetupFragment.FRAGMENT_REENTER_TRANSITION);
                        showFragment(fragment, true);
                        break;
                }
                break;
            case ScanResultFragment.ACTION_CATEGORY:
                switch (actionId) {
                    case SetupMultiPaneFragment.ACTION_DONE:
                        setResult(RESULT_OK);
                        finish();
                        break;
                    default:
                        SetupFragment fragment = new ConnectionTypeFragment();
                        fragment.setShortDistance(SetupFragment.FRAGMENT_ENTER_TRANSITION
                                | SetupFragment.FRAGMENT_RETURN_TRANSITION);
                        showFragment(fragment, true);
                        break;
                }
                break;
        }
    }

    @Override
    public boolean onKeyUp(int keyCode, KeyEvent event) {
        if (keyCode == KeyEvent.KEYCODE_BACK) {
            FragmentManager manager = getFragmentManager();
            int count = manager.getBackStackEntryCount();
            if (count > 0) {
                String lastTag = manager.getBackStackEntryAt(count - 1).getName();
                if (ScanResultFragment.class.getCanonicalName().equals(lastTag) && count >= 2) {
                    // Pops fragment including ScanFragment.
                    manager.popBackStack(manager.getBackStackEntryAt(count - 2).getName(),
                            FragmentManager.POP_BACK_STACK_INCLUSIVE);
                    return true;
                } else if (ScanFragment.class.getCanonicalName().equals(lastTag)) {
                    mLastScanFragment.finishScan(true);
                    return true;
                }
            }
        }
        return super.onKeyUp(keyCode, event);
    }

    /**
     * A callback to be invoked when the TvInputService is enabled or disabled.
     *
     * @param context a {@link Context} instance
     * @param enabled {@code true} for the {@link UsbTunerTvInputService} to be enabled;
     *                otherwise {@code false}
     */
    public static void onTvInputEnabled(Context context, boolean enabled) {
        // Send a recommendation card for USB channel tuner setup
        // if there's no channels and the USB tuner TV input setup has been not done.
        boolean channelScanDoneOnPreference = UsbTunerPreferences.isScanDone(context);
        int channelCountOnPreference = UsbTunerPreferences.getScannedChannelCount(context);
        if (enabled && !channelScanDoneOnPreference && channelCountOnPreference == 0) {
            UsbTunerPreferences.setShouldShowSetupActivity(context, true);
            sendRecommendationCard(context);
        } else {
            UsbTunerPreferences.setShouldShowSetupActivity(context, false);
            cancelRecommendationCard(context);
        }
    }

    /**
     * Returns a {@link Intent} to launch the USB tuner TV input service.
     *
     * @param context a {@link Context} instance
     */
    public static Intent createSetupActivity(Context context) {
        String inputId = TvContract.buildInputId(new ComponentName(context.getPackageName(),
                UsbTunerTvInputService.class.getName()));

        // Make an intent to launch the setup activity of USB tuner TV input.
        Intent intent = TvCommonUtils.createSetupIntent(
                new Intent(context, TunerSetupActivity.class), inputId);
        intent.putExtra(TvCommonConstants.EXTRA_INPUT_ID, inputId);
        Intent tvActivityIntent = new Intent();
        tvActivityIntent.setComponent(new ComponentName(context, TV_ACTIVITY_CLASS_NAME));
        intent.putExtra(TvCommonConstants.EXTRA_ACTIVITY_AFTER_COMPLETION, tvActivityIntent);
        return intent;
    }

    /**
     * Returns a {@link PendingIntent} to launch the USB tuner TV input service.
     *
     * @param context a {@link Context} instance
     */
    private static PendingIntent createPendingIntentForSetupActivity(Context context) {
        return PendingIntent.getActivity(context, 0, createSetupActivity(context),
                PendingIntent.FLAG_UPDATE_CURRENT);
    }

    /**
     * Sends the recommendation card to start the USB tuner TV input setup activity.
     *
     * @param context a {@link Context} instance
     */
    private static void sendRecommendationCard(Context context) {
        Resources resources = context.getResources();
        String focusedTitle = resources.getString(
                R.string.ut_setup_recommendation_card_focused_title);
        String title = resources.getString(R.string.ut_setup_recommendation_card_title);
        Bitmap largeIcon = BitmapFactory.decodeResource(resources,
                R.drawable.recommendation_antenna);

        // Build and send the notification.
        Notification notification = new NotificationCompat.BigPictureStyle(
                new NotificationCompat.Builder(context)
                        .setAutoCancel(false)
                        .setContentTitle(focusedTitle)
                        .setContentText(title)
                        .setContentInfo(title)
                        .setCategory(Notification.CATEGORY_RECOMMENDATION)
                        .setLargeIcon(largeIcon)
                        .setSmallIcon(resources.getIdentifier(
                                TAG_ICON, TAG_DRAWABLE, context.getPackageName()))
                        .setContentIntent(createPendingIntentForSetupActivity(context)))
                .build();
        NotificationManager notificationManager = (NotificationManager) context
                .getSystemService(Context.NOTIFICATION_SERVICE);
        notificationManager.notify(NOTIFY_TAG, NOTIFY_ID, notification);
    }

    /**
     * Cancels the previously shown recommendation card.
     *
     * @param context a {@link Context} instance
     */
    public static void cancelRecommendationCard(Context context) {
        NotificationManager notificationManager = (NotificationManager) context
                .getSystemService(Context.NOTIFICATION_SERVICE);
        notificationManager.cancel(NOTIFY_TAG, NOTIFY_ID);
    }
}
