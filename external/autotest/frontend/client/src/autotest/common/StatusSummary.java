// Copyright 2008 Google Inc. All Rights Reserved.

package autotest.common;

import com.google.gwt.json.client.JSONObject;

import java.util.Arrays;

public class StatusSummary extends AbstractStatusSummary {
    public int passed = 0;
    public int complete = 0;
    public int incomplete = 0;
    public int total = 0; // TEST_NA is included here, but not in any other

    private String[] contents = null;

    public static StatusSummary getStatusSummary(JSONObject group, String passCountField,
                                                String completeCountField, String incompleteCountField,
                                                String groupCountField) {
        StatusSummary summary = new StatusSummary();
        summary.passed = getField(group, passCountField);
        summary.complete = getField(group, completeCountField);
        summary.incomplete = getField(group, incompleteCountField);
        summary.total = getField(group, groupCountField);

        if (group.containsKey("extra_info")) {
            summary.contents = Utils.JSONtoStrings(group.get("extra_info").isArray());
        }

        return summary;
    }

    private static int getField(JSONObject group, String field) {
        return (int) group.get(field).isNumber().doubleValue();
    }

    /**
     * Force construction to go through getStatusSummary() factory method.
     */
    private StatusSummary() {}

    public int getTotal() {
        return total;
    }

    public String formatContents() {
        String result = formatStatusCounts();

        if (contents != null) {
            result += "<br>";
            result += Utils.joinStrings("<br>", Arrays.asList(contents), true);
        }

        return result;
    }

    @Override
    protected int getComplete() {
        return complete;
    }

    @Override
    protected int getIncomplete() {
        return incomplete;
    }

    @Override
    protected int getPassed() {
        return passed;
    }
}
