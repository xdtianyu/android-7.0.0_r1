package autotest.afe;

import autotest.afe.HostDetailView.HostDetailListener;
import autotest.afe.HostListView.HostListListener;
import autotest.afe.JobDetailView.JobDetailListener;
import autotest.afe.JobListView.JobSelectListener;
import autotest.afe.RecurringView.RecurringSelectListener;
import autotest.afe.UserPreferencesView.UserPreferencesListener;
import autotest.afe.create.CreateJobViewPresenter.JobCreateListener;
import autotest.afe.create.CreateJobViewTab;
import autotest.common.CustomHistory;
import autotest.common.JsonRpcProxy;
import autotest.common.SiteCommonClassFactory;
import autotest.common.StaticDataRepository;
import autotest.common.ui.CustomTabPanel;
import autotest.common.ui.NotifyManager;
import autotest.common.ui.TabView;

import com.google.gwt.core.client.EntryPoint;
import com.google.gwt.dom.client.Document;
import com.google.gwt.json.client.JSONValue;
import com.google.gwt.user.client.ui.RootPanel;
import com.google.gwt.user.client.Window.Location;


public class AfeClient implements EntryPoint {
    private JobListView jobList;
    private JobDetailView jobDetail;
    private RecurringView recurringView;
    private CreateJobViewTab createJob;
    private HostListView hostListView;
    private HostDetailView hostDetailView;
    private UserPreferencesView userPreferencesView;

    public CustomTabPanel mainTabPanel = new CustomTabPanel();

    /**
     * Application entry point.
     */
    public void onModuleLoad() {
        JsonRpcProxy.setDefaultBaseUrl(JsonRpcProxy.AFE_BASE_URL);
        NotifyManager.getInstance().initialize();

        // initialize static data, and don't show main UI until that's done
        StaticDataRepository.getRepository().refresh(
                                 new StaticDataRepository.FinishedCallback() {
            public void onFinished() {
                finishLoading();
            }
        });
    }

    private JobCreateListener jobCreateListener = new JobCreateListener() {
        public void onJobCreated(int jobId) {
            showJob(jobId);
        }
    };

    protected void finishLoading() {
        SiteCommonClassFactory.globalInitialize();

        String wmatrixUrl = StaticDataRepository.getRepository().getData(
            "wmatrix_url").isString().stringValue();
        if (!wmatrixUrl.equals("")) {
            Document.get().getElementById("wmatrix-link").setAttribute(
                "href", wmatrixUrl);
            Document.get().getElementById("wmatrix").removeClassName("hidden");
        }
        boolean is_moblab = StaticDataRepository.getRepository().getData(
            "is_moblab").isBoolean().booleanValue();
        if (is_moblab) {
            Document.get().getElementById("moblab_setup").removeClassName("hidden");
            Document.get().getElementById("mobmonitor_link").setAttribute("href",
                "http://" + Location.getHostName() + ":9991");
        }

        jobList = new JobListView(new JobSelectListener() {
            public void onJobSelected(int jobId) {
                showJob(jobId);
            }
        });
        jobDetail = new JobDetailView(new JobDetailListener() {
            public void onHostSelected(String hostId) {
                showHost(hostId);
            }

            public void onCloneJob(JSONValue cloneInfo) {
                createJob.ensureInitialized();
                createJob.cloneJob(cloneInfo);
                mainTabPanel.selectTabView(createJob);
            }

            public void onCreateRecurringJob(int jobId) {
                recurringView.ensureInitialized();
                recurringView.createRecurringJob(jobId);
                mainTabPanel.selectTabView(recurringView);
            }
        });

        recurringView = new RecurringView(new RecurringSelectListener() {
            public void onRecurringSelected(int jobId) {
                showJob(jobId);
            }
        });

        createJob = AfeUtils.factory.getCreateJobView(jobCreateListener);

        hostListView = new HostListView(new HostListListener() {
            public void onHostSelected(String hostId) {
                showHost(hostId);
            }
        }, jobCreateListener);

        hostDetailView = new HostDetailView(new HostDetailListener() {
            public void onJobSelected(int jobId) {
                showJob(jobId);
            }
        }, jobCreateListener);

        userPreferencesView = new UserPreferencesView(new UserPreferencesListener() {
            public void onPreferencesChanged() {
                createJob.onPreferencesChanged();
            }
        });

        TabView[] tabViews = new TabView[] {jobList, jobDetail, recurringView, createJob,
                                            hostListView, hostDetailView, userPreferencesView};
        for (TabView tabView : tabViews) {
            mainTabPanel.addTabView(tabView);
        }

        final RootPanel tabsRoot = RootPanel.get("tabs");
        tabsRoot.add(mainTabPanel);
        CustomHistory.processInitialToken();
        mainTabPanel.initialize();
        tabsRoot.setStyleName("");
    }

    protected void showJob(int jobId) {
        jobDetail.ensureInitialized();
        jobDetail.updateObjectId(Integer.toString(jobId));
        mainTabPanel.selectTabView(jobDetail);
    }

    protected void showHost(String hostId) {
        hostDetailView.ensureInitialized();
        hostDetailView.updateObjectId(hostId);
        mainTabPanel.selectTabView(hostDetailView);
    }
}
