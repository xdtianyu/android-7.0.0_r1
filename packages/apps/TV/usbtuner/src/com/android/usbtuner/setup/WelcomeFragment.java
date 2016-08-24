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

import android.os.Bundle;
import android.support.annotation.NonNull;
import android.support.v17.leanback.widget.GuidanceStylist.Guidance;
import android.support.v17.leanback.widget.GuidedAction;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;

import com.android.tv.common.ui.setup.SetupGuidedStepFragment;
import com.android.tv.common.ui.setup.SetupMultiPaneFragment;
import com.android.usbtuner.R;
import com.android.usbtuner.UsbTunerPreferences;

import java.util.List;

/**
 * A fragment for initial screen.
 */
public class WelcomeFragment extends SetupMultiPaneFragment {
    public static final String ACTION_CATEGORY =
            "com.android.usbtuner.setup.WelcomeFragment";

    @Override
    protected SetupGuidedStepFragment onCreateContentFragment() {
        return new ContentFragment();
    }

    @Override
    protected String getActionCategory() {
        return ACTION_CATEGORY;
    }

    @Override
    protected boolean needsDoneButton() {
        return false;
    }

    public static class ContentFragment extends SetupGuidedStepFragment {
        private int mChannelCountOnPreference;

        @Override
        public View onCreateView(LayoutInflater inflater, ViewGroup container,
                Bundle savedInstanceState) {
            mChannelCountOnPreference = UsbTunerPreferences
                    .getScannedChannelCount(getActivity().getApplicationContext());
            return super.onCreateView(inflater, container, savedInstanceState);
        }

        @NonNull
        @Override
        public Guidance onCreateGuidance(Bundle savedInstanceState) {
            String title;
            String description;
            if (mChannelCountOnPreference == 0) {
                title = getString(R.string.ut_setup_new_title);
                description = getString(R.string.ut_setup_new_description);
            } else {
                title = getString(R.string.ut_setup_again_title);
                description = getString(R.string.ut_setup_again_description);
            }
            return new Guidance(title, description, null, null);
        }

        @Override
        public void onCreateActions(@NonNull List<GuidedAction> actions,
                Bundle savedInstanceState) {
            String[] choices = getResources().getStringArray(mChannelCountOnPreference == 0
                    ? R.array.ut_setup_new_choices : R.array.ut_setup_again_choices);
            for (int i = 0; i < choices.length - 1; ++i) {
                actions.add(new GuidedAction.Builder(getActivity()).id(i).title(choices[i])
                        .build());
            }
            actions.add(new GuidedAction.Builder(getActivity()).id(ACTION_DONE)
                    .title(choices[choices.length - 1]).build());
        }

        @Override
        protected String getActionCategory() {
            return ACTION_CATEGORY;
        }
    }
}
