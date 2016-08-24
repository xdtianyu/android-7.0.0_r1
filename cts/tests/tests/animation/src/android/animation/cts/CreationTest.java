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

package android.animation.cts;

import android.animation.AnimatorInflater;
import android.animation.ObjectAnimator;
import android.animation.PropertyValuesHolder;
import android.animation.ValueAnimator;
import android.os.Debug;
import android.test.ActivityInstrumentationTestCase2;
import android.test.UiThreadTest;

import android.animation.cts.R;

public class CreationTest extends ActivityInstrumentationTestCase2<ButtonViewActivity> {

    private ButtonViewActivity mActivity;

    public CreationTest() {
        super(ButtonViewActivity.class);
    }

    @Override
    protected void setUp() throws Exception {
        super.setUp();
        setActivityInitialTouchMode(false);
        mActivity = getActivity();
    }

    @UiThreadTest
    public void testValueAnimatorCreation() {
        ValueAnimator animator = ValueAnimator.ofFloat(0, 1);
        verifyValues(animator, 0, 1);
    }

    @UiThreadTest
    public void testValueAnimatorResourceCreation() {
        ValueAnimator animator = (ValueAnimator)
                AnimatorInflater.loadAnimator(mActivity, R.animator.value_animator);
        verifyValues(animator, 0, 1);
    }

    @UiThreadTest
    public void testValueAnimatorPvh1() {
        ValueAnimator animator = (ValueAnimator)
                AnimatorInflater.loadAnimator(mActivity, R.animator.value_animator_pvh1);
        verifyValues(animator, 0, 1);
    }

    @UiThreadTest
    public void testValueAnimatorPvh2() {
        ValueAnimator animator = (ValueAnimator)
                AnimatorInflater.loadAnimator(mActivity, R.animator.value_animator_pvh2);
        verifyValues(animator, 0, 1);
    }

    @UiThreadTest
    public void testValueAnimatorPvhKf1() {
        ValueAnimator animator = (ValueAnimator)
                AnimatorInflater.loadAnimator(mActivity, R.animator.value_animator_pvh_kf1);
        verifyValues(animator, 0, 1);
    }

    @UiThreadTest
    public void testValueAnimatorPvhKf2() {
        ValueAnimator animator = (ValueAnimator)
                AnimatorInflater.loadAnimator(mActivity, R.animator.value_animator_pvh_kf2);
        verifyValues(animator, 0, 1);
    }

    @UiThreadTest
    public void testValueAnimatorPvhKf3() {
        ValueAnimator animator = (ValueAnimator)
                AnimatorInflater.loadAnimator(mActivity, R.animator.value_animator_pvh_kf3);
        verifyValues(animator, 0, .2f, 1);
    }

    @UiThreadTest
    public void testValueAnimatorPvhKf4() {
        ValueAnimator animator = (ValueAnimator)
                AnimatorInflater.loadAnimator(mActivity, R.animator.value_animator_pvh_kf4);
        verifyValues(animator, 0, .2f, 1);
    }

    @UiThreadTest
    public void testObjectAnimator() {
        ObjectAnimator animator = (ObjectAnimator)
                AnimatorInflater.loadAnimator(mActivity, R.animator.object_animator);
        animator.setTarget(new DummyAnimatorTarget());
        verifyValues(animator, "x", 0, 1);
    }

    @UiThreadTest
    public void testObjectAnimatorPvh1() {
        ObjectAnimator animator = (ObjectAnimator)
                AnimatorInflater.loadAnimator(mActivity, R.animator.object_animator_pvh1);
        animator.setTarget(new DummyAnimatorTarget());
        verifyValues(animator, "x", 0, 1);
        verifyValues(animator, "y", 10, 11);
    }

    @UiThreadTest
    public void testObjectAnimatorPvhKf1() {
        ObjectAnimator animator = (ObjectAnimator)
                AnimatorInflater.loadAnimator(mActivity, R.animator.object_animator_pvh_kf1);
        animator.setTarget(new DummyAnimatorTarget());
        verifyValues(animator, "x", 0, 1);
        verifyValues(animator, "y", 10, 11);
    }

    class DummyAnimatorTarget {
        public float getX() {
            return 0;
        }

        public void setX(float x) {
        }

        public float getY() {
            return 0;
        }

        public void setY(float y) {
        }
    }

    private void assertRoughlyEqual(float checkValue, float correctValue) {
        // use epsilon for float compares
        final float epsilon = .0001f;
        assertTrue(checkValue > correctValue - epsilon && checkValue < correctValue + epsilon);
    }

    private void verifyValues(ValueAnimator animator, float... values) {
        animator.setCurrentFraction(0);
        assertRoughlyEqual((Float) animator.getAnimatedValue(), values[0]);
        for (int i = 1; i < values.length - 1; ++i) {
            animator.setCurrentFraction((float) i / (values.length - 1));
            assertRoughlyEqual((Float) animator.getAnimatedValue(), values[i]);
        }
        animator.setCurrentFraction(1);
        assertRoughlyEqual((Float) animator.getAnimatedValue(), values[values.length - 1]);
    }

    private void verifyValues(ObjectAnimator animator, String propertyName, float... values) {
        animator.setCurrentFraction(0);
        assertRoughlyEqual((Float) animator.getAnimatedValue(propertyName), values[0]);
        for (int i = 1; i < values.length - 1; ++i) {
            animator.setCurrentFraction((float) i / (values.length - 1));
            assertRoughlyEqual((Float) animator.getAnimatedValue(propertyName), values[i]);
        }
        animator.setCurrentFraction(1);
        assertRoughlyEqual((Float) animator.getAnimatedValue(propertyName),
                values[values.length - 1]);
    }
}
