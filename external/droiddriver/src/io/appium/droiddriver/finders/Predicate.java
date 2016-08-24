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

/**
 * Determines a true or false value for a given input.
 *
 * This is replicated from the open-source <a
 * href="http://guava-libraries.googlecode.com">Guava libraries</a>.
 * <p>
 * Many apps use Guava. If a test apk also contains a copy of Guava, duplicated
 * classes in app and test apks may cause error at run-time:
 * "Class ref in pre-verified class resolved to unexpected implementation". To
 * simplify the build and deployment set-up, DroidDriver copies the code of some
 * Guava classes (often simplified) to this package such that it does not depend
 * on Guava.
 * </p>
 */
public interface Predicate<T> {
  /**
   * Returns the result of applying this predicate to {@code input}.
   */
  boolean apply(T input);

  /**
   * {@inheritDoc}
   *
   * <p>
   * It is recommended that this method return the description of this
   * Predicate.
   * </p>
   */
  @Override
  String toString();
}
