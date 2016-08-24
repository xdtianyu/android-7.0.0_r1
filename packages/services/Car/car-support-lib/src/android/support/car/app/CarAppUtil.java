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
package android.support.car.app;

import android.content.Context;

/**
 * @hide
 */
public final class CarAppUtil {

    /**
     * PackageManager.FEATURE_AUTOMOTIVE from M. But redefine here to support L.
     */
    private static final String FEATURE_AUTOMOTIVE = "android.hardware.type.automotive";

    public static boolean isEmbeddedCar(Context context) {
       return context.getPackageManager().hasSystemFeature(FEATURE_AUTOMOTIVE);
    }
}
