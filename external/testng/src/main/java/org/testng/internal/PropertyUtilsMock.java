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

/**
 * Utility class for setting JavaBeans-style properties on instances.
 *
 * <p>Mock version for platforms that don't support java beans</p>.
 */
public class PropertyUtilsMock implements IPropertyUtils {
  public void setProperty(Object instance, String name, String value) {
    throw new UnsupportedOperationException();
  }

  public Class getPropertyType(Class instanceClass, String propertyName) {
    throw new UnsupportedOperationException();
  }

  public void setPropertyRealValue(Object instance, String name, Object value) {
    throw new UnsupportedOperationException();
  }
}
