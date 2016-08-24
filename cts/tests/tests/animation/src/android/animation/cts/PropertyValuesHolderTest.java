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

import android.animation.Animator;
import android.animation.AnimatorListenerAdapter;
import android.animation.ArgbEvaluator;
import android.animation.Keyframe;
import android.animation.ObjectAnimator;
import android.animation.PropertyValuesHolder;
import android.animation.ValueAnimator;
import android.graphics.drawable.ShapeDrawable;
import android.test.ActivityInstrumentationTestCase2;
import android.util.Property;
import android.view.View;
import android.view.animation.AccelerateInterpolator;

import java.util.concurrent.CountDownLatch;
import java.util.concurrent.TimeUnit;

public class PropertyValuesHolderTest extends
        ActivityInstrumentationTestCase2<AnimationActivity> {
    private AnimationActivity mActivity;
    private Animator mAnimator;
    private long mDuration = 1000;
    private float mStartY;
    private float mEndY;
    private Object mObject;
    private String mProperty;

    public PropertyValuesHolderTest() {
        super(AnimationActivity.class);
    }

    public void setUp() throws Exception {
         super.setUp();
         setActivityInitialTouchMode(false);
         mActivity = getActivity();
         mAnimator = mActivity.createAnimatorWithDuration(mDuration);
         mProperty = "y";
         mStartY = mActivity.mStartY;
         mEndY = mActivity.mStartY + mActivity.mDeltaY;
         mObject = mActivity.view.newBall;
    }

    public void testGetPropertyName() {
        float[] values = {mStartY, mEndY};
        PropertyValuesHolder pVHolder = PropertyValuesHolder.ofFloat(mProperty, values);
        assertEquals(mProperty, pVHolder.getPropertyName());
    }

    public void testSetPropertyName() {
        float[] values = {mStartY, mEndY};
        PropertyValuesHolder pVHolder = PropertyValuesHolder.ofFloat("", values);
        pVHolder.setPropertyName(mProperty);
        assertEquals(mProperty, pVHolder.getPropertyName());
    }

    public void testClone() {
        float[] values = {mStartY, mEndY};
        PropertyValuesHolder pVHolder = PropertyValuesHolder.ofFloat(mProperty, values);
        PropertyValuesHolder cloneHolder = pVHolder.clone();
        assertEquals(pVHolder.getPropertyName(), cloneHolder.getPropertyName());
    }

    public void testSetValues() throws Throwable {
        float[] dummyValues = {100, 150};
        float[] values = {mStartY, mEndY};
        PropertyValuesHolder pVHolder = PropertyValuesHolder.ofFloat(mProperty, dummyValues);
        pVHolder.setFloatValues(values);

        ObjectAnimator objAnimator = ObjectAnimator.ofPropertyValuesHolder(mObject,pVHolder);
        assertTrue(objAnimator != null);
        setAnimatorProperties(objAnimator);

        startAnimation(objAnimator);
        assertTrue(objAnimator != null);
        float[] yArray = getYPosition();
        assertResults(yArray, mStartY, mEndY);
    }

    private ObjectAnimator createAnimator(Keyframe... keyframes) {
        PropertyValuesHolder pVHolder = PropertyValuesHolder.ofKeyframe(mProperty, keyframes);
        ObjectAnimator objAnimator = ObjectAnimator.ofPropertyValuesHolder(mObject,pVHolder);
        objAnimator.setDuration(mDuration);
        objAnimator.setInterpolator(new AccelerateInterpolator());
        return objAnimator;
    }

    private void waitUntilFinished(ObjectAnimator objectAnimator, long timeoutMilliseconds)
            throws InterruptedException {
        final CountDownLatch latch = new CountDownLatch(1);
        objectAnimator.addListener(new AnimatorListenerAdapter() {
            @Override
            public void onAnimationEnd(Animator animation) {
                super.onAnimationEnd(animation);
                latch.countDown();
            }
        });
        latch.await(timeoutMilliseconds, TimeUnit.MILLISECONDS);
        getInstrumentation().waitForIdleSync();
    }

    private void setTarget(final Animator animator, final Object target) throws Throwable {
        runTestOnUiThread(new Runnable() {
            @Override
            public void run() {
                animator.setTarget(target);
            }
        });
    }

    private void startSingleAnimation(final Animator animator) throws Throwable {
        runTestOnUiThread(new Runnable() {
            @Override
            public void run() {
                mActivity.startSingleAnimation(animator);
            }
        });
    }

    public void testResetValues() throws Throwable {
        final float initialY = mActivity.view.newBall.getY();
        Keyframe emptyKeyframe1 = Keyframe.ofFloat(.0f);
        ObjectAnimator objAnimator1 = createAnimator(emptyKeyframe1, Keyframe.ofFloat(1f, 100f));
        startSingleAnimation(objAnimator1);
        assertTrue("Keyframe should be assigned a value", emptyKeyframe1.hasValue());
        assertEquals("Keyframe should get the value from the target", emptyKeyframe1.getValue(),
                initialY);
        waitUntilFinished(objAnimator1, mDuration * 2);
        assertEquals(100f, mActivity.view.newBall.getY());
        startSingleAnimation(objAnimator1);
        waitUntilFinished(objAnimator1, mDuration * 2);

        // run another ObjectAnimator that will move the Y value to something else
        Keyframe emptyKeyframe2 = Keyframe.ofFloat(.0f);
        ObjectAnimator objAnimator2 = createAnimator(emptyKeyframe2, Keyframe.ofFloat(1f, 200f));
        startSingleAnimation(objAnimator2);
        assertTrue("Keyframe should be assigned a value", emptyKeyframe2.hasValue());
        assertEquals("Keyframe should get the value from the target", emptyKeyframe2.getValue(), 100f);
        waitUntilFinished(objAnimator2, mDuration * 2);
        assertEquals(200f, mActivity.view.newBall.getY());

        // re-run first object animator. since its target did not change, it should have the same
        // start value for kf1
        startSingleAnimation(objAnimator1);
        assertEquals(emptyKeyframe1.getValue(), initialY);
        waitUntilFinished(objAnimator1, mDuration * 2);

        Keyframe fullKeyframe = Keyframe.ofFloat(.0f, 333f);
        ObjectAnimator objAnimator3 = createAnimator(fullKeyframe, Keyframe.ofFloat(1f, 500f));
        startSingleAnimation(objAnimator3);
        assertEquals("When keyframe has value, should not be assigned from the target object",
                fullKeyframe.getValue(), 333f);
        waitUntilFinished(objAnimator3, mDuration * 2);

        // now, null out the target of the first animator
        float updatedY = mActivity.view.newBall.getY();
        setTarget(objAnimator1, null);
        startSingleAnimation(objAnimator1);
        assertTrue("Keyframe should get a value", emptyKeyframe1.hasValue());
        assertEquals("Keyframe should get the updated Y value", emptyKeyframe1.getValue(), updatedY);
        waitUntilFinished(objAnimator1, mDuration * 2);
        assertEquals("Animation should run as expected", 100f, mActivity.view.newBall.getY());

        // now, reset the target of the fully defined animation.
        setTarget(objAnimator3, null);
        startSingleAnimation(objAnimator3);
        assertEquals("When keyframe is fully defined, its value should not change when target is"
                + " reset", fullKeyframe.getValue(), 333f);
        waitUntilFinished(objAnimator3, mDuration * 2);

        // run the other one to change Y value
        startSingleAnimation(objAnimator2);
        waitUntilFinished(objAnimator2, mDuration * 2);
        // now, set another target w/ the same View type. it should still reset
        ShapeHolder view = new ShapeHolder(new ShapeDrawable());
        updatedY = mActivity.view.newBall.getY();
        setTarget(objAnimator1, view);
        startSingleAnimation(objAnimator1);
        assertTrue("Keyframe should get a value when target is set to another view of the same"
                + " class", emptyKeyframe1.hasValue());
        assertEquals("Keyframe should get the updated Y value when target is set to another view"
                + " of the same class", emptyKeyframe1.getValue(), updatedY);
        waitUntilFinished(objAnimator1, mDuration * 2);
        assertEquals("Animation should run as expected", 100f, mActivity.view.newBall.getY());
    }

    public void testOffloat() throws Throwable {
        float[] values = {mStartY, mEndY};
        PropertyValuesHolder pVHolder = PropertyValuesHolder.ofFloat(mProperty, values);
        assertNotNull(pVHolder);
        ObjectAnimator objAnimator = ObjectAnimator.ofPropertyValuesHolder(mObject,pVHolder);
        assertTrue(objAnimator != null);

        setAnimatorProperties(objAnimator);
        startAnimation(objAnimator);
        assertTrue(objAnimator != null);
        float[] yArray = getYPosition();
        assertResults(yArray, mStartY, mEndY);
    }

    public void testOfFloat_Property() throws Throwable {
        float[] values = {mStartY, mEndY};
        ShapeHolderYProperty property=new ShapeHolderYProperty(ShapeHolder.class.getClass(),"y");
        property.setObject(mObject);
        PropertyValuesHolder pVHolder = PropertyValuesHolder.ofFloat(property, values);
        assertNotNull(pVHolder);
        ObjectAnimator objAnimator = ObjectAnimator.ofPropertyValuesHolder(mObject,pVHolder);
        assertTrue(objAnimator != null);

        setAnimatorProperties(objAnimator);
        startAnimation(objAnimator);
        assertTrue(objAnimator != null);
        float[] yArray = getYPosition();
        assertResults(yArray, mStartY, mEndY);
    }

    public void testOfInt() throws Throwable {
        int start = 0;
        int end = 10;
        int[] values = {start, end};
        PropertyValuesHolder pVHolder = PropertyValuesHolder.ofInt(mProperty, values);
        assertNotNull(pVHolder);
        final ObjectAnimator objAnimator = ObjectAnimator.ofPropertyValuesHolder(mObject,pVHolder);
        assertTrue(objAnimator != null);
        setAnimatorProperties(objAnimator);
        this.runTestOnUiThread(new Runnable(){
            public void run() {
                objAnimator.start();
            }
        });
        Thread.sleep(1000);
        assertTrue(objAnimator.isRunning());
        Integer animatedValue = (Integer) objAnimator.getAnimatedValue();
        assertTrue(animatedValue >= start);
        assertTrue(animatedValue <= end);
    }

    public void testOfInt_Property() throws Throwable{
        Object object = mActivity.view;
        String property = "backgroundColor";
        int startColor = mActivity.view.RED;
        int endColor = mActivity.view.BLUE;
        int values[] = {startColor, endColor};

        ViewColorProperty colorProperty=new ViewColorProperty(Integer.class.getClass(),property);
        colorProperty.setObject(object);
        PropertyValuesHolder pVHolder = PropertyValuesHolder.ofInt(colorProperty, values);
        assertNotNull(pVHolder);

        ObjectAnimator colorAnimator = ObjectAnimator.ofPropertyValuesHolder(object,pVHolder);
        colorAnimator.setDuration(1000);
        colorAnimator.setEvaluator(new ArgbEvaluator());
        colorAnimator.setRepeatCount(ValueAnimator.INFINITE);
        colorAnimator.setRepeatMode(ValueAnimator.REVERSE);

        ObjectAnimator objectAnimator = (ObjectAnimator) mActivity.createAnimatorWithDuration(
            mDuration);
        startAnimation(objectAnimator, colorAnimator);
        Thread.sleep(1000);
        Integer i = (Integer) colorAnimator.getAnimatedValue();
        //We are going from less negative value to a more negative value
        assertTrue(i.intValue() <= startColor);
        assertTrue(endColor <= i.intValue());
    }

    public void testSetProperty() throws Throwable {
        float[] values = {mStartY, mEndY};
        ShapeHolderYProperty property=new ShapeHolderYProperty(ShapeHolder.class.getClass(),"y");
        property.setObject(mObject);
        PropertyValuesHolder pVHolder = PropertyValuesHolder.ofFloat("", values);
        pVHolder.setProperty(property);
        ObjectAnimator objAnimator = ObjectAnimator.ofPropertyValuesHolder(mObject,pVHolder);
        setAnimatorProperties(objAnimator);
        startAnimation(objAnimator);
        assertTrue(objAnimator != null);
        float[] yArray = getYPosition();
        assertResults(yArray, mStartY, mEndY);
    }

    class ShapeHolderYProperty extends Property {
        private ShapeHolder shapeHolder ;
        private Class type = Float.class.getClass();
        private String name = "y";
        @SuppressWarnings("unchecked")
        public ShapeHolderYProperty(Class type, String name) throws Exception {
            super(Float.class, name );
            if(!( type.equals(this.type) || ( name.equals(this.name))) ){
                throw new Exception("Type or name provided does not match with " +
                        this.type.getName() + " or " + this.name);
            }
        }

        public void setObject(Object object){
            shapeHolder = (ShapeHolder) object;
        }

        @Override
        public Object get(Object object) {
            return shapeHolder;
        }

        @Override
        public String getName() {
            return "y";
        }

        @Override
        public Class getType() {
            return super.getType();
        }

        @Override
        public boolean isReadOnly() {
            return false;
        }

        @Override
        public void set(Object object, Object value) {
            shapeHolder.setY((Float)value);
        }

    }

    class ViewColorProperty extends Property {
        private View view ;
        private Class type = Integer.class.getClass();
        private String name = "backgroundColor";
        @SuppressWarnings("unchecked")
        public ViewColorProperty(Class type, String name) throws Exception {
            super(Integer.class, name );
            if(!( type.equals(this.type) || ( name.equals(this.name))) ){
                throw new Exception("Type or name provided does not match with " +
                        this.type.getName() + " or " + this.name);
            }
        }

        public void setObject(Object object){
            view = (View) object;
        }

        @Override
        public Object get(Object object) {
            return view;
        }

        @Override
        public String getName() {
            return name;
        }

        @Override
        public Class getType() {
            return super.getType();
        }

        @Override
        public boolean isReadOnly() {
            return false;
        }

        @Override
        public void set(Object object, Object value) {
            view.setBackgroundColor((Integer)value);
        }
    }

    private void setAnimatorProperties(ObjectAnimator objAnimator) {
        objAnimator.setDuration(mDuration);
        objAnimator.setRepeatCount(ValueAnimator.INFINITE);
        objAnimator.setInterpolator(new AccelerateInterpolator());
        objAnimator.setRepeatMode(ValueAnimator.REVERSE);
    }

    public float[] getYPosition() throws Throwable{
        float[] yArray = new float[3];
        for(int i = 0; i < 3; i++) {
            float y = mActivity.view.newBall.getY();
            yArray[i] = y;
            Thread.sleep(300);
        }
        return yArray;
    }

    public void assertResults(float[] yArray,float startY, float endY) {
        for(int i = 0; i < 3; i++){
            float y = yArray[i];
            assertTrue(y >= startY);
            assertTrue(y <= endY);
            if(i < 2) {
                float yNext = yArray[i+1];
                assertTrue(y != yNext);
            }
        }
    }

    private void startAnimation(final Animator animator) throws Throwable {
        this.runTestOnUiThread(new Runnable() {
            public void run() {
                mActivity.startAnimation(animator);
            }
        });
    }

    private void startAnimation(final ObjectAnimator mObjectAnimator,
            final ObjectAnimator colorAnimator) throws Throwable {
        Thread mAnimationRunnable = new Thread() {
            public void run() {
                mActivity.startAnimation(mObjectAnimator, colorAnimator);
            }
        };
        this.runTestOnUiThread(mAnimationRunnable);
    }
}

