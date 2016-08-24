/*
 * Copyright (C) 2014 The Android Open Source Project
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

package com.android.cts.verifier.camera.its;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.res.Configuration;
import android.hardware.camera2.CameraAccessException;
import android.hardware.camera2.CameraCharacteristics;
import android.hardware.camera2.CameraManager;
import android.os.Bundle;
import android.util.Log;
import android.view.WindowManager;
import android.widget.Toast;

import java.util.ArrayList;
import java.util.HashSet;
import java.util.HashMap;
import java.util.Arrays;
import java.util.List;
import java.io.BufferedReader;
import java.io.FileReader;
import java.io.FileNotFoundException;
import java.io.IOException;

import com.android.compatibility.common.util.ResultType;
import com.android.compatibility.common.util.ResultUnit;
import com.android.cts.verifier.PassFailButtons;
import com.android.cts.verifier.R;


/**
 * Test for Camera features that require that the camera be aimed at a specific test scene.
 * This test activity requires a USB connection to a computer, and a corresponding host-side run of
 * the python scripts found in the CameraITS directory.
 */
public class ItsTestActivity extends PassFailButtons.Activity {
    private static final String TAG = "ItsTestActivity";
    private static final String EXTRA_CAMERA_ID = "camera.its.extra.CAMERA_ID";
    private static final String EXTRA_SUCCESS = "camera.its.extra.SUCCESS";
    private static final String EXTRA_SUMMARY = "camera.its.extra.SUMMARY";
    private static final String ACTION_ITS_RESULT =
            "com.android.cts.verifier.camera.its.ACTION_ITS_RESULT";

    class SuccessReceiver extends BroadcastReceiver {
        @Override
        public void onReceive(Context context, Intent intent) {
            Log.i(TAG, "Received result for Camera ITS tests");
            if (ACTION_ITS_RESULT.equals(intent.getAction())) {
                String cameraId = intent.getStringExtra(EXTRA_CAMERA_ID);
                String result = intent.getStringExtra(EXTRA_SUCCESS);
                String summaryPath = intent.getStringExtra(EXTRA_SUMMARY);
                if (!mNonLegacyCameraIds.contains(cameraId)) {
                    Log.e(TAG, "Unknown camera id " + cameraId + " reported to ITS");
                    return;
                }

                Log.i(TAG, "ITS summary path is: " + summaryPath);
                mSummaryMap.put(cameraId, summaryPath);
                // Create summary report
                if (mSummaryMap.keySet().containsAll(mNonLegacyCameraIds)) {
                    StringBuilder summary = new StringBuilder();
                    for (String id : mNonLegacyCameraIds) {
                        String path = mSummaryMap.get(id);
                        appendFileContentToSummary(summary, path);
                    }
                    ItsTestActivity.this.getReportLog().setSummary(
                            summary.toString(), 1.0, ResultType.NEUTRAL, ResultUnit.NONE);
                }
                boolean pass = result.equals("True");
                if(pass) {
                    Log.i(TAG, "Received Camera " + cameraId + " ITS SUCCESS from host.");
                    mITSPassedCameraIds.add(cameraId);
                    if (mNonLegacyCameraIds != null && mNonLegacyCameraIds.size() != 0 &&
                            mITSPassedCameraIds.containsAll(mNonLegacyCameraIds)) {
                        ItsTestActivity.this.showToast(R.string.its_test_passed);
                        ItsTestActivity.this.getPassButton().setEnabled(true);
                    }
                } else {
                    Log.i(TAG, "Received Camera " + cameraId + " ITS FAILURE from host.");
                    ItsTestActivity.this.showToast(R.string.its_test_failed);
                }
            }
        }

        private void appendFileContentToSummary(StringBuilder summary, String path) {
            BufferedReader reader = null;
            try {
                reader = new BufferedReader(new FileReader(path));
                String line = null;
                do {
                    line = reader.readLine();
                    if (line != null) {
                        summary.append(line);
                    }
                } while (line != null);
            } catch (FileNotFoundException e) {
                Log.e(TAG, "Cannot find ITS summary file at " + path);
                summary.append("Cannot find ITS summary file at " + path);
            } catch (IOException e) {
                Log.e(TAG, "IO exception when trying to read " + path);
                summary.append("IO exception when trying to read " + path);
            } finally {
                if (reader != null) {
                    try {
                        reader.close();
                    } catch (IOException e) {
                    }
                }
            }
        }
    }

    private final SuccessReceiver mSuccessReceiver = new SuccessReceiver();
    private final HashSet<String> mITSPassedCameraIds = new HashSet<>();
    // map camera id to ITS summary report path
    private final HashMap<String, String> mSummaryMap = new HashMap<>();
    ArrayList<String> mNonLegacyCameraIds = null;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.its_main);
        setInfoResources(R.string.camera_its_test, R.string.camera_its_test_info, -1);
        setPassFailButtonClickListeners();

        getWindow().addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);

        // Hide the test if all camera devices are legacy
        CameraManager manager = (CameraManager) this.getSystemService(Context.CAMERA_SERVICE);
        try {
            String[] cameraIds = manager.getCameraIdList();
            mNonLegacyCameraIds = new ArrayList<String>();
            boolean allCamerasAreLegacy = true;
            for (String id : cameraIds) {
                CameraCharacteristics characteristics = manager.getCameraCharacteristics(id);
                if (characteristics.get(CameraCharacteristics.INFO_SUPPORTED_HARDWARE_LEVEL)
                        != CameraCharacteristics.INFO_SUPPORTED_HARDWARE_LEVEL_LEGACY) {
                    mNonLegacyCameraIds.add(id);
                    allCamerasAreLegacy = false;
                }
            }
            if (allCamerasAreLegacy) {
                showToast(R.string.all_legacy_devices);
                ItsTestActivity.this.getReportLog().setSummary(
                        "PASS: all cameras on this device are LEGACY"
                        , 1.0, ResultType.NEUTRAL, ResultUnit.NONE);
                setTestResultAndFinish(true);
            }
        } catch (CameraAccessException e) {
            Toast.makeText(ItsTestActivity.this,
                    "Received error from camera service while checking device capabilities: "
                            + e, Toast.LENGTH_SHORT).show();
        }
        getPassButton().setEnabled(false);
    }

    @Override
    protected void onResume() {
        super.onResume();
        CameraManager manager = (CameraManager) this.getSystemService(Context.CAMERA_SERVICE);
        if (manager == null) {
            showToast(R.string.no_camera_manager);
        } else {
            Log.d(TAG, "register ITS result receiver");
            IntentFilter filter = new IntentFilter(ACTION_ITS_RESULT);
            registerReceiver(mSuccessReceiver, filter);
        }
    }

    @Override
    protected void onPause() {
        super.onPause();
        Log.d(TAG, "unregister ITS result receiver");
        unregisterReceiver(mSuccessReceiver);
    }

    @Override
    public void onConfigurationChanged(Configuration newConfig) {
        super.onConfigurationChanged(newConfig);
        setContentView(R.layout.its_main);
        setInfoResources(R.string.camera_its_test, R.string.camera_its_test_info, -1);
        setPassFailButtonClickListeners();
    }

    private void showToast(int messageId) {
        Toast.makeText(ItsTestActivity.this, messageId, Toast.LENGTH_SHORT).show();
    }

}
