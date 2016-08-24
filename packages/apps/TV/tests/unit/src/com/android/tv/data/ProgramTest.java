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
package com.android.tv.data;

import static android.media.tv.TvContract.Programs.Genres.COMEDY;
import static android.media.tv.TvContract.Programs.Genres.FAMILY_KIDS;

import android.test.suitebuilder.annotation.SmallTest;

import junit.framework.TestCase;

import java.util.Arrays;

/**
 * Tests for {@link Program}.
 */
@SmallTest
public class ProgramTest extends TestCase {

    private static final int NOT_FOUND_GENRE = 987;

    private static final int FAMILY_GENRE_ID = GenreItems.getId(FAMILY_KIDS);

    private static final int COMEDY_GENRE_ID = GenreItems.getId(COMEDY);

    public void testBuild() {
        Program program = new Program.Builder().build();
        assertEquals("isValid", false, program.isValid());
    }

    public void testNoGenres() {
        Program program = new Program.Builder()
                .setCanonicalGenres("")
                .build();
        assertNullCanonicalGenres(program);
        assertHasGenre(program, NOT_FOUND_GENRE, false);
        assertHasGenre(program, FAMILY_GENRE_ID, false);
        assertHasGenre(program, COMEDY_GENRE_ID, false);
        assertHasGenre(program, GenreItems.ID_ALL_CHANNELS, true);
    }

    public void testFamilyGenre() {
        Program program = new Program.Builder()
                .setCanonicalGenres(FAMILY_KIDS)
                .build();
        assertCanonicalGenres(program, FAMILY_KIDS);
        assertHasGenre(program, NOT_FOUND_GENRE, false);
        assertHasGenre(program, FAMILY_GENRE_ID, true);
        assertHasGenre(program, COMEDY_GENRE_ID, false);
        assertHasGenre(program, GenreItems.ID_ALL_CHANNELS, true);
    }

    public void testFamilyComedyGenre() {
        Program program = new Program.Builder()
                .setCanonicalGenres(FAMILY_KIDS + ", " + COMEDY)
                .build();
        assertCanonicalGenres(program, FAMILY_KIDS, COMEDY);
        assertHasGenre(program, NOT_FOUND_GENRE, false);
        assertHasGenre(program, FAMILY_GENRE_ID, true);
        assertHasGenre(program, COMEDY_GENRE_ID, true);
        assertHasGenre(program, GenreItems.ID_ALL_CHANNELS, true);
    }

    public void testOtherGenre() {
        Program program = new Program.Builder()
                .setCanonicalGenres("other")
                .build();
        assertCanonicalGenres(program);
        assertHasGenre(program, NOT_FOUND_GENRE, false);
        assertHasGenre(program, FAMILY_GENRE_ID, false);
        assertHasGenre(program, COMEDY_GENRE_ID, false);
        assertHasGenre(program, GenreItems.ID_ALL_CHANNELS, true);
    }

    private static void assertNullCanonicalGenres(Program program) {
        String[] actual = program.getCanonicalGenres();
        assertNull("Expected null canonical genres but was " + Arrays.toString(actual), actual);
    }

    private static void assertCanonicalGenres(Program program, String... expected) {
        assertEquals("canonical genres", Arrays.asList(expected),
                Arrays.asList(program.getCanonicalGenres()));
    }

    private static void assertHasGenre(Program program, int genreId, boolean expected) {
        assertEquals("hasGenre(" + genreId + ")", expected, program.hasGenre(genreId));
    }
}
