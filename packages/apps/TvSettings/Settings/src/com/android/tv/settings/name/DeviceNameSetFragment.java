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

package com.android.tv.settings.name;

import android.os.Build;
import android.os.Bundle;
import android.support.annotation.NonNull;
import android.support.v17.leanback.app.GuidedStepFragment;
import android.support.v17.leanback.widget.GuidanceStylist;
import android.support.v17.leanback.widget.GuidedAction;

import com.android.tv.settings.R;

import java.util.List;

public class DeviceNameSetFragment extends GuidedStepFragment {

    public static DeviceNameSetFragment newInstance() {
        return new DeviceNameSetFragment();
    }

    @NonNull
    @Override
    public GuidanceStylist.Guidance onCreateGuidance(Bundle savedInstanceState) {
        return new GuidanceStylist.Guidance(
                getString(R.string.select_device_name_title, Build.MODEL),
                getString(R.string.select_device_name_description, Build.MODEL),
                null,
                null);
    }

    @Override
    public void onCreateActions(@NonNull List<GuidedAction> actions, Bundle savedInstanceState) {
        final String[] options = getResources().getStringArray(R.array.rooms);
        final int length = options.length;
        for (int i = 0; i < length; i++) {
            actions.add(new GuidedAction.Builder()
                    .title(options[i])
                    .id(i)
                    .build());
        }
        actions.add(new GuidedAction.Builder()
                .title(getString(R.string.custom_room))
                .id(options.length)
                .build());
        super.onCreateActions(actions, savedInstanceState);
    }

    @Override
    public void onGuidedActionClicked(GuidedAction action) {
        final long id = action.getId();
        final String[] options = getResources().getStringArray(R.array.rooms);
        if (id < 0 || id > options.length) {
            throw new IllegalStateException("Unknown action ID");
        } else if (id < options.length) {
            DeviceManager.setDeviceName(getActivity(), options[(int)id]);
            getActivity().finish();
        } else if (id == options.length) {
            GuidedStepFragment.add(getFragmentManager(), DeviceNameSetCustomFragment.newInstance());
        }
    }
}
