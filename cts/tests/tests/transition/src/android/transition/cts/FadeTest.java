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
package android.transition.cts;

import android.transition.Fade;

/**
 * This tests the public API for Fade. The alpha cannot be easily tested as part of CTS,
 * so those are implementation tests.
 */
public class FadeTest extends BaseTransitionTest {
    Fade mFade;

    @Override
    protected void setUp() throws Exception {
        super.setUp();
        resetTransition();
    }

    private void resetTransition() {
        mFade = new Fade();
        mFade.setDuration(200);
        mTransition = mFade;
        resetListener();
    }

    public void testMode() throws Throwable {
        // Should animate in and out by default
        enterScene(R.layout.scene4);
        startTransition(R.layout.scene1);
        assertEquals(1, mListener.endLatch.getCount());
        waitForEnd(400);

        resetListener();
        startTransition(R.layout.scene4);
        assertEquals(1, mListener.endLatch.getCount());
        waitForEnd(400);

        // Now only animate in
        mFade = new Fade(Fade.IN);
        mTransition = mFade;
        resetListener();
        startTransition(R.layout.scene1);
        assertEquals(1, mListener.endLatch.getCount());
        waitForEnd(400);

        // No animation since it should only animate in
        resetListener();
        startTransition(R.layout.scene4);
        waitForEnd(0);

        // Now animate out, but no animation should happen since we're animating in.
        mFade = new Fade(Fade.OUT);
        mTransition = mFade;
        resetListener();
        startTransition(R.layout.scene1);
        waitForEnd(0);

        // but it should animate out
        resetListener();
        startTransition(R.layout.scene4);
        assertEquals(1, mListener.endLatch.getCount());
        waitForEnd(400);
    }
}

