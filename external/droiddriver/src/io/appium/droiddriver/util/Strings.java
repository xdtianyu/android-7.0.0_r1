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

package io.appium.droiddriver.util;

/**
 * Static helper methods for manipulating strings.
 */
public class Strings {
  public static String charSequenceToString(CharSequence input) {
    return input == null ? null : input.toString();
  }

  public static ToStringHelper toStringHelper(Object self) {
    return new ToStringHelper(self.getClass().getSimpleName());
  }

  public static final class ToStringHelper {
    private final StringBuilder builder;
    private boolean needsSeparator = false;

    /**
     * Use {@link #toStringHelper(Object)} to create an instance.
     */
    private ToStringHelper(String className) {
      this.builder = new StringBuilder(32).append(className).append('{');
    }

    public ToStringHelper addValue(Object value) {
      maybeAppendSeparator().append(value);
      return this;
    }

    public ToStringHelper add(String name, Object value) {
      maybeAppendSeparator().append(name).append('=').append(value);
      return this;
    }

    @Override
    public String toString() {
      return builder.append('}').toString();
    }

    private StringBuilder maybeAppendSeparator() {
      if (needsSeparator) {
        return builder.append(", ");
      } else {
        needsSeparator = true;
        return builder;
      }
    }
  }
}
