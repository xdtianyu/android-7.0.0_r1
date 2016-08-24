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

import android.animation.Animator;
import android.animation.ObjectAnimator;
import android.test.ActivityInstrumentationTestCase2;
import android.transition.Scene;
import android.transition.Transition;
import android.transition.TransitionManager;
import android.transition.TransitionValues;
import android.transition.Visibility;
import android.view.Choreographer;
import android.view.Choreographer.FrameCallback;
import android.view.View;
import android.view.ViewGroup;
import android.widget.FrameLayout;

import java.util.ArrayList;
import java.util.concurrent.CountDownLatch;
import java.util.concurrent.TimeUnit;

public class BaseTransitionTest extends ActivityInstrumentationTestCase2<TransitionActivity> {
    protected TransitionActivity mActivity;
    protected FrameLayout mSceneRoot;
    public float mAnimatedValue;
    protected ArrayList<View> mTargets = new ArrayList<View>();
    protected Transition mTransition;
    protected SimpleTransitionListener mListener;

    public BaseTransitionTest() {
        super(TransitionActivity.class);
    }

    @Override
    protected void setUp() throws Exception {
        super.setUp();
        setActivityInitialTouchMode(false);
        mActivity = getActivity();
        mSceneRoot = (FrameLayout) mActivity.findViewById(R.id.container);
        mTargets.clear();
        mTransition = new TestTransition();
        mListener = new SimpleTransitionListener();
        mTransition.addListener(mListener);
    }

    protected void waitForStart() throws InterruptedException {
        waitForStart(mListener);
    }

    protected void waitForStart(SimpleTransitionListener listener) throws InterruptedException {
        assertTrue(listener.startLatch.await(4000, TimeUnit.MILLISECONDS));
    }

    protected void waitForEnd(long waitMillis) throws InterruptedException {
        waitForEnd(mListener, waitMillis);
        getInstrumentation().waitForIdleSync();
    }

    protected static void waitForEnd(SimpleTransitionListener listener, long waitMillis)
            throws InterruptedException {
        listener.endLatch.await(waitMillis, TimeUnit.MILLISECONDS);
    }

    protected View loadLayout(final int layout) throws Throwable {
        View[] root = new View[1];

        runTestOnUiThread(new Runnable() {
            @Override
            public void run() {
                root[0] = mActivity.getLayoutInflater().inflate(layout, mSceneRoot, false);
            }
        });

        return root[0];
    }

    protected Scene loadScene(final View layout) throws Throwable {
        Scene[] scene = new Scene[1];
        runTestOnUiThread(new Runnable() {
            @Override
            public void run() {
                scene[0] = new Scene(mSceneRoot, layout);
            }
        });

        return scene[0];
    }

    protected Scene loadScene(final int layoutId) throws Throwable {
        Scene scene[] = new Scene[1];
        runTestOnUiThread(new Runnable() {
            @Override
            public void run() {
                scene[0] = Scene.getSceneForLayout(mSceneRoot, layoutId, mActivity);
            }
        });
        return scene[0];
    }

    protected void startTransition(final int layoutId) throws Throwable {
        startTransition(loadScene(layoutId));
    }

    protected void startTransition(final Scene scene) throws Throwable {
        runTestOnUiThread(new Runnable() {
            @Override
            public void run() {
                TransitionManager.go(scene, mTransition);
            }
        });
        waitForStart();
    }

    protected void endTransition() throws Throwable {
        runTestOnUiThread(new Runnable() {
            @Override
            public void run() {
                TransitionManager.endTransitions(mSceneRoot);
            }
        });
    }

    protected void enterScene(final int layoutId) throws Throwable {
        enterScene(loadScene(layoutId));
    }

    protected void enterScene(final Scene scene) throws Throwable {
        runTestOnUiThread(new Runnable() {
            @Override
            public void run() {
                scene.enter();
            }
        });
        getInstrumentation().waitForIdleSync();
    }

    protected void exitScene(final Scene scene) throws Throwable {
        runTestOnUiThread(new Runnable() {
            @Override
            public void run() {
                scene.exit();
            }
        });
        getInstrumentation().waitForIdleSync();
    }

    protected void resetListener() {
        mTransition.removeListener(mListener);
        mListener = new SimpleTransitionListener();
        mTransition.addListener(mListener);
    }

    public class TestTransition extends Visibility {

        public TestTransition() {
        }

        @Override
        public Animator onAppear(ViewGroup sceneRoot, View view, TransitionValues startValues,
                TransitionValues endValues) {
            mTargets.add(endValues.view);
            return ObjectAnimator.ofFloat(BaseTransitionTest.this, "mAnimatedValue", 0, 1);
        }

        @Override
        public Animator onDisappear(ViewGroup sceneRoot, View view, TransitionValues startValues,
                TransitionValues endValues) {
            mTargets.add(startValues.view);
            return ObjectAnimator.ofFloat(BaseTransitionTest.this, "mAnimatedValue", 1, 0);
        }
    }
}
