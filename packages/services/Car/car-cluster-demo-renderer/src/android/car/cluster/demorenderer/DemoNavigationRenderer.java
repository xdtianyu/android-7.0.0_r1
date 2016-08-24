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
package android.car.cluster.demorenderer;

import static android.car.navigation.CarNavigationManager.TURN_SIDE_LEFT;
import static android.car.navigation.CarNavigationManager.TURN_SIDE_RIGHT;
import static android.car.navigation.CarNavigationManager.TURN_TURN;

import android.car.cluster.renderer.NavigationRenderer;
import android.content.Context;
import android.graphics.Bitmap;
import android.util.Log;
import android.util.Pair;

import java.util.HashMap;
import java.util.Map;

/**
 * Demo implementation of {@link NavigationRenderer}.
 */
public class DemoNavigationRenderer extends NavigationRenderer {

    private static final String TAG = DemoNavigationRenderer.class.getSimpleName();

    private final DemoInstrumentClusterView mView;
    private final Context mContext;

    private final static Map<Pair<Integer, Integer>, Integer> sTurns;

    static {
        sTurns = new HashMap<>();
        sTurns.put(new Pair<>(TURN_TURN, TURN_SIDE_LEFT), R.string.turn_left);
        sTurns.put(new Pair<>(TURN_TURN, TURN_SIDE_RIGHT), R.string.turn_right);
        // TODO: add more localized strings here.
    }

    DemoNavigationRenderer(DemoInstrumentClusterView view) {
        mView = view;
        mContext = view.getContext();
    }

    @Override
    public void onStartNavigation() {
        mView.showNavigation();
    }

    @Override
    public void onStopNavigation() {
        mView.hideNavigation();
    }

    @Override
    public void onNextTurnChanged(int event, String road, int turnAngle, int turnNumber,
            final Bitmap image, int turnSide) {
        String localizedAction = getLocalizedNavigationAction(event, turnSide);
        final String localizedTitle = String.format(
                mContext.getString(R.string.nav_event_title_format), localizedAction, road);

        mView.setNextTurn(image, localizedTitle);
    }

    @Override
    public void onNextTurnDistanceChanged(final int distanceMeters, int timeSeconds) {
        mView.setNextTurnDistance(toHumanReadableDistance(distanceMeters));
    }

    private String getLocalizedNavigationAction(int event, int turnSide) {
        Pair<Integer, Integer> key = new Pair<>(event, turnSide);
        if (sTurns.containsKey(key)) {
            Integer resourceId = sTurns.get(key);
            return mContext.getResources().getString(resourceId);
        } else {
            Log.w(TAG, "Navigation event / turn not localized: " + event + ", " + turnSide);
            return String.format("Event: %d, Side: %d", event, turnSide);
        }
    }

    private String toHumanReadableDistance(int meters) {
        // TODO: implement.
        return "in " + String.valueOf(meters) + " " + mContext.getString(R.string.meters);
    }
}
