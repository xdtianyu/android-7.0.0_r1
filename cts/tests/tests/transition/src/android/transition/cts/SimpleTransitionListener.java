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
package android.transition.cts;

import android.transition.Transition;
import android.transition.Transition.TransitionListener;

import java.util.concurrent.CountDownLatch;

/**
 * Listener captures whether each of the methods is called.
 */
class SimpleTransitionListener implements TransitionListener {
    public Transition transition;

    public CountDownLatch startLatch = new CountDownLatch(1);
    public CountDownLatch endLatch = new CountDownLatch(1);
    public CountDownLatch cancelLatch = new CountDownLatch(1);
    public CountDownLatch pauseLatch = new CountDownLatch(1);
    public CountDownLatch resumeLatch = new CountDownLatch(1);

    @Override
    public void onTransitionStart(Transition transition) {
        this.transition = transition;
        startLatch.countDown();
    }

    @Override
    public void onTransitionEnd(Transition transition) {
        endLatch.countDown();
    }

    @Override
    public void onTransitionCancel(Transition transition) {
        cancelLatch.countDown();
    }

    @Override
    public void onTransitionPause(Transition transition) {
        pauseLatch.countDown();
    }

    @Override
    public void onTransitionResume(Transition transition) {
        resumeLatch.countDown();
    }
}
