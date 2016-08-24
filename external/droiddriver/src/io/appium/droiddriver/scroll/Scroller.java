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
package io.appium.droiddriver.scroll;

import io.appium.droiddriver.DroidDriver;
import io.appium.droiddriver.UiElement;
import io.appium.droiddriver.exceptions.ElementNotFoundException;
import io.appium.droiddriver.finders.Finder;
import io.appium.droiddriver.scroll.Direction.PhysicalDirection;

/**
 * Interface for scrolling to the desired item in a scrollable container view.
 */
public interface Scroller {
  /**
   * Scrolls {@code containerFinder} in both directions if necessary to find {@code itemFinder},
   * which is a descendant of {@code containerFinder}.
   *
   * @param driver          a DroidDriver instance
   * @param containerFinder Finder for the container that can scroll, for instance a ListView
   * @param itemFinder      Finder for the desired item; relative to {@code containerFinder}
   * @return the UiElement matching {@code itemFinder}
   * @throws ElementNotFoundException If no match is found
   */
  UiElement scrollTo(DroidDriver driver, Finder containerFinder, Finder itemFinder);

  /**
   * Scrolls {@code containerFinder} in {@code direction} if necessary to find {@code itemFinder},
   * which is a descendant of {@code containerFinder}.
   *
   * @param driver          a DroidDriver instance
   * @param containerFinder Finder for the container that can scroll, for instance a ListView
   * @param itemFinder      Finder for the desired item; relative to {@code containerFinder}
   * @param direction       specifies where the view port will move instead of the finger
   * @return the UiElement matching {@code itemFinder}
   * @throws ElementNotFoundException If no match is found
   */
  UiElement scrollTo(DroidDriver driver, Finder containerFinder, Finder itemFinder,
                     PhysicalDirection direction);
}
