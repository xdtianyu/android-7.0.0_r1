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

import android.animation.ArgbEvaluator;
import android.animation.ObjectAnimator;
import android.animation.PropertyValuesHolder;
import android.animation.ValueAnimator;
import android.test.ActivityInstrumentationTestCase2;
import android.util.Property;
import android.view.animation.AccelerateInterpolator;
import android.view.animation.Interpolator;

public class ObjectAnimatorTest extends
        ActivityInstrumentationTestCase2<AnimationActivity> {
    private AnimationActivity mActivity;
    private ObjectAnimator mObjectAnimator;
    private long mDuration = 1000;

    public ObjectAnimatorTest() {
        super(AnimationActivity.class);
    }

    @Override
    protected void setUp() throws Exception {
        super.setUp();
        setActivityInitialTouchMode(false);
        mActivity = getActivity();
        mObjectAnimator = (ObjectAnimator) mActivity.createAnimatorWithDuration(mDuration);
    }

    public void testDuration() throws Throwable {
        final long duration = 2000;
        ObjectAnimator objectAnimatorLocal = (ObjectAnimator)mActivity.createAnimatorWithDuration(
            duration);
        startAnimation(objectAnimatorLocal);
        assertEquals(duration, objectAnimatorLocal.getDuration());
    }
    public void testOfFloat() throws Throwable {
        Object object = mActivity.view.newBall;
        String property = "y";
        float startY = mActivity.mStartY;
        float endY = mActivity.mStartY + mActivity.mDeltaY;
        ObjectAnimator objAnimator = ObjectAnimator.ofFloat(object, property, startY, endY);
        assertTrue(objAnimator != null);
        objAnimator.setDuration(mDuration);
        objAnimator.setRepeatCount(ValueAnimator.INFINITE);
        objAnimator.setInterpolator(new AccelerateInterpolator());
        objAnimator.setRepeatMode(ValueAnimator.REVERSE);
        startAnimation(objAnimator);
        assertTrue(objAnimator != null);
        Thread.sleep(100);
        float x = mActivity.view.newBall.getX();
        float y = mActivity.view.newBall.getY();
        assertTrue( y >= startY);
        assertTrue( y <= endY);
    }

    public void testOfFloatBase() throws Throwable {
        Object object = mActivity.view.newBall;
        String property = "y";
        float startY = mActivity.mStartY;
        float endY = mActivity.mStartY + mActivity.mDeltaY;
        ObjectAnimator animator = ObjectAnimator.ofFloat(object, property, startY, endY);
        ObjectAnimator objAnimator = new ObjectAnimator();
        objAnimator.setTarget(object);
        objAnimator.setPropertyName(property);
        assertEquals(animator.getTarget(), objAnimator.getTarget());
        assertEquals(animator.getPropertyName(), objAnimator.getPropertyName());
    }

    public void testOfInt() throws Throwable {
        Object object = mActivity.view.newBall;
        String property = "backgroundColor";
        int startColor = mActivity.view.RED;
        int endColor = mActivity.view.BLUE;

        final ObjectAnimator colorAnimator = ObjectAnimator.ofInt(object, property,
                startColor, endColor);
        colorAnimator.setDuration(1000);
        colorAnimator.setEvaluator(new ArgbEvaluator());
        colorAnimator.setRepeatCount(1);
        colorAnimator.setRepeatMode(ValueAnimator.REVERSE);
        mActivity.runOnUiThread(new Runnable() {
            public void run() {
                colorAnimator.start();
            }
        });
        getInstrumentation().waitForIdleSync();
        startAnimation(mObjectAnimator, colorAnimator);
        Thread.sleep(100);
        Integer i = (Integer) colorAnimator.getAnimatedValue();
        //We are going from less negative value to a more negative value
        assertTrue(i.intValue() <= startColor);
        assertTrue(endColor <= i.intValue());
    }

    public void testOfObject() throws Throwable {
        Object object = mActivity.view.newBall;
        String property = "backgroundColor";
        int startColor = mActivity.view.RED;
        int endColor = mActivity.view.BLUE;
        Object[] values = {new Integer(startColor), new Integer(endColor)};
        ArgbEvaluator evaluator = new ArgbEvaluator();
        final ObjectAnimator colorAnimator = ObjectAnimator.ofObject(object, property,
                evaluator, values);
        colorAnimator.setDuration(1000);
        colorAnimator.setRepeatCount(1);
        colorAnimator.setRepeatMode(ValueAnimator.REVERSE);
        mActivity.runOnUiThread(new Runnable() {
            public void run() {
                colorAnimator.start();
            }
        });
        getInstrumentation().waitForIdleSync();
        startAnimation(mObjectAnimator, colorAnimator);
        Thread.sleep(100);
        Integer i = (Integer) colorAnimator.getAnimatedValue();
        //We are going from less negative value to a more negative value
        assertTrue(i.intValue() <= startColor);
        assertTrue(endColor <= i.intValue());
    }

    public void testOfPropertyValuesHolder() throws Throwable {
        Object object = mActivity.view.newBall;
        String propertyName = "backgroundColor";
        int startColor = mActivity.view.RED;
        int endColor = mActivity.view.BLUE;
        int values[] = {startColor, endColor};
        ArgbEvaluator evaluator = new ArgbEvaluator();
        PropertyValuesHolder propertyValuesHolder = PropertyValuesHolder.ofInt(propertyName, values);
        final ObjectAnimator colorAnimator = ObjectAnimator.ofPropertyValuesHolder(object,
            propertyValuesHolder);
        colorAnimator.setDuration(1000);
        colorAnimator.setRepeatCount(1);
        colorAnimator.setRepeatMode(ValueAnimator.REVERSE);
        mActivity.runOnUiThread(new Runnable() {
            public void run() {
                colorAnimator.start();
            }
        });
        getInstrumentation().waitForIdleSync();
        startAnimation(mObjectAnimator, colorAnimator);
        Thread.sleep(100);
        Integer i = (Integer) colorAnimator.getAnimatedValue();
        //We are going from less negative value to a more negative value
        assertTrue(i.intValue() <= startColor);
        assertTrue(endColor <= i.intValue());
    }

    public void testGetPropertyName() throws Throwable {
        Object object = mActivity.view.newBall;
        String propertyName = "backgroundColor";
        int startColor = mActivity.view.RED;
        int endColor = mActivity.view.BLUE;
        Object[] values = {new Integer(startColor), new Integer(endColor)};
        ArgbEvaluator evaluator = new ArgbEvaluator();
        ObjectAnimator colorAnimator = ObjectAnimator.ofObject(object, propertyName,
                evaluator, values);
        String actualPropertyName = colorAnimator.getPropertyName();
        assertEquals(propertyName, actualPropertyName);
    }

    public void testSetFloatValues() throws Throwable {
        Object object = mActivity.view.newBall;
        String property = "y";
        float startY = mActivity.mStartY;
        float endY = mActivity.mStartY + mActivity.mDeltaY;
        float[] values = {startY, endY};
        ObjectAnimator objAnimator = new ObjectAnimator();
        objAnimator.setTarget(object);
        objAnimator.setPropertyName(property);
        objAnimator.setFloatValues(values);
        objAnimator.setDuration(mDuration);
        objAnimator.setRepeatCount(ValueAnimator.INFINITE);
        objAnimator.setInterpolator(new AccelerateInterpolator());
        objAnimator.setRepeatMode(ValueAnimator.REVERSE);
        startAnimation(objAnimator);
        Thread.sleep(100);
        float y = mActivity.view.newBall.getY();
        assertTrue( y >= startY);
        assertTrue( y <= endY);
    }

    public void testGetTarget() throws Throwable {
        Object object = mActivity.view.newBall;
        String propertyName = "backgroundColor";
        int startColor = mActivity.view.RED;
        int endColor = mActivity.view.BLUE;
        Object[] values = {new Integer(startColor), new Integer(endColor)};
        ArgbEvaluator evaluator = new ArgbEvaluator();
        ObjectAnimator colorAnimator = ObjectAnimator.ofObject(object, propertyName,
                evaluator, values);
        Object target = colorAnimator.getTarget();
        assertEquals(object, target);
    }

    public void testClone() throws Throwable {
        Object object = mActivity.view.newBall;
        String property = "y";
        float startY = mActivity.mStartY;
        float endY = mActivity.mStartY + mActivity.mDeltaY;
        Interpolator interpolator = new AccelerateInterpolator();
        ObjectAnimator objAnimator = ObjectAnimator.ofFloat(object, property, startY, endY);
        objAnimator.setDuration(mDuration);
        objAnimator.setRepeatCount(ValueAnimator.INFINITE);
        objAnimator.setInterpolator(interpolator);
        objAnimator.setRepeatMode(ValueAnimator.REVERSE);
        ObjectAnimator cloneAnimator = objAnimator.clone();

        assertEquals(mDuration, cloneAnimator.getDuration());
        assertEquals(ValueAnimator.INFINITE, cloneAnimator.getRepeatCount());
        assertEquals(ValueAnimator.REVERSE, cloneAnimator.getRepeatMode());
        assertEquals(object, cloneAnimator.getTarget());
        assertEquals(property, cloneAnimator.getPropertyName());
        assertEquals(interpolator, cloneAnimator.getInterpolator());
    }

    public void testIsStarted() throws Throwable {
        Object object = mActivity.view.newBall;
        String property = "y";
        float startY = mActivity.mStartY;
        float endY = mActivity.mStartY + mActivity.mDeltaY;
        Interpolator interpolator = new AccelerateInterpolator();
        ObjectAnimator objAnimator = ObjectAnimator.ofFloat(object, property, startY, endY);
        objAnimator.setDuration(mDuration);
        objAnimator.setRepeatCount(ValueAnimator.INFINITE);
        objAnimator.setInterpolator(interpolator);
        objAnimator.setRepeatMode(ValueAnimator.REVERSE);
        startAnimation(objAnimator);
        Thread.sleep(100);
        assertTrue(objAnimator.isStarted());
        Thread.sleep(100);
    }

    public void testSetStartEndValues() throws Throwable {
        final float startValue = 100, endValue = 500;
        final AnimTarget target = new AnimTarget();
        final ObjectAnimator anim1 = ObjectAnimator.ofFloat(target, "testValue", 0);
        target.setTestValue(startValue);
        anim1.setupStartValues();
        target.setTestValue(endValue);
        anim1.setupEndValues();
        runTestOnUiThread(new Runnable() {
            @Override
            public void run() {
                anim1.start();
                assertEquals(startValue, (Float) anim1.getAnimatedValue());
                anim1.setCurrentFraction(1);
                assertEquals(endValue, (Float) anim1.getAnimatedValue());
                anim1.cancel();
            }
        });

        final Property property = AnimTarget.TEST_VALUE;
        final ObjectAnimator anim2 = ObjectAnimator.ofFloat(target, AnimTarget.TEST_VALUE, 0);
        target.setTestValue(startValue);
        final float startValueExpected = (Float) property.get(target);
        anim2.setupStartValues();
        target.setTestValue(endValue);
        final float endValueExpected = (Float) property.get(target);
        anim2.setupEndValues();
        runTestOnUiThread(new Runnable() {
            @Override
            public void run() {
                anim2.start();
                assertEquals(startValueExpected, (Float) anim2.getAnimatedValue());
                anim2.setCurrentFraction(1);
                assertEquals(endValueExpected, (Float) anim2.getAnimatedValue());
                anim2.cancel();
            }
        });

        // This is a test that ensures that the values set on a Property-based animator
        // are determined by the property, not by the setter/getter of the target object
        final Property doubler = AnimTarget.TEST_DOUBLING_VALUE;
        final ObjectAnimator anim3 = ObjectAnimator.ofFloat(target,
                doubler, 0);
        target.setTestValue(startValue);
        final float startValueExpected3 = (Float) doubler.get(target);
        anim3.setupStartValues();
        target.setTestValue(endValue);
        final float endValueExpected3 = (Float) doubler.get(target);
        anim3.setupEndValues();
        runTestOnUiThread(new Runnable() {
            @Override
            public void run() {
                anim3.start();
                assertEquals(startValueExpected3, (Float) anim3.getAnimatedValue());
                anim3.setCurrentFraction(1);
                assertEquals(endValueExpected3, (Float) anim3.getAnimatedValue());
                anim3.cancel();
            }
        });
    }

    static class AnimTarget {
        private float mTestValue = 0;

        public void setTestValue(float value) {
            mTestValue = value;
        }

        public float getTestValue() {
            return mTestValue;
        }

        public static final Property<AnimTarget, Float> TEST_VALUE =
                new Property<AnimTarget, Float>(Float.class, "testValue") {
                    @Override
                    public void set(AnimTarget object, Float value) {
                        object.setTestValue(value);
                    }

                    @Override
                    public Float get(AnimTarget object) {
                        return object.getTestValue();
                    }
                };
        public static final Property<AnimTarget, Float> TEST_DOUBLING_VALUE =
                new Property<AnimTarget, Float>(Float.class, "testValue") {
                    @Override
                    public void set(AnimTarget object, Float value) {
                        object.setTestValue(value);
                    }

                    @Override
                    public Float get(AnimTarget object) {
                        // purposely different from getTestValue, to verify that properties
                        // are independent of setters/getters
                        return object.getTestValue() * 2;
                    }
                };
    }

    private void startAnimation(final ObjectAnimator mObjectAnimator) throws Throwable {
        Thread mAnimationRunnable = new Thread() {
            public void run() {
                mActivity.startAnimation(mObjectAnimator);
            }
        };
        this.runTestOnUiThread(mAnimationRunnable);
    }
    private void startAnimation(final ObjectAnimator mObjectAnimator, final
            ObjectAnimator colorAnimator) throws Throwable {
        Thread mAnimationRunnable = new Thread() {
            public void run() {
                mActivity.startAnimation(mObjectAnimator, colorAnimator);
            }
        };
        this.runTestOnUiThread(mAnimationRunnable);
    }
}
