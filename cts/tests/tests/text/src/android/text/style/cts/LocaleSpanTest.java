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
import android.os.LocaleList;
import android.os.Parcel;
import android.test.suitebuilder.annotation.SmallTest;
import android.text.TextPaint;
import android.text.style.LocaleSpan;

public class LocaleSpanTest extends TestCase {

    private void checkGetLocales(@NonNull final LocaleList locales) {
        final LocaleSpan span = new LocaleSpan(locales);
        assertEquals(locales.get(0), span.getLocale());
        assertEquals(locales, span.getLocales());

        final LocaleSpan cloned = cloneViaParcel(span);
        assertEquals(locales.get(0), cloned.getLocale());
        assertEquals(locales, cloned.getLocales());
    }

    @SmallTest
    public void testGetLocales() {
        checkGetLocales(LocaleList.getEmptyLocaleList());
        checkGetLocales(LocaleList.forLanguageTags("en"));
        checkGetLocales(LocaleList.forLanguageTags("en-GB,en"));
        checkGetLocales(LocaleList.forLanguageTags("de-DE-u-co-phonebk,en-GB,en"));
    }

    @SmallTest
    public void testConstructorWithLocaleList() {
        try {
            new LocaleSpan((LocaleList) null);
        } catch (NullPointerException e) {
            // Expected.
            return;
        }
        fail("NullPointerException must have been thrown.");
    }

    @NonNull
    LocaleSpan cloneViaParcel(@NonNull final LocaleSpan original) {
        Parcel parcel = null;
        try {
            parcel = Parcel.obtain();
            original.writeToParcel(parcel, 0);
            parcel.setDataPosition(0);
            return new LocaleSpan(parcel);
        } finally {
            if (parcel != null) {
                parcel.recycle();
            }
        }
    }

    @SmallTest
    public void testDescribeContents_doesNotThrowException() {
        LocaleSpan localeSpan = new LocaleSpan(LocaleList.getEmptyLocaleList());
        localeSpan.describeContents();
    }

    @SmallTest
    public void testGetSpanTypeId_doesNotThrowException() {
        LocaleSpan localeSpan = new LocaleSpan(LocaleList.getEmptyLocaleList());
        localeSpan.getSpanTypeId();
    }

    @SmallTest
    public void testUpdateDrawState() {
        LocaleList localeListForSpan = LocaleList.forLanguageTags("en");
        LocaleSpan localeSpan = new LocaleSpan(localeListForSpan);

        TextPaint tp = new TextPaint();
        LocaleList localeList = LocaleList.forLanguageTags("fr,de");
        tp.setTextLocales(localeList);
        assertEquals(localeList, tp.getTextLocales());
        assertEquals(localeList.get(0), tp.getTextLocale());

        localeSpan.updateDrawState(tp);
        assertEquals(localeListForSpan, tp.getTextLocales());
        assertEquals(localeListForSpan.get(0), tp.getTextLocale());
    }

    @SmallTest
    public void testUpdateMeasureState() {
        LocaleList localeListForSpan = LocaleList.forLanguageTags("en");
        LocaleSpan localeSpan = new LocaleSpan(localeListForSpan);

        TextPaint tp = new TextPaint();
        LocaleList localeList = LocaleList.forLanguageTags("fr,de");
        tp.setTextLocales(localeList);
        assertEquals(localeList, tp.getTextLocales());
        assertEquals(localeList.get(0), tp.getTextLocale());

        localeSpan.updateMeasureState(tp);
        assertEquals(localeListForSpan, tp.getTextLocales());
        assertEquals(localeListForSpan.get(0), tp.getTextLocale());
    }
}
