/*
 * Copyright (C) 2013 DroidDriver committers
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

package io.appium.droiddriver.helpers;

import android.app.Activity;
import android.test.ActivityInstrumentationTestCase2;
import android.test.ActivityTestCase;

import java.lang.reflect.Field;
import java.lang.reflect.Modifier;

/**
 * Fixes bugs in {@link ActivityInstrumentationTestCase2}.
 */
public abstract class D2ActivityInstrumentationTestCase2<T extends Activity> extends
    ActivityInstrumentationTestCase2<T> {
  protected D2ActivityInstrumentationTestCase2(Class<T> activityClass) {
    super(activityClass);
  }

  /**
   * Fixes a bug in {@link ActivityTestCase#scrubClass} that causes
   * NullPointerException if your leaf-level test class declares static fields.
   * This is <a href="https://code.google.com/p/android/issues/detail?id=4244">a
   * known bug</a> that has been fixed in ICS Android release. But it still
   * exists on devices older than ICS. If your test class extends this class, it
   * can work on older devices.
   * <p>
   * In addition to the official fix in ICS and beyond, which skips
   * {@code final} fields, the fix below also skips {@code static} fields, which
   * should be the expectation of Java programmers.
   * </p>
   */
  @Override
  protected void scrubClass(final Class<?> testCaseClass) throws IllegalAccessException {
    final Field[] fields = getClass().getDeclaredFields();
    for (Field field : fields) {
      final Class<?> fieldClass = field.getDeclaringClass();
      if (testCaseClass.isAssignableFrom(fieldClass) && !field.getType().isPrimitive()
          && !Modifier.isFinal(field.getModifiers()) && !Modifier.isStatic(field.getModifiers())) {
        try {
          field.setAccessible(true);
          field.set(this, null);
        } catch (Exception e) {
          android.util.Log.d("TestCase", "Error: Could not nullify field!");
        }

        if (field.get(this) != null) {
          android.util.Log.d("TestCase", "Error: Could not nullify field!");
        }
      }
    }
  }
}
