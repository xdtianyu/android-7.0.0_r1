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

package com.android.tv.onboarding;

import android.app.Fragment;
import android.content.Context;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.os.Build;
import android.os.Bundle;
import android.support.annotation.NonNull;
import android.widget.Toast;

import com.android.tv.R;
import com.android.tv.TvApplication;
import com.android.tv.common.ui.setup.SetupActivity;
import com.android.tv.common.ui.setup.SetupMultiPaneFragment;
import com.android.tv.data.ChannelDataManager;
import com.android.tv.util.OnboardingUtils;
import com.android.tv.util.PermissionUtils;
import com.android.tv.util.SetupUtils;

public class OnboardingActivity extends SetupActivity {
    private static final String KEY_INTENT_AFTER_COMPLETION = "key_intent_after_completion";

    private static final int PERMISSIONS_REQUEST_READ_TV_LISTINGS = 1;
    private static final String PERMISSION_READ_TV_LISTINGS = "android.permission.READ_TV_LISTINGS";

    private static final int SHOW_RIPPLE_DURATION_MS = 266;

    private ChannelDataManager mChannelDataManager;
    private final ChannelDataManager.Listener mChannelListener = new ChannelDataManager.Listener() {
        @Override
        public void onLoadFinished() {
            mChannelDataManager.removeListener(this);
            SetupUtils.getInstance(OnboardingActivity.this).markNewChannelsBrowsable();
        }

        @Override
        public void onChannelListUpdated() { }

        @Override
        public void onChannelBrowsableChanged() { }
    };

    /**
     * Returns an intent to start {@link OnboardingActivity}.
     *
     * @param context context to create an intent. Should not be {@code null}.
     * @param intentAfterCompletion intent which will be used to start a new activity when this
     * activity finishes. Should not be {@code null}.
     */
    public static Intent buildIntent(@NonNull Context context,
            @NonNull Intent intentAfterCompletion) {
        return new Intent(context, OnboardingActivity.class)
                .putExtra(OnboardingActivity.KEY_INTENT_AFTER_COMPLETION, intentAfterCompletion);
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        if (!PermissionUtils.hasAccessAllEpg(this)) {
            if (Build.VERSION.SDK_INT < Build.VERSION_CODES.M) {
                Toast.makeText(this, R.string.msg_not_supported_device, Toast.LENGTH_LONG).show();
                finish();
                return;
            } else if (checkSelfPermission(PERMISSION_READ_TV_LISTINGS)
                    != PackageManager.PERMISSION_GRANTED) {
                requestPermissions(new String[]{PERMISSION_READ_TV_LISTINGS},
                        PERMISSIONS_REQUEST_READ_TV_LISTINGS);
            }
        }
    }

    @Override
    protected void onDestroy() {
        if (mChannelDataManager != null) {
            mChannelDataManager.removeListener(mChannelListener);
        }
        super.onDestroy();
    }

    @Override
    protected Fragment onCreateInitialFragment() {
        if (PermissionUtils.hasAccessAllEpg(this) || PermissionUtils.hasReadTvListings(this)) {
            // Make the channels of the new inputs which have been setup outside Live TV
            // browsable.
            mChannelDataManager = TvApplication.getSingletons(this).getChannelDataManager();
            if (mChannelDataManager.isDbLoadFinished()) {
                SetupUtils.getInstance(this).markNewChannelsBrowsable();
            } else {
                mChannelDataManager.addListener(mChannelListener);
            }
            return OnboardingUtils.isFirstRunWithCurrentVersion(this) ? new WelcomeFragment()
                    : new SetupSourcesFragment();
        }
        return null;
    }

    @Override
    public void onRequestPermissionsResult(int requestCode, @NonNull String[] permissions,
            @NonNull int[] grantResults) {
        if (requestCode == PERMISSIONS_REQUEST_READ_TV_LISTINGS) {
            if (grantResults != null && grantResults.length > 0
                    && grantResults[0] == PackageManager.PERMISSION_GRANTED) {
                finish();
                Intent intentForNextActivity = getIntent().getParcelableExtra(
                        KEY_INTENT_AFTER_COMPLETION);
                startActivity(buildIntent(this, intentForNextActivity));
            } else {
                Toast.makeText(this, R.string.msg_read_tv_listing_permission_denied,
                        Toast.LENGTH_LONG).show();
                finish();
            }
        }
    }

    void finishActivity() {
        Intent intentForNextActivity = getIntent().getParcelableExtra(
                KEY_INTENT_AFTER_COMPLETION);
        if (intentForNextActivity != null) {
            startActivity(intentForNextActivity);
        }
        finish();
    }

    void showMerchantCollection() {
        executeActionWithDelay(new Runnable() {
            @Override
            public void run() {
                startActivity(OnboardingUtils.PLAY_STORE_INTENT);
            }
        }, SHOW_RIPPLE_DURATION_MS);
    }

    @Override
    protected void executeAction(String category, int actionId) {
        switch (category) {
            case WelcomeFragment.ACTION_CATEGORY:
                switch (actionId) {
                    case WelcomeFragment.ACTION_NEXT:
                        OnboardingUtils.setFirstRunWithCurrentVersionCompleted(
                                OnboardingActivity.this);
                        showFragment(new SetupSourcesFragment(), false);
                        break;
                }
                break;
            case SetupSourcesFragment.ACTION_CATEGORY:
                switch (actionId) {
                    case SetupSourcesFragment.ACTION_PLAY_STORE:
                        showMerchantCollection();
                        break;
                    case SetupMultiPaneFragment.ACTION_DONE: {
                        ChannelDataManager manager = TvApplication.getSingletons(
                                OnboardingActivity.this).getChannelDataManager();
                        if (manager.getChannelCount() == 0) {
                            finish();
                        } else {
                            finishActivity();
                        }
                        break;
                    }
                }
                break;
        }
    }
}
