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

package com.android.usbtuner;

import android.content.Context;

/**
 * A class to handle one internal tuner device.
 */
public class InternalTunerHal extends TunerHal {

    private boolean isDeviceOpen;

    protected InternalTunerHal(Context context) {
        super(context);
        isDeviceOpen = false;
    }

    @Override
    protected boolean openFirstAvailable() {
        isDeviceOpen = true;
        return true;
    }

    @Override
    public void close() {
        if (isStreaming()) {
            stopTune();
        }
        nativeFinalize(getDeviceId());
        isDeviceOpen = false;
    }

    @Override
    protected boolean isDeviceOpen() {
        return (isDeviceOpen);
    }

    @Override
    protected long getDeviceId() {
        return 0L;
    }
}
