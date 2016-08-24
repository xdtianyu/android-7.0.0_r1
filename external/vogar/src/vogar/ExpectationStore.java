/*
 * Copyright (C) 2010 The Android Open Source Project
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

package vogar;

import com.google.common.base.Joiner;
import com.google.common.base.Splitter;
import com.google.common.collect.Iterables;
import com.google.gson.stream.JsonReader;
import java.io.File;
import java.io.FileReader;
import java.io.IOException;
import java.util.EnumSet;
import java.util.EnumMap;
import java.util.LinkedHashMap;
import java.util.LinkedHashSet;
import java.util.Map;
import java.util.Set;
import java.util.regex.Pattern;

/**
 * A database of expected outcomes. Entries in this database come in two forms.
 * <ul>
 *   <li>Outcome expectations name an outcome (or its prefix, such as
 *       "java.util"), its expected result, and an optional pattern to match
 *       the expected output.
 *   <li>Failure expectations include a pattern that may match the output of any
 *       outcome. These expectations are useful for hiding failures caused by
 *       cross-cutting features that aren't supported.
 * </ul>
 *
 * <p>If an outcome matches both an outcome expectation and a failure
 * expectation, the outcome expectation will be returned.
 */
final class ExpectationStore {
    private static final int PATTERN_FLAGS = Pattern.MULTILINE | Pattern.DOTALL;

    private final Log log;
    private final Map<String, Expectation> outcomes = new LinkedHashMap<String, Expectation>();
    private final Map<String, Expectation> failures = new LinkedHashMap<String, Expectation>();

    private ExpectationStore(Log log) {
        this.log = log;
    }

    /**
     * Finds the expected result for the specified action or outcome name. This
     * returns a value for all names, even if no explicit expectation was set.
     */
    public Expectation get(String name) {
        Expectation byName = getByNameOrPackage(name);
        return byName != null ? byName : Expectation.SUCCESS;
    }

    /**
     * Finds the expected result for the specified outcome after it has
     * completed. Unlike {@code get()}, this also takes into account the
     * outcome's output.
     *
     * <p>For outcomes that have both a name match and an output match,
     * exact name matches are preferred, then output matches, then inexact
     * name matches.
     */
    public Expectation get(Outcome outcome) {
        Expectation exactNameMatch = outcomes.get(outcome.getName());
        if (exactNameMatch != null) {
            return exactNameMatch;
        }

        for (Map.Entry<String, Expectation> entry : failures.entrySet()) {
            if (entry.getValue().matches(outcome)) {
                return entry.getValue();
            }
        }

        Expectation byName = getByNameOrPackage(outcome.getName());
        return byName != null ? byName : Expectation.SUCCESS;
    }

    private Expectation getByNameOrPackage(String name) {
        while (true) {
            Expectation expectation = outcomes.get(name);
            if (expectation != null) {
                return expectation;
            }

            int dotOrHash = Math.max(name.lastIndexOf('.'), name.lastIndexOf('#'));
            if (dotOrHash == -1) {
                return null;
            }

            name = name.substring(0, dotOrHash);
        }
    }

    public static ExpectationStore parse(Log log,
                                         Set<File> expectationFiles,
                                         ModeId mode,
                                         Variant variant)
            throws IOException {
        ExpectationStore result = new ExpectationStore(log);
        for (File f : expectationFiles) {
            if (f.exists()) {
                result.parse(f, mode, variant);
            }
        }
        return result;
    }

    public void parse(File expectationsFile, ModeId mode, Variant variant) throws IOException {
        log.verbose("loading expectations file " + expectationsFile);

        int count = 0;
        JsonReader reader = null;
        try {
            reader = new JsonReader(new FileReader(expectationsFile));
            reader.setLenient(true);
            reader.beginArray();
            while (reader.hasNext()) {
                readExpectation(reader, mode, variant);
                count++;
            }
            reader.endArray();

            log.verbose("loaded " + count + " expectations from " + expectationsFile);
        } finally {
            if (reader != null) {
                reader.close();
            }
        }
    }

    private void readExpectation(JsonReader reader, ModeId mode, Variant variant)
          throws IOException {
        boolean isFailure = false;
        Result result = Result.SUCCESS;
        Pattern pattern = Expectation.MATCH_ALL_PATTERN;
        Set<String> names = new LinkedHashSet<String>();
        Set<String> tags = new LinkedHashSet<String>();
        Map<ModeId, Set<Variant>> modeVariants = null;
        Set<ModeId> modes = null;
        String description = "";
        long buganizerBug = -1;

        reader.beginObject();
        while (reader.hasNext()) {
            String name = reader.nextName();
            if (name.equals("result")) {
                result = Result.valueOf(reader.nextString());
            } else if (name.equals("name")) {
                names.add(reader.nextString());
            } else if (name.equals("names")) {
                readStrings(reader, names);
            } else if (name.equals("failure")) {
                isFailure = true;
                names.add(reader.nextString());
            } else if (name.equals("pattern")) {
                pattern = Pattern.compile(reader.nextString(), PATTERN_FLAGS);
            } else if (name.equals("substring")) {
                pattern = Pattern.compile(
                        ".*" + Pattern.quote(reader.nextString()) + ".*", PATTERN_FLAGS);
            } else if (name.equals("tags")) {
                readStrings(reader, tags);
            } else if (name.equals("description")) {
                Iterable<String> split = Splitter.on("\n").omitEmptyStrings().trimResults()
                        .split(reader.nextString());
                description = Joiner.on("\n").join(split);
            } else if (name.equals("bug")) {
                buganizerBug = reader.nextLong();
            } else if (name.equals("modes")) {
                modes = readModes(reader);
            } else if (name.equals("modes_variants")) {
                modeVariants = readModesAndVariants(reader);
            } else {
                log.warn("Unhandled name in expectations file: " + name);
                reader.skipValue();
            }
        }
        reader.endObject();

        if (names.isEmpty()) {
            throw new IllegalArgumentException("Missing 'name' or 'failure' key in " + reader);
        }
        if (modes != null && !modes.contains(mode)) {
            return;
        }
        if (modeVariants != null) {
            Set<Variant> variants = modeVariants.get(mode);
            if (variants == null || !variants.contains(variant)) {
                return;
            }
        }

        Expectation expectation =
              new Expectation(result, pattern, tags, description, buganizerBug, true);
        Map<String, Expectation> map = isFailure ? failures : outcomes;
        for (String name : names) {
            if (map.put(name, expectation) != null) {
                throw new IllegalArgumentException("Duplicate expectations for " + name);
            }
        }
    }

    private void readStrings(JsonReader reader, Set<String> output) throws IOException {
        reader.beginArray();
        while (reader.hasNext()) {
            output.add(reader.nextString());
        }
        reader.endArray();
    }

    private Set<ModeId> readModes(JsonReader reader) throws IOException {
        Set<ModeId> result = EnumSet.noneOf(ModeId.class);
        reader.beginArray();
        while (reader.hasNext()) {
            result.add(ModeId.valueOf(reader.nextString().toUpperCase()));
        }
        reader.endArray();
        return result;
    }

    /**
     * Expected format: mode_variants: [["host", "X32"], ["host", "X64"]]
     */
    private Map<ModeId, Set<Variant>> readModesAndVariants(JsonReader reader) throws IOException {
        Map<ModeId, Set<Variant>> result = new EnumMap<ModeId, Set<Variant>>(ModeId.class);
        reader.beginArray();
        while (reader.hasNext()) {
            reader.beginArray();
            ModeId mode = ModeId.valueOf(reader.nextString().toUpperCase());
            Set<Variant> set = result.get(mode);
            if (set == null) {
                set = EnumSet.noneOf(Variant.class);
                result.put(mode, set);
            }
            set.add(Variant.valueOf(reader.nextString().toUpperCase()));
            // Note that the following checks that we are at the end of the array.
            reader.endArray();
        }
        reader.endArray();
        return result;
    }

    /**
     * Sets the bugIsOpen status on all expectations by querying an external bug
     * tracker.
     */
    public void loadBugStatuses(BugDatabase bugDatabase) {
        Iterable<Expectation> allExpectations
                = Iterables.concat(outcomes.values(), failures.values());

        // figure out what bug IDs we're interested in
        Set<Long> bugs = new LinkedHashSet<Long>();
        for (Expectation expectation : allExpectations) {
            if (expectation.getBug() != -1) {
                bugs.add(expectation.getBug());
            }
        }
        if (bugs.isEmpty()) {
            return;
        }

        Set<Long> openBugs = bugDatabase.bugsToOpenBugs(bugs);

        log.verbose("tracking " + openBugs.size() + " open bugs: " + openBugs);

        // update our expectations with that set
        for (Expectation expectation : allExpectations) {
            if (openBugs.contains(expectation.getBug())) {
                expectation.setBugIsOpen(true);
            }
        }
    }

    interface BugDatabase {
        Set<Long> bugsToOpenBugs(Set<Long> bugs);
    }

}
