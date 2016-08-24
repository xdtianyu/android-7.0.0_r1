package autotest.afe;

import autotest.common.table.DataSource;
import autotest.common.table.DynamicTable;

import com.google.gwt.json.client.JSONObject;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;

public class HostTable extends DynamicTable {
    private static final String[][] HOST_COLUMNS = {
        {"hostname", "Hostname"}, {"platform", "Platform"},
        {HostDataSource.OTHER_LABELS, "Other labels"}, {"status", "Status"},
        {HostDataSource.LOCKED_TEXT, "Locked"},
        {"lock_reason", "Lock Reason"}
    };

    private static final String[][] HOST_COLUMNS_SELECT;

    static {
        ArrayList<String[]> list = new ArrayList<String[]>(Arrays.asList(HOST_COLUMNS));
        list.add(0, new String[] {CLICKABLE_WIDGET_COLUMN, "Select"});
        HOST_COLUMNS_SELECT = list.toArray(new String[0][0]);
    }

    public HostTable(DataSource dataSource) {
        this(dataSource, false);
    }

    public HostTable(DataSource dataSource, boolean wantSelect) {
        super(wantSelect ? HOST_COLUMNS_SELECT : HOST_COLUMNS, dataSource);
    }

    @Override
    public void handlePage(List<JSONObject> data) {
        super.handlePage(data);
        // No wrap on "Hostname" of host table.
        addStyleNameByColumnName("Hostname", "nowrap");
    }
}
