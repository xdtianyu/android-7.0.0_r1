/*
 * Copyright (C) 2016 The Android Open Source Project
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

package com.android.tv.settings.system;

import android.os.Bundle;
import android.support.annotation.Keep;
import android.support.annotation.NonNull;
import android.support.v17.leanback.app.GuidedStepFragment;
import android.support.v17.leanback.widget.GuidanceStylist;
import android.support.v17.leanback.widget.GuidedAction;

import com.android.tv.settings.R;

import java.util.List;

@Keep
public class UnknownSourcesConfirmationFragment extends GuidedStepFragment {
    public interface Callback {
        void onConfirmUnknownSources(boolean success);
    }

    @NonNull
    @Override
    public GuidanceStylist.Guidance onCreateGuidance(Bundle savedInstanceState) {
        return new GuidanceStylist.Guidance(
                getString(R.string.security_unknown_sources_confirm_title),
                getString(R.string.security_unknown_sources_confirm_desc),
                null,
                getActivity().getDrawable(R.drawable.ic_warning_132dp)
        );
    }

    @Override
    public void onCreateActions(@NonNull List<GuidedAction> actions, Bundle savedInstanceState) {
        actions.add(new GuidedAction.Builder(getContext())
                .clickAction(GuidedAction.ACTION_ID_OK)
                .build());
        actions.add(new GuidedAction.Builder(getContext())
                .clickAction(GuidedAction.ACTION_ID_CANCEL)
                .build());
    }

    @Override
    public void onGuidedActionClicked(GuidedAction action) {
        final Callback callback = (Callback) getTargetFragment();
        if (action.getId() == GuidedAction.ACTION_ID_OK) {
            callback.onConfirmUnknownSources(true);
            getFragmentManager().popBackStack();
        } else if (action.getId() == GuidedAction.ACTION_ID_CANCEL) {
            callback.onConfirmUnknownSources(false);
            getFragmentManager().popBackStack();
        } else {
            throw new IllegalArgumentException("Unknown action");
        }
    }
}
