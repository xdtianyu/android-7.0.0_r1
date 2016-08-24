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

import android.content.Context;

import io.appium.droiddriver.UiElement;
import io.appium.droiddriver.exceptions.ElementNotFoundException;
import io.appium.droiddriver.util.InstrumentationUtils;

import static io.appium.droiddriver.util.Preconditions.checkNotNull;

/**
 * Convenience methods to create commonly used finders.
 */
public class By {
  private static final MatchFinder ANY = new MatchFinder(null);

  /** Matches any UiElement. */
  public static MatchFinder any() {
    return ANY;
  }

  /** Matches a UiElement whose {@code attribute} is {@code true}. */
  public static MatchFinder is(Attribute attribute) {
    return new MatchFinder(Predicates.attributeTrue(attribute));
  }

  /**
   * Matches a UiElement whose {@code attribute} is {@code false} or is not set.
   */
  public static MatchFinder not(Attribute attribute) {
    return new MatchFinder(Predicates.attributeFalse(attribute));
  }

  /** Matches a UiElement by a resource id defined in the AUT. */
  public static MatchFinder resourceId(int resourceId) {
    Context targetContext = InstrumentationUtils.getInstrumentation().getTargetContext();
    return resourceId(targetContext.getResources().getResourceName(resourceId));
  }

  /**
   * Matches a UiElement by the string representation of a resource id. This works for resources
   * not belonging to the AUT.
   */
  public static MatchFinder resourceId(String resourceId) {
    return new MatchFinder(Predicates.attributeEquals(Attribute.RESOURCE_ID, resourceId));
  }

  /** Matches a UiElement by package name. */
  public static MatchFinder packageName(String name) {
    return new MatchFinder(Predicates.attributeEquals(Attribute.PACKAGE, name));
  }

  /** Matches a UiElement by the exact text. */
  public static MatchFinder text(String text) {
    return new MatchFinder(Predicates.attributeEquals(Attribute.TEXT, text));
  }

  /** Matches a UiElement whose text matches {@code regex}. */
  public static MatchFinder textRegex(String regex) {
    return new MatchFinder(Predicates.attributeMatches(Attribute.TEXT, regex));
  }

  /** Matches a UiElement whose text contains {@code substring}. */
  public static MatchFinder textContains(String substring) {
    return new MatchFinder(Predicates.attributeContains(Attribute.TEXT, substring));
  }

  /** Matches a UiElement by content description. */
  public static MatchFinder contentDescription(String contentDescription) {
    return new MatchFinder(Predicates.attributeEquals(Attribute.CONTENT_DESC, contentDescription));
  }

  /** Matches a UiElement whose content description contains {@code substring}. */
  public static MatchFinder contentDescriptionContains(String substring) {
    return new MatchFinder(Predicates.attributeContains(Attribute.CONTENT_DESC, substring));
  }

  /** Matches a UiElement by class name. */
  public static MatchFinder className(String className) {
    return new MatchFinder(Predicates.attributeEquals(Attribute.CLASS, className));
  }

  /** Matches a UiElement by class name. */
  public static MatchFinder className(Class<?> clazz) {
    return className(clazz.getName());
  }

  /** Matches a UiElement that is selected. */
  public static MatchFinder selected() {
    return is(Attribute.SELECTED);
  }

  /**
   * Matches by XPath. When applied on an non-root element, it will not evaluate
   * above the context element.
   * <p>
   * XPath is the domain-specific-language for navigating a node tree. It is
   * ideal if the UiElement to match has a complex relationship with surrounding
   * nodes. For simple cases, {@link #withParent} or {@link #withAncestor} are
   * preferred, which can combine with other {@link MatchFinder}s in
   * {@link #allOf}. For complex cases like below, XPath is superior:
   *
   * <pre>
   * {@code
   * <View><!-- a custom view to group a cluster of items -->
   *   <LinearLayout>
   *     <TextView text='Albums'/>
   *     <TextView text='4 MORE'/>
   *   </LinearLayout>
   *   <RelativeLayout>
   *     <TextView text='Forever'/>
   *     <ImageView/>
   *   </RelativeLayout>
   * </View><!-- end of Albums cluster -->
   * <!-- imagine there are other clusters for Artists and Songs -->
   * }
   * </pre>
   *
   * If we need to locate the RelativeLayout containing the album "Forever"
   * instead of a song or an artist named "Forever", this XPath works:
   *
   * <pre>
   * {@code //*[LinearLayout/*[@text='Albums']]/RelativeLayout[*[@text='Forever']]}
   * </pre>
   *
   * @param xPath The xpath to use
   * @return a finder which locates elements via XPath
   */
  public static ByXPath xpath(String xPath) {
    return new ByXPath(xPath);
  }

  /**
   * Returns a finder that uses the UiElement returned by first Finder as
   * context for the second Finder.
   * <p>
   * typically first Finder finds the ancestor, then second Finder finds the
   * target UiElement, which is a descendant.
   * </p>
   * Note that if the first Finder matches multiple UiElements, only the first
   * match is tried, which usually is not what callers expect. In this case,
   * allOf(second, withAncesor(first)) may work.
   */
  public static ChainFinder chain(Finder first, Finder second) {
    return new ChainFinder(first, second);
  }

  private static Predicate<? super UiElement>[] getPredicates(MatchFinder... finders) {
    @SuppressWarnings("unchecked")
    Predicate<? super UiElement>[] predicates = new Predicate[finders.length];
    for (int i = 0; i < finders.length; i++) {
      predicates[i] = finders[i].predicate;
    }
    return predicates;
  }

  /**
   * Evaluates given {@code finders} in short-circuit fashion in the order
   * they are passed. Costly finders (for example those returned by with*
   * methods that navigate the node tree) should be passed after cheap finders
   * (for example the ByAttribute finders).
   *
   * @return a finder that is the logical conjunction of given finders
   */
  public static MatchFinder allOf(final MatchFinder... finders) {
    return new MatchFinder(Predicates.allOf(getPredicates(finders)));
  }

  /**
   * Evaluates given {@code finders} in short-circuit fashion in the order
   * they are passed. Costly finders (for example those returned by with*
   * methods that navigate the node tree) should be passed after cheap finders
   * (for example the ByAttribute finders).
   *
   * @return a finder that is the logical disjunction of given finders
   */
  public static MatchFinder anyOf(final MatchFinder... finders) {
    return new MatchFinder(Predicates.anyOf(getPredicates(finders)));
  }

  /**
   * Matches a UiElement whose parent matches the given parentFinder. For
   * complex cases, consider {@link #xpath}.
   */
  public static MatchFinder withParent(MatchFinder parentFinder) {
    checkNotNull(parentFinder);
    return new MatchFinder(Predicates.withParent(parentFinder.predicate));
  }

  /**
   * Matches a UiElement whose ancestor matches the given ancestorFinder. For
   * complex cases, consider {@link #xpath}.
   */
  public static MatchFinder withAncestor(MatchFinder ancestorFinder) {
    checkNotNull(ancestorFinder);
    return new MatchFinder(Predicates.withAncestor(ancestorFinder.predicate));
  }

  /**
   * Matches a UiElement which has a visible sibling matching the given
   * siblingFinder. This could be inefficient; consider {@link #xpath}.
   */
  public static MatchFinder withSibling(MatchFinder siblingFinder) {
    checkNotNull(siblingFinder);
    return new MatchFinder(Predicates.withSibling(siblingFinder.predicate));
  }

  /**
   * Matches a UiElement which has a visible child matching the given
   * childFinder. This could be inefficient; consider {@link #xpath}.
   */
  public static MatchFinder withChild(MatchFinder childFinder) {
    checkNotNull(childFinder);
    return new MatchFinder(Predicates.withChild(childFinder.predicate));
  }

  /**
   * Matches a UiElement whose descendant (including self) matches the given
   * descendantFinder. This could be VERY inefficient; consider {@link #xpath}.
   */
  public static MatchFinder withDescendant(final MatchFinder descendantFinder) {
    checkNotNull(descendantFinder);
    return new MatchFinder(new Predicate<UiElement>() {
      @Override
      public boolean apply(UiElement element) {
        try {
          descendantFinder.find(element);
          return true;
        } catch (ElementNotFoundException enfe) {
          return false;
        }
      }

      @Override
      public String toString() {
        return "withDescendant(" + descendantFinder + ")";
      }
    });
  }

  /** Matches a UiElement that does not match the provided {@code finder}. */
  public static MatchFinder not(MatchFinder finder) {
    checkNotNull(finder);
    return new MatchFinder(Predicates.not(finder.predicate));
  }

  private By() {}
}
