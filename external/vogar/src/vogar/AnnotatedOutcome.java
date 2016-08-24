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

import com.google.common.collect.Lists;
import com.google.common.collect.Ordering;
import java.util.ArrayList;
import java.util.List;
import java.util.SortedMap;
import java.util.TreeMap;

/**
 * Contains an outcome for a test, along with some metadata pertaining to the history of this test,
 * including a list of previous outcomes, an outcome corresponding to the tag Vogar is being run
 * with, if applicable, and the expectation for this test, so that result value information is
 * available.
 */
public final class AnnotatedOutcome {
    public static Ordering<AnnotatedOutcome> ORDER_BY_NAME = new Ordering<AnnotatedOutcome>() {
        @Override public int compare(AnnotatedOutcome a, AnnotatedOutcome b) {
            return a.getName().compareTo(b.getName());
       }
    };

    private final Expectation expectation;
    private final Outcome outcome;
    /** a list of previous outcomes for the same action, sorted in chronological order */
    private final SortedMap<Long, Outcome> previousOutcomes = new TreeMap<Long, Outcome>();

    AnnotatedOutcome(Outcome outcome, Expectation expectation) {
        this.expectation = expectation;
        this.outcome = outcome;
    }

    public void add(long date, Outcome outcome) {
        previousOutcomes.put(date, outcome);
    }

    public Outcome getOutcome() {
        return outcome;
    }

    public String getName() {
        return outcome.getName();
    }

    public ResultValue getResultValue() {
        return outcome.getResultValue(expectation);
    }

    public List<ResultValue> getPreviousResultValues() {
        List<ResultValue> previousResultValues = new ArrayList<ResultValue>();
        for (Outcome previousOutcome : previousOutcomes.values()) {
            previousResultValues.add(previousOutcome.getResultValue(expectation));
        }
        return previousResultValues;
    }

    /**
     * Returns the most recent result value of a run of this test (before the current run).
     */
    public ResultValue getMostRecentResultValue(ResultValue defaultValue) {
        List<ResultValue> previousResultValues = getPreviousResultValues();
        return previousResultValues.isEmpty() ?
                defaultValue :
                previousResultValues.get(previousResultValues.size() - 1);
    }

    /**
     * Returns true if the outcome is noteworthy given the result value and previous history.
     */
    public boolean isNoteworthy() {
        return getResultValue() != ResultValue.OK || recentlyChanged();
    }

    /**
     * Returns true if the outcome recently changed in result value.
     */
    private boolean recentlyChanged() {
        List<ResultValue> previousResultValues = getPreviousResultValues();
        if (previousResultValues.isEmpty()) {
            return false;
        }
        return previousResultValues.get(previousResultValues.size() - 1) != getResultValue();
    }

    /**
     * Returns a Long representing the time the outcome was last run. Returns {@code defaultValue}
     * if the outcome is not known to have run before.
     */
    public Long lastRun(Long defaultValue) {
        List<Long> runTimes = Lists.newArrayList(previousOutcomes.keySet());
        return runTimes.isEmpty() ? defaultValue : runTimes.get(runTimes.size() - 1);
    }
}
