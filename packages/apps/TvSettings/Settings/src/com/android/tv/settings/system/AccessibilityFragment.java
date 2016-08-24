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

import android.accessibilityservice.AccessibilityServiceInfo;
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.pm.ServiceInfo;
import android.os.Bundle;
import android.provider.Settings;
import android.support.annotation.NonNull;
import android.support.v14.preference.SwitchPreference;
import android.support.v17.leanback.app.GuidedStepFragment;
import android.support.v17.leanback.widget.GuidanceStylist;
import android.support.v17.leanback.widget.GuidedAction;
import android.support.v17.preference.LeanbackPreferenceFragment;
import android.support.v7.preference.CheckBoxPreference;
import android.support.v7.preference.Preference;
import android.support.v7.preference.PreferenceCategory;
import android.support.v7.preference.PreferenceGroup;
import android.support.v7.preference.PreferenceScreen;
import android.text.TextUtils;
import android.view.accessibility.AccessibilityManager;

import com.android.settingslib.accessibility.AccessibilityUtils;
import com.android.tv.settings.R;

import java.util.List;
import java.util.Set;

public class AccessibilityFragment extends LeanbackPreferenceFragment {
    private static final String TOGGLE_HIGH_TEXT_CONTRAST_PREFERENCE =
            "toggle_high_text_contrast_preference";

    private PreferenceGroup mServicesPref;

    public static AccessibilityFragment newInstance() {
        return new AccessibilityFragment();
    }

    @Override
    public void onResume() {
        super.onResume();
        if (mServicesPref != null) {
            refreshServices(mServicesPref);
        }
    }

    @Override
    public void onCreatePreferences(Bundle savedInstanceState, String rootKey) {
        final Context themedContext = getPreferenceManager().getContext();
        final PreferenceScreen screen =
                getPreferenceManager().createPreferenceScreen(themedContext);
        screen.setTitle(R.string.system_accessibility);

        final Preference captionsPreference = new Preference(themedContext);
        captionsPreference.setTitle(R.string.accessibility_captions);
        captionsPreference.setIntent(new Intent(Intent.ACTION_MAIN).setComponent(
                new ComponentName(getActivity(), CaptionSetupActivity.class)));
        screen.addPreference(captionsPreference);

        final SwitchPreference highContrastPreference = new SwitchPreference(themedContext);
        highContrastPreference.setKey(TOGGLE_HIGH_TEXT_CONTRAST_PREFERENCE);
        highContrastPreference.setPersistent(false);
        highContrastPreference.setTitle(
                R.string.accessibility_toggle_high_text_contrast_preference_title);
        highContrastPreference.setSummary(R.string.experimental_preference);
        highContrastPreference.setChecked(Settings.Secure.getInt(getContext().getContentResolver(),
                Settings.Secure.ACCESSIBILITY_HIGH_TEXT_CONTRAST_ENABLED, 0) == 1);
        screen.addPreference(highContrastPreference);

        mServicesPref = new PreferenceCategory(themedContext);
        mServicesPref.setTitle(R.string.system_services);
        screen.addPreference(mServicesPref);
        refreshServices(mServicesPref);

        final Preference ttsPref = new Preference(themedContext);
        ttsPref.setTitle(R.string.system_accessibility_tts_output);
        ttsPref.setFragment(TextToSpeechFragment.class.getName());
        screen.addPreference(ttsPref);

        setPreferenceScreen(screen);
    }

    @Override
    public boolean onPreferenceTreeClick(Preference preference) {
        if (TextUtils.equals(preference.getKey(), TOGGLE_HIGH_TEXT_CONTRAST_PREFERENCE)) {
            Settings.Secure.putInt(getActivity().getContentResolver(),
                    Settings.Secure.ACCESSIBILITY_HIGH_TEXT_CONTRAST_ENABLED,
                    (((SwitchPreference) preference).isChecked() ? 1 : 0));
            return true;
        } else {
            return super.onPreferenceTreeClick(preference);
        }
    }

    private void refreshServices(PreferenceGroup group) {
        final List<AccessibilityServiceInfo> installedServiceInfos = AccessibilityManager
                .getInstance(getActivity()).getInstalledAccessibilityServiceList();
        final Set<ComponentName> enabledServices =
                AccessibilityUtils.getEnabledServicesFromSettings(getActivity());
        final boolean accessibilityEnabled = Settings.Secure.getInt(
                getActivity().getContentResolver(),
                Settings.Secure.ACCESSIBILITY_ENABLED, 0) == 1;

        for (final AccessibilityServiceInfo accInfo : installedServiceInfos) {
            final ServiceInfo serviceInfo = accInfo.getResolveInfo().serviceInfo;
            final ComponentName componentName = new ComponentName(serviceInfo.packageName,
                    serviceInfo.name);

            final boolean serviceEnabled = accessibilityEnabled
                    && enabledServices.contains(componentName);

            final String title = accInfo.getResolveInfo()
                    .loadLabel(getActivity().getPackageManager()).toString();

            final String key = "ServicePref:" + componentName.flattenToString();
            Preference servicePref = findPreference(key);
            if (servicePref == null) {
                servicePref = new Preference(group.getContext());
                servicePref.setKey(key);
            }
            servicePref.setTitle(title);
            servicePref.setSummary(serviceEnabled ? R.string.settings_on : R.string.settings_off);
            servicePref.setFragment(AccessibilityServiceFragment.class.getName());
            AccessibilityServiceFragment.prepareArgs(servicePref.getExtras(),
                    serviceInfo.packageName,
                    serviceInfo.name,
                    accInfo.getSettingsActivityName(),
                    title);
            group.addPreference(servicePref);
        }
    }

    public static class AccessibilityServiceFragment extends LeanbackPreferenceFragment {
        private static final String ARG_PACKAGE_NAME = "packageName";
        private static final String ARG_SERVICE_NAME = "serviceName";
        private static final String ARG_SETTINGS_ACTIVITY_NAME = "settingsActivityName";
        private static final String ARG_LABEL = "label";

        private CheckBoxPreference mEnablePref;

        public static void prepareArgs(@NonNull Bundle args, String packageName, String serviceName,
                String activityName, String label) {
            args.putString(ARG_PACKAGE_NAME, packageName);
            args.putString(ARG_SERVICE_NAME, serviceName);
            args.putString(ARG_SETTINGS_ACTIVITY_NAME, activityName);
            args.putString(ARG_LABEL, label);
        }

        @Override
        public void onCreatePreferences(Bundle savedInstanceState, String rootKey) {
            final Context themedContext = getPreferenceManager().getContext();
            final PreferenceScreen screen =
                    getPreferenceManager().createPreferenceScreen(themedContext);
            screen.setTitle(getArguments().getString(ARG_LABEL));

            mEnablePref = new CheckBoxPreference(themedContext);
            mEnablePref.setTitle(R.string.system_accessibility_status);
            mEnablePref.setFragment(EnableDisableConfirmationFragment.class.getName());
            screen.addPreference(mEnablePref);

            final Preference settingsPref = new Preference(themedContext);
            settingsPref.setTitle(R.string.system_accessibility_config);
            final String activityName = getArguments().getString(ARG_SETTINGS_ACTIVITY_NAME);
            if (!TextUtils.isEmpty(activityName)) {
                final String packageName = getArguments().getString(ARG_PACKAGE_NAME);
                settingsPref.setIntent(new Intent(Intent.ACTION_MAIN)
                        .setComponent(new ComponentName(packageName, activityName)));
            } else {
                settingsPref.setEnabled(false);
            }
            screen.addPreference(settingsPref);

            setPreferenceScreen(screen);
        }

        @Override
        public void onResume() {
            super.onResume();

            final String packageName = getArguments().getString(ARG_PACKAGE_NAME);
            final String serviceName = getArguments().getString(ARG_SERVICE_NAME);

            final ComponentName serviceComponent = new ComponentName(packageName, serviceName);
            final Set<ComponentName> enabledServices =
                    AccessibilityUtils.getEnabledServicesFromSettings(getActivity());
            final boolean enabled = enabledServices.contains(serviceComponent);

            mEnablePref.setChecked(enabled);
            EnableDisableConfirmationFragment.prepareArgs(mEnablePref.getExtras(),
                    new ComponentName(packageName, serviceName),
                    getArguments().getString(ARG_LABEL), !enabled);
        }

        public static class EnableDisableConfirmationFragment extends GuidedStepFragment {
            private static final String ARG_LABEL = "label";
            private static final String ARG_COMPONENT = "component";
            private static final String ARG_ENABLING = "enabling";

            public static void prepareArgs(@NonNull Bundle args, ComponentName cn, String label,
                    boolean enabling) {
                args.putParcelable(ARG_COMPONENT, cn);
                args.putString(ARG_LABEL, label);
                args.putBoolean(ARG_ENABLING, enabling);
            }

            @NonNull
            @Override
            public GuidanceStylist.Guidance onCreateGuidance(Bundle savedInstanceState) {
                final String label = getArguments().getString(ARG_LABEL);
                if (getArguments().getBoolean(ARG_ENABLING)) {
                    return new GuidanceStylist.Guidance(
                            getString(R.string.system_accessibility_service_on_confirm_title,
                                    label),
                            getString(R.string.system_accessibility_service_on_confirm_desc,
                                    label),
                            null,
                            getActivity().getDrawable(R.drawable.ic_accessibility_new_132dp)
                    );
                } else {
                    return new GuidanceStylist.Guidance(
                            getString(R.string.system_accessibility_service_off_confirm_title,
                                    label),
                            getString(R.string.system_accessibility_service_off_confirm_desc,
                                    label),
                            null,
                            getActivity().getDrawable(R.drawable.ic_accessibility_new_132dp)
                    );
                }
            }

            @Override
            public void onCreateActions(@NonNull List<GuidedAction> actions,
                    Bundle savedInstanceState) {
                final Context context = getActivity();
                actions.add(new GuidedAction.Builder(context)
                        .clickAction(GuidedAction.ACTION_ID_OK).build());
                actions.add(new GuidedAction.Builder(context)
                        .clickAction(GuidedAction.ACTION_ID_CANCEL).build());
            }

            @Override
            public void onGuidedActionClicked(GuidedAction action) {
                if (action.getId() == GuidedAction.ACTION_ID_OK) {
                    final ComponentName component = getArguments().getParcelable(ARG_COMPONENT);
                    AccessibilityUtils.setAccessibilityServiceState(getActivity(),
                            component, getArguments().getBoolean(ARG_ENABLING));
                    getFragmentManager().popBackStack();
                } else if (action.getId() == GuidedAction.ACTION_ID_CANCEL) {
                    getFragmentManager().popBackStack();
                } else {
                    super.onGuidedActionClicked(action);
                }
            }
        }
    }
}
