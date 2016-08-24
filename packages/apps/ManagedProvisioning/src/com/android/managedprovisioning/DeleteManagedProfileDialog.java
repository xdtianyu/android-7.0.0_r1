package com.android.managedprovisioning;
/*
 * Copyright 2015, The Android Open Source Project
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

import android.annotation.Nullable;
import android.app.Dialog;
import android.app.DialogFragment;
import android.content.ComponentName;
import android.content.DialogInterface;
import android.graphics.drawable.Drawable;
import android.os.Bundle;
import android.view.View;
import android.view.View.OnClickListener;
import android.widget.Button;
import android.widget.ImageView;
import android.widget.TextView;

import com.android.managedprovisioning.common.MdmPackageInfo;
import com.android.managedprovisioning.common.Utils;
import com.android.setupwizardlib.util.SystemBarHelper;

/**
 * Displays information about an existing managed profile and asks the user if it should be deleted.
 *
 * <p>Expects parent component to implement {@link DeleteManagedProfileCallback} for user-response
 * handling.
 */
public class DeleteManagedProfileDialog extends DialogFragment {
    private static final String KEY_USER_PROFILE_CALLBACK_ID = "user_profile_callback_id";
    private static final String KEY_MDM_PACKAGE_NAME = "mdm_package_name";
    private static final String KEY_PROFILE_OWNER_DOMAIN = "profile_owner_domain";

    private final Utils mUtils = new Utils();

    /**
     * @param managedProfileUserId user-id for the managed profile which will be passed back to the
     *     parent component in the {@link DeleteManagedProfileCallback#onRemoveProfileApproval(int)}
     *     call
     * @param mdmPackagename package name of the MDM application for the current managed profile, or
     *     null if the managed profile has no profile owner associated.
     * @param profileOwnerDomain domain name of the organization which owns the managed profile, or
     *     null if not known
     * @return initialized dialog
     */
    public static DeleteManagedProfileDialog newInstance(
            int managedProfileUserId, @Nullable ComponentName mdmPackagename,
            @Nullable String profileOwnerDomain) {
        Bundle args = new Bundle();
        args.putInt(KEY_USER_PROFILE_CALLBACK_ID, managedProfileUserId);
        // The device could be in a inconsistent state where it has a managed profile but no
        // associated profile owner package, for example after an unexpected reboot in the middle
        // of provisioning.
        if (mdmPackagename != null) {
            args.putString(KEY_MDM_PACKAGE_NAME, mdmPackagename.getPackageName());
        }
        args.putString(KEY_PROFILE_OWNER_DOMAIN, profileOwnerDomain);

        DeleteManagedProfileDialog dialog = new DeleteManagedProfileDialog();
        dialog.setArguments(args);
        return dialog;
    }

    @Override
    public Dialog onCreateDialog(Bundle savedInstanceState) {
        if (!(getActivity() instanceof DeleteManagedProfileCallback)) {
            throw new IllegalStateException("Calling activity must implement " +
                    "DeleteManagedProfileCallback, found: " + getActivity().getLocalClassName());
        }

        Bundle arguments = getArguments();
        final int callbackUserProfileId = arguments.getInt(KEY_USER_PROFILE_CALLBACK_ID);
        String mdmPackageName = arguments.getString(KEY_MDM_PACKAGE_NAME);

        String appLabel;
        Drawable appIcon;
        MdmPackageInfo mdmPackageInfo = null;
        if (mdmPackageName != null) {
            mdmPackageInfo = MdmPackageInfo.createFromPackageName(getActivity(), mdmPackageName);
        }
        if (mdmPackageInfo != null) {
            appLabel = mdmPackageInfo.appLabel;
            appIcon = mdmPackageInfo.packageIcon;
        } else {
            appLabel= getResources().getString(android.R.string.unknownName);
            appIcon = getActivity().getPackageManager().getDefaultActivityIcon();
        }

        final Dialog dialog = new Dialog(getActivity(), R.style.ManagedProvisioningDialogTheme);
        dialog.setTitle(R.string.delete_profile_title);
        dialog.setContentView(R.layout.delete_managed_profile_dialog);
        dialog.setCanceledOnTouchOutside(false);
        if (!mUtils.isUserSetupCompleted(getActivity())) {
            SystemBarHelper.hideSystemBars(dialog);
        }

        ImageView imageView = (ImageView) dialog.findViewById(
                R.id.device_manager_icon_view);
        imageView.setImageDrawable(appIcon);
        imageView.setContentDescription(
                    getResources().getString(R.string.mdm_icon_label, appLabel));

        TextView deviceManagerName = (TextView) dialog.findViewById(
                R.id.device_manager_name);
        deviceManagerName.setText(appLabel);

        Button positiveButton = (Button) dialog.findViewById(
                R.id.delete_managed_profile_positive_button);
        positiveButton.setOnClickListener(new OnClickListener() {
                @Override
                public void onClick(View v) {
                    dialog.dismiss();
                    ((DeleteManagedProfileCallback) getActivity())
                            .onRemoveProfileApproval(callbackUserProfileId);
                }
            });

        Button negativeButton = (Button) dialog.findViewById(
                R.id.delete_managed_profile_negative_button);
        negativeButton.setOnClickListener(new OnClickListener() {
                @Override
                public void onClick(View v) {
                    dialog.dismiss();
                    ((DeleteManagedProfileCallback) getActivity()).onRemoveProfileCancel();
                }
            });

        return dialog;
    }

    @Override
    public void onCancel(DialogInterface dialog) {
        dialog.dismiss();
        ((DeleteManagedProfileCallback) getActivity()).onRemoveProfileCancel();
    }

    /**
     * Callback interface for outcome of {@link DeleteManagedProfileDialog} presentation.
     */
    public interface DeleteManagedProfileCallback {

        /**
         * Invoked if the user hits the positive response (perform removal) button.
         *
         * @param managedProfileUserId user-id of the managed-profile that the dialog was presented
         *                             for
         */
        public abstract void onRemoveProfileApproval(int managedProfileUserId);

        /**
         * Invoked if the user hits the negative response (DO NOT perform removal) button, or the
         * dialog was otherwise dismissed.
         */
        public abstract void onRemoveProfileCancel();
    }
}
