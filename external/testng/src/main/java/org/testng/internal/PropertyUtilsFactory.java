/*
 * Copyright 2016 The Android Open Source Project
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

package org.testng.internal;

import java.lang.reflect.Constructor;

/**
 * Factory for IPropertyUtils that returns a concrete instance.
 */
public class PropertyUtilsFactory {

  /**
   * Tries to make a real PropertyUtils, if the platform supports it. Otherwise creates
   * a mock PropertyUtils that throws UnsupportedOperationException if any method is called on it.
   */
  public static IPropertyUtils newInstance() {
    try {
      Class<?> propertyUtilsClass = Class.forName("org.testng.internal.PropertyUtils");
      Constructor<?> constructor = propertyUtilsClass.getConstructor();
      try {
        return (IPropertyUtils)constructor.newInstance();
      }
      catch (Exception e) {
        // Impossible: Constructor should not be failing.
        throw new AssertionError(e);
      }
    } catch (ClassNotFoundException e) {
      // OK: On a platform where java beans are not supported
      return new PropertyUtilsMock();
    } catch (NoSuchMethodException e) {
      // Impossible. PropertyUtils should have a 0-arg constructor.
      throw new AssertionError(e);
    }
  }

  private PropertyUtilsFactory() {}
}
