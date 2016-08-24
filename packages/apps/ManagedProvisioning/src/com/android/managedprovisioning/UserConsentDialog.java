/*
 * Copyright 2014, The Android Open Source Project
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

package com.android.managedprovisioning;

import android.app.Dialog;
import android.app.DialogFragment;
import android.content.DialogInterface;
import android.content.Intent;
import android.net.Uri;
import android.os.Bundle;
import android.view.View;
import android.view.View.OnClickListener;
import android.widget.TextView;
import android.widget.Button;
import android.widget.CheckBox;
import android.widget.CompoundButton;
import android.widget.CompoundButton.OnCheckedChangeListener;

import com.android.managedprovisioning.common.Utils;
import com.android.managedprovisioning.uiflows.WebActivity;
import com.android.setupwizardlib.util.SystemBarHelper;

/**
 * Dialog used to notify the user that the admin will have full control over the profile/device.
 * Custom runnables can be passed that are run on consent or cancel.
 */
public class UserConsentDialog extends DialogFragment {
    private static final int PROFILE_OWNER = 1;
    private static final int DEVICE_OWNER = 2;

    private static final String LEARN_MORE_URL_PROFILE_OWNER =
            "https://support.google.com/android/work/answer/6090512";
    // TODO: replace by the final device owner learn more link.
    private static final String LEARN_MORE_URL_DEVICE_OWNER =
            "https://support.google.com/android/work/answer/6090512";

    // Only urls starting with this base can be visisted in the device owner case.
    private static final String LEARN_MORE_ALLOWED_BASE_URL =
            "https://support.google.com/";

    private static final String KEY_OWNER_TYPE = "owner_type";
    private static final String KEY_SHOW_CONSENT_CHECKBOX = "consent_checkbox";

    private final Utils mUtils = new Utils();

    public static UserConsentDialog newProfileOwnerInstance() {
        return newInstance(PROFILE_OWNER, false);
    }

    public static UserConsentDialog newDeviceOwnerInstance(boolean showConsentCheckbox) {
        return newInstance(DEVICE_OWNER, showConsentCheckbox);
    }

    private static UserConsentDialog newInstance(int ownerType, boolean showConsentCheckbox) {
        UserConsentDialog dialog = new UserConsentDialog();
        Bundle args = new Bundle();
        args.putInt(KEY_OWNER_TYPE, ownerType);
        args.putBoolean(KEY_SHOW_CONSENT_CHECKBOX, showConsentCheckbox);
        dialog.setArguments(args);
        return dialog;
    }

    @Override
    public Dialog onCreateDialog(Bundle savedInstanceState) {
        int ownerType = getArguments().getInt(KEY_OWNER_TYPE);
        if (ownerType != PROFILE_OWNER && ownerType != DEVICE_OWNER) {
            throw new IllegalArgumentException("Illegal value for argument ownerType.");
        }
        boolean isProfileOwner = (ownerType == PROFILE_OWNER);

        final Dialog dialog = new Dialog(getActivity(), R.style.ManagedProvisioningDialogTheme);
        dialog.setContentView(R.layout.learn_more_dialog);
        dialog.setCanceledOnTouchOutside(false);
        if (!mUtils.isUserSetupCompleted(getActivity())) {
            SystemBarHelper.hideSystemBars(dialog);
        }

        final TextView learnMoreMsg = (TextView) dialog.findViewById(R.id.learn_more_text1);
        final TextView linkText = (TextView) dialog.findViewById(R.id.learn_more_link);
        final TextView textFrpWarning = (TextView) dialog.findViewById(
                R.id.learn_more_frp_warning);

        initializeLearnMoreLink(linkText, isProfileOwner);
        learnMoreMsg.setText(isProfileOwner ? R.string.admin_has_ability_to_monitor_profile
                : R.string.admin_has_ability_to_monitor_device);

        if (!isProfileOwner && mUtils.isFrpSupported(getActivity())) {
            // For device owner, show a warning that FRP might not be fully active
            textFrpWarning.setVisibility(View.VISIBLE);
        }

        final Button positiveButton = (Button) dialog.findViewById(R.id.positive_button);
        positiveButton.setOnClickListener(new OnClickListener() {
                @Override
                public void onClick(View v) {
                    dialog.dismiss();
                    ((ConsentCallback) getActivity()).onDialogConsent();
                }
            });

        final CheckBox consentCheckbox =
                (CheckBox) dialog.findViewById(R.id.user_consent_checkbox);
        if (getArguments().getBoolean(KEY_SHOW_CONSENT_CHECKBOX)) {
            consentCheckbox.setOnCheckedChangeListener(new OnCheckedChangeListener() {
                    @Override
                    public void onCheckedChanged(CompoundButton cb, boolean isChecked) {
                        positiveButton.setEnabled(isChecked);
                    }
                });
            consentCheckbox.setVisibility(View.VISIBLE);
            consentCheckbox.setChecked(false);
            positiveButton.setEnabled(false);
        }

        final Button negativeButton = (Button) dialog.findViewById(R.id.negative_button);
        negativeButton.setOnClickListener(new OnClickListener() {
                @Override
                public void onClick(View v) {
                    dialog.dismiss();
                    ((ConsentCallback) getActivity()).onDialogCancel();
                }
            });

        return dialog;
    }

    private void initializeLearnMoreLink(TextView linkText, boolean isProfileOwner) {
        if (!mUtils.isConnectedToNetwork(getActivity())) {
            // If the device has currently no connectivity, don't show the "learn more" link.
            linkText.setVisibility(View.GONE);
        } else {
            // Otherwise register a listener that starts a webview activity.
            final String url = isProfileOwner ? LEARN_MORE_URL_PROFILE_OWNER
                    : LEARN_MORE_URL_DEVICE_OWNER;
            linkText.setOnClickListener(new OnClickListener() {
                @Override
                public void onClick(View v) {
                    getActivity().startActivity(WebActivity.createIntent(getActivity(), url,
                            LEARN_MORE_ALLOWED_BASE_URL));
                }
            });
        }
    }

    @Override
    public void onCancel(DialogInterface dialog) {
        ((ConsentCallback) getActivity()).onDialogCancel();
    }

    public interface ConsentCallback {
        public abstract void onDialogConsent();
        public abstract void onDialogCancel();
    }
}
