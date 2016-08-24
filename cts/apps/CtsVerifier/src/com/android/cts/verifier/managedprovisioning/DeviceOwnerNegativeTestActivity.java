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

import android.app.Activity;
import android.app.admin.DevicePolicyManager;
import android.content.ComponentName;
import android.content.Intent;
import android.database.DataSetObserver;
import android.os.Bundle;

import com.android.cts.verifier.ArrayTestListAdapter;
import com.android.cts.verifier.IntentDrivenTestActivity;
import com.android.cts.verifier.IntentDrivenTestActivity.ButtonInfo;
import com.android.cts.verifier.IntentDrivenTestActivity.TestInfo;
import com.android.cts.verifier.PassFailButtons;
import com.android.cts.verifier.R;
import com.android.cts.verifier.TestListAdapter.TestListItem;

/**
 * Activity that lists all device owner negative tests.
 */
public class DeviceOwnerNegativeTestActivity extends PassFailButtons.TestListActivity {

    private static final String DEVICE_OWNER_NEGATIVE_TEST = "DEVICE_OWNER_PROVISIONING_NEGATIVE";
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.pass_fail_list);
        setInfoResources(R.string.device_owner_provisioning_tests,
                R.string.device_owner_provisioning_tests_info, 0);
        setPassFailButtonClickListeners();

        TestInfo deviceOwnerNegativeTestInfo = new TestInfo(
                DEVICE_OWNER_NEGATIVE_TEST,
                R.string.device_owner_negative_test,
                R.string.device_owner_negative_test_info,
                new ButtonInfo(
                        R.string.start_device_owner_provisioning_button,
                        new Intent(this, TrampolineActivity.class)));

        final ArrayTestListAdapter adapter = new ArrayTestListAdapter(this);
        adapter.add(TestListItem.newCategory(this, R.string.device_owner_provisioning_category));

        Intent startTestIntent = new Intent(this, IntentDrivenTestActivity.class)
                    .putExtra(IntentDrivenTestActivity.EXTRA_ID,
                            deviceOwnerNegativeTestInfo.getTestId())
                    .putExtra(IntentDrivenTestActivity.EXTRA_TITLE,
                            deviceOwnerNegativeTestInfo.getTitle())
                    .putExtra(IntentDrivenTestActivity.EXTRA_INFO,
                            deviceOwnerNegativeTestInfo.getInfoText())
                    .putExtra(IntentDrivenTestActivity.EXTRA_BUTTONS,
                            deviceOwnerNegativeTestInfo.getButtons());

        adapter.add(TestListItem.newTest(this, deviceOwnerNegativeTestInfo.getTitle(),
                deviceOwnerNegativeTestInfo.getTestId(), startTestIntent, null));

        adapter.registerDataSetObserver(new DataSetObserver() {
            @Override
            public void onChanged() {
                updatePassButton();
            }
        });

        setTestListAdapter(adapter);
    }

    /**
     * This is needed because IntentDrivenTestActivity fires the intent by startActivity when
     * a button is clicked, but ACTION_PROVISION_MANAGED_DEVICE requires to be fired by
     * startActivityForResult.
     */
    public static class TrampolineActivity extends Activity {
        @Override
        protected void onCreate(Bundle savedInstanceState) {
            super.onCreate(savedInstanceState);
            Intent provisionDeviceIntent = new Intent(
                    DevicePolicyManager.ACTION_PROVISION_MANAGED_DEVICE);
            provisionDeviceIntent.putExtra(
                    DevicePolicyManager.EXTRA_PROVISIONING_DEVICE_ADMIN_COMPONENT_NAME,
                    new ComponentName(this, DeviceAdminTestReceiver.class.getName()));
            startActivityForResult(provisionDeviceIntent, 0);
        }

        @Override
        protected void onActivityResult(int requestCode, int resultCode, Intent data) {
            super.onActivityResult(requestCode, resultCode, data);
            finish();
        }
    }
}

