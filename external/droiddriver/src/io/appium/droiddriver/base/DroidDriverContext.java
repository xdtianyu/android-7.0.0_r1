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

package io.appium.droiddriver.base;

import android.app.Instrumentation;

import java.util.Map;
import java.util.WeakHashMap;

import io.appium.droiddriver.finders.ByXPath;

/**
 * Internal helper for DroidDriver implementation.
 */
public class DroidDriverContext<R, E extends BaseUiElement<R, E>> {
  private final Instrumentation instrumentation;
  private final BaseDroidDriver<R, E> driver;
  private final Map<R, E> map;

  public DroidDriverContext(Instrumentation instrumentation, BaseDroidDriver<R, E> driver) {
    this.instrumentation = instrumentation;
    this.driver = driver;
    map = new WeakHashMap<R, E>();
  }

  public Instrumentation getInstrumentation() {
    return instrumentation;
  }

  public BaseDroidDriver<R, E> getDriver() {
    return driver;
  }

  public E getElement(R rawElement, E parent) {
    E element = map.get(rawElement);
    if (element == null) {
      element = driver.newUiElement(rawElement, parent);
      map.put(rawElement, element);
    }
    return element;
  }

  public E newRootElement(R rawRoot) {
    clearData();
    return getElement(rawRoot, null /* parent */);
  }

  private void clearData() {
    map.clear();
    ByXPath.clearData();
  }
}
