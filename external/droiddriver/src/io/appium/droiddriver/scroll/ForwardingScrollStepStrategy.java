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
 * An abstract base class for implementing the <a
 * href="http://en.wikipedia.org/wiki/Decorator_pattern">decorator pattern</a>,
 * forwarding calls to all methods of {@link ScrollStepStrategy} to delegate.
 */
public abstract class ForwardingScrollStepStrategy implements ScrollStepStrategy {

  protected ForwardingScrollStepStrategy() {}

  /**
   * Returns the backing delegate instance that methods are forwarded to.
   */
  protected abstract ScrollStepStrategy delegate();

  public boolean scroll(DroidDriver driver, Finder containerFinder, PhysicalDirection direction) {
    return delegate().scroll(driver, containerFinder, direction);
  }

  @Override
  public final DirectionConverter getDirectionConverter() {
    return delegate().getDirectionConverter();
  }

  @Override
  public void beginScrolling(DroidDriver driver, Finder containerFinder, Finder itemFinder,
      PhysicalDirection direction) {
    delegate().beginScrolling(driver, containerFinder, itemFinder, direction);
  }

  @Override
  public void endScrolling(DroidDriver driver, Finder containerFinder, Finder itemFinder,
      PhysicalDirection direction) {
    delegate().endScrolling(driver, containerFinder, itemFinder, direction);
  }

  @Override
  public void doScroll(UiElement container, PhysicalDirection direction) {
    delegate().doScroll(container, direction);
  }
}
