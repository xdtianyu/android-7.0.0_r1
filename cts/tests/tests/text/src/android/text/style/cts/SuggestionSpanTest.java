/*
 * Copyright (C) 2016 The Android Open Source Project
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

package android.text.style.cts;

import junit.framework.TestCase;

import android.annotation.NonNull;
import android.annotation.Nullable;
import android.os.Parcel;
import android.text.style.SuggestionSpan;

import java.util.Locale;

public class SuggestionSpanTest extends TestCase {

    /**
     * @param locale a {@link Locale} object.
     * @return A well-formed BCP 47 language tag representation.
     */
    @Nullable
    private Locale toWellFormedLocale(@Nullable final Locale locale) {
        if (locale == null) {
            return null;
        }
        // Drop all the malformed data.
        return Locale.forLanguageTag(locale.toLanguageTag());
    }

    @NonNull
    private String getNonNullLocaleString(@Nullable final Locale original) {
        if (original == null) {
            return "";
        }
        return original.toString();
    }

    private void checkGetLocaleObject(final Locale locale) {
        final SuggestionSpan span = new SuggestionSpan(locale, new String[0],
                SuggestionSpan.FLAG_AUTO_CORRECTION);
        // In the context of SuggestionSpan#getLocaleObject(), we do care only about subtags that
        // can be interpreted as LanguageTag.
        assertEquals(toWellFormedLocale(locale), span.getLocaleObject());
        assertEquals(getNonNullLocaleString(locale), span.getLocale());

        final SuggestionSpan cloned = cloneViaParcel(span);
        assertEquals(span, cloned);
        assertEquals(toWellFormedLocale(locale), cloned.getLocaleObject());
        assertEquals(getNonNullLocaleString(locale), cloned.getLocale());
    }

    public void testGetLocaleObject() {
        checkGetLocaleObject(Locale.forLanguageTag("en"));
        checkGetLocaleObject(Locale.forLanguageTag("en-GB"));
        checkGetLocaleObject(Locale.forLanguageTag("EN-GB"));
        checkGetLocaleObject(Locale.forLanguageTag("en-gb"));
        checkGetLocaleObject(Locale.forLanguageTag("En-gB"));
        checkGetLocaleObject(Locale.forLanguageTag("und"));
        checkGetLocaleObject(Locale.forLanguageTag("de-DE-u-co-phonebk"));
        checkGetLocaleObject(Locale.forLanguageTag(""));
        checkGetLocaleObject(null);
        checkGetLocaleObject(new Locale(" an  ", " i n v a l i d ", "data"));
    }

    @NonNull
    SuggestionSpan cloneViaParcel(@NonNull final SuggestionSpan original) {
        Parcel parcel = null;
        try {
            parcel = Parcel.obtain();
            original.writeToParcel(parcel, 0);
            parcel.setDataPosition(0);
            return new SuggestionSpan(parcel);
        } finally {
            if (parcel != null) {
                parcel.recycle();
            }
        }
    }
}
