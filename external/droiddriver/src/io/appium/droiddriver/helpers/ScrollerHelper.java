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

import io.appium.droiddriver.DroidDriver;
import io.appium.droiddriver.UiElement;
import io.appium.droiddriver.exceptions.ElementNotFoundException;
import io.appium.droiddriver.finders.Finder;
import io.appium.droiddriver.scroll.Direction.PhysicalDirection;
import io.appium.droiddriver.scroll.Scroller;

/**
 * Helper for Scroller.
 */
public class ScrollerHelper {
  private final DroidDriver driver;
  private final Finder containerFinder;
  private final Scroller scroller;

  public ScrollerHelper(Scroller scroller, DroidDriver driver, Finder containerFinder) {
    this.scroller = scroller;
    this.driver = driver;
    this.containerFinder = containerFinder;
  }

  /**
   * Scrolls {@code containerFinder} in both directions if necessary to find
   * {@code itemFinder}, which is a descendant of {@code containerFinder}.
   *
   * @param itemFinder Finder for the desired item; relative to
   *        {@code containerFinder}
   * @return the UiElement matching {@code itemFinder}
   * @throws ElementNotFoundException If no match is found
   */
  public UiElement scrollTo(Finder itemFinder) {
    return scroller.scrollTo(driver, containerFinder, itemFinder);
  }

  /**
   * Scrolls {@code containerFinder} in {@code direction} if necessary to find
   * {@code itemFinder}, which is a descendant of {@code containerFinder}.
   *
   * @param itemFinder Finder for the desired item; relative to
   *        {@code containerFinder}
   * @param direction specifies where the view port will move instead of the finger
   * @return the UiElement matching {@code itemFinder}
   * @throws ElementNotFoundException If no match is found
   */
  public UiElement scrollTo(Finder itemFinder, PhysicalDirection direction) {
    return scroller.scrollTo(driver, containerFinder, itemFinder, direction);
  }

  /**
   * Scrolls to {@code itemFinder} and returns true, otherwise returns false.
   *
   * @param itemFinder Finder for the desired item
   * @return true if successful, otherwise false
   */
  public boolean canScrollTo(Finder itemFinder) {
    try {
      scrollTo(itemFinder);
      return true;
    } catch (ElementNotFoundException e) {
      return false;
    }
  }
}
