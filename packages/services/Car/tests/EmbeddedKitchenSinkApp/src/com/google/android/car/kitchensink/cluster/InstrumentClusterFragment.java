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
package com.google.android.car.kitchensink.cluster;

import android.app.AlertDialog;
import android.os.Bundle;
import android.support.annotation.Nullable;
import android.support.car.CarAppContextManager;
import android.support.car.CarAppContextManager.AppContextChangeListener;
import android.support.car.CarAppContextManager.AppContextOwnershipChangeListener;
import android.support.car.CarNotConnectedException;
import android.support.car.navigation.CarNavigationManager;
import android.support.v4.app.Fragment;
import android.util.Log;
import android.view.LayoutInflater;
import android.view.View;
import android.view.View.OnClickListener;
import android.view.ViewGroup;

import com.google.android.car.kitchensink.R;

/**
 * Contains functions to test instrument cluster API.
 */
public class InstrumentClusterFragment extends Fragment {
    private static final String TAG = InstrumentClusterFragment.class.getSimpleName();

    private CarNavigationManager mCarNavigationManager;
    private CarAppContextManager mCarAppContextManager;

    public void setCarNavigationManager(CarNavigationManager carNavigationManager) {
        mCarNavigationManager = carNavigationManager;
    }

    public void setCarAppContextManager(CarAppContextManager carAppContextManager) {
        mCarAppContextManager = carAppContextManager;
    }

    @Nullable
    @Override
    public View onCreateView(LayoutInflater inflater, @Nullable ViewGroup container,
            @Nullable Bundle savedInstanceState) {
        View view = inflater.inflate(R.layout.instrument_cluster, container);

        view.findViewById(R.id.cluster_start_button).setOnClickListener(new OnClickListener() {
            @Override
            public void onClick(View v) {
                initCluster();
            }
        });
        view.findViewById(R.id.cluster_turn_left_button).setOnClickListener(new OnClickListener() {
            @Override
            public void onClick(View v) {
                turnLeft();
            }
        });

        return super.onCreateView(inflater, container, savedInstanceState);
    }

    private void turnLeft() {
        try {
            mCarNavigationManager.sendNavigationTurnEvent(CarNavigationManager.TURN_TURN,
                    "Huff Ave", 90, -1, null, CarNavigationManager.TURN_SIDE_LEFT);
            mCarNavigationManager.sendNavigationTurnDistanceEvent(500, 10);
        } catch (CarNotConnectedException e) {
            e.printStackTrace();
        }
    }

    private void initCluster() {
        try {
            mCarAppContextManager.registerContextListener(new AppContextChangeListener() {
                @Override
                public void onAppContextChange(int activeContexts) {
                    Log.d(TAG, "onAppContextChange, activeContexts: " + activeContexts);
                }
            }, CarAppContextManager.APP_CONTEXT_NAVIGATION);
        } catch (CarNotConnectedException e) {
            Log.e(TAG, "Failed to register context listener", e);
        }

        try {
            mCarAppContextManager.setActiveContexts(new AppContextOwnershipChangeListener() {
                @Override
                public void onAppContextOwnershipLoss(int context) {
                    Log.w(TAG, "onAppContextOwnershipLoss, context: " + context);
                    new AlertDialog.Builder(getContext())
                            .setTitle(getContext().getApplicationInfo().name)
                            .setMessage(R.string.cluster_nav_app_context_loss)
                            .show();
                }
            }, CarAppContextManager.APP_CONTEXT_NAVIGATION);
        } catch (CarNotConnectedException e) {
            Log.e(TAG, "Failed to set active context", e);
        }

        try {
            boolean ownsContext =
                    mCarAppContextManager.isOwningContext(
                            CarAppContextManager.APP_CONTEXT_NAVIGATION);
            Log.d(TAG, "Owns APP_CONTEXT_NAVIGATION: " + ownsContext);
            if (!ownsContext) {
                throw new RuntimeException("Context was not acquired.");
            }
        } catch (CarNotConnectedException e) {
            Log.e(TAG, "Failed to get owned context", e);
        }

        try {
            mCarNavigationManager.sendNavigationStatus(CarNavigationManager.STATUS_ACTIVE);
        } catch (CarNotConnectedException e) {
            Log.e(TAG, "Failed to set navigation status", e);
        }
    }
}
