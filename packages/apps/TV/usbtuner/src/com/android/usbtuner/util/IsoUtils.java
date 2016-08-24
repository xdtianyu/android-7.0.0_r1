/*
 * Copyright (C) 2015 The Android Open Source Project
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

package com.android.usbtuner.util;

import java.util.HashSet;
import java.util.Locale;

/**
 * Utility class for ISO codes.
 */
public final class IsoUtils {
    private static final HashSet<String> ISO3_LANGUAGES = new HashSet<>();

    static {
        String[] languages = Locale.getISOLanguages();
        for (String lang : languages) {
            Locale locale = new Locale(lang);
            ISO3_LANGUAGES.add(locale.getISO3Language());
        }
    }

    private IsoUtils() { }

    /**
     * Returns {@code true} if a {@link String} is a valid ISO-639-2/T language code.
     */
    public static boolean isValidIso3Language(String langCode) {
        return ISO3_LANGUAGES.contains(langCode);
    }
}
