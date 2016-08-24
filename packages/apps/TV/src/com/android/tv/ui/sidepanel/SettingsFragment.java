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

package com.android.tv.ui.sidepanel;

import android.view.View;
import android.widget.Toast;

import com.android.tv.MainActivity;
import com.android.tv.R;
import com.android.tv.TvApplication;
import com.android.tv.dialog.PinDialogFragment;
import com.android.tv.dialog.WebDialogFragment;
import com.android.tv.license.LicenseUtils;
import com.android.tv.ui.sidepanel.parentalcontrols.ParentalControlsFragment;
import com.android.tv.util.PermissionUtils;
import com.android.tv.util.SetupUtils;

import java.util.ArrayList;
import java.util.List;

/**
 * Shows Live TV settings.
 */
public class SettingsFragment extends SideFragment {
    private static final String TRACKER_LABEL = "settings";

    private final long mCurrentChannelId;

    public SettingsFragment(long currentChannelId) {
        mCurrentChannelId = currentChannelId;
    }

    /**
     * Opens a dialog showing open source licenses.
     */
    public static final class LicenseActionItem extends ActionItem {
        public final static String DIALOG_TAG = LicenseActionItem.class.getSimpleName();
        public static final String TRACKER_LABEL = "Open Source Licenses";
        private final MainActivity mMainActivity;

        public LicenseActionItem(MainActivity mainActivity) {
            super(mainActivity.getString(R.string.settings_menu_licenses));
            mMainActivity = mainActivity;
        }

        @Override
        protected void onSelected() {
            WebDialogFragment dialog = WebDialogFragment.newInstance(LicenseUtils.LICENSE_FILE,
                    mMainActivity.getString(R.string.dialog_title_licenses), TRACKER_LABEL);
            mMainActivity.getOverlayManager().showDialogFragment(DIALOG_TAG, dialog, false);
        }
    }

   @Override
    protected String getTitle() {
        return getResources().getString(R.string.side_panel_title_settings);
    }

    @Override
    public String getTrackerLabel() {
        return TRACKER_LABEL;
    }

    @Override
    protected List<Item> getItemList() {
        List<Item> items = new ArrayList<>();
        final Item customizeChannelListItem = new SubMenuItem(
                getString(R.string.settings_channel_source_item_customize_channels),
                getString(R.string.settings_channel_source_item_customize_channels_description),
                0, getMainActivity().getOverlayManager().getSideFragmentManager()) {
            @Override
            protected SideFragment getFragment() {
                return new CustomizeChannelListFragment(mCurrentChannelId);
            }

            @Override
            protected void onBind(View view) {
                super.onBind(view);
                setEnabled(false);
            }

            @Override
            protected void onUpdate() {
                super.onUpdate();
                setEnabled(getChannelDataManager().getChannelCount() != 0);
            }
        };
        customizeChannelListItem.setEnabled(false);
        items.add(customizeChannelListItem);
        final MainActivity activity = getMainActivity();
        boolean hasNewInput = SetupUtils.getInstance(activity).hasNewInput(
                activity.getTvInputManagerHelper());
        items.add(new ActionItem(
                getString(R.string.settings_channel_source_item_setup),
                hasNewInput ? getString(R.string.settings_channel_source_item_setup_new_inputs)
                        : null) {
            @Override
            protected void onSelected() {
                closeFragment();
                activity.getOverlayManager().showSetupFragment();
            }
        });
        if (PermissionUtils.hasModifyParentalControls(getMainActivity())) {
            items.add(new ActionItem(getString(R.string.settings_parental_controls),
                    getString(activity.getParentalControlSettings().isParentalControlsEnabled()
                            ? R.string.option_toggle_parental_controls_on
                            : R.string.option_toggle_parental_controls_off)) {
                @Override
                protected void onSelected() {
                    final MainActivity tvActivity = getMainActivity();
                    final SideFragmentManager sideFragmentManager = tvActivity.getOverlayManager()
                            .getSideFragmentManager();
                    sideFragmentManager.hideSidePanel(true);
                    PinDialogFragment fragment = new PinDialogFragment(
                            PinDialogFragment.PIN_DIALOG_TYPE_ENTER_PIN,
                            new PinDialogFragment.ResultListener() {
                                @Override
                                public void done(boolean success) {
                                    if (success) {
                                        sideFragmentManager.show(new ParentalControlsFragment(),
                                                false);
                                        sideFragmentManager.showSidePanel(true);
                                    } else {
                                        sideFragmentManager.hideAll(false);
                                    }
                                }
                            });
                    tvActivity.getOverlayManager().showDialogFragment(PinDialogFragment.DIALOG_TAG,
                            fragment, true);
                }
            });
        } else {
            // Note: parental control is turned off, when MODIFY_PARENTAL_CONTROLS is not granted.
            // But, we may be able to turn on channel lock feature regardless of the permission.
            // It's TBD.
        }
        if (LicenseUtils.hasLicenses(activity.getAssets())) {
            items.add(new LicenseActionItem(activity));
        }
        // Show version.
        items.add(new SimpleItem(getString(R.string.settings_menu_version),
                ((TvApplication) activity.getApplicationContext()).getVersionName()));
        return items;
    }

    @Override
    public void onResume() {
        super.onResume();
        if (getChannelDataManager().areAllChannelsHidden()) {
            Toast.makeText(getActivity(), R.string.msg_all_channels_hidden, Toast.LENGTH_SHORT)
                    .show();
        }
    }
}
