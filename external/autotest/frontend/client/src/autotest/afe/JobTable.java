package autotest.afe;

import autotest.common.StaticDataRepository;
import autotest.common.StatusSummary;
import autotest.common.table.DynamicTable;
import autotest.common.table.RpcDataSource;
import autotest.common.table.DataSource.SortDirection;

import com.google.gwt.json.client.JSONObject;
import com.google.gwt.json.client.JSONArray;
import com.google.gwt.json.client.JSONString;


/**
 * A table to display jobs, including a summary of host queue entries.
 */
public class JobTable extends DynamicTable {
    public static final String HOSTS_SUMMARY = "hosts_summary";
    public static final String RESULTS_SUMMARY = "results_summary";
    public static final String CREATED_TEXT = "created_text";

    protected StaticDataRepository staticData = StaticDataRepository.getRepository();

    private static final String GROUP_COUNT_FIELD = "group_count";
    private static final String PASS_COUNT_FIELD = "pass_count";
    private static final String COMPLETE_COUNT_FIELD = "complete_count";
    private static final String INCOMPLETE_COUNT_FIELD = "incomplete_count";
    private static final String[][] DEFAULT_JOB_COLUMNS = {
        {CLICKABLE_WIDGET_COLUMN, "Select"},
        { "id", "ID" }, { "owner", "Owner" }, { "name", "Name" },
        { "priority", "Priority" }, { "control_type", "Client/Server" },
        { CREATED_TEXT, "Created" }, { HOSTS_SUMMARY, "Status" }
    };

    private static String[][] jobColumns;

    public JobTable() {
        this(DEFAULT_JOB_COLUMNS);
    }

    public JobTable(String[][] jobColumns) {
        super(jobColumns, new RpcDataSource("get_jobs_summary", "get_num_jobs"));
        this.jobColumns = jobColumns;
        sortOnColumn("id", SortDirection.DESCENDING);
    }

    @Override
    protected void preprocessRow(JSONObject row) {
        JSONObject status_counts = row.get("status_counts").isObject();
        String statusCountString = AfeUtils.formatStatusCounts(status_counts, "\n");
        row.put(HOSTS_SUMMARY, new JSONString(statusCountString));

        JSONArray result_counts = row.get("result_counts").isObject().get("groups").isArray();
        if (result_counts.size() > 0) {
            StatusSummary statusSummary = StatusSummary.getStatusSummary(
                result_counts.get(0).isObject(), PASS_COUNT_FIELD,
                COMPLETE_COUNT_FIELD, INCOMPLETE_COUNT_FIELD,
                GROUP_COUNT_FIELD);
            String resultCountString = statusSummary.formatContents();
            row.put(RESULTS_SUMMARY, new JSONString(resultCountString));
        }

        Double priorityValue = row.get("priority").isNumber().getValue();
        String priorityName = staticData.getPriorityName(priorityValue);
        row.put("priority", new JSONString(priorityName));
        
        // remove seconds from created time
        AfeUtils.removeSecondsFromDateField(row, "created_on", CREATED_TEXT);
    }
}
