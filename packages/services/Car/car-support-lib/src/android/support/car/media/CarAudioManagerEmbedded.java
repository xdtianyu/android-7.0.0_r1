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
package android.support.car.media;

import android.media.AudioAttributes;
import android.media.AudioManager.OnAudioFocusChangeListener;

/**
 * @hide
 */
public class CarAudioManagerEmbedded extends CarAudioManager {

    private final android.car.media.CarAudioManager mManager;

    public CarAudioManagerEmbedded(Object manager) {
        mManager = (android.car.media.CarAudioManager) manager;
    }

    @Override
    public AudioAttributes getAudioAttributesForCarUsage(@CarAudioUsage int carUsage) {
        return mManager.getAudioAttributesForCarUsage(carUsage);
    }

    @Override
    public int requestAudioFocus(OnAudioFocusChangeListener l,
                                          AudioAttributes requestAttributes,
                                          int durationHint,
                                          int flags) throws IllegalArgumentException {
        return mManager.requestAudioFocus(l, requestAttributes, durationHint, flags);
    }

    @Override
    public int abandonAudioFocus(OnAudioFocusChangeListener l, AudioAttributes aa) {
        return mManager.abandonAudioFocus(l, aa);
    }

    @Override
    public void onCarDisconnected() {
        //nothing to do
    }
}
