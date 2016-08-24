/*
 * Copyright (C) 2014 The Android Open Source Project
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

package com.android.messageformat;

import android.content.Context;

import java.util.Locale;

public final class MessageFormat {
  /**
   * Formats a message pattern string with a variable number of name/value pair arguments.
   * Creates an ICU MessageFormat for the locale and pattern,
   * and formats with the arguments.
   *
   * @param locale Locale for number formatting and plural selection etc.
   * @param msg an ICU-MessageFormat-syntax string
   * @param nameValuePairs (argument name, argument value) pairs
   */
  public static final String formatNamedArgs(Locale locale, String msg, Object... nameValuePairs) {
    return com.ibm.icu.simple.MessageFormat.formatNamedArgs(locale, msg, nameValuePairs);
  }

  /**
   * Formats a message pattern string for the default locale with a variable number of name/value
   * pair arguments.
   * Creates an ICU MessageFormat for Locale.getDefault() and pattern,
   * and formats with the arguments.
   *
   * @param msg an ICU-MessageFormat-syntax string
   * @param nameValuePairs (argument name, argument value) pairs
   */
  public static final String formatNamedArgs(String msg, Object... nameValuePairs) {
    return formatNamedArgs(Locale.getDefault(), msg, nameValuePairs);
  }

  /**
   * Formats a message pattern from Android resource for the default locale with a variable number
   * of name/value pair arguments.
   * Creates an ICU MessageFormat for Locale.getDefault() and pattern,
   * and formats with the arguments.
   *
   * @param context Android context object
   * @param id Android string resource ID representing ICU-MessageFormat-syntax string
   * @param nameValuePairs (argument name, argument value) pairs
   */
  public static final String formatNamedArgs(Context context, int id, Object... nameValuePairs) {
    return formatNamedArgs(Locale.getDefault(),
        context.getResources().getString(id), nameValuePairs);
  }

  // Non instantiable
  private MessageFormat() {
  }
}
