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

package android.car.hardware.camera;

import android.car.hardware.camera.CarCameraState;
import android.graphics.Rect;

/** @hide */
interface ICarCamera {
    int[] getCameraList() = 0;

    int getCapabilities(in int cameraType) = 1;

    Rect getCameraCrop(in int cameraType) = 2;

    void setCameraCrop(in int cameraType, in Rect rect) = 3;

    Rect getCameraPosition(in int cameraType) = 4;

    void setCameraPosition(in int cameraType, in Rect rect) = 5;

    CarCameraState getCameraState(in int cameraType) = 6;

    void setCameraState(in int cameraType, in CarCameraState state) = 7;
}
