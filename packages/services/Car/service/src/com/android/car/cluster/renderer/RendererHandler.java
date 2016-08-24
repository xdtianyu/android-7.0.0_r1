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

import android.os.Handler;
import android.os.Looper;
import android.os.Message;

import java.lang.ref.WeakReference;

/**
 * Abstract {@link Handler} class that holds reference to a renderer object.
 */
public abstract class RendererHandler<T> extends Handler {

    private final WeakReference<T> mRendererRef;

    RendererHandler(Looper looper, T renderer) {
        super(looper);
        mRendererRef = new WeakReference<>(renderer);
    }

    @Override
    public void handleMessage(Message msg) {
        T renderer = mRendererRef.get();
        if (renderer != null) {
            handleMessage(msg, renderer);
        }
    }

    public abstract void handleMessage(Message msg, T renderer);
}
