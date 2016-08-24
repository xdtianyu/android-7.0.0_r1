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
import io.appium.droiddriver.finders.Finder;
import io.appium.droiddriver.scroll.Direction.DirectionConverter;
import io.appium.droiddriver.scroll.Direction.PhysicalDirection;

/**
 * Interface for determining whether scrolling is possible.
 */
public interface ScrollStepStrategy {
  /**
   * Tries to scroll {@code containerFinder} in {@code direction}. Returns whether scrolling is
   * effective.
   *
   * @param driver          a DroidDriver instance
   * @param containerFinder Finder for the container that can scroll, for instance a ListView
   * @param direction       specifies where the view port will move instead of the finger
   * @return whether scrolling is effective
   */
  boolean scroll(DroidDriver driver, Finder containerFinder, PhysicalDirection direction);

  /**
   * Returns the {@link DirectionConverter}.
   */
  DirectionConverter getDirectionConverter();

  /**
   * Called only if this step is at the beginning of a series of scroll steps with regard to the
   * given arguments.
   *
   * @param driver          a DroidDriver instance
   * @param containerFinder Finder for the container that can scroll, for instance a ListView
   * @param itemFinder      Finder for the desired item; relative to {@code containerFinder}
   * @param direction       specifies where the view port will move instead of the finger
   */
  void beginScrolling(DroidDriver driver, Finder containerFinder, Finder itemFinder,
                      PhysicalDirection direction);

  /**
   * Called only if this step is at the end of a series of scroll steps with regard to the given
   * arguments.
   *
   * @param driver          a DroidDriver instance
   * @param containerFinder Finder for the container that can scroll, for instance a ListView
   * @param itemFinder      Finder for the desired item; relative to {@code containerFinder}
   * @param direction       specifies where the view port will move instead of the finger
   */
  void endScrolling(DroidDriver driver, Finder containerFinder, Finder itemFinder,
                    PhysicalDirection direction);

  /**
   * Performs the scroll action on {@code container}. Subclasses can override this to customize the
   * scroll action, for example, to adjust the scroll margins.
   *
   * @param container the container that can scroll
   * @param direction specifies where the view port will move instead of the finger
   */
  void doScroll(UiElement container, PhysicalDirection direction);

  /**
   * {@inheritDoc} It is recommended that this method return a description to help debugging.
   */
  @Override
  String toString();
}
