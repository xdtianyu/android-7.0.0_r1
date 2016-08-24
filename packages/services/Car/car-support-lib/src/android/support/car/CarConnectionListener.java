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

package android.support.car;

/**
 * Listener for monitoring car's connection status.
 * Callbacks are called from the looper specified when constructing {@link Car}.
 */
public interface CarConnectionListener {
    /**
     * Car has been connected. Does not guarantee that the car is still connected whilst this
     * callback is running, so {@link CarNotConnectedException}s may still be thrown from
     * {@link Car} method calls.
     * @param connectionType Type of car connected.
     */
    void onConnected(@Car.ConnectionType int connectionType);
    /** Car disconnected */
    void onDisconnected();
}
