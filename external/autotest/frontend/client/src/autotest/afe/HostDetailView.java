package autotest.afe;

import autotest.afe.create.CreateJobViewPresenter.JobCreateListener;
import autotest.common.CustomHistory.HistoryToken;
import autotest.common.SimpleCallback;
import autotest.common.Utils;
import autotest.common.table.DataSource;
import autotest.common.table.DataSource.DataCallback;
import autotest.common.table.DataSource.Query;
import autotest.common.table.DataSource.SortDirection;
import autotest.common.table.DataTable;
import autotest.common.table.DatetimeSegmentFilter;
import autotest.common.table.DynamicTable;
import autotest.common.table.DynamicTable.DynamicTableListener;
import autotest.common.table.JSONObjectSet;
import autotest.common.table.RpcDataSource;
import autotest.common.table.SelectionManager;
import autotest.common.table.SimpleFilter;
import autotest.common.table.TableDecorator;
import autotest.common.ui.ContextMenu;
import autotest.common.ui.DetailView;
import autotest.common.ui.NotifyManager;
import autotest.common.ui.TableActionsPanel.TableActionsListener;

import com.google.gwt.event.dom.client.ClickEvent;
import com.google.gwt.event.dom.client.ClickHandler;
import com.google.gwt.event.dom.client.KeyCodes;
import com.google.gwt.event.dom.client.KeyPressEvent;
import com.google.gwt.event.dom.client.KeyPressHandler;
import com.google.gwt.event.logical.shared.ValueChangeEvent;
import com.google.gwt.event.logical.shared.ValueChangeHandler;
import com.google.gwt.json.client.JSONArray;
import com.google.gwt.json.client.JSONBoolean;
import com.google.gwt.json.client.JSONNumber;
import com.google.gwt.json.client.JSONObject;
import com.google.gwt.json.client.JSONString;
import com.google.gwt.json.client.JSONValue;
import com.google.gwt.user.client.Command;
import com.google.gwt.user.client.ui.Button;
import com.google.gwt.user.client.ui.CheckBox;
import com.google.gwt.user.client.ui.TextBox;

import java.util.List;
import java.util.Map;
import java.util.Set;

public class HostDetailView extends DetailView implements DataCallback, TableActionsListener {
    private static final String[][] HOST_JOBS_COLUMNS = {
            {DataTable.WIDGET_COLUMN, ""}, {"type", "Type"}, {"job__id", "Job ID"},
            {"job_owner", "Job Owner"}, {"job_name", "Job Name"}, {"started_on", "Time started"},
            {"status", "Status"}
    };
    public static final int JOBS_PER_PAGE = 20;

    public interface HostDetailListener {
        public void onJobSelected(int jobId);
    }

    private static class HostQueueEntryDataSource extends RpcDataSource {
        public HostQueueEntryDataSource() {
            super("get_host_queue_entries", "get_num_host_queue_entries");
        }

        @Override
        protected List<JSONObject> handleJsonResult(JSONValue result) {
            List<JSONObject> resultArray = super.handleJsonResult(result);
            for (JSONObject row : resultArray) {
                // get_host_queue_entries() doesn't return type, so fill it in for consistency with
                // get_host_queue_entries_and_special_tasks()
                row.put("type", new JSONString("Job"));
            }
            return resultArray;
        }
    }

    private static class HostJobsTable extends DynamicTable {
        private static final DataSource normalDataSource = new HostQueueEntryDataSource();
        private static final DataSource dataSourceWithSpecialTasks =
            new RpcDataSource("get_host_queue_entries_and_special_tasks",
                              "get_num_host_queue_entries_and_special_tasks");

        private SimpleFilter hostFilter = new SimpleFilter();
        private String hostId;
        private String startTime;
        private String endTime;

        public HostJobsTable() {
            super(HOST_JOBS_COLUMNS, normalDataSource);
            addFilter(hostFilter);
        }

        public void setHostId(String hostId) {
            this.hostId = hostId;
            updateFilter();
        }

        public void setStartTime(String startTime) {
            this.startTime = startTime;
            updateFilter();
        }

        public void setEndTime(String endTime) {
            this.endTime = endTime;
            updateFilter();
        }

        private void updateFilter() {
            if (getDataSource() == normalDataSource) {
                sortOnColumn("job__id", SortDirection.DESCENDING);
            } else {
                clearSorts();
            }

            hostFilter.clear();
            hostFilter.setParameter("host",
                                    new JSONNumber(Double.parseDouble(hostId)));
            if (startTime != null && startTime != "")
                hostFilter.setParameter("start_time", new JSONString(startTime));
            if (endTime != null && endTime != "")
                hostFilter.setParameter("end_time", new JSONString(endTime));
        }

        public void setSpecialTasksEnabled(boolean enabled) {
            if (enabled) {
                setDataSource(dataSourceWithSpecialTasks);
            } else {
                setDataSource(normalDataSource);
            }

            updateFilter();
        }

        @Override
        protected void preprocessRow(JSONObject row) {
            JSONObject job = row.get("job").isObject();
            JSONString blank = new JSONString("");
            JSONString jobId = blank, owner = blank, name = blank;
            if (job != null) {
                int id = (int) job.get("id").isNumber().doubleValue();
                jobId = new JSONString(Integer.toString(id));
                owner = job.get("owner").isString();
                name = job.get("name").isString();
            }

            row.put("job__id", jobId);
            row.put("job_owner", owner);
            row.put("job_name", name);
        }
    }

    private String hostname = "";
    private String hostId = "";
    private DataSource hostDataSource = new HostDataSource();
    private HostJobsTable jobsTable = new HostJobsTable();
    private TableDecorator tableDecorator = new TableDecorator(jobsTable);
    private HostDetailListener hostDetailListener = null;
    private JobCreateListener jobCreateListener = null;
    private SelectionManager selectionManager;

    private JSONObject currentHostObject;

    private Button lockButton = new Button();
    private Button reverifyButton = new Button("Reverify");
    private Button reinstallButton = new Button("Reinstall");
    private Button repairButton = new Button("Repair");
    private CheckBox showSpecialTasks = new CheckBox();
    private DatetimeSegmentFilter startedTimeFilter = new DatetimeSegmentFilter();
    private TextBox hostnameInput = new TextBox();
    private Button hostnameFetchButton = new Button("Go");
    private TextBox lockReasonInput = new TextBox();

    public HostDetailView(HostDetailListener hostDetailListener,
                          JobCreateListener jobCreateListener) {
        this.hostDetailListener = hostDetailListener;
        this.jobCreateListener = jobCreateListener;
    }

    @Override
    public String getElementId() {
        return "view_host";
    }

    @Override
    protected String getFetchControlsElementId() {
        return "view_host_fetch_controls";
    }

    private String getFetchByHostnameControlsElementId() {
        return "view_host_fetch_by_hostname_controls";
    }

    @Override
    protected String getDataElementId() {
        return "view_host_data";
    }

    @Override
    protected String getTitleElementId() {
        return "view_host_title";
    }

    @Override
    protected String getNoObjectText() {
        return "No host selected";
    }

    @Override
    protected String getObjectId() {
        return hostId;
    }

    protected String getHostname() {
        return hostname;
    }

    @Override
    protected void setObjectId(String id) {
        if (id.length() == 0) {
            throw new IllegalArgumentException();
        }
        this.hostId = id;
    }

    @Override
    protected void fetchData() {
        JSONObject params = new JSONObject();
        params.put("id", new JSONString(getObjectId()));
        fetchDataCommmon(params);
    }

    private void fetchDataByHostname(String hostname) {
        JSONObject params = new JSONObject();
        params.put("hostname", new JSONString(hostname));
        fetchDataCommmon(params);
    }

    private void fetchDataCommmon(JSONObject params) {
        params.put("valid_only", JSONBoolean.getInstance(false));
        hostDataSource.query(params, this);
    }

    private void fetchByHostname(String hostname) {
        fetchDataByHostname(hostname);
        updateHistory();
    }

    @Override
    public void handleTotalResultCount(int totalCount) {}

    @Override
    public void onQueryReady(Query query) {
        query.getPage(null, null, null, this);
    }

    public void handlePage(List<JSONObject> data) {
        try {
            currentHostObject = Utils.getSingleObjectFromList(data);
        }
        catch (IllegalArgumentException exc) {
            NotifyManager.getInstance().showError("No such host found");
            resetPage();
            return;
        }

        setObjectId(currentHostObject.get("id").toString());

        String lockedText = Utils.jsonToString(currentHostObject.get(HostDataSource.LOCKED_TEXT));
        if (currentHostObject.get("locked").isBoolean().booleanValue()) {
            String lockedBy = Utils.jsonToString(currentHostObject.get("locked_by"));
            String lockedTime = Utils.jsonToString(currentHostObject.get("lock_time"));
            String lockReasonText = Utils.jsonToString(currentHostObject.get("lock_reason"));
            lockedText += ", by " + lockedBy + " on " + lockedTime;
            lockedText += ", reason: " + lockReasonText;
        }

        showField(currentHostObject, "status", "view_host_status");
        showField(currentHostObject, "platform", "view_host_platform");
        showField(currentHostObject, HostDataSource.HOST_ACLS, "view_host_acls");
        showField(currentHostObject, HostDataSource.OTHER_LABELS, "view_host_labels");
        showText(lockedText, "view_host_locked");
        String shard_url = Utils.jsonToString(currentHostObject.get("shard")).trim();
        String host_id = Utils.jsonToString(currentHostObject.get("id")).trim();
        if (shard_url.equals("<null>")){
            shard_url = "";
        } else {
            shard_url = "http://" + shard_url;
        }
        shard_url = shard_url + "/afe/#tab_id=view_host&object_id=" + host_id;
        showField(currentHostObject, "shard", "view_host_shard");
        getElementById("view_host_shard").setAttribute("href", shard_url);

        String job_id = Utils.jsonToString(currentHostObject.get("current_job")).trim();
        if (!job_id.equals("<null>")){
            String job_url = "#tab_id=view_job&object_id=" + job_id;
            showField(currentHostObject, "current_job", "view_host_current_job");
            getElementById("view_host_current_job").setAttribute("href", job_url);
        }
        hostname = currentHostObject.get("hostname").isString().stringValue();

        String task = Utils.jsonToString(currentHostObject.get("current_special_task")).trim();
        if (!task.equals("<null>")){
            String task_url = Utils.getRetrieveLogsUrl("hosts/" + hostname + "/" + task);
            showField(currentHostObject, "current_special_task", "view_host_current_special_task");
            getElementById("view_host_current_special_task").setAttribute("href", task_url);
        }

        showField(currentHostObject, "protection", "view_host_protection");
        String pageTitle = "Host " + hostname;
        hostnameInput.setText(hostname);
        hostnameInput.setWidth("240px");
        updateLockButton();
        updateLockReasonInput();
        displayObjectData(pageTitle);

        jobsTable.setHostId(getObjectId());
        jobsTable.refresh();
    }

    @Override
    public void initialize() {
        super.initialize();

        // Replace fetch by id with fetch by hostname
        addWidget(hostnameInput, getFetchByHostnameControlsElementId());
        addWidget(hostnameFetchButton, getFetchByHostnameControlsElementId());

        hostnameInput.addKeyPressHandler(new KeyPressHandler() {
            public void onKeyPress (KeyPressEvent event) {
                if (event.getCharCode() == (char) KeyCodes.KEY_ENTER)
                    fetchByHostname(hostnameInput.getText());
            }
        });
        hostnameFetchButton.addClickHandler(new ClickHandler() {
            public void onClick(ClickEvent event) {
                fetchByHostname(hostnameInput.getText());
            }
        });

        jobsTable.setRowsPerPage(JOBS_PER_PAGE);
        jobsTable.setClickable(true);
        jobsTable.addListener(new DynamicTableListener() {
            public void onRowClicked(int rowIndex, JSONObject row, boolean isRightClick) {
                if (isJobRow(row)) {
                    JSONObject job = row.get("job").isObject();
                    int jobId = (int) job.get("id").isNumber().doubleValue();
                    hostDetailListener.onJobSelected(jobId);
                } else {
                    String resultsPath = Utils.jsonToString(row.get("execution_path"));
                    Utils.openUrlInNewWindow(Utils.getRetrieveLogsUrl(resultsPath));
                }
            }

            public void onTableRefreshed() {}
        });
        tableDecorator.addPaginators();
        selectionManager = tableDecorator.addSelectionManager(false);
        jobsTable.setWidgetFactory(selectionManager);
        tableDecorator.addTableActionsPanel(this, true);
        tableDecorator.addControl("Show verifies, repairs, cleanups and resets",
                                  showSpecialTasks);
        tableDecorator.addFilter("Filter by time started:",
                                 startedTimeFilter);
        addWidget(tableDecorator, "view_host_jobs_table");

        showSpecialTasks.addClickHandler(new ClickHandler() {
            public void onClick(ClickEvent event) {
                selectionManager.deselectAll();
                jobsTable.setSpecialTasksEnabled(showSpecialTasks.getValue());
                jobsTable.refresh();
            }
        });

        startedTimeFilter.addValueChangeHandler(
            new ValueChangeHandler() {
                public void onValueChange(ValueChangeEvent event) {
                    String value = (String) event.getValue();
                    jobsTable.setStartTime(value);
                    if (value == "")
                        startedTimeFilter.setStartTimeToPlaceHolderValue();
                    jobsTable.refresh();
                }
            },
            new ValueChangeHandler() {
                public void onValueChange(ValueChangeEvent event) {
                    String value = (String) event.getValue();
                    jobsTable.setEndTime(value);
                    if (value == "")
                        startedTimeFilter.setEndTimeToPlaceHolderValue();
                    jobsTable.refresh();
                }
            }
        );

        addWidget(lockButton, "view_host_lock_button");
        addWidget(lockReasonInput, "view_host_lock_reason_input");

        lockButton.addClickHandler(new ClickHandler() {
            public void onClick(ClickEvent event) {
               boolean locked = currentHostObject.get("locked").isBoolean().booleanValue();
               changeLock(!locked);
            }
        });
        lockReasonInput.addKeyPressHandler(new KeyPressHandler() {
            public void onKeyPress (KeyPressEvent event) {
                if (event.getCharCode() == (char) KeyCodes.KEY_ENTER) {
                    boolean locked = currentHostObject.get("locked").isBoolean().booleanValue();
                    changeLock(!locked);
                }
            }
        });

        reverifyButton.addClickHandler(new ClickHandler() {
            public void onClick(ClickEvent event) {
                JSONObject params = new JSONObject();

                params.put("id", currentHostObject.get("id"));
                AfeUtils.callReverify(params, new SimpleCallback() {
                    public void doCallback(Object source) {
                       refresh();
                    }
                }, "Host " + hostname);
            }
        });
        addWidget(reverifyButton, "view_host_reverify_button");

        reinstallButton.addClickHandler(new ClickHandler() {
            public void onClick(ClickEvent event) {
                JSONArray array = new JSONArray();
                array.set(0, new JSONString(hostname));
                AfeUtils.scheduleReinstall(array, hostname, jobCreateListener);
            }
        });
        addWidget(reinstallButton, "view_host_reinstall_button");

        repairButton.addClickHandler(new ClickHandler() {
            public void onClick(ClickEvent event) {
                JSONObject params = new JSONObject();

                params.put("id", currentHostObject.get("id"));
                AfeUtils.callRepair(params, new SimpleCallback() {
                    public void doCallback(Object source) {
                       refresh();
                    }
                }, "Host " + hostname);
            }
        });
        addWidget(repairButton, "view_host_repair_button");
    }

    public void onError(JSONObject errorObject) {
        // RPC handler will display error
    }

    public ContextMenu getActionMenu() {
        ContextMenu menu = new ContextMenu();
        menu.addItem("Abort", new Command() {
            public void execute() {
                abortSelectedQueueEntriesAndSpecialTasks();
            }
        });
        if (selectionManager.isEmpty())
            menu.setEnabled(false);
        return menu;
    }

    private void abortSelectedQueueEntriesAndSpecialTasks() {
        Set<JSONObject> selectedEntries = selectionManager.getSelectedObjects();
        Set<JSONObject> selectedQueueEntries = new JSONObjectSet<JSONObject>();
        JSONArray selectedSpecialTaskIds = new JSONArray();
        for (JSONObject entry : selectedEntries) {
            if (isJobRow(entry))
                selectedQueueEntries.add(entry);
            else
                selectedSpecialTaskIds.set(selectedSpecialTaskIds.size(),
                                           entry.get("oid"));
        }
        if (!selectedQueueEntries.isEmpty()) {
            AfeUtils.abortHostQueueEntries(selectedQueueEntries, new SimpleCallback() {
                public void doCallback(Object source) {
                    refresh();
                }
            });
        }
        if (selectedSpecialTaskIds.size() > 0) {
            AfeUtils.abortSpecialTasks(selectedSpecialTaskIds, new SimpleCallback() {
                public void doCallback(Object source) {
                    refresh();
                }
            });
        }
    }

    private void updateLockButton() {
        boolean locked = currentHostObject.get("locked").isBoolean().booleanValue();
        if (locked) {
            lockButton.setText("Unlock");
        } else {
            lockButton.setText("Lock");
        }
    }

    private void updateLockReasonInput() {
        boolean locked = currentHostObject.get("locked").isBoolean().booleanValue();
        if (locked) {
            lockReasonInput.setText("");
            lockReasonInput.setEnabled(false);
        } else {
            lockReasonInput.setEnabled(true);
        }
    }

    private void changeLock(final boolean lock) {
        JSONArray hostIds = new JSONArray();
        hostIds.set(0, currentHostObject.get("id"));

        AfeUtils.changeHostLocks(hostIds, lock, lockReasonInput.getText(),
                                 "Host " + hostname, new SimpleCallback() {
            public void doCallback(Object source) {
                refresh();
            }
        });
    }

    private boolean isJobRow(JSONObject row) {
        String type = Utils.jsonToString(row.get("type"));
        return type.equals("Job");
    }

    public boolean isRowSelectable(JSONObject row) {
        return isJobRow(row);
    }

    @Override
    public void handleHistoryArguments(Map<String, String> arguments) {
        String hostname = arguments.get("hostname");
        String objectId = arguments.get("object_id");

        if (objectId != null) {
            try {
                updateObjectId(objectId);
            }
            catch (IllegalArgumentException exc) {
                return;
            }
        } else if (hostname != null) {
            fetchDataByHostname(hostname);
        } else {
            resetPage();
        }
    }
}
