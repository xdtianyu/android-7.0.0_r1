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

import android.text.TextUtils;

import io.appium.droiddriver.UiElement;

/**
 * Static utility methods pertaining to {@code Predicate} instances.
 */
public final class Predicates {
  private Predicates() {}

  private static final Predicate<Object> ANY = new Predicate<Object>() {
    @Override
    public boolean apply(Object o) {
      return true;
    }

    @Override
    public String toString() {
      return "any";
    }
  };

  /**
   * Returns a predicate that always evaluates to {@code true}.
   */
  @SuppressWarnings("unchecked")
  public static <T> Predicate<T> any() {
    return (Predicate<T>) ANY;
  }

  /**
   * Returns a predicate that is the negation of the provided {@code predicate}.
   */
  public static <T> Predicate<T> not(final Predicate<T> predicate) {
    return new Predicate<T>() {
      @Override
      public boolean apply(T input) {
        return !predicate.apply(input);
      }

      @Override
      public String toString() {
        return "not(" + predicate + ")";
      }
    };
  }

  /**
   * Returns a predicate that evaluates to {@code true} if both arguments
   * evaluate to {@code true}. The arguments are evaluated in order, and
   * evaluation will be "short-circuited" as soon as a false predicate is found.
   */
  @SuppressWarnings("unchecked")
  public static <T> Predicate<T> allOf(final Predicate<? super T> first,
      final Predicate<? super T> second) {
    if (first == null || first == ANY) {
      return (Predicate<T>) second;
    }
    if (second == null || second == ANY) {
      return (Predicate<T>) first;
    }

    return new Predicate<T>() {
      @Override
      public boolean apply(T input) {
        return first.apply(input) && second.apply(input);
      }

      @Override
      public String toString() {
        return "allOf(" + first + ", " + second + ")";
      }
    };
  }

  /**
   * Returns a predicate that evaluates to {@code true} if each of its
   * components evaluates to {@code true}. The components are evaluated in
   * order, and evaluation will be "short-circuited" as soon as a false
   * predicate is found.
   */
  @SuppressWarnings("unchecked")
  public static <T> Predicate<T> allOf(final Predicate<? super T>... components) {
    return new Predicate<T>() {
      @Override
      public boolean apply(T input) {
        for (Predicate<? super T> each : components) {
          if (!each.apply(input)) {
            return false;
          }
        }
        return true;
      }

      @Override
      public String toString() {
        return "allOf(" + TextUtils.join(", ", components) + ")";
      }
    };
  }

  /**
   * Returns a predicate that evaluates to {@code true} if any one of its
   * components evaluates to {@code true}. The components are evaluated in
   * order, and evaluation will be "short-circuited" as soon as a true predicate
   * is found.
   */
  @SuppressWarnings("unchecked")
  public static <T> Predicate<T> anyOf(final Predicate<? super T>... components) {
    return new Predicate<T>() {
      @Override
      public boolean apply(T input) {
        for (Predicate<? super T> each : components) {
          if (each.apply(input)) {
            return true;
          }
        }
        return false;
      }

      @Override
      public String toString() {
        return "anyOf(" + TextUtils.join(", ", components) + ")";
      }
    };
  }

  /**
   * Returns a predicate that evaluates to {@code true} on a {@link UiElement}
   * if its {@code attribute} is {@code true}.
   */
  public static Predicate<UiElement> attributeTrue(final Attribute attribute) {
    return new Predicate<UiElement>() {
      @Override
      public boolean apply(UiElement element) {
        Boolean actual = element.get(attribute);
        return actual != null && actual;
      }

      @Override
      public String toString() {
        return String.format("{%s}", attribute);
      }
    };
  }

  /**
   * Returns a predicate that evaluates to {@code true} on a {@link UiElement}
   * if its {@code attribute} is {@code false}.
   */
  public static Predicate<UiElement> attributeFalse(final Attribute attribute) {
    return new Predicate<UiElement>() {
      @Override
      public boolean apply(UiElement element) {
        Boolean actual = element.get(attribute);
        return actual == null || !actual;
      }

      @Override
      public String toString() {
        return String.format("{not %s}", attribute);
      }
    };
  }

  /**
   * Returns a predicate that evaluates to {@code true} on a {@link UiElement}
   * if its {@code attribute} equals {@code expected}.
   */
  public static Predicate<UiElement> attributeEquals(final Attribute attribute,
      final Object expected) {
    return new Predicate<UiElement>() {
      @Override
      public boolean apply(UiElement element) {
        Object actual = element.get(attribute);
        return actual == expected || (actual != null && actual.equals(expected));
      }

      @Override
      public String toString() {
        return String.format("{%s=%s}", attribute, expected);
      }
    };
  }

  /**
   * Returns a predicate that evaluates to {@code true} on a {@link UiElement}
   * if its {@code attribute} matches {@code regex}.
   */
  public static Predicate<UiElement> attributeMatches(final Attribute attribute, final String regex) {
    return new Predicate<UiElement>() {
      @Override
      public boolean apply(UiElement element) {
        String actual = element.get(attribute);
        return actual != null && actual.matches(regex);
      }

      @Override
      public String toString() {
        return String.format("{%s matches %s}", attribute, regex);
      }
    };
  }

  /**
   * Returns a predicate that evaluates to {@code true} on a {@link UiElement}
   * if its {@code attribute} contains {@code substring}.
   */
  public static Predicate<UiElement> attributeContains(final Attribute attribute,
      final String substring) {
    return new Predicate<UiElement>() {
      @Override
      public boolean apply(UiElement element) {
        String actual = element.get(attribute);
        return actual != null && actual.contains(substring);
      }

      @Override
      public String toString() {
        return String.format("{%s contains %s}", attribute, substring);
      }
    };
  }

  public static Predicate<UiElement> withParent(final Predicate<? super UiElement> parentPredicate) {
    return new Predicate<UiElement>() {
      @Override
      public boolean apply(UiElement element) {
        UiElement parent = element.getParent();
        return parent != null && parentPredicate.apply(parent);
      }

      @Override
      public String toString() {
        return "withParent(" + parentPredicate + ")";
      }
    };
  }

  public static Predicate<UiElement> withAncestor(
      final Predicate<? super UiElement> ancestorPredicate) {
    return new Predicate<UiElement>() {
      @Override
      public boolean apply(UiElement element) {
        UiElement parent = element.getParent();
        while (parent != null) {
          if (ancestorPredicate.apply(parent)) {
            return true;
          }
          parent = parent.getParent();
        }
        return false;
      }

      @Override
      public String toString() {
        return "withAncestor(" + ancestorPredicate + ")";
      }
    };
  }

  public static Predicate<UiElement> withSibling(final Predicate<? super UiElement> siblingPredicate) {
    return new Predicate<UiElement>() {
      @Override
      public boolean apply(UiElement element) {
        UiElement parent = element.getParent();
        if (parent == null) {
          return false;
        }
        for (UiElement sibling : parent.getChildren(UiElement.VISIBLE)) {
          if (sibling != element && siblingPredicate.apply(sibling)) {
            return true;
          }
        }
        return false;
      }

      @Override
      public String toString() {
        return "withSibling(" + siblingPredicate + ")";
      }
    };
  }

  public static Predicate<UiElement> withChild(final Predicate<? super UiElement> childPredicate) {
    return new Predicate<UiElement>() {
      @Override
      public boolean apply(UiElement element) {
        for (UiElement child : element.getChildren(UiElement.VISIBLE)) {
          if (childPredicate.apply(child)) {
            return true;
          }
        }
        return false;
      }

      @Override
      public String toString() {
        return "withChild(" + childPredicate + ")";
      }
    };
  }
}
