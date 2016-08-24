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
package com.android.car.cluster.renderer;

import android.annotation.Nullable;
import android.car.cluster.renderer.NavigationRenderer;
import android.graphics.Bitmap;
import android.os.Handler;
import android.os.Looper;
import android.os.Message;

/**
 * A wrapper over {@link NavigationRenderer} that runs all its methods in the context of provided
 * looper. It is guaranteed that all calls will be invoked in order they were called.
 */
public class ThreadSafeNavigationRenderer extends NavigationRenderer {

    private final Handler mHandler;

    private final static int MSG_NAV_START = 1;
    private final static int MSG_NAV_STOP = 2;
    private final static int MSG_NAV_NEXT_TURN = 3;
    private final static int MSG_NAV_NEXT_TURN_DISTANCE = 4;

    /** Creates thread-safe {@link NavigationRenderer}. Returns null if renderer == null */
    @Nullable
    public static NavigationRenderer createFor(Looper looper, NavigationRenderer renderer) {
        return renderer == null ? null : new ThreadSafeNavigationRenderer(looper, renderer);
    }

    private ThreadSafeNavigationRenderer(Looper looper, NavigationRenderer renderer) {
        mHandler = new NavigationRendererHandler(looper, renderer);
    }

    @Override
    public void onStartNavigation() {
        mHandler.sendMessage(mHandler.obtainMessage(MSG_NAV_START));
    }

    @Override
    public void onStopNavigation() {
        mHandler.sendMessage(mHandler.obtainMessage(MSG_NAV_STOP));
    }

    @Override
    public void onNextTurnChanged(int event, String road, int turnAngle, int turnNumber,
            Bitmap image, int turnSide) {
        mHandler.sendMessage(mHandler.obtainMessage(MSG_NAV_NEXT_TURN,
                new NextTurn(event, road, turnAngle, turnNumber, image, turnSide)));
    }

    @Override
    public void onNextTurnDistanceChanged(int distanceMeters, int timeSeconds) {
        mHandler.sendMessage(mHandler.obtainMessage(
                    MSG_NAV_NEXT_TURN_DISTANCE, distanceMeters, timeSeconds));
    }

    private static class NavigationRendererHandler extends RendererHandler<NavigationRenderer> {

        NavigationRendererHandler(Looper looper, NavigationRenderer renderer) {
            super(looper, renderer);
        }

        @Override
        public void handleMessage(Message msg, NavigationRenderer renderer) {

            switch (msg.what) {
                case MSG_NAV_START:
                    renderer.onStartNavigation();
                    break;
                case MSG_NAV_STOP:
                    renderer.onStopNavigation();
                    break;
                case MSG_NAV_NEXT_TURN:
                    NextTurn nt = (NextTurn) msg.obj;
                    renderer.onNextTurnChanged(nt.event, nt.road, nt.turnAngle, nt.turnNumber,
                            nt.bitmap, nt.turnSide);
                    break;
                case MSG_NAV_NEXT_TURN_DISTANCE:
                    renderer.onNextTurnDistanceChanged(msg.arg1, msg.arg2);
                    break;
                default:
                    throw new IllegalArgumentException("Msg: " + msg.what);
            }
        }
    }

    private static class NextTurn {
        private final int event;
        private final String road;
        private final int turnAngle;
        private final int turnNumber;
        private final Bitmap bitmap;
        private final int turnSide;

        NextTurn(int event, String road, int turnAngle, int turnNumber, Bitmap bitmap,
                int turnSide) {
            this.event = event;
            this.road = road;
            this.turnAngle = turnAngle;
            this.turnNumber = turnNumber;
            this.bitmap = bitmap;
            this.turnSide = turnSide;
        }
    }
}
