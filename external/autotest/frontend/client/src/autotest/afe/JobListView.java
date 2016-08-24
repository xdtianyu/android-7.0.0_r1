package autotest.afe;

import autotest.common.SimpleCallback;
import autotest.common.CustomHistory.HistoryToken;
import autotest.common.table.Filter;
import autotest.common.table.RadioButtonSetFilter;
import autotest.common.table.SearchFilter;
import autotest.common.table.SelectionManager;
import autotest.common.table.TableDecorator;
import autotest.common.table.DynamicTable.DynamicTableListener;
import autotest.common.ui.ContextMenu;
import autotest.common.ui.NotifyManager;
import autotest.common.ui.TabView;
import autotest.common.ui.TableActionsPanel.TableActionsListener;
import autotest.common.ui.ToolTip;

import com.google.gwt.json.client.JSONArray;
import com.google.gwt.json.client.JSONBoolean;
import com.google.gwt.json.client.JSONObject;
import com.google.gwt.user.client.Command;
import com.google.gwt.user.client.ui.HorizontalPanel;
import com.google.gwt.user.client.ui.HTMLPanel;

import java.util.Map;
import java.util.Set;


public class JobListView extends TabView implements TableActionsListener {
    protected static final String SELECTED_LINK_STYLE = "selected-link";
    protected static final int JOBS_PER_PAGE = 30;
    protected static final int STATUS_QUEUED = 0, STATUS_RUNNING = 1,
                               STATUS_FINISHED = 2, STATUS_ALL = 3,
                               STATUS_RADIO_BUTTON_COUNT = 4;
    protected static final int STATUS_DEFAULT_BUTTON = STATUS_ALL;
    protected static final int TYPE_SUITE = 0, TYPE_SUB = 1, TYPE_STANDALONE = 2,
                               TYPE_ALL = 3, TYPE_RADIO_BUTTON_COUNT = 4;
    protected static final int TYPE_DEFAULT_BUTTON = TYPE_ALL;
    private static final String[] statusHistoryTokens = {"queued", "running",
                                                         "finished", "all"};
    private static final String[] statusRadioButtonLabels = {"Queued Jobs", "Running Jobs",
                                                             "Finished Jobs", "All Jobs"};
    private static final String[] statusFilterStrings = {"not_yet_run", "running",
                                                         "finished"};
    private static final String[] typeHistoryTokens = {"suite", "sub", "standalone",
                                                       "all"};
    private static final String[] typeRadioButtonLabels = {"Parent Jobs", "Child Jobs",
                                                            "Standalone Jobs",
                                                            "All Jobs"};
    private static final String[] typeFilterStrings = {"suite", "sub", "standalone"};

    private JobSelectListener selectListener;

    private JobTable jobTable;
    private TableDecorator tableDecorator;
    private JobStateFilter jobStateFilter;
    private JobTypeFilter jobTypeFilter;
    private Filter ownerFilter;
    private SearchFilter nameFilter;
    private SelectionManager selectionManager;

    interface JobSelectListener {
        public void onJobSelected(int jobId);
    }

    static class JobStateFilter extends RadioButtonSetFilter {
        public JobStateFilter() {
            super("job-status");
        }

        @Override
        public void addParams(JSONObject params) {
            params.put(statusFilterStrings[getSelectedButtonIndex()],
                       JSONBoolean.getInstance(true));
        }

        @Override
        public boolean isActive() {
            return getSelectedButtonIndex() < STATUS_ALL;
        }
    }

    static class JobTypeFilter extends RadioButtonSetFilter {
        public JobTypeFilter() {
            super("job-type");
        }

        @Override
        public void addParams(JSONObject params) {
            params.put(typeFilterStrings[getSelectedButtonIndex()],
                       JSONBoolean.getInstance(true));
        }

        @Override
        public boolean isActive() {
            return getSelectedButtonIndex() < TYPE_ALL;
        }
    }

    public void abortSelectedJobs() {
        Set<JSONObject> selectedSet = selectionManager.getSelectedObjects();
        if (selectedSet.isEmpty()) {
            NotifyManager.getInstance().showError("No jobs selected");
            return;
        }

        JSONArray ids = new JSONArray();
        for(JSONObject jsonObj : selectedSet) {
            ids.set(ids.size(), jsonObj.get("id"));
        }

        JSONObject params = new JSONObject();
        params.put("job__id__in", ids);
        AfeUtils.callAbort(params, new SimpleCallback() {
            public void doCallback(Object source) {
               refresh();
            }
        });
    }
    
    @Override
    public String getElementId() {
        return "job_list";
    }

    @Override
    public void refresh() {
        super.refresh();
        jobTable.refresh();
    }

    public JobListView(JobSelectListener listener) {
        selectListener = listener;
    }
    
    @Override
    public void initialize() {
        super.initialize();
        jobTable = new JobTable();
        jobTable.setRowsPerPage(JOBS_PER_PAGE);
        jobTable.setClickable(true);
        jobTable.addListener(new DynamicTableListener() {
            public void onRowClicked(int rowIndex, JSONObject row, boolean isRightClick) {
                int jobId = (int) row.get("id").isNumber().doubleValue();
                selectListener.onJobSelected(jobId);
            }

            public void onTableRefreshed() {}
        });
        
        tableDecorator = new TableDecorator(jobTable);
        tableDecorator.addPaginators();
        selectionManager = tableDecorator.addSelectionManager(false);
        jobTable.setWidgetFactory(selectionManager);
        tableDecorator.addTableActionsPanel(this, true);
        addWidget(tableDecorator, "job_table");
        
        ownerFilter = new JobOwnerFilter("owner");
        jobTable.addFilter(ownerFilter);
        addWidget(ownerFilter.getWidget(), "user_list");
        
        nameFilter = new SearchFilter("name", "Filter", false);
        jobTable.addFilter(nameFilter);
        addWidget(nameFilter.getWidget(), "jl_name_search");
        
        jobStateFilter = new JobStateFilter();
        for (int i = 0; i < STATUS_RADIO_BUTTON_COUNT; i++)
            jobStateFilter.addRadioButon(statusRadioButtonLabels[i]);
        // All Jobs is selected by default
        jobStateFilter.setSelectedButton(STATUS_DEFAULT_BUTTON);
        jobStateFilter.addCallback(new SimpleCallback() {
            public void doCallback(Object source) {
                updateHistory();
            }
        });
        jobTable.addFilter(jobStateFilter);
        addWidget(jobStateFilter.getWidget(), "job_status_controls");

        jobTypeFilter = new JobTypeFilter();
        for (int i = 0; i < TYPE_RADIO_BUTTON_COUNT; i++)
            jobTypeFilter.addRadioButon(typeRadioButtonLabels[i]);
        // All Jobs is selected by default
        jobTypeFilter.setSelectedButton(TYPE_DEFAULT_BUTTON);
        jobTypeFilter.addCallback(new SimpleCallback() {
            public void doCallback(Object source) {
                updateHistory();
            }
        });
        jobTable.addFilter(jobTypeFilter);
        addWidget(new ToolTip("?", "Suite jobs: jobs with child jobs. " +
                                   "Sub jobs: jobs with a parent jobs. " +
                                   "Standalone jobs: jobs without a parent or child job."),
                  "job_type_tooltip");
        addWidget(jobTypeFilter.getWidget(), "job_type_controls");
    }

    @Override
    public HistoryToken getHistoryArguments() {
        HistoryToken arguments = super.getHistoryArguments();
        arguments.put("state_filter",
                      statusHistoryTokens[jobStateFilter.getSelectedButtonIndex()]);
        arguments.put("type_filter",
                      typeHistoryTokens[jobTypeFilter.getSelectedButtonIndex()]);
        return arguments;
    }
    
    @Override
    public void handleHistoryArguments(Map<String, String> arguments) {
        String stateFilter = arguments.get("state_filter");
        if (stateFilter == null) {
            jobStateFilter.setSelectedButton(STATUS_DEFAULT_BUTTON);
        } else {
            for (int i = 0; i < STATUS_RADIO_BUTTON_COUNT; i++) {
                if (stateFilter.equals(statusHistoryTokens[i])) {
                    jobStateFilter.setSelectedButton(i);
                    break;
                }
            }
        }

        String typeFilter = arguments.get("type_filter");
        if (typeFilter == null) {
            jobTypeFilter.setSelectedButton(TYPE_DEFAULT_BUTTON);
        } else {
            for (int i = 0; i < TYPE_RADIO_BUTTON_COUNT; i++) {
                if (typeFilter.equals(typeHistoryTokens[i])) {
                    jobTypeFilter.setSelectedButton(i);
                    break;
                }
            }
        }
    }

    public ContextMenu getActionMenu() {
        ContextMenu menu = new ContextMenu();
        menu.addItem("Abort jobs", new Command() {
            public void execute() {
                abortSelectedJobs();
            }
        });
        if (selectionManager.isEmpty())
            menu.setEnabled(false);
        return menu;
    }
}
