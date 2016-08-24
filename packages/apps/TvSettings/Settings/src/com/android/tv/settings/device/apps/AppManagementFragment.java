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
 * limitations under the License
 */

package com.android.tv.settings.device.apps;

import android.app.Activity;
import android.app.ActivityManager;
import android.content.Context;
import android.content.Intent;
import android.content.pm.IPackageDataObserver;
import android.content.pm.PackageManager;
import android.graphics.drawable.Drawable;
import android.os.Bundle;
import android.os.Handler;
import android.os.UserHandle;
import android.support.annotation.NonNull;
import android.support.v17.preference.LeanbackPreferenceFragment;
import android.support.v7.preference.Preference;
import android.support.v7.preference.PreferenceScreen;
import android.util.Log;

import com.android.settingslib.applications.ApplicationsState;
import com.android.tv.settings.R;

import java.util.ArrayList;

public class AppManagementFragment extends LeanbackPreferenceFragment {
    private static final String TAG = "AppManagementFragment";

    private static final String ARG_PACKAGE_NAME = "packageName";

    // Result code identifiers
    private static final int REQUEST_UNINSTALL = 1;
    private static final int REQUEST_MANAGE_SPACE = 2;
    private static final int REQUEST_UNINSTALL_UPDATES = 3;

    private PackageManager mPackageManager;
    private String mPackageName;
    private ApplicationsState mApplicationsState;
    private ApplicationsState.Session mSession;
    private ApplicationsState.AppEntry mEntry;
    private final ApplicationsState.Callbacks mCallbacks = new ApplicationsStateCallbacks();

    private ForceStopPreference mForceStopPreference;
    private UninstallPreference mUninstallPreference;
    private EnableDisablePreference mEnableDisablePreference;
    private AppStoragePreference mAppStoragePreference;
    private ClearDataPreference mClearDataPreference;
    private ClearCachePreference mClearCachePreference;
    private ClearDefaultsPreference mClearDefaultsPreference;
    private NotificationsPreference mNotificationsPreference;

    private final Handler mHandler = new Handler();
    private Runnable mBailoutRunnable = new Runnable() {
        @Override
        public void run() {
            if (isResumed() && !getFragmentManager().popBackStackImmediate()) {
                getActivity().onBackPressed();
            }
        }
    };

    public static void prepareArgs(@NonNull Bundle args, String packageName) {
        args.putString(ARG_PACKAGE_NAME, packageName);
    }

    @Override
    public void onCreate(Bundle savedInstanceState) {
        mPackageName = getArguments().getString(ARG_PACKAGE_NAME);

        final Activity activity = getActivity();
        mPackageManager = activity.getPackageManager();
        mApplicationsState = ApplicationsState.getInstance(activity.getApplication());
        mSession = mApplicationsState.newSession(mCallbacks);
        mEntry = mApplicationsState.getEntry(mPackageName, UserHandle.myUserId());

        super.onCreate(savedInstanceState);
    }

    @Override
    public void onResume() {
        super.onResume();
        mSession.resume();

        if (mEntry == null) {
            Log.w(TAG, "App not found, trying to bail out");
            navigateBack();
        }

        if (mClearDefaultsPreference != null) {
            mClearDefaultsPreference.refresh();
        }
        if (mEnableDisablePreference != null) {
            mEnableDisablePreference.refresh();
        }
    }

    @Override
    public void onPause() {
        super.onPause();
        mSession.pause();
        mHandler.removeCallbacks(mBailoutRunnable);
    }

    @Override
    public void onActivityResult(int requestCode, int resultCode, Intent data) {
        super.onActivityResult(requestCode, resultCode, data);
        if (mEntry == null) {
            return;
        }
        switch (requestCode) {
            case REQUEST_UNINSTALL:
                if (resultCode == Activity.RESULT_OK) {
                    final int userId =  UserHandle.getUserId(mEntry.info.uid);
                    mApplicationsState.removePackage(mPackageName, userId);
                    navigateBack();
                }
                break;
            case REQUEST_MANAGE_SPACE:
                mClearDataPreference.setClearingData(false);
                if(resultCode == Activity.RESULT_OK) {
                    final int userId = UserHandle.getUserId(mEntry.info.uid);
                    mApplicationsState.requestSize(mPackageName, userId);
                } else {
                    Log.w(TAG, "Failed to clear data!");
                }
                break;
            case REQUEST_UNINSTALL_UPDATES:
                mUninstallPreference.refresh();
                break;
        }
    }

    private void navigateBack() {
        // need to post this to avoid recursing in the fragment manager.
        mHandler.removeCallbacks(mBailoutRunnable);
        mHandler.post(mBailoutRunnable);
    }

    @Override
    public void onCreatePreferences(Bundle savedInstanceState, String rootKey) {
        final Context themedContext = getPreferenceManager().getContext();
        final PreferenceScreen screen =
                getPreferenceManager().createPreferenceScreen(themedContext);
        screen.setTitle(getAppName());

        if (mEntry == null) {
            setPreferenceScreen(screen);
            return;
        }

        final Preference versionPreference = new Preference(themedContext);
        versionPreference.setSelectable(false);
        versionPreference.setTitle(getString(R.string.device_apps_app_management_version,
                mEntry.getVersion(getActivity())));
        versionPreference.setSummary(mPackageName);
        screen.addPreference(versionPreference);

        // Open
        Intent appLaunchIntent =
                mPackageManager.getLeanbackLaunchIntentForPackage(mEntry.info.packageName);
        if (appLaunchIntent == null) {
            appLaunchIntent = mPackageManager.getLaunchIntentForPackage(mEntry.info.packageName);
        }
        if (appLaunchIntent != null) {
            final Preference openPreference = new Preference(themedContext);
            openPreference.setIntent(appLaunchIntent);
            openPreference.setTitle(R.string.device_apps_app_management_open);
            screen.addPreference(openPreference);
        }

        // Force stop
        mForceStopPreference = new ForceStopPreference(themedContext, mEntry);
        screen.addPreference(mForceStopPreference);

        // Uninstall
        mUninstallPreference = new UninstallPreference(themedContext, mEntry);
        screen.addPreference(mUninstallPreference);

        // Disable/Enable
        mEnableDisablePreference = new EnableDisablePreference(themedContext, mEntry);
        screen.addPreference(mEnableDisablePreference);

        // Storage used
        mAppStoragePreference = new AppStoragePreference(themedContext, mEntry);
        screen.addPreference(mAppStoragePreference);

        // Clear data
        mClearDataPreference = new ClearDataPreference(themedContext, mEntry);
        screen.addPreference(mClearDataPreference);

        // Clear cache
        mClearCachePreference = new ClearCachePreference(themedContext, mEntry);
        screen.addPreference(mClearCachePreference);

        // Clear defaults
        mClearDefaultsPreference = new ClearDefaultsPreference(themedContext, mEntry);
        screen.addPreference(mClearDefaultsPreference);

        // Notifications
        mNotificationsPreference = new NotificationsPreference(themedContext, mEntry);
        screen.addPreference(mNotificationsPreference);

        // Permissions
        final Preference permissionsPreference = new Preference(themedContext);
        permissionsPreference.setTitle(R.string.device_apps_app_management_permissions);
        permissionsPreference.setIntent(new Intent(Intent.ACTION_MANAGE_APP_PERMISSIONS)
                .putExtra(Intent.EXTRA_PACKAGE_NAME, mPackageName));
        screen.addPreference(permissionsPreference);

        setPreferenceScreen(screen);
    }

    public String getAppName() {
        if (mEntry == null) {
            return null;
        }
        mEntry.ensureLabel(getActivity());
        return mEntry.label;
    }

    public Drawable getAppIcon() {
        if (mEntry == null) {
            return null;
        }
        mApplicationsState.ensureIcon(mEntry);
        return mEntry.icon;
    }

    public void clearData() {
        mClearDataPreference.setClearingData(true);
        String spaceManagementActivityName = mEntry.info.manageSpaceActivityName;
        if (spaceManagementActivityName != null) {
            if (!ActivityManager.isUserAMonkey()) {
                Intent intent = new Intent(Intent.ACTION_DEFAULT);
                intent.setClassName(mEntry.info.packageName, spaceManagementActivityName);
                startActivityForResult(intent, REQUEST_MANAGE_SPACE);
            }
        } else {
            ActivityManager am = (ActivityManager) getActivity().getSystemService(
                    Context.ACTIVITY_SERVICE);
            boolean success = am.clearApplicationUserData(
                    mEntry.info.packageName, new IPackageDataObserver.Stub() {
                        public void onRemoveCompleted(
                                final String packageName, final boolean succeeded) {
                            mHandler.post(new Runnable() {
                                @Override
                                public void run() {
                                    mClearDataPreference.setClearingData(false);
                                    if (succeeded) {
                                        dataCleared(true);
                                    } else {
                                        dataCleared(false);
                                    }
                                }
                            });
                        }
                    });
            if (!success) {
                mClearDataPreference.setClearingData(false);
                dataCleared(false);
            }
        }
        mClearDataPreference.refresh();
    }

    private void dataCleared(boolean succeeded) {
        if (succeeded) {
            final int userId =  UserHandle.getUserId(mEntry.info.uid);
            mApplicationsState.requestSize(mPackageName, userId);
        } else {
            Log.w(TAG, "Failed to clear data!");
            mClearDataPreference.refresh();
        }
    }

    public void clearCache() {
        mClearCachePreference.setClearingCache(true);
        mPackageManager.deleteApplicationCacheFiles(mEntry.info.packageName,
                new IPackageDataObserver.Stub() {
                    public void onRemoveCompleted(final String packageName,
                            final boolean succeeded) {
                        mHandler.post(new Runnable() {
                            @Override
                            public void run() {
                                mClearCachePreference.setClearingCache(false);
                                cacheCleared(succeeded);
                            }
                        });
                    }
                });
        mClearCachePreference.refresh();
    }

    private void cacheCleared(boolean succeeded) {
        if (succeeded) {
            final int userId =  UserHandle.getUserId(mEntry.info.uid);
            mApplicationsState.requestSize(mPackageName, userId);
        } else {
            Log.w(TAG, "Failed to clear cache!");
            mClearCachePreference.refresh();
        }
    }

    public void launchUninstall() {
        if (mUninstallPreference.canUninstall() || mUninstallPreference.canUninstallUpdates()) {
            Intent uninstallIntent = mUninstallPreference.getUninstallIntent();
            uninstallIntent.putExtra(Intent.EXTRA_RETURN_RESULT, true);
            startActivityForResult(uninstallIntent, REQUEST_UNINSTALL);
        }
        mUninstallPreference.refresh();
    }

    private class ApplicationsStateCallbacks implements ApplicationsState.Callbacks {

        @Override
        public void onRunningStateChanged(boolean running) {
            if (mForceStopPreference != null) {
                mForceStopPreference.refresh();
            }
        }

        @Override
        public void onPackageListChanged() {
            if (mEntry == null) {
                return;
            }
            final int userId = UserHandle.getUserId(mEntry.info.uid);
            mEntry = mApplicationsState.getEntry(mPackageName, userId);
            if (mEntry == null) {
                navigateBack();
            }
            onCreatePreferences(null, null);
        }

        @Override
        public void onRebuildComplete(ArrayList<ApplicationsState.AppEntry> apps) {}

        @Override
        public void onPackageIconChanged() {}

        @Override
        public void onPackageSizeChanged(String packageName) {
            if (mAppStoragePreference == null) {
                // Nothing to do here.
                return;
            }
            mAppStoragePreference.refresh();
            mClearDataPreference.refresh();
            mClearCachePreference.refresh();
        }

        @Override
        public void onAllSizesComputed() {
            if (mAppStoragePreference == null) {
                // Nothing to do here.
                return;
            }
            mAppStoragePreference.refresh();
            mClearDataPreference.refresh();
            mClearCachePreference.refresh();
        }

        @Override
        public void onLauncherInfoChanged() {}

        @Override
        public void onLoadEntriesCompleted() {
            if (mAppStoragePreference == null) {
                // Nothing to do here.
                return;
            }
            mAppStoragePreference.refresh();
            mClearDataPreference.refresh();
            mClearCachePreference.refresh();
        }
    }
}
