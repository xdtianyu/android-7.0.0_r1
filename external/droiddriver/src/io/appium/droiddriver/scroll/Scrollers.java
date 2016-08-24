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

import android.app.UiAutomation;
import android.widget.ProgressBar;

import io.appium.droiddriver.DroidDriver;
import io.appium.droiddriver.finders.By;
import io.appium.droiddriver.finders.Finder;
import io.appium.droiddriver.scroll.Direction.Axis;
import io.appium.droiddriver.scroll.Direction.DirectionConverter;
import io.appium.droiddriver.scroll.Direction.PhysicalDirection;

/**
 * Static utility classes and methods pertaining to {@link Scroller} instances.
 */
public class Scrollers {
  /**
   * Augments the delegate {@link ScrollStepStrategy} - after a successful
   * scroll, waits until ProgressBar is gone.
   */
  public static abstract class ProgressBarScrollStepStrategy extends ForwardingScrollStepStrategy {
    @Override
    public boolean scroll(DroidDriver driver, Finder containerFinder, PhysicalDirection direction) {
      if (super.scroll(driver, containerFinder, direction)) {
        driver.checkGone(By.className(ProgressBar.class));
        return true;
      }
      return false;
    }

    /** Convenience method to wrap {@code delegate} with this class */
    public static ScrollStepStrategy wrap(final ScrollStepStrategy delegate) {
      return new ProgressBarScrollStepStrategy() {
        @Override
        protected ScrollStepStrategy delegate() {
          return delegate;
        }
      };
    }
  }

  /**
   * Returns a new default Scroller that works in simple cases. In complex cases
   * you may try a {@link StepBasedScroller} with a custom
   * {@link ScrollStepStrategy}:
   * <ul>
   * <li>If the Scroller is used with InstrumentationDriver,
   * StaticSentinelStrategy may work and it's the simplest.</li>
   * <li>Otherwise, DynamicSentinelStrategy should work in all cases, including
   * the case of dynamic list, which shows more items when scrolling beyond the
   * end. On the other hand, it's complex and needs more configuration.</li>
   * </ul>
   * Note if a {@link StepBasedScroller} is returned, it is constructed with
   * arguments that apply to typical cases. You may want to customize them for
   * specific cases. For instance, {@code perScrollTimeoutMillis} can be 0L if
   * there are no asynchronously updated views. To that extent, this method
   * serves as an example of how to construct {@link Scroller}s rather than
   * providing the "official" {@link Scroller}.
   */
  public static Scroller newScroller(UiAutomation uiAutomation) {
    if (uiAutomation != null) {
      return new StepBasedScroller(100/* maxScrolls */, 1000L/* perScrollTimeoutMillis */,
          Axis.VERTICAL, new AccessibilityEventScrollStepStrategy(uiAutomation, 1000L,
              DirectionConverter.STANDARD_CONVERTER), true/* startFromBeginning */);
    }
    // TODO: A {@link Scroller} that directly jumps to the view if an
    // InstrumentationDriver is used.
    return new StepBasedScroller(100/* maxScrolls */, 1000L/* perScrollTimeoutMillis */,
        Axis.VERTICAL, StaticSentinelStrategy.DEFAULT, true/* startFromBeginning */);
  }
}
