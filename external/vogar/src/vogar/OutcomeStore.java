/*
 * Copyright (C) 2011 The Android Open Source Project
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

import com.google.gson.stream.JsonReader;
import com.google.gson.stream.JsonWriter;
import java.io.File;
import java.io.FileReader;
import java.io.FileWriter;
import java.io.IOException;
import java.text.SimpleDateFormat;
import java.util.Arrays;
import java.util.Collections;
import java.util.Comparator;
import java.util.Date;
import java.util.LinkedHashMap;
import java.util.Map;
import java.util.TimeZone;
import vogar.commands.Mkdir;
import vogar.commands.Rm;

public final class OutcomeStore {
    private static final String FILE_NAME_DATE_FORMAT = "yyyy-MM-dd'T'HH:mm:ssz";
    private static final int PRESERVE_TOTAL = 10;

    private static final Comparator<File> ORDER_BY_LAST_MODIFIED = new Comparator<File>() {
        @Override public int compare(File a, File b) {
            if (a.lastModified() != b.lastModified()) {
                return a.lastModified() < b.lastModified() ? -1 : 1;
            }
            return 0;
        }
    };

    private final Log log;
    private final Mkdir mkdir;
    private final Rm rm;
    private final File resultsDir;
    private final boolean recordResults;
    private final ExpectationStore expectationStore;
    private final Date date;

    public OutcomeStore(Log log, Mkdir mkdir, Rm rm, File resultsDir, boolean recordResults,
            ExpectationStore expectationStore, Date date) {
        this.log = log;
        this.mkdir = mkdir;
        this.rm = rm;
        this.resultsDir = resultsDir;
        this.recordResults = recordResults;
        this.expectationStore = expectationStore;
        this.date = date;
    }

    public Map<String, AnnotatedOutcome> read(Map<String, Outcome> outcomes) {
        Map<String, AnnotatedOutcome> result = new LinkedHashMap<String, AnnotatedOutcome>();
        for (Map.Entry<String, Outcome> entry : outcomes.entrySet()) {
            Outcome outcome = entry.getValue();
            Expectation expectation = expectationStore.get(outcome);
            result.put(entry.getKey(), new AnnotatedOutcome(outcome, expectation));
        }

        try {
            File[] oldOutcomes = getOutcomeFiles();
            log.verbose("parsing outcomes from " + oldOutcomes.length + " files");
            for (File file : oldOutcomes) {
                if (!file.getName().endsWith(".json")) {
                    continue;
                }

                loadOutcomes(result, file, file.lastModified());
            }
        } catch (IOException e) {
            log.info("Failed to read outcomes from " + resultsDir, e);
        }

        return result;
    }

    private void loadOutcomes(Map<String, AnnotatedOutcome> map, File file, long fileDate)
            throws IOException {
        JsonReader in = new JsonReader(new FileReader(file));
        in.beginObject();
        while (in.hasNext()) {
            String outcomeName = in.nextName();
            AnnotatedOutcome annotatedOutcome = map.get(outcomeName);
            if (annotatedOutcome == null) {
                in.skipValue();
                continue;
            }

            Result result = null;
            in.beginObject();
            while (in.hasNext()) {
                String fieldName = in.nextName();
                if (fieldName.equals("result")) {
                    result = Result.valueOf(in.nextString());
                } else {
                    in.skipValue();
                }
            }
            in.endObject();

            annotatedOutcome.add(fileDate, new Outcome(outcomeName, result,
                    Collections.<String>emptyList()));
        }
        in.endObject();
        in.close();
    }

    public void write(Map<String, Outcome> outcomes) {
        if (!recordResults) {
            return;
        }

        makeRoom();
        File outputFile = getOutputFile();
        try {
            mkdir.mkdirs(outputFile.getParentFile());
            JsonWriter out = new JsonWriter(new FileWriter(outputFile));
            out.setIndent("  ");
            out.beginObject();
            for (Map.Entry<String, Outcome> entry : outcomes.entrySet()) {
                out.name(entry.getKey());
                out.beginObject();
                out.name("result");
                out.value(entry.getValue().getResult().toString());
                out.endObject();
            }
            out.endObject();
            out.close();
        } catch (IOException e) {
            log.info("Failed to write outcomes to " + outputFile, e);
        }
    }

    private File[] getOutcomeFiles() {
        File[] result = resultsDir.listFiles();
        if (result == null) {
            return new File[0];
        }
        Arrays.sort(result, ORDER_BY_LAST_MODIFIED);
        return result;
    }

    private File getOutputFile() {
        SimpleDateFormat dateFormat = new SimpleDateFormat(FILE_NAME_DATE_FORMAT);
        dateFormat.setTimeZone(TimeZone.getTimeZone("UTC"));
        dateFormat.setLenient(true);
        String timestamp = dateFormat.format(date);
        return new File(resultsDir, timestamp + ".json");
    }

    /**
     * Removes the oldest result files until only (PRESERVE_TOTAL - 1) remain.
     */
    private void makeRoom() {
        File[] outcomeFiles = getOutcomeFiles();
        for (int i = 0; i <= (outcomeFiles.length - PRESERVE_TOTAL); i++) {
            rm.file(outcomeFiles[i]);
            log.verbose("garbage collected results file: " + outcomeFiles[i]);
        }
    }
}
