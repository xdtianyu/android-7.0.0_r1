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

package com.android.server.wifi.nan;

import java.lang.reflect.Field;

/**
 * Mock class for NAN HAL. Provides access to HAL API and to callbacks. To
 * extend:
 * <ul>
 * <li>HAL API: create a {@code public void} method which takes any fixed
 * arguments (e.g. a {@code short transactionId} and a second argument to
 * provide the rest of the argument as a JSON string: {@code String jsonArgs}.
 * <li>Callbacks from HAL: create a {@code public static native} function which
 * is used to trigger the callback from the test harness. The arguments are
 * similar to the HAL API arguments.
 * </ul>
 */
public class WifiNanHalMock {
    public void getCapabilitiesHalMockNative(short transactionId) {
        throw new IllegalStateException("Please mock this class!");
    }

    public void enableHalMockNative(short transactionId, String jsonArgs) {
        throw new IllegalStateException("Please mock this class!");
    }

    public void disableHalMockNative(short transactionId) {
        throw new IllegalStateException("Please mock this class!");
    }

    public void publishHalMockNative(short transactionId, String jsonArgs) {
        throw new IllegalStateException("Please mock this class!");
    }

    public void publishCancelHalMockNative(short transactionId, String jsonArgs) {
        throw new IllegalStateException("Please mock this class!");
    }

    public void subscribeHalMockNative(short transactionId, String jsonArgs) {
        throw new IllegalStateException("Please mock this class!");
    }

    public void subscribeCancelHalMockNative(short transactionId, String jsonArgs) {
        throw new IllegalStateException("Please mock this class!");
    }

    public void transmitFollowupHalMockNative(short transactionId, String jsonArgs) {
        throw new IllegalStateException("Please mock this class!");
    }

    /*
     * trigger callbacks - called by test harness with arguments passed by JSON
     * string.
     */

    public static native void callNotifyResponse(short transactionId, String jsonArgs);

    public static native void callPublishTerminated(String jsonArgs);

    public static native void callSubscribeTerminated(String jsonArgs);

    public static native void callFollowup(String jsonArgs);

    public static native void callMatch(String jsonArgs);

    public static native void callDiscEngEvent(String jsonArgs);

    public static native void callDisabled(String jsonArgs);

    /**
     * initialize NAN mock
     */
    private static native int initNanHalMock();

    public static void initNanHalMockLibrary() throws Exception {
        Field field = WifiNanNative.class.getDeclaredField("sNanNativeInit");
        field.setAccessible(true);
        field.setBoolean(null, true);

        initNanHalMock();
    }
}
