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

package android.util.cts;

import android.graphics.Point;
import android.util.FloatProperty;
import android.util.IntProperty;
import android.util.Property;
import junit.framework.TestCase;

public class PropertyTest extends TestCase {

    float mFloatValue = -1;
    int mIntValue = -2;
    Point mPointValue = new Point(-3, -4);

    public void testProperty() throws Exception {
        float testFloatValue = 5;
        Point testPointValue = new Point(10, 20);

        assertFalse(getFloatProp() == testFloatValue);
        assertFalse(getPointProp().equals(testPointValue));
        assertEquals(RAW_FLOAT_PROP.get(this), getFloatProp());
        assertEquals(RAW_POINT_PROP.get(this), getPointProp());

        RAW_FLOAT_PROP.set(this, testFloatValue);
        assertEquals(RAW_FLOAT_PROP.get(this), mFloatValue);

        RAW_POINT_PROP.set(this, testPointValue);
        assertEquals(RAW_POINT_PROP.get(this), testPointValue);
    }

    public void testFloatProperty() throws Exception {
        float testFloatValue = 5;

        assertFalse(getFloatProp() == testFloatValue);
        assertEquals(FLOAT_PROP.get(this), getFloatProp());

        FLOAT_PROP.set(this, testFloatValue);
        assertEquals(FLOAT_PROP.get(this), testFloatValue);
    }

    public void testIntProperty() throws Exception {
        int testIntValue = 5;

        assertFalse(getIntProp() == testIntValue);
        assertEquals(INT_PROP.get(this).intValue(), getIntProp());

        INT_PROP.set(this, testIntValue);
        assertEquals(INT_PROP.get(this).intValue(), testIntValue);
    }

    // Utility methods to get/set instance values. Used by Property classes below.

    private void setFloatProp(float value) {
        mFloatValue = value;
    }

    private float getFloatProp() {
        return mFloatValue;
    }

    private void setIntProp(int value) {
        mIntValue = value;
    }

    private int getIntProp() {
        return mIntValue;
    }

    private void setPointProp(Point value) {
        mPointValue = value;
    }

    private Point getPointProp() {
        return mPointValue;
    }

    // Properties. RAW subclass from the generic Property class, the others subclass from
    // the primtive-friendly IntProperty and FloatProperty subclasses.

    public static final Property<PropertyTest, Point> RAW_POINT_PROP =
            new Property<PropertyTest, Point>(Point.class, "rawPoint") {
                @Override
                public void set(PropertyTest object, Point value) {
                    object.setPointProp(value);
                }

                @Override
                public Point get(PropertyTest object) {
                    return object.getPointProp();
                }
            };

    public static final Property<PropertyTest, Float> RAW_FLOAT_PROP =
            new Property<PropertyTest, Float>(Float.class, "rawFloat") {
                @Override
                public void set(PropertyTest object, Float value) {
                    object.setFloatProp(value);
                }

                @Override
                public Float get(PropertyTest object) {
                    return object.getFloatProp();
                }
            };

    public static final Property<PropertyTest, Float> FLOAT_PROP =
            new FloatProperty<PropertyTest>("float") {

                @Override
                public void setValue(PropertyTest object, float value) {
                    object.setFloatProp(value);
                }

                @Override
                public Float get(PropertyTest object) {
                    return object.getFloatProp();
                }
            };

    public static final Property<PropertyTest, Integer> INT_PROP =
            new IntProperty<PropertyTest>("int") {

                @Override
                public void setValue(PropertyTest object, int value) {
                    object.setIntProp(value);
                }

                @Override
                public Integer get(PropertyTest object) {
                    return object.getIntProp();
                }
            };
}
