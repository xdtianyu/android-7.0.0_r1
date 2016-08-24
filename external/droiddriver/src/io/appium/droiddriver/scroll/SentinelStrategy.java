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

import android.util.Log;

import java.util.List;

import io.appium.droiddriver.DroidDriver;
import io.appium.droiddriver.UiElement;
import io.appium.droiddriver.exceptions.ElementNotFoundException;
import io.appium.droiddriver.finders.By;
import io.appium.droiddriver.finders.Finder;
import io.appium.droiddriver.finders.Predicate;
import io.appium.droiddriver.finders.Predicates;
import io.appium.droiddriver.scroll.Direction.DirectionConverter;
import io.appium.droiddriver.scroll.Direction.LogicalDirection;
import io.appium.droiddriver.scroll.Direction.PhysicalDirection;
import io.appium.droiddriver.util.Logs;

/**
 * A {@link ScrollStepStrategy} that determines whether scrolling is possible
 * based on a sentinel.
 */
public abstract class SentinelStrategy implements ScrollStepStrategy {
  /**
   * A {@link Finder} for sentinel. Note that unlike {@link Finder}, invisible
   * UiElements are not skipped by default.
   */
  public static abstract class Getter implements Finder {
    protected final Predicate<? super UiElement> predicate;

    protected Getter() {
      // Include invisible children by default.
      this(null);
    }

    protected Getter(Predicate<? super UiElement> predicate) {
      this.predicate = predicate;
    }

    /**
     * Gets the sentinel, which must be an immediate child of {@code container}
     * - not a descendant. Note sentinel may not exist if {@code container} has
     * not finished updating.
     */
    @Override
    public UiElement find(UiElement container) {
      UiElement sentinel = getSentinel(container.getChildren(predicate));
      if (sentinel == null) {
        throw new ElementNotFoundException(this);
      }
      Logs.log(Log.INFO, "Found sentinel: " + sentinel);
      return sentinel;
    }


    protected abstract UiElement getSentinel(List<? extends UiElement> children);

    @Override
    public abstract String toString();
  }

  /**
   * Returns the first child as the sentinel.
   */
  public static final Getter FIRST_CHILD_GETTER = new Getter() {
    @Override
    protected UiElement getSentinel(List<? extends UiElement> children) {
      return children.isEmpty() ? null : children.get(0);
    }

    @Override
    public String toString() {
      return "FIRST_CHILD";
    }
  };
  /**
   * Returns the last child as the sentinel.
   */
  public static final Getter LAST_CHILD_GETTER = new Getter() {
    @Override
    protected UiElement getSentinel(List<? extends UiElement> children) {
      return children.isEmpty() ? null : children.get(children.size() - 1);
    }

    @Override
    public String toString() {
      return "LAST_CHILD";
    }
  };
  /**
   * Returns the second last child as the sentinel. Useful when the activity
   * always shows the last child as an anchor (for example a footer).
   * <p>
   * Sometimes uiautomatorviewer may not show the anchor as the last child, due
   * to the reordering by layout described in {@link UiElement#getChildren}.
   * This is not a problem with UiAutomationDriver because it sees the same as
   * uiautomatorviewer does, but could be a problem with InstrumentationDriver.
   * </p>
   */
  public static final Getter SECOND_LAST_CHILD_GETTER = new Getter() {
    @Override
    protected UiElement getSentinel(List<? extends UiElement> children) {
      return children.size() < 2 ? null : children.get(children.size() - 2);
    }

    @Override
    public String toString() {
      return "SECOND_LAST_CHILD";
    }
  };
  /**
   * Returns the second child as the sentinel. Useful when the activity shows a
   * fixed first child.
   */
  public static final Getter SECOND_CHILD_GETTER = new Getter() {
    @Override
    protected UiElement getSentinel(List<? extends UiElement> children) {
      return children.size() <= 1 ? null : children.get(1);
    }

    @Override
    public String toString() {
      return "SECOND_CHILD";
    }
  };

  /**
   * Decorates a {@link Getter} by adding another {@link Predicate}.
   */
  public static class MorePredicateGetter extends Getter {
    private final Getter original;

    public MorePredicateGetter(Getter original, Predicate<? super UiElement> extraPredicate) {
      super(Predicates.allOf(original.predicate, extraPredicate));
      this.original = original;
    }

    @Override
    protected UiElement getSentinel(List<? extends UiElement> children) {
      return original.getSentinel(children);
    }

    @Override
    public String toString() {
      return predicate.toString() + " " + original;
    }
  }

  private final Getter backwardGetter;
  private final Getter forwardGetter;
  private final DirectionConverter directionConverter;

  protected SentinelStrategy(Getter backwardGetter, Getter forwardGetter,
      DirectionConverter directionConverter) {
    this.backwardGetter = backwardGetter;
    this.forwardGetter = forwardGetter;
    this.directionConverter = directionConverter;
  }

  protected UiElement getSentinel(DroidDriver driver, Finder containerFinder,
      PhysicalDirection direction) {
    Logs.call(this, "getSentinel", driver, containerFinder, direction);
    Finder sentinelFinder;
    LogicalDirection logicalDirection = directionConverter.toLogicalDirection(direction);
    if (logicalDirection == LogicalDirection.BACKWARD) {
      sentinelFinder = By.chain(containerFinder, backwardGetter);
    } else {
      sentinelFinder = By.chain(containerFinder, forwardGetter);
    }
    return driver.on(sentinelFinder);
  }

  @Override
  public final DirectionConverter getDirectionConverter() {
    return directionConverter;
  }

  @Override
  public void beginScrolling(DroidDriver driver, Finder containerFinder, Finder itemFinder,
      PhysicalDirection direction) {}

  @Override
  public void endScrolling(DroidDriver driver, Finder containerFinder, Finder itemFinder,
      PhysicalDirection direction) {}

  @Override
  public String toString() {
    return String.format("{backwardGetter=%s, forwardGetter=%s}", backwardGetter, forwardGetter);
  }

  @Override
  public void doScroll(UiElement container, PhysicalDirection direction) {
    container.scroll(direction);
  }
}
