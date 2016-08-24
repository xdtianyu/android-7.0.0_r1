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

import android.transition.cts.R;

import android.transition.Scene;
import android.transition.TransitionManager;
import android.view.View;

import java.util.concurrent.CountDownLatch;
import java.util.concurrent.TimeUnit;

public class TransitionManagerTest extends BaseTransitionTest {

    public TransitionManagerTest() {
    }

    public void testBeginDelayedTransition() throws Throwable {
        runTestOnUiThread(new Runnable() {
            @Override
            public void run() {
                TransitionManager.beginDelayedTransition(mSceneRoot, mTransition);
                View view = mActivity.getLayoutInflater().inflate(R.layout.scene1, mSceneRoot,
                        false);
                mSceneRoot.addView(view);
            }
        });

        waitForStart();
        waitForEnd(300);
        assertEquals(1, mListener.resumeLatch.getCount());
        assertEquals(1, mListener.pauseLatch.getCount());
        assertEquals(1, mListener.cancelLatch.getCount());
        assertNotNull(mListener.transition);
        assertEquals(TestTransition.class, mListener.transition.getClass());
        assertTrue(mTransition != mListener.transition);
        runTestOnUiThread(new Runnable() {
            @Override
            public void run() {
                assertNotNull(mActivity.findViewById(R.id.redSquare));
                assertNotNull(mActivity.findViewById(R.id.greenSquare));
            }
        });
    }

    public void testGo() throws Throwable {
        startTransition(R.layout.scene1);
        waitForStart();
        waitForEnd(300);

        assertEquals(1, mListener.resumeLatch.getCount());
        assertEquals(1, mListener.pauseLatch.getCount());
        assertEquals(1, mListener.cancelLatch.getCount());
        assertNotNull(mListener.transition);
        assertEquals(TestTransition.class, mListener.transition.getClass());
        assertTrue(mTransition != mListener.transition);
        runTestOnUiThread(new Runnable() {
            @Override
            public void run() {
                assertNotNull(mActivity.findViewById(R.id.redSquare));
                assertNotNull(mActivity.findViewById(R.id.greenSquare));
            }
        });
    }

    public void testSetTransition1() throws Throwable {
        final TransitionManager transitionManager = new TransitionManager();

        runTestOnUiThread(new Runnable() {
            @Override
            public void run() {
                Scene scene = Scene.getSceneForLayout(mSceneRoot, R.layout.scene1, mActivity);
                transitionManager.setTransition(scene, mTransition);
                transitionManager.transitionTo(scene);
            }
        });

        waitForStart();
        waitForEnd(300);
        assertEquals(1, mListener.resumeLatch.getCount());
        assertEquals(1, mListener.pauseLatch.getCount());
        assertEquals(1, mListener.cancelLatch.getCount());
        assertNotNull(mListener.transition);
        assertEquals(TestTransition.class, mListener.transition.getClass());
        assertTrue(mTransition != mListener.transition);
        runTestOnUiThread(new Runnable() {
            @Override
            public void run() {
                mListener.startLatch = new CountDownLatch(1);
                mListener.endLatch = new CountDownLatch(1);
                assertNotNull(mActivity.findViewById(R.id.redSquare));
                assertNotNull(mActivity.findViewById(R.id.greenSquare));
                Scene scene = Scene.getSceneForLayout(mSceneRoot, R.layout.scene2, mActivity);
                transitionManager.transitionTo(scene);
            }
        });
        assertFalse(mListener.startLatch.await(50, TimeUnit.MILLISECONDS));
        endTransition();
    }

    public void testSetTransition2() throws Throwable {
        final TransitionManager transitionManager = new TransitionManager();
        final Scene[] scenes = new Scene[3];

        runTestOnUiThread(new Runnable() {
            @Override
            public void run() {
                scenes[0] = Scene.getSceneForLayout(mSceneRoot, R.layout.scene1, mActivity);
                scenes[1] = Scene.getSceneForLayout(mSceneRoot, R.layout.scene2, mActivity);
                scenes[2] = Scene.getSceneForLayout(mSceneRoot, R.layout.scene3, mActivity);
                transitionManager.setTransition(scenes[0], scenes[1], mTransition);
                transitionManager.transitionTo(scenes[0]);
            }
        });
        assertFalse(mListener.startLatch.await(100, TimeUnit.MILLISECONDS));

        runTestOnUiThread(new Runnable() {
            @Override
            public void run() {
                transitionManager.transitionTo(scenes[1]);
            }
        });

        waitForStart();
        waitForEnd(300);
        assertEquals(1, mListener.resumeLatch.getCount());
        assertEquals(1, mListener.pauseLatch.getCount());
        assertEquals(1, mListener.cancelLatch.getCount());
        assertNotNull(mListener.transition);
        assertEquals(TestTransition.class, mListener.transition.getClass());
        assertTrue(mTransition != mListener.transition);
        runTestOnUiThread(new Runnable() {
            @Override
            public void run() {
                mListener.startLatch = new CountDownLatch(1);
                mListener.endLatch = new CountDownLatch(1);
                transitionManager.transitionTo(scenes[2]);
            }
        });
        assertFalse(mListener.startLatch.await(50, TimeUnit.MILLISECONDS));
        endTransition();
    }

    public void testEndTransitions() throws Throwable {
        mTransition.setDuration(400);

        startTransition(R.layout.scene1);
        waitForStart();
        endTransition();
        waitForEnd(100);
    }

    public void testEndTransitionsBeforeStarted() throws Throwable {
        mTransition.setDuration(400);

        runTestOnUiThread(new Runnable() {
            @Override
            public void run() {
                Scene scene = Scene.getSceneForLayout(mSceneRoot, R.layout.scene1, mActivity);
                TransitionManager.go(scene, mTransition);
                TransitionManager.endTransitions(mSceneRoot);
            }
        });
        assertFalse(mListener.startLatch.await(100, TimeUnit.MILLISECONDS));
        assertFalse(mListener.endLatch.await(10, TimeUnit.MILLISECONDS));
    }
}

