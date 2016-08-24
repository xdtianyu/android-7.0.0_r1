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

import android.content.Context;
import android.content.res.Resources;
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
public class ScanResultFragment extends SetupMultiPaneFragment {
    public static final String ACTION_CATEGORY =
            "com.android.usbtuner.setup.ScanResultFragment";

    /**
     * An action which moves to previous page when the user presses BACK button.
     * In some cases, more than one page can be popped out.
     */
    public static final int ACTION_BACK_TO_CONNECTION_TYPE = ACTION_DONE - 1;

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
        public void onAttach(Context context) {
            super.onAttach(context);
            mChannelCountOnPreference = UsbTunerPreferences.getScannedChannelCount(context);
        }

        @NonNull
        @Override
        public Guidance onCreateGuidance(Bundle savedInstanceState) {
            String title;
            String description;
            String breadcrumb;
            if (mChannelCountOnPreference > 0) {
                Resources res = getResources();
                title = res.getQuantityString(R.plurals.ut_result_found_title,
                        mChannelCountOnPreference, mChannelCountOnPreference);
                description = res.getQuantityString(R.plurals.ut_result_found_description,
                        mChannelCountOnPreference, mChannelCountOnPreference);
                breadcrumb = null;
            } else {
                title = getString(R.string.ut_result_not_found_title);
                description = getString(R.string.ut_result_not_found_description);
                breadcrumb = getString(R.string.ut_setup_breadcrumb);
            }
            return new Guidance(title, description, breadcrumb, null);
        }

        @Override
        public void onCreateActions(@NonNull List<GuidedAction> actions,
                Bundle savedInstanceState) {
            String[] choices;
            int doneActionIndex;
            if (mChannelCountOnPreference > 0) {
                choices = getResources().getStringArray(R.array.ut_result_found_choices);
                doneActionIndex = 0;
            } else {
                choices = getResources().getStringArray(R.array.ut_result_not_found_choices);
                doneActionIndex = 1;
            }
            for (int i = 0; i < choices.length; ++i) {
                if (i == doneActionIndex) {
                    actions.add(new GuidedAction.Builder(getActivity()).id(ACTION_DONE)
                            .title(choices[i]).build());
                } else {
                    actions.add(new GuidedAction.Builder(getActivity()).id(i).title(choices[i])
                            .build());
                }
            }
        }

        @Override
        protected String getActionCategory() {
            return ACTION_CATEGORY;
        }
    }
}
