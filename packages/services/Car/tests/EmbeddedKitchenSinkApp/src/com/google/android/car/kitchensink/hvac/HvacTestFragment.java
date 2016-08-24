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

package com.google.android.car.kitchensink.hvac;

import static java.lang.Integer.toHexString;

import android.car.CarNotConnectedException;
import android.car.hardware.CarPropertyConfig;
import android.car.hardware.CarPropertyValue;
import android.car.hardware.hvac.CarHvacManager;
import android.car.hardware.hvac.CarHvacManager.HvacPropertyId;
import android.os.Bundle;
import android.support.v4.app.Fragment;
import android.util.Log;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.Button;
import android.widget.RadioButton;
import android.widget.RadioGroup;
import android.widget.TextView;
import android.widget.ToggleButton;

import com.google.android.car.kitchensink.R;

import com.android.car.vehiclenetwork.VehicleNetworkConsts.VehicleHvacFanDirection;
import com.android.car.vehiclenetwork.VehicleNetworkConsts.VehicleWindow;
import com.android.car.vehiclenetwork.VehicleNetworkConsts.VehicleZone;

import java.util.ArrayList;
import java.util.List;

public class HvacTestFragment extends Fragment {
    private final boolean DBG = true;
    private final String TAG = "HvacTestFragment";
    private RadioButton mRbFanPositionFace;
    private RadioButton mRbFanPositionFloor;
    private RadioButton mRbFanPositionFaceAndFloor;
    private ToggleButton mTbAc;
    private ToggleButton mTbDefrostFront;
    private ToggleButton mTbDefrostRear;
    private TextView mTvFanSpeed;
    private TextView mTvDTemp;
    private TextView mTvPTemp;
    private int mCurFanSpeed = 1;
    private float mCurDTemp = 23;
    private float mCurPTemp = 23;
    private CarHvacManager mCarHvacManager;
    private int mZoneForAcOn;
    private int mZoneForSetTempD;
    private int mZoneForSetTempP;
    private int mZoneForFanSpeed;
    private int mZoneForFanPosition;

    private final CarHvacManager.CarHvacEventListener mHvacListener =
            new CarHvacManager.CarHvacEventListener () {
                @Override
                public void onChangeEvent(final CarPropertyValue value) {
                    int zones = value.getAreaId();
                    switch(value.getPropertyId()) {
                        case HvacPropertyId.ZONED_AC_ON:
                            mTbAc.setChecked((boolean)value.getValue());
                            break;
                        case HvacPropertyId.ZONED_FAN_POSITION:
                            switch((int)value.getValue()) {
                                case VehicleHvacFanDirection.VEHICLE_HVAC_FAN_DIRECTION_FACE:
                                    mRbFanPositionFace.setChecked(true);
                                    break;
                                case VehicleHvacFanDirection.VEHICLE_HVAC_FAN_DIRECTION_FLOOR:
                                    mRbFanPositionFloor.setChecked(true);
                                    break;
                                case VehicleHvacFanDirection.
                                        VEHICLE_HVAC_FAN_DIRECTION_FACE_AND_FLOOR:
                                    mRbFanPositionFaceAndFloor.setChecked(true);
                                    break;
                                default:
                                    if (DBG) {
                                        Log.e(TAG, "Unknown fan position: " + value.getValue());
                                    }
                                    break;
                            }
                            break;
                        case HvacPropertyId.ZONED_FAN_SPEED_SETPOINT:
                            if ((zones & mZoneForFanSpeed) != 0) {
                                mCurFanSpeed = (int)value.getValue();
                                mTvFanSpeed.setText(String.valueOf(mCurFanSpeed));
                            }
                            break;
                        case HvacPropertyId.ZONED_TEMP_SETPOINT:
                            if ((zones & mZoneForSetTempD) != 0) {
                                mCurDTemp = (float)value.getValue();
                                mTvDTemp.setText(String.valueOf(mCurDTemp));
                            }
                            if ((zones & mZoneForSetTempP) != 0) {
                                mCurPTemp = (float)value.getValue();
                                mTvPTemp.setText(String.valueOf(mCurPTemp));
                            }
                            break;
                        case HvacPropertyId.WINDOW_DEFROSTER_ON:
                            if((zones & VehicleWindow.VEHICLE_WINDOW_FRONT_WINDSHIELD) ==
                                    VehicleWindow.VEHICLE_WINDOW_FRONT_WINDSHIELD) {
                                mTbDefrostFront.setChecked((boolean)value.getValue());
                            }
                            if((zones & VehicleWindow.VEHICLE_WINDOW_REAR_WINDSHIELD) ==
                                    VehicleWindow.VEHICLE_WINDOW_REAR_WINDSHIELD) {
                                mTbDefrostRear.setChecked((boolean)value.getValue());
                            }
                            break;
                        default:
                            Log.d(TAG, "onChangeEvent(): unknown property id = " + value
                                    .getPropertyId());
                    }
                }

                @Override
                public void onErrorEvent(final int propertyId, final int zone) {
                    Log.w(TAG, "Error:  propertyId=0x" + toHexString(propertyId)
                            + ", zone=0x" + toHexString(zone));
                }
            };

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        try {
            mCarHvacManager.registerListener(mHvacListener);
        } catch (CarNotConnectedException e) {
            Log.e(TAG, "Car is not connected!");
        }
    }

    @Override
    public void onDestroy() {
        super.onDestroy();
        try {
            mCarHvacManager.unregisterListener(mHvacListener);
        } catch (CarNotConnectedException e) {
            Log.e(TAG, "Failed to unregister listener", e);
        }
    }

    @Override
    public View onCreateView(LayoutInflater inflater, ViewGroup container, Bundle savedInstance) {
        View v = inflater.inflate(R.layout.hvac_test, container, false);

        List<CarPropertyConfig> props;
        try {
            props = mCarHvacManager.getPropertyList();
        } catch (CarNotConnectedException e) {
            Log.e(TAG, "Failed to get list of properties", e);
            props = new ArrayList<>();
        }

        for(CarPropertyConfig prop : props) {
            int propId = prop.getPropertyId();

            if(DBG) {
                Log.d(TAG, prop.toString());
            }

            switch(propId) {
                case HvacPropertyId.ZONED_AC_ON:
                    configureAcOn(v, prop);
                    break;
                case HvacPropertyId.ZONED_FAN_POSITION:
                    configureFanPosition(v, prop);
                    break;
                case HvacPropertyId.ZONED_FAN_SPEED_SETPOINT:
                    configureFanSpeed(v, prop);
                    break;
                case HvacPropertyId.ZONED_TEMP_SETPOINT:
                    configureTempSetpoint(v, prop);
                    break;
                case HvacPropertyId.WINDOW_DEFROSTER_ON:
                    configureDefrosterOn(v, prop);
                    break;
                default:
                    Log.w(TAG, "propertyId " + propId + " is not handled");
                    break;
            }
        }

        mTvFanSpeed = (TextView) v.findViewById(R.id.tvFanSpeed);
        mTvFanSpeed.setText(String.valueOf(mCurFanSpeed));
        mTvDTemp = (TextView) v.findViewById(R.id.tvDTemp);
        mTvDTemp.setText(String.valueOf(mCurDTemp));
        mTvPTemp = (TextView) v.findViewById(R.id.tvPTemp);
        mTvPTemp.setText(String.valueOf(mCurPTemp));

        if(DBG) {
            Log.d(TAG, "Starting HvacTestFragment");
        }

        return v;
    }

    public void setHvacManager(CarHvacManager hvacManager) {
        Log.d(TAG, "setHvacManager()");
        mCarHvacManager = hvacManager;
    }

    private void configureAcOn(View v, CarPropertyConfig prop) {
        mZoneForAcOn = prop.getFirstAndOnlyAreaId();
        mTbAc = (ToggleButton)v.findViewById(R.id.tbAc);
        mTbAc.setEnabled(true);

        mTbAc.setOnClickListener(view -> {
            // TODO handle zone properly
            try {
                mCarHvacManager.setBooleanProperty(HvacPropertyId.ZONED_AC_ON, mZoneForAcOn,
                        mTbAc.isChecked());
            } catch (CarNotConnectedException e) {
                Log.e(TAG, "Failed to set HVAC boolean property", e);
            }
        });
    }

    private void configureFanPosition(View v, CarPropertyConfig prop) {
        mZoneForFanPosition = prop.getFirstAndOnlyAreaId();
        RadioGroup rg = (RadioGroup)v.findViewById(R.id.rgFanPosition);
        rg.setOnCheckedChangeListener((group, checkedId) -> {
            int position;
            switch(checkedId) {
                case R.id.rbPositionFace:
                    position = VehicleHvacFanDirection.VEHICLE_HVAC_FAN_DIRECTION_FACE;
                    break;
                case R.id.rbPositionFloor:
                    position = VehicleHvacFanDirection.VEHICLE_HVAC_FAN_DIRECTION_FLOOR;
                    break;
                case R.id.rbPositionFaceAndFloor:
                    position = VehicleHvacFanDirection.VEHICLE_HVAC_FAN_DIRECTION_FACE_AND_FLOOR;
                    break;
                default:
                    throw new IllegalStateException("Unexpected fan position: " + checkedId);
            }
            try {
                mCarHvacManager.setIntProperty(HvacPropertyId.ZONED_FAN_POSITION,
                        mZoneForFanPosition,
                        position);
            } catch (CarNotConnectedException e) {
                Log.e(TAG, "Failed to set HVAC integer property", e);
            }
        });

        mRbFanPositionFace = (RadioButton)v.findViewById(R.id.rbPositionFace);
        mRbFanPositionFace.setClickable(true);
        mRbFanPositionFloor = (RadioButton)v.findViewById(R.id.rbPositionFloor);
        mRbFanPositionFaceAndFloor = (RadioButton)v.findViewById(R.id.rbPositionFaceAndFloor);
        mRbFanPositionFaceAndFloor.setClickable(true);
        mRbFanPositionFloor.setClickable(true);
    }

    private void configureFanSpeed(View v, CarPropertyConfig prop) {
        mZoneForFanSpeed = prop.getFirstAndOnlyAreaId();
        try {
            mCurFanSpeed = mCarHvacManager.getIntProperty(
                    HvacPropertyId.ZONED_FAN_SPEED_SETPOINT,
                    mZoneForFanSpeed);
        } catch (CarNotConnectedException e) {
            Log.e(TAG, "Failed to get HVAC int property", e);
        }

        Button btnFanSpeedUp = (Button) v.findViewById(R.id.btnFanSpeedUp);
        btnFanSpeedUp.setEnabled(true);
        btnFanSpeedUp.setOnClickListener(view -> {
            if (mCurFanSpeed < 7) {
                mCurFanSpeed++;
                mTvFanSpeed.setText(String.valueOf(mCurFanSpeed));
                try {
                    mCarHvacManager.setIntProperty(HvacPropertyId.ZONED_FAN_SPEED_SETPOINT,
                            mZoneForFanSpeed, mCurFanSpeed);
                } catch (CarNotConnectedException e) {
                    Log.e(TAG, "Failed to set HVAC int property", e);
                }
            }
        });

        Button btnFanSpeedDn = (Button) v.findViewById(R.id.btnFanSpeedDn);
        btnFanSpeedDn.setEnabled(true);
        btnFanSpeedDn.setOnClickListener(view -> {
            if (mCurFanSpeed > 1) {
                mCurFanSpeed--;
                mTvFanSpeed.setText(String.valueOf(mCurFanSpeed));
                try {
                    mCarHvacManager.setIntProperty(HvacPropertyId.ZONED_FAN_SPEED_SETPOINT,
                            mZoneForFanSpeed, mCurFanSpeed);
                } catch (CarNotConnectedException e) {
                    Log.e(TAG, "Failed to set HVAC fan speed property", e);
                }
            }
        });
    }

    private void configureTempSetpoint(View v, CarPropertyConfig prop) {
        mZoneForSetTempD = 0;
        if (prop.hasArea(VehicleZone.VEHICLE_ZONE_ROW_1_LEFT)) {
            mZoneForSetTempD = VehicleZone.VEHICLE_ZONE_ROW_1_LEFT;
        }
        mZoneForSetTempP = 0;
        if (prop.hasArea(VehicleZone.VEHICLE_ZONE_ROW_1_RIGHT)) {
            mZoneForSetTempP = VehicleZone.VEHICLE_ZONE_ROW_1_RIGHT;
        }
        int[] areas = prop.getAreaIds();
        if (mZoneForSetTempD == 0 && areas.length > 1) {
            mZoneForSetTempD = areas[0];
        }
        if (mZoneForSetTempP == 0 && areas.length > 2) {
            mZoneForSetTempP = areas[1];
        }
        Button btnDTempUp = (Button) v.findViewById(R.id.btnDTempUp);
        if (mZoneForSetTempD != 0) {
            try {
                mCurDTemp = mCarHvacManager.getFloatProperty(
                        HvacPropertyId.ZONED_TEMP_SETPOINT,
                        mZoneForSetTempD);
            } catch (CarNotConnectedException e) {
                Log.e(TAG, "Failed to get HVAC zoned temp property", e);
            }
            btnDTempUp.setEnabled(true);
            btnDTempUp.setOnClickListener(view -> {
                if(mCurDTemp < 29.5) {
                    mCurDTemp += 0.5;
                    mTvDTemp.setText(String.valueOf(mCurDTemp));
                    try {
                        mCarHvacManager.setFloatProperty(
                                HvacPropertyId.ZONED_TEMP_SETPOINT,
                                mZoneForSetTempD, mCurDTemp);
                    } catch (CarNotConnectedException e) {
                        Log.e(TAG, "Failed to set HVAC zoned temp property", e);
                    }
                }
            });

            Button btnDTempDn = (Button) v.findViewById(R.id.btnDTempDn);
            btnDTempDn.setEnabled(true);
            btnDTempDn.setOnClickListener(view -> {
                if(mCurDTemp > 15.5) {
                    mCurDTemp -= 0.5;
                    mTvDTemp.setText(String.valueOf(mCurDTemp));
                    try {
                        mCarHvacManager.setFloatProperty(
                                HvacPropertyId.ZONED_TEMP_SETPOINT,
                                mZoneForSetTempD, mCurDTemp);
                    } catch (CarNotConnectedException e) {
                        Log.e(TAG, "Failed to set HVAC zoned temp property", e);
                    }
                }
            });
        } else {
            btnDTempUp.setEnabled(false);
        }

        Button btnPTempUp = (Button) v.findViewById(R.id.btnPTempUp);
        if (mZoneForSetTempP !=0 ) {
            try {
                mCurPTemp = mCarHvacManager.getFloatProperty(
                        HvacPropertyId.ZONED_TEMP_SETPOINT,
                        mZoneForSetTempP);
            } catch (CarNotConnectedException e) {
                Log.e(TAG, "Failed to get HVAC zoned temp property", e);
            }
            btnPTempUp.setEnabled(true);
            btnPTempUp.setOnClickListener(view -> {
                if (mCurPTemp < 29.5) {
                    mCurPTemp += 0.5;
                    mTvPTemp.setText(String.valueOf(mCurPTemp));
                    try {
                        mCarHvacManager.setFloatProperty(
                                HvacPropertyId.ZONED_TEMP_SETPOINT,
                                mZoneForSetTempP, mCurPTemp);
                    } catch (CarNotConnectedException e) {
                        Log.e(TAG, "Failed to set HVAC zoned temp property", e);
                    }
                }
            });

            Button btnPTempDn = (Button) v.findViewById(R.id.btnPTempDn);
            btnPTempDn.setEnabled(true);
            btnPTempDn.setOnClickListener(view -> {
                if (mCurPTemp > 15.5) {
                    mCurPTemp -= 0.5;
                    mTvPTemp.setText(String.valueOf(mCurPTemp));
                    try {
                        mCarHvacManager.setFloatProperty(
                                HvacPropertyId.ZONED_TEMP_SETPOINT,
                                mZoneForSetTempP, mCurPTemp);
                    } catch (CarNotConnectedException e) {
                        Log.e(TAG, "Failed to set HVAC zoned temp property", e);
                    }
                }
            });
        } else {
            btnPTempUp.setEnabled(false);
        }
    }

    private void configureDefrosterOn(View v, CarPropertyConfig prop1) {
        if (prop1.hasArea(VehicleWindow.VEHICLE_WINDOW_FRONT_WINDSHIELD)) {
            mTbDefrostFront = (ToggleButton) v.findViewById(R.id.tbDefrostFront);
            mTbDefrostFront.setEnabled(true);
            mTbDefrostFront.setOnClickListener(view -> {
                try {
                    mCarHvacManager.setBooleanProperty(HvacPropertyId.WINDOW_DEFROSTER_ON,
                            VehicleWindow.VEHICLE_WINDOW_FRONT_WINDSHIELD,
                            mTbDefrostFront.isChecked());
                } catch (CarNotConnectedException e) {
                    Log.e(TAG, "Failed to set HVAC window defroster property", e);
                }
            });
        }
        if (prop1.hasArea(VehicleWindow.VEHICLE_WINDOW_REAR_WINDSHIELD)) {
            mTbDefrostRear = (ToggleButton) v.findViewById(R.id.tbDefrostRear);
            mTbDefrostRear.setEnabled(true);
            mTbDefrostRear.setOnClickListener(view -> {
                try {
                    mCarHvacManager.setBooleanProperty(HvacPropertyId.WINDOW_DEFROSTER_ON,
                            VehicleWindow.VEHICLE_WINDOW_REAR_WINDSHIELD,
                            mTbDefrostRear.isChecked());
                } catch (CarNotConnectedException e) {
                    Log.e(TAG, "Failed to set HVAC window defroster property", e);
                }
            });
        }
    }
}
