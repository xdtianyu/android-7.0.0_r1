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

import android.transition.ChangeScroll;
import android.transition.TransitionManager;
import android.view.View;

public class ChangeScrollTest extends BaseTransitionTest {
    ChangeScroll mChangeScroll;

    public ChangeScrollTest() {
    }

    @Override
    protected void setUp() throws Exception {
        super.setUp();
        mChangeScroll = new ChangeScroll();
        mTransition = mChangeScroll;
        resetListener();
    }

    public void testChangeScroll() throws Throwable {
        enterScene(R.layout.scene5);
        runTestOnUiThread(new Runnable() {
            @Override
            public void run() {
                final View view = mActivity.findViewById(R.id.text);
                assertEquals(0, view.getScrollX());
                assertEquals(0, view.getScrollY());
                TransitionManager.beginDelayedTransition(mSceneRoot, mChangeScroll);
                view.scrollTo(150, 300);
            }
        });
        waitForStart();
        Thread.sleep(150);
        runTestOnUiThread(new Runnable() {
            @Override
            public void run() {
                final View view = mActivity.findViewById(R.id.text);
                final int scrollX = view.getScrollX();
                final int scrollY = view.getScrollY();
                assertTrue(scrollX > 0);
                assertTrue(scrollX < 150);
                assertTrue(scrollY > 0);
                assertTrue(scrollY < 300);
            }
        });
        waitForEnd(400);
        runTestOnUiThread(new Runnable() {
            @Override
            public void run() {
                final View view = mActivity.findViewById(R.id.text);
                assertEquals(150, view.getScrollX());
                assertEquals(300, view.getScrollY());
            }
        });
    }
}

