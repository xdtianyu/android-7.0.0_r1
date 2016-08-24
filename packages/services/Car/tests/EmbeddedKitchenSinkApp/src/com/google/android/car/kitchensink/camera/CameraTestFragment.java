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
 * limitations under the License.
 */

package com.google.android.car.kitchensink.camera;

import android.car.CarNotConnectedException;
import android.car.hardware.camera.CarCamera;
import android.car.hardware.camera.CarCameraManager;
import android.car.hardware.camera.CarCameraState;
import android.graphics.Rect;
import android.os.Bundle;
import android.support.v4.app.Fragment;
import android.util.Log;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.Button;
import android.widget.TextView;
import android.widget.ToggleButton;

import com.google.android.car.kitchensink.R;

import java.lang.Override;

public class CameraTestFragment extends Fragment {
    private final boolean DBG = true;
    private final String TAG = "CameraTestFragment";
    private TextView mTvCap;
    private TextView mTvRvcCrop;
    private TextView mTvRvcPos;
    private TextView mTvCameraState;
    private CarCameraManager mCarCameraManager;
    private CarCamera mRvcCamera;

    @Override
    public View onCreateView(LayoutInflater inflater, ViewGroup container, Bundle savedInstance) {
        View v = inflater.inflate(R.layout.camera_test, container, false);

        int[] cameraList = mCarCameraManager.getCameraList();
        for (int camera : cameraList) {
            if (camera == CarCameraManager.CAR_CAMERA_TYPE_RVC) {
                mRvcCamera = mCarCameraManager.openCamera(1);
                break;
            }
        }

        mTvCap = (TextView)v.findViewById(R.id.tvCap);
        mTvRvcCrop = (TextView)v.findViewById(R.id.tvRvcCrop);
        mTvRvcPos = (TextView)v.findViewById(R.id.tvRvcPos);
        mTvCameraState = (TextView)v.findViewById(R.id.tvCameraState);

        Button btn = (Button) v.findViewById(R.id.btnGetCap);
        btn.setOnClickListener(new View.OnClickListener() {
            public void onClick(View v) {
                try {
                    int cap = mRvcCamera.getCapabilities();
                    mTvCap.setText(String.valueOf(cap));
                } catch (CarNotConnectedException e) {
                    Log.e(TAG, "Failed to get camera capabilities", e);
                }
            }
        });

        btn = (Button) v.findViewById(R.id.btnGetRvcCrop);
        btn.setOnClickListener(new View.OnClickListener() {
            public void onClick(View v) {
                try {
                    Rect rect = mRvcCamera.getCameraCrop();
                    if(rect != null) {
                        mTvRvcCrop.setText(rect.toString());
                    } else {
                        mTvRvcCrop.setText("null");
                    }
                } catch (CarNotConnectedException e) {
                    Log.e(TAG, "Failed to get camera crop", e);
                }
            }
        });

        btn = (Button) v.findViewById(R.id.btnGetRvcPos);
        btn.setOnClickListener(new View.OnClickListener() {
            public void onClick(View v) {
                try {
                    Rect rect = mRvcCamera.getCameraPosition();
                    if(rect != null) {
                        mTvRvcPos.setText(String.valueOf(rect));
                    } else {
                        mTvRvcPos.setText("null");
                    }
                } catch (CarNotConnectedException e) {
                    Log.e(TAG, "Failed to get camere position", e);
                }
            }
        });

        btn = (Button) v.findViewById(R.id.btnGetCameraState);
        btn.setOnClickListener(new View.OnClickListener() {
            public void onClick(View v) {
                try {
                    CarCameraState state = mRvcCamera.getCameraState();
                    if(state != null) {
                        mTvCameraState.setText(state.toString());
                    } else {
                        mTvCameraState.setText("null");
                    }
                } catch (CarNotConnectedException e) {
                    Log.e(TAG, "Failed to get camere state", e);
                }
            }
        });

        btn = (Button) v.findViewById(R.id.btnSetRvcCrop);
        btn.setOnClickListener(new View.OnClickListener() {
            public void onClick(View v) {
                Rect rect = new Rect(160, 240, 560, 480);
                try {
                    mRvcCamera.setCameraCrop(rect);
                } catch (CarNotConnectedException e) {
                    Log.e(TAG, "Failed to set camera crop", e);
                }
            }
        });

        btn = (Button) v.findViewById(R.id.btnSetRvcCrop2);
        btn.setOnClickListener(new View.OnClickListener() {
            public void onClick(View v) {
                Rect rect = new Rect(0, 0, 720, 480);
                try {
                    mRvcCamera.setCameraCrop(rect);
                } catch (CarNotConnectedException e) {
                    Log.e(TAG, "Failed to set camera crop", e);
                }
            }
        });

        btn = (Button) v.findViewById(R.id.btnSetRvcPos);
        btn.setOnClickListener(new View.OnClickListener() {
            public void onClick(View v) {
                Rect rect = new Rect(300, 0, 800, 480);
                try {
                    mRvcCamera.setCameraPosition(rect);
                } catch (CarNotConnectedException e) {
                    Log.e(TAG, "Failed to set camera position", e);
                }
            }
        });

        btn = (Button) v.findViewById(R.id.btnSetRvcPos2);
        btn.setOnClickListener(new View.OnClickListener() {
            public void onClick(View v) {
                Rect rect = new Rect(500, 0, 800, 480);
                try {
                    mRvcCamera.setCameraPosition(rect);
                } catch (CarNotConnectedException e) {
                    Log.e(TAG, "Failed to set camera position", e);
                }
            }
        });

        btn = (Button) v.findViewById(R.id.btnSetRvcPos3);
        btn.setOnClickListener(new View.OnClickListener() {
            public void onClick(View v) {
                Rect rect = new Rect(300, 0, 500, 300);
                try {
                    mRvcCamera.setCameraPosition(rect);
                } catch (CarNotConnectedException e) {
                    Log.e(TAG, "Failed to set camera position", e);
                }
            }
        });

        final ToggleButton toggleBtn = (ToggleButton) v.findViewById(R.id.btnRvcState);
        toggleBtn.setOnClickListener(new View.OnClickListener() {
            public void onClick(View v) {
                CarCameraState state = new CarCameraState(false, toggleBtn.isChecked());
                try {
                    mRvcCamera.setCameraState(state);
                } catch (CarNotConnectedException e) {
                    Log.e(TAG, "Failed to set camera state", e);
                }
            }
        });

        if(DBG) {
            Log.d(TAG, "Starting CameraTestFragment");
        }
        return v;
    }

    public void setCameraManager(CarCameraManager cameraManager) {
        Log.d(TAG, "setCameraManager()");
        mCarCameraManager = cameraManager;
    }
}
