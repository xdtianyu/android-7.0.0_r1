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

import io.appium.droiddriver.DroidDriver;
import io.appium.droiddriver.UiElement;
import io.appium.droiddriver.exceptions.ElementNotFoundException;
import io.appium.droiddriver.finders.By;
import io.appium.droiddriver.finders.Finder;
import io.appium.droiddriver.scroll.Direction.DirectionConverter;
import io.appium.droiddriver.scroll.Direction.PhysicalDirection;
import io.appium.droiddriver.util.Logs;
import io.appium.droiddriver.util.Strings;

/**
 * Determines whether scrolling is possible by checking whether the sentinel
 * child is updated after scrolling. Use this when {@link UiElement#getChildren}
 * is not reliable. This can happen, for instance, when UiAutomationDriver is
 * used, which skips invisible children, or in the case of dynamic list, which
 * shows more items when scrolling beyond the end.
 */
public class DynamicSentinelStrategy extends SentinelStrategy {

  /**
   * Interface for determining whether sentinel is updated.
   */
  public static interface IsUpdatedStrategy {
    /**
     * Returns whether {@code newSentinel} is updated from {@code oldSentinel}.
     */
    boolean isSentinelUpdated(UiElement newSentinel, UiElement oldSentinel);

    /**
     * {@inheritDoc}
     *
     * <p>
     * It is recommended that this method return a description to help
     * debugging.
     */
    @Override
    String toString();
  }

  /**
   * Determines whether the sentinel is updated by checking a single unique
   * String attribute of a descendant element of the sentinel (or itself).
   */
  public static abstract class SingleStringUpdated implements IsUpdatedStrategy {
    private final Finder uniqueStringFinder;

    /**
     * @param uniqueStringFinder a Finder relative to the sentinel that finds
     *        its descendant or self which contains a unique String.
     */
    public SingleStringUpdated(Finder uniqueStringFinder) {
      this.uniqueStringFinder = uniqueStringFinder;
    }

    /**
     * @param uniqueStringElement the descendant or self that contains the
     *        unique String
     * @return the unique String
     */
    protected abstract String getUniqueString(UiElement uniqueStringElement);

    private String getUniqueStringFromSentinel(UiElement sentinel) {
      try {
        return getUniqueString(uniqueStringFinder.find(sentinel));
      } catch (ElementNotFoundException e) {
        return null;
      }
    }

    @Override
    public boolean isSentinelUpdated(UiElement newSentinel, UiElement oldSentinel) {
      // If the sentinel moved, scrolling has some effect. This is both an
      // optimization - getBounds is cheaper than find - and necessary in
      // certain cases, e.g. user is looking for a sibling of the unique string;
      // the scroll is close to the end therefore the unique string does not
      // change, but the target could be revealed.
      if (!newSentinel.getBounds().equals(oldSentinel.getBounds())) {
        return true;
      }

      String newString = getUniqueStringFromSentinel(newSentinel);
      // A legitimate case for newString being null is when newSentinel is
      // partially shown. We return true to allow further scrolling. But program
      // error could also cause this, e.g. a bad choice of Getter, which
      // results in unnecessary scroll actions that have no visual effect. This
      // log helps troubleshooting in the latter case.
      if (newString == null) {
        Logs.logfmt(Log.WARN, "Unique String is null: sentinel=%s, uniqueStringFinder=%s",
            newSentinel, uniqueStringFinder);
        return true;
      }
      if (newString.equals(getUniqueStringFromSentinel(oldSentinel))) {
        Logs.log(Log.INFO, "Unique String is not updated: " + newString);
        return false;
      }
      return true;
    }

    @Override
    public String toString() {
      return Strings.toStringHelper(this).addValue(uniqueStringFinder).toString();
    }
  }

  /**
   * Determines whether the sentinel is updated by checking the text of a
   * descendant element of the sentinel (or itself).
   */
  public static class TextUpdated extends SingleStringUpdated {
    public TextUpdated(Finder uniqueStringFinder) {
      super(uniqueStringFinder);
    }

    @Override
    protected String getUniqueString(UiElement uniqueStringElement) {
      return uniqueStringElement.getText();
    }
  }

  /**
   * Determines whether the sentinel is updated by checking the content
   * description of a descendant element of the sentinel (or itself).
   */
  public static class ContentDescriptionUpdated extends SingleStringUpdated {
    public ContentDescriptionUpdated(Finder uniqueStringFinder) {
      super(uniqueStringFinder);
    }

    @Override
    protected String getUniqueString(UiElement uniqueStringElement) {
      return uniqueStringElement.getContentDescription();
    }
  }

  /**
   * Determines whether the sentinel is updated by checking the resource-id of a
   * descendant element of the sentinel (often itself). This is useful when the
   * children of the container are heterogeneous -- they don't have a common
   * pattern to get a unique string.
   */
  public static class ResourceIdUpdated extends SingleStringUpdated {
    /**
     * Uses the resource-id of the sentinel itself.
     */
    public static final ResourceIdUpdated SELF = new ResourceIdUpdated(By.any());

    public ResourceIdUpdated(Finder uniqueStringFinder) {
      super(uniqueStringFinder);
    }

    @Override
    protected String getUniqueString(UiElement uniqueStringElement) {
      return uniqueStringElement.getResourceId();
    }
  }

  private final IsUpdatedStrategy isUpdatedStrategy;
  private UiElement lastSentinel;

  /**
   * Constructs with {@code Getter}s that decorate the given {@code Getter}s
   * with {@link UiElement#VISIBLE}, and the given {@code isUpdatedStrategy} and
   * {@code directionConverter}. Be careful with {@code Getter}s: the sentinel
   * after each scroll should be unique.
   */
  public DynamicSentinelStrategy(IsUpdatedStrategy isUpdatedStrategy, Getter backwardGetter,
      Getter forwardGetter, DirectionConverter directionConverter) {
    super(new MorePredicateGetter(backwardGetter, UiElement.VISIBLE), new MorePredicateGetter(
        forwardGetter, UiElement.VISIBLE), directionConverter);
    this.isUpdatedStrategy = isUpdatedStrategy;
  }

  /**
   * Defaults to the standard {@link DirectionConverter}.
   */
  public DynamicSentinelStrategy(IsUpdatedStrategy isUpdatedStrategy, Getter backwardGetter,
      Getter forwardGetter) {
    this(isUpdatedStrategy, backwardGetter, forwardGetter, DirectionConverter.STANDARD_CONVERTER);
  }

  /**
   * Defaults to LAST_CHILD_GETTER for forward scrolling, and the standard
   * {@link DirectionConverter}.
   */
  public DynamicSentinelStrategy(IsUpdatedStrategy isUpdatedStrategy, Getter backwardGetter) {
    this(isUpdatedStrategy, backwardGetter, LAST_CHILD_GETTER,
        DirectionConverter.STANDARD_CONVERTER);
  }

  @Override
  public boolean scroll(DroidDriver driver, Finder containerFinder, PhysicalDirection direction) {
    UiElement oldSentinel = getOldSentinel(driver, containerFinder, direction);
    doScroll(oldSentinel.getParent(), direction);
    UiElement newSentinel = getSentinel(driver, containerFinder, direction);
    lastSentinel = newSentinel;
    return isUpdatedStrategy.isSentinelUpdated(newSentinel, oldSentinel);
  }

  private UiElement getOldSentinel(DroidDriver driver, Finder containerFinder,
      PhysicalDirection direction) {
    return lastSentinel != null ? lastSentinel : getSentinel(driver, containerFinder, direction);
  }

  @Override
  public void beginScrolling(DroidDriver driver, Finder containerFinder, Finder itemFinder,
      PhysicalDirection direction) {
    lastSentinel = null;
  }

  @Override
  public void endScrolling(DroidDriver driver, Finder containerFinder, Finder itemFinder,
      PhysicalDirection direction) {
    // Prevent memory leak
    lastSentinel = null;
  }

  @Override
  public String toString() {
    return String.format("DynamicSentinelStrategy{%s, isUpdatedStrategy=%s}", super.toString(),
        isUpdatedStrategy);
  }
}
