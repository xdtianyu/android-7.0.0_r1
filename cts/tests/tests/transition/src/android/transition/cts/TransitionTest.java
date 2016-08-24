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
import android.animation.AnimatorListenerAdapter;
import android.graphics.Rect;
import android.os.Debug;
import android.os.SystemClock;
import android.transition.ArcMotion;
import android.transition.AutoTransition;
import android.transition.ChangeBounds;
import android.transition.PathMotion;
import android.transition.Scene;
import android.transition.Transition;
import android.transition.Transition.EpicenterCallback;
import android.transition.Transition.TransitionListener;
import android.transition.TransitionManager;
import android.transition.TransitionPropagation;
import android.transition.TransitionValues;
import android.view.View;
import android.view.ViewGroup;
import android.view.animation.AccelerateInterpolator;
import android.widget.ImageView;
import android.widget.RelativeLayout;
import android.widget.TextView;

import java.util.HashMap;
import java.util.concurrent.CountDownLatch;
import java.util.concurrent.TimeUnit;

public class TransitionTest extends BaseTransitionTest {

    public TransitionTest() {
    }

    public void testAddListener() throws Throwable {
        startTransition(R.layout.scene1);
        waitForStart();

        final SimpleTransitionListener listener2 = new SimpleTransitionListener();

        runTestOnUiThread(new Runnable() {
            @Override
            public void run() {
                AutoTransition autoTransition = new AutoTransition();
                autoTransition.setDuration(100);
                autoTransition.addListener(listener2);
                Scene scene = Scene.getSceneForLayout(mSceneRoot, R.layout.scene2, mActivity);
                TransitionManager.go(scene, autoTransition);
            }
        });

        waitForStart(listener2);

        assertEquals(0, mListener.pauseLatch.getCount());
        assertEquals(0, mListener.resumeLatch.getCount());
        assertEquals(1, mListener.cancelLatch.getCount());
        assertEquals(1, mListener.endLatch.getCount());
        assertEquals(0, mListener.startLatch.getCount());

        assertEquals(1, listener2.pauseLatch.getCount());
        assertEquals(1, listener2.resumeLatch.getCount());
        assertEquals(1, listener2.cancelLatch.getCount());
        assertEquals(1, listener2.endLatch.getCount());
        assertEquals(0, listener2.startLatch.getCount());
        endTransition();
    }

    public void testRemoveListener() throws Throwable {
        startTransition(R.layout.scene1);
        waitForStart();

        runTestOnUiThread(new Runnable() {
            @Override
            public void run() {
                mTransition.removeListener(mListener);
            }
        });

        assertFalse(mListener.endLatch.await(250, TimeUnit.MILLISECONDS));
    }

    public void testAddTargetId() throws Throwable {
        enterScene(R.layout.scene4);
        assertNotNull(mTransition.getTargetIds());
        assertTrue(mTransition.getTargetIds().isEmpty());
        mTransition.addTarget(R.id.holder);
        mTransition.addTarget(R.id.hello);
        assertEquals(2, mTransition.getTargetIds().size());
        startTransition(R.layout.scene1);
        assertEquals(1, mTargets.size());
        assertEquals(R.id.hello, mTargets.get(0).getId());
        endTransition();
    }

    public void testRemoveTargetId() throws Throwable {
        enterScene(R.layout.scene4);
        mTransition.addTarget(R.id.holder);
        mTransition.addTarget(R.id.hello);
        mTransition.addTarget(R.id.redSquare);
        assertEquals(3, mTransition.getTargetIds().size());
        mTransition.removeTarget(0); // nothing should happen
        mTransition.removeTarget(R.id.redSquare);
        assertEquals(2, mTransition.getTargetIds().size());

        startTransition(R.layout.scene1);
        assertEquals(1, mTargets.size());
        assertEquals(R.id.hello, mTargets.get(0).getId());
        endTransition();
    }

    public void testAddTargetClass() throws Throwable {
        enterScene(R.layout.scene4);
        assertNull(mTransition.getTargetTypes());
        mTransition.addTarget(RelativeLayout.class);
        mTransition.addTarget(TextView.class);
        assertEquals(2, mTransition.getTargetTypes().size());
        startTransition(R.layout.scene1);
        assertEquals(1, mTargets.size());
        assertTrue(mTargets.get(0) instanceof TextView);
        endTransition();
    }

    public void testRemoveTargetClass() throws Throwable {
        enterScene(R.layout.scene4);
        mTransition.addTarget(TextView.class);
        mTransition.addTarget(View.class);
        mTransition.addTarget(RelativeLayout.class);
        assertEquals(3, mTransition.getTargetTypes().size());
        mTransition.removeTarget(ImageView.class); // should do nothing
        mTransition.removeTarget(View.class);
        assertEquals(2, mTransition.getTargetTypes().size());
        startTransition(R.layout.scene1);
        assertEquals(1, mTargets.size());
        assertTrue(mTargets.get(0) instanceof TextView);
        endTransition();
    }

    public void testAddTargetView() throws Throwable {
        enterScene(R.layout.scene1);

        final View[] target = new View[1];
        runTestOnUiThread(new Runnable() {
            @Override
            public void run() {
                target[0] = mActivity.findViewById(R.id.hello);
            }
        });
        mTransition.addTarget(target[0]);
        assertEquals(1, mTransition.getTargets().size());
        runTestOnUiThread(new Runnable() {
            @Override
            public void run() {
                TransitionManager.beginDelayedTransition(mSceneRoot, mTransition);
                target[0].setVisibility(View.GONE);
            }
        });
        waitForStart();
        assertEquals(1, mTargets.size());
        assertEquals(target[0], mTargets.get(0));
        endTransition();
    }

    public void testRemoveTargetView() throws Throwable {
        enterScene(R.layout.scene1);

        final View[] target = new View[3];
        runTestOnUiThread(new Runnable() {
            @Override
            public void run() {
                target[0] = mActivity.findViewById(R.id.hello);
                target[1] = mActivity.findViewById(R.id.greenSquare);
                target[2] = mActivity.findViewById(R.id.redSquare);
            }
        });

        mTransition.addTarget(target[0]);
        mTransition.addTarget(target[1]);
        assertEquals(2, mTransition.getTargets().size());
        mTransition.removeTarget(target[2]); // should do nothing
        mTransition.removeTarget(target[1]);
        assertEquals(1, mTransition.getTargets().size());
        runTestOnUiThread(new Runnable() {
            @Override
            public void run() {
                TransitionManager.beginDelayedTransition(mSceneRoot, mTransition);
                target[0].setVisibility(View.GONE);
            }
        });
        waitForStart();
        assertEquals(1, mTargets.size());
        assertEquals(target[0], mTargets.get(0));
        endTransition();
    }

    public void testAddTargetName() throws Throwable {
        enterScene(R.layout.scene4);
        assertNull(mTransition.getTargetNames());
        mTransition.addTarget("red");
        mTransition.addTarget("holder");
        assertEquals(2, mTransition.getTargetNames().size());
        assertEquals(0, mTargets.size());
        startTransition(R.layout.scene2);
        assertEquals(1, mTargets.size());
        assertEquals(R.id.redSquare, mTargets.get(0).getId());
        endTransition();
    }

    public void testRemoveTargetName() throws Throwable {
        enterScene(R.layout.scene4);
        mTransition.addTarget("holder");
        mTransition.addTarget("red");
        mTransition.addTarget("green");
        assertEquals(3, mTransition.getTargetNames().size());
        mTransition.removeTarget("purple"); // should do nothing
        // try to force a different String instance
        String greenName = new StringBuilder("gre").append("en").toString();
        mTransition.removeTarget(greenName);
        assertEquals(2, mTransition.getTargetNames().size());
        startTransition(R.layout.scene1);
        assertEquals(1, mTargets.size());
        assertEquals(R.id.redSquare, mTargets.get(0).getId());
        endTransition();
    }

    public void testIsTransitionRequired() throws Throwable {
        enterScene(R.layout.scene1);
        mTransition = new NotRequiredTransition();
        resetListener();
        runTestOnUiThread(new Runnable() {
            @Override
            public void run() {
                TransitionManager.beginDelayedTransition(mSceneRoot, mTransition);
                mActivity.findViewById(R.id.hello).setVisibility(View.GONE);
            }
        });
        waitForStart();
        assertEquals(0, mTargets.size());
        endTransition();
    }

    public void testCanRemoveViews() throws Throwable {
        enterScene(R.layout.scene1);
        assertFalse(mTransition.canRemoveViews());
        mTransition.addListener(new TransitionListener() {
            @Override
            public void onTransitionStart(Transition transition) {
                assertTrue(transition.canRemoveViews());
            }

            @Override
            public void onTransitionEnd(Transition transition) {
            }

            @Override
            public void onTransitionCancel(Transition transition) {
            }

            @Override
            public void onTransitionPause(Transition transition) {
            }

            @Override
            public void onTransitionResume(Transition transition) {
            }
        });
        startTransition(R.layout.scene2);
    }

    public void testExcludeChildrenView() throws Throwable {
        View layout1 = loadLayout(R.layout.scene1);
        Scene scene1 = loadScene(layout1);
        enterScene(R.layout.scene1);
        View holder1 = layout1.findViewById(R.id.holder);
        mTransition.excludeChildren(holder1, true);
        View layout2 = loadLayout(R.layout.scene2);
        Scene scene2 = loadScene(layout2);
        View holder2 = layout2.findViewById(R.id.holder);
        mTransition.excludeChildren(holder2, true);
        startTransition(scene2);
        waitForEnd(0); // Should already be ended, since no children are transitioning

        mTransition.excludeChildren(holder1, false); // remove it
        mTransition.excludeChildren(holder2, false); // remove it
        resetListener();
        startTransition(scene1);
        assertEquals(1, mListener.endLatch.getCount()); // it is running as expected
        endTransition();
    }

    public void testExcludeChildrenId() throws Throwable {
        enterScene(R.layout.scene1);
        mTransition.excludeChildren(R.id.holder, true);
        startTransition(R.layout.scene2);
        waitForEnd(0); // Should already be ended, since no children are transitioning

        resetListener();
        mTransition.excludeChildren(R.id.holder, false); // remove it
        startTransition(R.layout.scene1);
        assertEquals(1, mListener.endLatch.getCount()); // It is running
        endTransition();
    }

    public void testExcludeTargetView() throws Throwable {
        View layout1 = loadLayout(R.layout.scene1);
        Scene scene1 = loadScene(layout1);
        enterScene(R.layout.scene1);
        View redSquare1 = layout1.findViewById(R.id.redSquare);
        mTransition.excludeTarget(redSquare1, true);
        startTransition(R.layout.scene7);
        waitForEnd(0); // Should already be ended, since no children are transitioning

        mTransition.excludeTarget(redSquare1, false); // remove it
        resetListener();
        startTransition(scene1);
        assertEquals(1, mListener.endLatch.getCount()); // it is running as expected
        endTransition();
    }

    public void testExcludeTargetId() throws Throwable {
        enterScene(R.layout.scene1);
        mTransition.excludeTarget(R.id.redSquare, true);
        startTransition(R.layout.scene7);
        waitForEnd(0); // Should already be ended, since no children are transitioning

        resetListener();
        mTransition.excludeTarget(R.id.redSquare, false); // remove it
        startTransition(R.layout.scene1);
        assertEquals(1, mListener.endLatch.getCount()); // It is running
        endTransition();
    }

    public void testExcludeTargetClass() throws Throwable {
        enterScene(R.layout.scene1);
        mTransition.excludeTarget(TextView.class, true);
        startTransition(R.layout.scene3);
        waitForEnd(0); // Should already be ended, since no children are transitioning

        resetListener();
        mTransition.excludeTarget(TextView.class, false); // remove it
        startTransition(R.layout.scene1);
        assertEquals(1, mListener.endLatch.getCount()); // It is running
        endTransition();
    }

    public void testExcludeTargetName() throws Throwable {
        enterScene(R.layout.scene1);
        mTransition.excludeTarget("hello", true);
        startTransition(R.layout.scene3);
        waitForEnd(0); // Should already be ended, since no children are transitioning

        resetListener();
        mTransition.excludeTarget("hello", false); // remove it
        startTransition(R.layout.scene1);
        assertEquals(1, mListener.endLatch.getCount()); // It is running
        endTransition();
    }

    public void testDuration() throws Throwable {
        assertEquals(-1, mTransition.getDuration());
        enterScene(R.layout.scene1);
        mTransition.setDuration(500);
        assertEquals(500, mTransition.getDuration());
        startTransition(R.layout.scene3);
        long startTime = SystemClock.uptimeMillis();
        waitForEnd(600);
        long endTime = SystemClock.uptimeMillis();
        assertEquals(500, endTime - startTime, 100);
    }

    public void testEpicenter() throws Throwable {
        EpicenterCallback callback = new EpicenterCallback() {
            @Override
            public Rect onGetEpicenter(Transition transition) {
                return new Rect(0, 0, 1, 1);
            }
        };
        mTransition.setEpicenterCallback(callback);
        assertEquals(callback, mTransition.getEpicenterCallback());
    }

    public void testInterpolator() throws Throwable {
        enterScene(R.layout.scene1);
        View redSquare = mActivity.findViewById(R.id.redSquare);
        CaptureAnimatorTransition transition = new CaptureAnimatorTransition();
        assertNull(transition.getInterpolator());
        AccelerateInterpolator interpolator = new AccelerateInterpolator();
        transition.setInterpolator(interpolator);
        assertSame(interpolator, transition.getInterpolator());
        mTransition = transition;
        resetListener();
        startTransition(R.layout.scene4);
        assertFalse(transition.animators.isEmpty());
        Animator animator = transition.animators.get(redSquare);
        AnimationStartListener listener = transition.listeners.get(redSquare);
        assertTrue(listener.startLatch.await(100, TimeUnit.MILLISECONDS));
        assertSame(interpolator, animator.getInterpolator());
        endTransition();
    }

    public void testName() throws Throwable {
        assertEquals("android.transition.cts.BaseTransitionTest$TestTransition",
                mTransition.getName());
    }

    public void testPathMotion() throws Throwable {
        PathMotion pathMotion = new ArcMotion();
        mTransition.setPathMotion(pathMotion);
        assertEquals(pathMotion, mTransition.getPathMotion());
    }

    public void testPropagation() throws Throwable {
        enterScene(R.layout.scene1);
        CaptureAnimatorTransition transition = new CaptureAnimatorTransition();
        mTransition = transition;
        TransitionPropagation yPropagation = new TransitionPropagation() {
            private static final String TOP = "top value";
            private final String[] PROPERTIES = {TOP};

            @Override
            public long getStartDelay(ViewGroup viewGroup, Transition transition,
                    TransitionValues startValues, TransitionValues endValues) {
                int startTop = startValues == null ? 0 : (Integer) startValues.values.get(TOP);
                int endTop = endValues == null ? 0 : (Integer) endValues.values.get(TOP);
                return (startTop == 0) ? endTop : startTop;
            }

            @Override
            public void captureValues(TransitionValues transitionValues) {
                if (transitionValues.view != null) {
                    transitionValues.values.put(TOP, transitionValues.view.getTop());
                }
            }

            @Override
            public String[] getPropagationProperties() {
                return PROPERTIES;
            }
        };
        mTransition.setPropagation(yPropagation);
        resetListener();

        View redSquare = mActivity.findViewById(R.id.redSquare);
        View greenSquare = mActivity.findViewById(R.id.greenSquare);
        int diffTop = greenSquare.getTop() - redSquare.getTop();
        startTransition(R.layout.scene4);
        Animator redSquareAnimator = transition.animators.get(redSquare);
        Animator greenSquareAnimator = transition.animators.get(greenSquare);
        AnimationStartListener listener = transition.listeners.get(redSquare);
        assertTrue(listener.startLatch.await(100, TimeUnit.MILLISECONDS));
        assertEquals(0, redSquareAnimator.getStartDelay());
        assertEquals(diffTop, greenSquareAnimator.getStartDelay());
        endTransition();
    }

    public void testStartDelay() throws Throwable {
        CaptureAnimatorTransition transition = new CaptureAnimatorTransition();
        mTransition = transition;
        resetListener();
        enterScene(R.layout.scene1);
        View redSquare = mActivity.findViewById(R.id.redSquare);

        assertEquals(-1, mTransition.getStartDelay());
        mTransition.setStartDelay(200);
        assertEquals(200, mTransition.getStartDelay());

        startTransition(R.layout.scene4);

        Animator animator = transition.animators.get(redSquare);
        assertFalse(animator.isRunning());
        AnimationStartListener listener = transition.listeners.get(redSquare);
        assertTrue(listener.startLatch.await(250, TimeUnit.MILLISECONDS));
        endTransition();
    }

    public void testTransitionValues() throws Throwable {
        enterScene(R.layout.scene1);
        mTransition = new CheckTransitionValuesTransition();
        mTransition.setDuration(10);
        resetListener();
        startTransition(R.layout.scene4);
        // The transition has all the asserts in it, so we can just end it now.
        endTransition();
    }

    public void testMatchOrder() throws Throwable {
        mTransition = new ChangeBounds();
        resetListener();
        enterScene(R.layout.scene1);
        startTransition(R.layout.scene8);

        // scene 8 swaps the ids, but not the names. No transition should happen.
        waitForEnd(0);

        // now change the match order to prefer the id
        mTransition.setMatchOrder(new int[] {Transition.MATCH_ID, Transition.MATCH_NAME});

        resetListener();
        startTransition(R.layout.scene1);
        assertEquals(1, mListener.endLatch.getCount());
        waitForEnd(400);
    }

    private class NotRequiredTransition extends TestTransition {
        @Override
        public boolean isTransitionRequired(TransitionValues startValues,
                TransitionValues newValues) {
            return false;
        }
    }

    private class CaptureAnimatorTransition extends TestTransition {
        public HashMap<View, Animator> animators = new HashMap<>();
        public HashMap<View, AnimationStartListener> listeners = new HashMap<>();

        @Override
        public Animator onAppear(ViewGroup sceneRoot, View view, TransitionValues startValues,
                TransitionValues endValues) {
            return setupAnimator(super.onAppear(sceneRoot, view, startValues, endValues),
                    endValues.view);
        }

        @Override
        public Animator onDisappear(ViewGroup sceneRoot, View view, TransitionValues startValues,
                TransitionValues endValues) {
            return setupAnimator(super.onDisappear(sceneRoot, view, startValues, endValues),
                    startValues.view);
        }

        private Animator setupAnimator(Animator animator, View view) {
            animators.put(view, animator);
            AnimationStartListener listener = new AnimationStartListener();
            animator.addListener(listener);
            listeners.put(view, listener);
            return animator;
        }
    }

    private class CheckTransitionValuesTransition extends TestTransition {
        @Override
        public Animator onAppear(ViewGroup sceneRoot, View view, TransitionValues startValues,
                TransitionValues endValues) {
            assertNull(getTransitionValues(endValues.view, true));
            assertEquals(endValues, getTransitionValues(endValues.view, false));
            return super.onAppear(sceneRoot, view, startValues, endValues);
        }

        @Override
        public Animator onDisappear(ViewGroup sceneRoot, View view, TransitionValues startValues,
                TransitionValues endValues) {
            assertNull(getTransitionValues(startValues.view, false));
            assertEquals(startValues, getTransitionValues(startValues.view, true));
            return super.onDisappear(sceneRoot, view, startValues, endValues);
        }
    }

    private class AnimationStartListener extends AnimatorListenerAdapter {
        public CountDownLatch startLatch = new CountDownLatch(1);

        @Override
        public void onAnimationStart(Animator animation) {
            startLatch.countDown();
        }
    }
}

