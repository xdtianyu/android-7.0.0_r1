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

package android.widget.cts.util;

import junit.framework.Assert;

import android.app.Instrumentation;
import android.support.annotation.NonNull;
import android.support.annotation.Nullable;
import android.view.View;
import android.view.ViewTreeObserver;
import android.view.ViewTreeObserver.OnDrawListener;

import java.util.concurrent.CountDownLatch;
import java.util.concurrent.TimeUnit;

/**
 * Utilities for testing View behavior.
 */
public class ViewTestUtils {

    /**
     * Runs the specified Runnable on the main thread and ensures that the
     * specified View's tree is drawn before returning.
     *
     * @param instrumentation the instrumentation used to run the test
     * @param view the view whose tree should be drawn before returning
     * @param runner the runnable to run on the main thread, or {@code null} to
     *               simply force invalidation and a draw pass
     */
    public static void runOnMainAndDrawSync(@NonNull Instrumentation instrumentation,
            @NonNull final View view, @Nullable final Runnable runner) {
        final CountDownLatch latch = new CountDownLatch(1);

        instrumentation.runOnMainSync(() -> {
            final ViewTreeObserver observer = view.getViewTreeObserver();
            final OnDrawListener listener = new OnDrawListener() {
                @Override
                public void onDraw() {
                    observer.removeOnDrawListener(this);
                    view.post(() -> latch.countDown());
                }
            };

            observer.addOnDrawListener(listener);

            if (runner != null) {
                runner.run();
            } else {
                view.invalidate();
            }
        });

        try {
            Assert.assertTrue("Expected draw pass occurred within 5 seconds",
                    latch.await(5, TimeUnit.SECONDS));
        } catch (InterruptedException e) {
            throw new RuntimeException(e);
        }
    }
}
