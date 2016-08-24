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
package com.android.cts.verifier.managedprovisioning;

import android.app.admin.DevicePolicyManager;
import android.app.Activity;
import android.content.ComponentName;
import android.content.ContentResolver;
import android.content.Intent;
import android.graphics.Color;
import android.net.Uri;
import android.os.Bundle;

import com.android.cts.verifier.ArrayTestListAdapter;
import com.android.cts.verifier.IntentDrivenTestActivity.ButtonInfo;
import com.android.cts.verifier.PassFailButtons;
import com.android.cts.verifier.R;

public class ByodProvisioningTestActivity extends PassFailButtons.TestListActivity {
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.pass_fail_list);
        setPassFailButtonClickListeners();

        final ArrayTestListAdapter adapter = new ArrayTestListAdapter(this);

        Intent colorIntent = new Intent(this, ProvisioningStartingActivity.class)
                .putExtra(DevicePolicyManager.EXTRA_PROVISIONING_MAIN_COLOR, Color.GREEN);
        adapter.add(Utils.createInteractiveTestItem(this, "BYOD_CustomColor",
                        R.string.provisioning_tests_byod_custom_color,
                        R.string.provisioning_tests_byod_custom_color_info,
                        new ButtonInfo(R.string.go_button_text, colorIntent)));
        Uri logoUri = Uri.parse(ContentResolver.SCHEME_ANDROID_RESOURCE + "://" + getPackageName()
                + "/" + R.drawable.icon);
        Intent imageIntent = new Intent(this, ProvisioningStartingActivity.class)
                .putExtra(DevicePolicyManager.EXTRA_PROVISIONING_LOGO_URI, logoUri);
        adapter.add(Utils.createInteractiveTestItem(this, "BYOD_CustomImage",
                        R.string.provisioning_tests_byod_custom_image,
                        R.string.provisioning_tests_byod_custom_image_info,
                        new ButtonInfo(R.string.go_button_text, imageIntent)));

        setTestListAdapter(adapter);
    }

    @Override
    public void finish() {
        // Pass and fail buttons are known to call finish() when clicked, and this is when we want
        // to clean up the provisioned profile in case the user has added one.
        Utils.requestDeleteManagedProfile(this);
        super.finish();
    }

    // We need this activity because the provisioning needs to be started with
    // startActivityForResult
    public static class ProvisioningStartingActivity extends Activity {
        @Override
        protected void onCreate(Bundle savedInstanceState) {
            super.onCreate(savedInstanceState);
            Intent provisioningIntent = new Intent(
                    DevicePolicyManager.ACTION_PROVISION_MANAGED_PROFILE);
            // forward all the extras we received.
            provisioningIntent.putExtras(getIntent().getExtras());
            provisioningIntent.putExtra(
                    DevicePolicyManager.EXTRA_PROVISIONING_DEVICE_ADMIN_COMPONENT_NAME,
                    new ComponentName(this, DeviceAdminTestReceiver.class.getName()));
            startActivityForResult(provisioningIntent, 0);
            finish();
        }
    }
}
