/*
 * Copyright (C) 2012 The Android Open Source Project
 *
 * Licensed under the Eclipse Public License, Version 1.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.eclipse.org/org/documents/epl-v10.php
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package com.android.ide.eclipse.adt.internal.editors.layout.configuration;

import static com.android.ide.common.resources.configuration.LocaleQualifier.FAKE_VALUE;

import com.android.annotations.NonNull;
import com.android.annotations.Nullable;
import com.android.ide.common.resources.configuration.FolderConfiguration;
import com.android.ide.common.resources.configuration.LocaleQualifier;

import org.eclipse.swt.graphics.Image;

/**
 * A language,region pair
 */
public class Locale {
    /**
     * A special marker region qualifier representing any region
     */
    public static final LocaleQualifier ANY_QUALIFIER = new LocaleQualifier(FAKE_VALUE);

    /**
     * A locale which matches any language and region
     */
    public static final Locale ANY = new Locale(ANY_QUALIFIER);

    /**
     * The locale qualifier, or {@link #ANY_QUALIFIER} if this locale matches
     * any locale
     */
    @NonNull
    public final LocaleQualifier qualifier;

    /**
     * Constructs a new {@linkplain Locale} matching a given language in a given
     * locale.
     *
     * @param locale the locale
     */
    private Locale(@NonNull
    LocaleQualifier locale) {
        qualifier = locale;
    }

    /**
     * Constructs a new {@linkplain Locale} matching a given language in a given
     * specific locale.
     *
     * @param locale the locale
     * @return a locale with the given locale
     */
    @NonNull
    public static Locale create(@NonNull
    LocaleQualifier locale) {
        return new Locale(locale);
    }

    /**
     * Constructs a new {@linkplain Locale} for the given folder configuration
     *
     * @param folder the folder configuration
     * @return a locale with the given language and region
     */
    public static Locale create(FolderConfiguration folder) {
        LocaleQualifier locale = folder.getLocaleQualifier();
        if (locale == null) {
            return ANY;
        } else {
            return new Locale(locale);
        }
    }

    /**
     * Constructs a new {@linkplain Locale} for the given locale string, e.g.
     * "zh", "en-rUS", or "b+eng+US".
     *
     * @param localeString the locale description
     * @return the corresponding locale
     */
    @NonNull
    public static Locale create(@NonNull
    String localeString) {
        // Load locale. Note that this can get overwritten by the
        // project-wide settings read below.

        LocaleQualifier qualifier = LocaleQualifier.getQualifier(localeString);
        if (qualifier != null) {
            return new Locale(qualifier);
        } else {
            return ANY;
        }
    }

    /**
     * Returns a flag image to use for this locale
     *
     * @return a flag image, or a default globe icon
     */
    @NonNull
    public Image getFlagImage() {
        String languageCode = qualifier.hasLanguage() ? qualifier.getLanguage() : null;
        if (languageCode == null) {
            return FlagManager.getGlobeIcon();
        }
        String regionCode = hasRegion() ? qualifier.getRegion() : null;
        FlagManager icons = FlagManager.get();
        Image image = icons.getFlag(languageCode, regionCode);
        if (image != null) {
            return image;
        } else {
            return FlagManager.getGlobeIcon();
        }
    }

    /**
     * Returns true if this locale specifies a specific language. This is true
     * for all locales except {@link #ANY}.
     *
     * @return true if this locale specifies a specific language
     */
    public boolean hasLanguage() {
        return !qualifier.hasFakeValue();
    }

    /**
     * Returns true if this locale specifies a specific region
     *
     * @return true if this locale specifies a region
     */
    public boolean hasRegion() {
        return qualifier.getRegion() != null && !FAKE_VALUE.equals(qualifier.getRegion());
    }

    /**
     * Returns the locale formatted as language-region. If region is not set,
     * language is returned. If language is not set, empty string is returned.
     */
    public String toLocaleId() {
        return qualifier == ANY_QUALIFIER ? "" : qualifier.getTag();
    }

    @Override
    public int hashCode() {
        final int prime = 31;
        int result = 1;
        result = prime * result + qualifier.hashCode();
        return result;
    }

    @Override
    public boolean equals(@Nullable
    Object obj) {
        if (this == obj)
            return true;
        if (obj == null)
            return false;
        if (getClass() != obj.getClass())
            return false;
        Locale other = (Locale) obj;
        if (!qualifier.equals(other.qualifier))
            return false;
        return true;
    }

    @Override
    public String toString() {
        return qualifier.getTag();
    }
}
