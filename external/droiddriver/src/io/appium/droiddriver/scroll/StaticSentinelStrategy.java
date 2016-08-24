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

import android.graphics.Rect;

import io.appium.droiddriver.DroidDriver;
import io.appium.droiddriver.UiElement;
import io.appium.droiddriver.finders.Finder;
import io.appium.droiddriver.instrumentation.InstrumentationDriver;
import io.appium.droiddriver.scroll.Direction.DirectionConverter;
import io.appium.droiddriver.scroll.Direction.PhysicalDirection;

/**
 * Determines whether scrolling is possible by checking whether the last child
 * in the logical scroll direction is fully visible. This assumes the count of
 * children is static, and {@link UiElement#getChildren} includes all children
 * no matter if it is visible. Currently {@link InstrumentationDriver} behaves
 * this way.
 * <p>
 * This does not work if a child is larger than the physical size of the
 * container.
 */
public class StaticSentinelStrategy extends SentinelStrategy {
  /**
   * Defaults to FIRST_CHILD_GETTER for backward scrolling, LAST_CHILD_GETTER
   * for forward scrolling, and the standard {@link DirectionConverter}.
   */
  public static final StaticSentinelStrategy DEFAULT = new StaticSentinelStrategy(
      FIRST_CHILD_GETTER, LAST_CHILD_GETTER, DirectionConverter.STANDARD_CONVERTER);

  public StaticSentinelStrategy(Getter backwardGetter, Getter forwardGetter,
      DirectionConverter directionConverter) {
    super(backwardGetter, forwardGetter, directionConverter);
  }

  @Override
  public boolean scroll(DroidDriver driver, Finder containerFinder, PhysicalDirection direction) {
    UiElement sentinel = getSentinel(driver, containerFinder, direction);
    UiElement container = sentinel.getParent();
    // If the last child in the logical scroll direction is fully visible, no
    // more scrolling is possible
    Rect visibleBounds = container.getVisibleBounds();
    if (visibleBounds.contains(sentinel.getBounds())) {
      return false;
    }

    doScroll(container, direction);
    return true;
  }
}
