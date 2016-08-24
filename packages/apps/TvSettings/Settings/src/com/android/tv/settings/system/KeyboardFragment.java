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

package com.android.tv.settings.system;

import android.app.ActivityManagerNative;
import android.content.Context;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.os.Bundle;
import android.os.RemoteException;
import android.os.UserHandle;
import android.provider.Settings;
import android.support.v17.preference.LeanbackPreferenceFragment;
import android.support.v7.preference.ListPreference;
import android.support.v7.preference.Preference;
import android.support.v7.preference.PreferenceScreen;
import android.text.TextUtils;
import android.util.Log;
import android.view.inputmethod.InputMethodInfo;
import android.view.inputmethod.InputMethodManager;

import com.android.tv.settings.R;

import java.util.ArrayList;
import java.util.Iterator;
import java.util.List;

public class KeyboardFragment extends LeanbackPreferenceFragment {
    private static final String TAG = "KeyboardFragment";
    private static final String INPUT_METHOD_SEPARATOR = ":";
    private static final String KEY_CURRENT_KEYBOARD = "currentKeyboard";

    private InputMethodManager mInputMethodManager;

    public static KeyboardFragment newInstance() {
        return new KeyboardFragment();
    }

    @Override
    public void onCreate(Bundle savedInstanceState) {
        mInputMethodManager =
                (InputMethodManager) getActivity().getSystemService(Context.INPUT_METHOD_SERVICE);

        enableAllInputMethods();

        super.onCreate(savedInstanceState);
    }

    @Override
    public void onCreatePreferences(Bundle savedInstanceState, String rootKey) {
        final Context preferenceContext = getPreferenceManager().getContext();
        final PackageManager packageManager = getActivity().getPackageManager();

        final PreferenceScreen screen =
                getPreferenceManager().createPreferenceScreen(preferenceContext);
        screen.setTitle(R.string.system_keyboard);

        List<InputMethodInfo> enabledInputMethodInfos = getEnabledSystemInputMethodList();

        final List<CharSequence> entries = new ArrayList<>(enabledInputMethodInfos.size());
        final List<CharSequence> values = new ArrayList<>(enabledInputMethodInfos.size());

        int defaultIndex = 0;
        final String defaultId = Settings.Secure.getString(getActivity().getContentResolver(),
                Settings.Secure.DEFAULT_INPUT_METHOD);

        for (final InputMethodInfo info : enabledInputMethodInfos) {
            entries.add(info.loadLabel(packageManager));
            final String id = info.getId();
            values.add(id);
            if (TextUtils.equals(id, defaultId)) {
                defaultIndex = values.size() - 1;
            }
        }

        final ListPreference currentKeyboard = new ListPreference(preferenceContext);
        currentKeyboard.setPersistent(false);
        currentKeyboard.setTitle(R.string.title_current_keyboard);
        currentKeyboard.setDialogTitle(R.string.title_current_keyboard);
        currentKeyboard.setSummary("%s");
        currentKeyboard.setKey(KEY_CURRENT_KEYBOARD);
        currentKeyboard.setEntries(entries.toArray(new CharSequence[entries.size()]));
        currentKeyboard.setEntryValues(values.toArray(new CharSequence[values.size()]));
        currentKeyboard.setValueIndex(defaultIndex);
        currentKeyboard.setOnPreferenceChangeListener(new Preference.OnPreferenceChangeListener() {
            @Override
            public boolean onPreferenceChange(Preference preference, Object newValue) {
                setInputMethod((String) newValue);
                return true;
            }
        });
        screen.addPreference(currentKeyboard);

        // Add per-IME settings
        for (final InputMethodInfo info : enabledInputMethodInfos) {
            final Intent settingsIntent = getInputMethodSettingsIntent(info);
            if (settingsIntent == null) {
                continue;
            }
            final Preference preference = new Preference(preferenceContext);
            preference.setTitle(info.loadLabel(packageManager));
            preference.setKey("keyboardSettings:" + info.getId());
            preference.setIntent(settingsIntent);
            screen.addPreference(preference);
        }
        setPreferenceScreen(screen);
    }



    private void setInputMethod(String imid) {
        if (imid == null) {
            throw new IllegalArgumentException("Null ID");
        }

        int userId;
        try {
            userId = ActivityManagerNative.getDefault().getCurrentUser().id;
            Settings.Secure.putStringForUser(getActivity().getContentResolver(),
                    Settings.Secure.DEFAULT_INPUT_METHOD, imid, userId);

            if (ActivityManagerNative.isSystemReady()) {
                Intent intent = new Intent(Intent.ACTION_INPUT_METHOD_CHANGED);
                intent.addFlags(Intent.FLAG_RECEIVER_REPLACE_PENDING);
                intent.putExtra("input_method_id", imid);
                getActivity().sendBroadcastAsUser(intent, UserHandle.CURRENT);
            }
        } catch (RemoteException e) {
            Log.d(TAG, "set default input method remote exception");
        }
    }

    private List<InputMethodInfo> getEnabledSystemInputMethodList() {
        List<InputMethodInfo> enabledInputMethodInfos =
                new ArrayList<>(mInputMethodManager.getEnabledInputMethodList());
        // Filter auxiliary keyboards out
        for (Iterator<InputMethodInfo> it = enabledInputMethodInfos.iterator(); it.hasNext();) {
            if (it.next().isAuxiliaryIme()) {
                it.remove();
            }
        }
        return enabledInputMethodInfos;
    }

    private Intent getInputMethodSettingsIntent(InputMethodInfo imi) {
        final Intent intent;
        final String settingsActivity = imi.getSettingsActivity();
        if (!TextUtils.isEmpty(settingsActivity)) {
            intent = new Intent(Intent.ACTION_MAIN);
            intent.setClassName(imi.getPackageName(), settingsActivity);
        } else {
            intent = null;
        }
        return intent;
    }

    private void enableAllInputMethods() {
        List<InputMethodInfo> allInputMethodInfos =
                new ArrayList<>(mInputMethodManager.getInputMethodList());
        boolean needAppendSeparator = false;
        StringBuilder builder = new StringBuilder();
        for (InputMethodInfo imi : allInputMethodInfos) {
            if (needAppendSeparator) {
                builder.append(INPUT_METHOD_SEPARATOR);
            } else {
                needAppendSeparator = true;
            }
            builder.append(imi.getId());
        }
        Settings.Secure.putString(getActivity().getContentResolver(),
                Settings.Secure.ENABLED_INPUT_METHODS, builder.toString());
    }

}
