/*
 * Copyright (C) 2012 The Android Open Source Project
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
package android.animation.cts;

import java.lang.Override;
import java.lang.Runnable;
import java.lang.Thread;
import java.util.ArrayList;
import java.util.HashSet;
import java.util.Set;
import java.util.concurrent.CountDownLatch;
import java.util.concurrent.TimeUnit;

import android.animation.Animator;
import android.animation.AnimatorListenerAdapter;
import android.animation.AnimatorSet;
import android.animation.ObjectAnimator;
import android.animation.TimeInterpolator;
import android.animation.ValueAnimator;
import android.test.ActivityInstrumentationTestCase2;
import android.view.animation.AccelerateDecelerateInterpolator;
import android.view.animation.AccelerateInterpolator;
import android.view.animation.LinearInterpolator;

public class AnimatorSetTest extends
        ActivityInstrumentationTestCase2<AnimationActivity> {
    private AnimationActivity mActivity;
    private AnimatorSet mAnimatorSet;
    private long mDuration = 1000;
    private Object object;
    private ObjectAnimator yAnimator;
    private ObjectAnimator xAnimator;
    Set<Integer> identityHashes = new HashSet<Integer>();

    public AnimatorSetTest() {
        super(AnimationActivity.class);
    }

    @Override
    protected void setUp() throws Exception {
        super.setUp();
        setActivityInitialTouchMode(false);
        mActivity = getActivity();
        object = mActivity.view.newBall;
        yAnimator = getYAnimator(object);
        xAnimator = getXAnimator(object);
    }

     public void testPlaySequentially() throws Throwable {
         xAnimator.setRepeatCount(0);
         yAnimator.setRepeatCount(0);
         xAnimator.setDuration(50);
         yAnimator.setDuration(50);
         Animator[] animatorArray = {xAnimator, yAnimator};
         mAnimatorSet = new AnimatorSet();
         mAnimatorSet.playSequentially(animatorArray);
         verifySequentialPlayOrder(mAnimatorSet, animatorArray);

         ValueAnimator anim1 = ValueAnimator.ofFloat(0f, 1f);
         ValueAnimator anim2 = ValueAnimator.ofInt(0, 100);
         anim1.setDuration(50);
         anim2.setDuration(50);
         AnimatorSet set = new AnimatorSet();
         set.playSequentially(anim1, anim2);
         verifySequentialPlayOrder(set, new Animator[] {anim1, anim2});
    }

    /**
     * Start the animator, and verify the animators are played sequentially in the order that is
     * defined in the array.
     *
     * @param set AnimatorSet to be started and verified
     * @param animators animators that we put in the AnimatorSet, in the order that they'll play
     */
    private void verifySequentialPlayOrder(final AnimatorSet set, Animator[] animators)
            throws Throwable {

        final MyListener[] listeners = new MyListener[animators.length];
        for (int i = 0; i < animators.length; i++) {
            if (i == 0) {
                listeners[i] = new MyListener();
            } else {
                final int current = i;
                listeners[i] = new MyListener() {
                    @Override
                    public void onAnimationStart(Animator anim) {
                        super.onAnimationStart(anim);
                        // Check that the previous animator has finished.
                        assertTrue(listeners[current - 1].mEndIsCalled);
                    }
                };
            }
            animators[i].addListener(listeners[i]);
        }

        final CountDownLatch startLatch = new CountDownLatch(1);
        final CountDownLatch endLatch = new CountDownLatch(1);

        set.addListener(new MyListener() {
            @Override
            public void onAnimationEnd(Animator anim) {
                endLatch.countDown();
            }
        });

        long totalDuration = set.getTotalDuration();
        assertFalse(set.isRunning());
        runTestOnUiThread(new Runnable() {
            @Override
            public void run() {
                set.start();
                startLatch.countDown();
            }
        });

        // Set timeout to 100ms, if current count reaches 0 before the timeout, startLatch.await(...)
        // will return immediately.
        assertTrue(startLatch.await(100, TimeUnit.MILLISECONDS));
        assertTrue(set.isRunning());
        assertTrue(endLatch.await(totalDuration * 2, TimeUnit.MILLISECONDS));
        // Check that all the animators have finished.
        for (int i = 0; i < listeners.length; i++) {
            assertTrue(listeners[i].mEndIsCalled);
        }
    }

    public void testPlayTogether() throws Throwable {
        xAnimator.setRepeatCount(ValueAnimator.INFINITE);
        Animator[] animatorArray = {xAnimator, yAnimator};

        mAnimatorSet = new AnimatorSet();
        mAnimatorSet.playTogether(animatorArray);

        assertFalse(mAnimatorSet.isRunning());
        assertFalse(xAnimator.isRunning());
        assertFalse(yAnimator.isRunning());
        startAnimation(mAnimatorSet);
        Thread.sleep(100);
        assertTrue(mAnimatorSet.isRunning());
        assertTrue(xAnimator.isRunning());
        assertTrue(yAnimator.isRunning());

        // Now assemble another animator set
        ValueAnimator anim1 = ValueAnimator.ofFloat(0f, 100f);
        ValueAnimator anim2 = ValueAnimator.ofFloat(10f, 100f);
        AnimatorSet set = new AnimatorSet();
        set.playTogether(anim1, anim2);

        assertFalse(set.isRunning());
        assertFalse(anim1.isRunning());
        assertFalse(anim2.isRunning());
        startAnimation(set);
        Thread.sleep(100);
        assertTrue(set.isRunning());
        assertTrue(anim1.isRunning());
        assertTrue(anim2.isRunning());
    }

    public void testPlayBeforeAfter() throws Throwable {
        xAnimator.setRepeatCount(0);
        yAnimator.setRepeatCount(0);
        final ValueAnimator zAnimator = ValueAnimator.ofFloat(0f, 100f);

        xAnimator.setDuration(50);
        yAnimator.setDuration(50);
        zAnimator.setDuration(50);

        AnimatorSet set = new AnimatorSet();
        set.play(yAnimator).before(zAnimator).after(xAnimator);

        verifySequentialPlayOrder(set, new Animator[] {xAnimator, yAnimator, zAnimator});
    }

    public void testPauseAndResume() throws Throwable {
        final AnimatorSet set = new AnimatorSet();
        ValueAnimator a1 = ValueAnimator.ofFloat(0f, 100f);
        a1.setDuration(50);
        ValueAnimator a2 = ValueAnimator.ofFloat(0f, 100f);
        a2.setDuration(50);
        a1.addListener(new AnimatorListenerAdapter() {
            @Override
            public void onAnimationStart(Animator animation) {
                // Pause non-delayed set once the child animator starts
                set.pause();
            }
        });
        set.playTogether(a1, a2);

        final AnimatorSet delayedSet = new AnimatorSet();
        ValueAnimator a3 = ValueAnimator.ofFloat(0f, 100f);
        a3.setDuration(50);
        ValueAnimator a4 = ValueAnimator.ofFloat(0f, 100f);
        a4.setDuration(50);
        delayedSet.playSequentially(a3, a4);
        delayedSet.setStartDelay(50);

        MyListener l1 = new MyListener();
        MyListener l2 = new MyListener();
        set.addListener(l1);
        delayedSet.addListener(l2);

        runTestOnUiThread(new Runnable() {
            @Override
            public void run() {
                set.start();
                delayedSet.start();

                // Pause the delayed set during start delay
                delayedSet.pause();
            }
        });

        // Sleep long enough so that if the sets are not properly paused, they would have
        // finished.
        Thread.sleep(300);
        // Verify that both sets have been paused and *not* finished.
        assertTrue(set.isPaused());
        assertTrue(delayedSet.isPaused());
        assertTrue(l1.mStartIsCalled);
        assertTrue(l2.mStartIsCalled);
        assertFalse(l1.mEndIsCalled);
        assertFalse(l2.mEndIsCalled);

        runTestOnUiThread(new Runnable() {
            @Override
            public void run() {
                set.resume();
                delayedSet.resume();
            }
        });
        Thread.sleep(300);

        assertFalse(set.isPaused());
        assertFalse(delayedSet.isPaused());
        assertTrue(l1.mEndIsCalled);
        assertTrue(l2.mEndIsCalled);
    }

    public void testPauseBeforeStart() throws Throwable {
        final AnimatorSet set = new AnimatorSet();
        ValueAnimator a1 = ValueAnimator.ofFloat(0f, 100f);
        a1.setDuration(50);
        ValueAnimator a2 = ValueAnimator.ofFloat(0f, 100f);
        a2.setDuration(50);
        set.setStartDelay(50);
        set.playSequentially(a1, a2);

        final MyListener listener = new MyListener();
        set.addListener(listener);

        runTestOnUiThread(new Runnable() {
            @Override
            public void run() {
                // Pause animator set before calling start()
                set.pause();
                // Verify that pause should have no effect on a not-yet-started animator.
                assertFalse(set.isPaused());
                set.start();
            }
        });
        Thread.sleep(300);

        // Animator set should finish running by now since it's not paused.
        assertTrue(listener.mStartIsCalled);
        assertTrue(listener.mEndIsCalled);
    }

    public void testDuration() throws Throwable {
        xAnimator.setRepeatCount(ValueAnimator.INFINITE);
        Animator[] animatorArray = { xAnimator, yAnimator };

        mAnimatorSet = new AnimatorSet();
        mAnimatorSet.playTogether(animatorArray);
        mAnimatorSet.setDuration(1000);

        startAnimation(mAnimatorSet);
        Thread.sleep(100);
        assertEquals(mAnimatorSet.getDuration(), 1000);
    }

    public void testStartDelay() throws Throwable {
        xAnimator.setRepeatCount(ValueAnimator.INFINITE);
        Animator[] animatorArray = { xAnimator, yAnimator };

        mAnimatorSet = new AnimatorSet();
        mAnimatorSet.playTogether(animatorArray);
        mAnimatorSet.setStartDelay(10);

        startAnimation(mAnimatorSet);
        Thread.sleep(100);
        assertEquals(mAnimatorSet.getStartDelay(), 10);
    }

    public void testgetChildAnimations() throws Throwable {
        Animator[] animatorArray = { xAnimator, yAnimator };

        mAnimatorSet = new AnimatorSet();
        ArrayList<Animator> childAnimations = mAnimatorSet.getChildAnimations();
        assertEquals(0, mAnimatorSet.getChildAnimations().size());
        mAnimatorSet.playSequentially(animatorArray);
        assertEquals(2, mAnimatorSet.getChildAnimations().size());
    }

    public void testSetInterpolator() throws Throwable {
        xAnimator.setRepeatCount(ValueAnimator.INFINITE);
        Animator[] animatorArray = {xAnimator, yAnimator};
        TimeInterpolator interpolator = new AccelerateDecelerateInterpolator();
        mAnimatorSet = new AnimatorSet();
        mAnimatorSet.playTogether(animatorArray);
        mAnimatorSet.setInterpolator(interpolator);

        assertFalse(mAnimatorSet.isRunning());
        startAnimation(mAnimatorSet);
        Thread.sleep(100);

        ArrayList<Animator> animatorList = mAnimatorSet.getChildAnimations();
        assertEquals(interpolator, ((ObjectAnimator)animatorList.get(0)).getInterpolator());
        assertEquals(interpolator, ((ObjectAnimator)animatorList.get(1)).getInterpolator());
    }

    public ObjectAnimator getXAnimator(Object object) {
        String propertyX = "x";
        float startX = mActivity.mStartX;
        float endX = mActivity.mStartX + mActivity.mDeltaX;
        ObjectAnimator xAnimator = ObjectAnimator.ofFloat(object, propertyX, startX, endX);
        xAnimator.setDuration(mDuration);
        xAnimator.setRepeatCount(ValueAnimator.INFINITE);
        xAnimator.setInterpolator(new AccelerateInterpolator());
        xAnimator.setRepeatMode(ValueAnimator.REVERSE);
        return xAnimator;
    }

    public ObjectAnimator getYAnimator(Object object) {
         String property = "y";
         float startY = mActivity.mStartY;
         float endY = mActivity.mStartY + mActivity.mDeltaY;
         ObjectAnimator yAnimator = ObjectAnimator.ofFloat(object, property, startY, endY);
         yAnimator.setDuration(mDuration);
         yAnimator.setRepeatCount(2);
         yAnimator.setInterpolator(new AccelerateInterpolator());
         yAnimator.setRepeatMode(ValueAnimator.REVERSE);
        return yAnimator;
    }

    private void startAnimation(final AnimatorSet animatorSet) throws Throwable {
        this.runTestOnUiThread(new Runnable() {
            public void run() {
                mActivity.startAnimatorSet(animatorSet);
            }
        });
    }

    private void assertUnique(Object object) {
        assertUnique(object, "");
    }

    private void assertUnique(Object object, String msg) {
        final int code = System.identityHashCode(object);
        assertTrue("object should be unique " + msg + ", obj:" + object, identityHashes.add(code));

    }

    public void testClone() throws Throwable {
        final AnimatorSet set1 = new AnimatorSet();
        final AnimatorListenerAdapter setListener = new AnimatorListenerAdapter() {};
        set1.addListener(setListener);
        ObjectAnimator animator1 = new ObjectAnimator();
        animator1.setDuration(100);
        animator1.setPropertyName("x");
        animator1.setIntValues(5);
        animator1.setInterpolator(new LinearInterpolator());
        AnimatorListenerAdapter listener1 = new AnimatorListenerAdapter(){};
        AnimatorListenerAdapter listener2 = new AnimatorListenerAdapter(){};
        animator1.addListener(listener1);

        ObjectAnimator animator2 = new ObjectAnimator();
        animator2.setDuration(100);
        animator2.setInterpolator(new LinearInterpolator());
        animator2.addListener(listener2);
        animator2.setPropertyName("y");
        animator2.setIntValues(10);

        set1.playTogether(animator1, animator2);

        AnimateObject target = new AnimateObject();
        set1.setTarget(target);
        runTestOnUiThread(new Runnable() {
            @Override
            public void run() {
                set1.start();
            }
        });
        assertTrue(set1.isStarted());

        animator1.getListeners();
        AnimatorSet set2 = set1.clone();
        assertFalse(set2.isStarted());

        assertUnique(set1);
        assertUnique(animator1);
        assertUnique(animator2);

        assertUnique(set2);
        assertEquals(2, set2.getChildAnimations().size());

        Animator clone1 = set2.getChildAnimations().get(0);
        Animator clone2 = set2.getChildAnimations().get(1);

        for (Animator animator : set2.getChildAnimations()) {
            assertUnique(animator);
        }

        assertTrue(clone1.getListeners().contains(listener1));
        assertTrue(clone2.getListeners().contains(listener2));

        assertTrue(set2.getListeners().contains(setListener));

        for (Animator.AnimatorListener listener : set1.getListeners()) {
            assertTrue(set2.getListeners().contains(listener));
        }

        assertEquals(animator1.getDuration(), clone1.getDuration());
        assertEquals(animator2.getDuration(), clone2.getDuration());
        assertSame(animator1.getInterpolator(), clone1.getInterpolator());
        assertSame(animator2.getInterpolator(), clone2.getInterpolator());
    }

    class AnimateObject {
        int x = 1;
        int y = 2;
    }

    class MyListener extends AnimatorListenerAdapter {
        boolean mStartIsCalled = false;
        boolean mEndIsCalled = false;

        public void onAnimationStart(Animator animation) {
            mStartIsCalled = true;
        }

        public void onAnimationEnd(Animator animation) {
            mEndIsCalled = true;
        }
    }
}
