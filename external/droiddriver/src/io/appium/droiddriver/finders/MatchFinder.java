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

package io.appium.droiddriver.finders;

import android.util.Log;

import io.appium.droiddriver.UiElement;
import io.appium.droiddriver.exceptions.ElementNotFoundException;
import io.appium.droiddriver.util.Logs;

/**
 * Traverses the UiElement tree and returns the first UiElement satisfying
 * {@link #predicate}.
 */
public class MatchFinder implements Finder {
  protected final Predicate<? super UiElement> predicate;

  public MatchFinder(Predicate<? super UiElement> predicate) {
    if (predicate == null) {
      this.predicate = Predicates.any();
    } else {
      this.predicate = predicate;
    }
  }

  @Override
  public String toString() {
    return predicate.toString();
  }

  @Override
  public UiElement find(UiElement context) {
    if (matches(context)) {
      Logs.log(Log.INFO, "Found match: " + context);
      return context;
    }
    for (UiElement child : context.getChildren(UiElement.VISIBLE)) {
      try {
        return find(child);
      } catch (ElementNotFoundException enfe) {
        // Do nothing. Continue searching.
      }
    }
    throw new ElementNotFoundException(this);
  }

  /**
   * Returns true if the {@code element} matches this finder. This can be used
   * to test the exact match of {@code element} when this finder is used in
   * {@link By#anyOf(MatchFinder...)}.
   *
   * @param element The element to validate against
   * @return true if the element matches
   */
  public final boolean matches(UiElement element) {
    return predicate.apply(element);
  }
}
