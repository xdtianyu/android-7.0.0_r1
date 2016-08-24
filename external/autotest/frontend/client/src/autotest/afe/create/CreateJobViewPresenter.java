package autotest.afe.create;

import autotest.afe.AfeUtils;
import autotest.afe.CheckBoxPanel;
import autotest.afe.ControlTypeSelect;
import autotest.afe.HostSelector;
import autotest.afe.IButton;
import autotest.afe.ICheckBox;
import autotest.afe.ITextArea;
import autotest.afe.ITextBox;
import autotest.afe.RadioChooser;
import autotest.afe.TestSelector;
import autotest.afe.TestSelector.TestSelectorListener;
import autotest.common.JSONArrayList;
import autotest.common.JsonRpcCallback;
import autotest.common.JsonRpcProxy;
import autotest.common.SimpleCallback;
import autotest.common.StaticDataRepository;
import autotest.common.Utils;
import autotest.common.ui.NotifyManager;
import autotest.common.ui.SimplifiedList;
import autotest.common.ui.ExtendedListBox;

import com.google.gwt.event.dom.client.BlurEvent;
import com.google.gwt.event.dom.client.BlurHandler;
import com.google.gwt.event.dom.client.ChangeEvent;
import com.google.gwt.event.dom.client.ChangeHandler;
import com.google.gwt.event.dom.client.ClickEvent;
import com.google.gwt.event.dom.client.ClickHandler;
import com.google.gwt.event.dom.client.HasClickHandlers;
import com.google.gwt.event.dom.client.KeyCodes;
import com.google.gwt.event.dom.client.KeyPressEvent;
import com.google.gwt.event.dom.client.KeyPressHandler;
import com.google.gwt.event.logical.shared.CloseEvent;
import com.google.gwt.event.logical.shared.CloseHandler;
import com.google.gwt.event.logical.shared.HasCloseHandlers;
import com.google.gwt.event.logical.shared.HasOpenHandlers;
import com.google.gwt.event.logical.shared.OpenEvent;
import com.google.gwt.event.logical.shared.OpenHandler;
import com.google.gwt.json.client.JSONArray;
import com.google.gwt.json.client.JSONBoolean;
import com.google.gwt.json.client.JSONNull;
import com.google.gwt.json.client.JSONNumber;
import com.google.gwt.json.client.JSONObject;
import com.google.gwt.json.client.JSONString;
import com.google.gwt.json.client.JSONValue;
import com.google.gwt.user.client.DOM;
import com.google.gwt.user.client.Window;
import com.google.gwt.user.client.ui.DisclosurePanel;
import com.google.gwt.user.client.ui.HasText;
import com.google.gwt.user.client.ui.HasValue;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

import java.lang.Math;

public class CreateJobViewPresenter implements TestSelectorListener {
    public static interface Display {
        public CheckBoxPanel.Display getCheckBoxPanelDisplay();
        public ControlTypeSelect.Display getControlTypeSelectDisplay();
        public TestSelector.Display getTestSelectorDisplay();
        public IButton getEditControlButton();
        public HasText getJobName();
        public ExtendedListBox getPriorityList();
        public HasText getTimeout();
        public HasText getMaxRuntime();
        public HasText getTestRetry();
        public HasText getEmailList();
        public ICheckBox getSkipVerify();
        public ICheckBox getSkipReset();
        public RadioChooser.Display getRebootBefore();
        public RadioChooser.Display getRebootAfter();
        public HasValue<Boolean> getParseFailedRepair();
        public ICheckBox getHostless();
        public ICheckBox getRequireSSP();
        public HasText getPool();
        public ITextBox getArgs();
        public HostSelector.Display getHostSelectorDisplay();
        public SimplifiedList getDroneSet();
        public ITextBox getSynchCountInput();
        public ITextArea getControlFile();
        public void setControlFilePanelOpen(boolean isOpen);
        public ICheckBox getRunNonProfiledIteration();
        public ITextBox getKernel();
        public ITextBox getKernelCmdline();
        public ITextBox getImageUrl();
        public HasText getViewLink();
        public HasCloseHandlers<DisclosurePanel> getControlFilePanelClose();
        public HasOpenHandlers<DisclosurePanel> getControlFilePanelOpen();
        public IButton getSubmitJobButton();
        public HasClickHandlers getCreateTemplateJobButton();
        public HasClickHandlers getResetButton();
        public HasClickHandlers getFetchImageTestsButton();
        public ITextBox getFirmwareRWBuild();
        public ITextBox getFirmwareROBuild();
        public ExtendedListBox getTestSourceBuildList();
    }

    private static final String EDIT_CONTROL_STRING = "Edit control file";
    private static final String UNEDIT_CONTROL_STRING= "Revert changes";
    private static final String VIEW_CONTROL_STRING = "View control file";
    private static final String HIDE_CONTROL_STRING = "Hide control file";
    private static final String FIRMWARE_RW_BUILD = "firmware_rw_build";
    private static final String FIRMWARE_RO_BUILD = "firmware_ro_build";
    private static final String TEST_SOURCE_BUILD = "test_source_build";

    public interface JobCreateListener {
        public void onJobCreated(int jobId);
    }

    private interface IPredicate<T> {
        boolean apply(T item);
    }

    private IPredicate<Integer> positiveIntegerPredicate = new IPredicate<Integer>() {
        public boolean apply(Integer item) {
            return item > 0;
        }
    };

    private IPredicate<Integer> nonnegativeIntegerPredicate = new IPredicate<Integer>() {
        public boolean apply(Integer item) {
            return item >= 0;
        }
    };

    private IPredicate<Integer> anyIntegerPredicate = new IPredicate<Integer>() {
        public boolean apply(Integer item) {
            return true;
        }
    };

    private JsonRpcProxy rpcProxy = JsonRpcProxy.getProxy();
    private JobCreateListener listener;

    private StaticDataRepository staticData = StaticDataRepository.getRepository();

    private CheckBoxPanel profilersPanel = new CheckBoxPanel();
    private ControlTypeSelect controlTypeSelect = new ControlTypeSelect();
    protected TestSelector testSelector = new TestSelector();
    private RadioChooser rebootBefore = new RadioChooser();
    private RadioChooser rebootAfter = new RadioChooser();
    private HostSelector hostSelector;

    private boolean controlEdited = false;
    private boolean controlReadyForSubmit = false;
    private JSONArray dependencies = new JSONArray();

    private Display display;

    public void bindDisplay(Display display) {
        this.display = display;
    }

    public CreateJobViewPresenter(JobCreateListener listener) {
        this.listener = listener;
    }

    public void cloneJob(JSONValue cloneInfo) {
        // reset() fires the TestSelectorListener, which will generate a new control file. We do
        // no want this, so we'll stop listening to it for a bit.
        testSelector.setListener(null);
        reset();
        testSelector.setListener(this);

        disableInputs();
        openControlFileEditor();
        JSONObject cloneObject = cloneInfo.isObject();
        JSONObject jobObject = cloneObject.get("job").isObject();

        display.getJobName().setText(jobObject.get("name").isString().stringValue());

        ArrayList<String> builds = new ArrayList<String>();
        if (jobObject.containsKey("image")) {
            String image = jobObject.get("image").isString().stringValue();
            builds.add(image);
            display.getImageUrl().setText(image);
            display.getFirmwareRWBuild().setEnabled(true);
            display.getFirmwareROBuild().setEnabled(true);
            display.getTestSourceBuildList().setEnabled(true);
        }

        if (jobObject.containsKey(FIRMWARE_RW_BUILD)) {
            String firmwareRWBuild = jobObject.get(FIRMWARE_RW_BUILD).isString().stringValue();
            builds.add(firmwareRWBuild);
            display.getFirmwareRWBuild().setText(firmwareRWBuild);
        }

        if (jobObject.containsKey(FIRMWARE_RO_BUILD)) {
            String firmwareROBuild = jobObject.get(FIRMWARE_RO_BUILD).isString().stringValue();
            builds.add(firmwareROBuild);
            display.getFirmwareROBuild().setText(firmwareROBuild);
        }

        for (String build : builds) {
            display.getTestSourceBuildList().addItem(build);
        }

        if (jobObject.containsKey(TEST_SOURCE_BUILD)) {
            String testSourceBuild = jobObject.get(TEST_SOURCE_BUILD).isString().stringValue();
            if (builds.indexOf(testSourceBuild) >= 0) {
                display.getTestSourceBuildList().setSelectedIndex(builds.indexOf(testSourceBuild));
            }
        }

        Double priorityValue = jobObject.get("priority").isNumber().getValue();
        Double maxPriority = staticData.getData("max_schedulable_priority").isNumber().getValue();
        if (priorityValue > maxPriority) {
            priorityValue = maxPriority;
        }
        String priorityName = staticData.getPriorityName(priorityValue);
        display.getPriorityList().selectByName(priorityName);

        display.getTimeout().setText(Utils.jsonToString(jobObject.get("timeout_mins")));
        display.getMaxRuntime().setText(Utils.jsonToString(jobObject.get("max_runtime_mins")));
        display.getTestRetry().setText(Utils.jsonToString(jobObject.get("test_retry")));
        display.getEmailList().setText(
                jobObject.get("email_list").isString().stringValue());

        display.getSkipVerify().setValue(!jobObject.get("run_verify").isBoolean().booleanValue());
        display.getSkipReset().setValue(!jobObject.get("run_reset").isBoolean().booleanValue());
        rebootBefore.setSelectedChoice(Utils.jsonToString(jobObject.get("reboot_before")));
        rebootAfter.setSelectedChoice(Utils.jsonToString(jobObject.get("reboot_after")));
        display.getParseFailedRepair().setValue(
                jobObject.get("parse_failed_repair").isBoolean().booleanValue());
        display.getHostless().setValue(cloneObject.get("hostless").isBoolean().booleanValue());
        if (display.getHostless().getValue()) {
            hostSelector.setEnabled(false);
        }
        if (jobObject.get("require_ssp").isNull() != null)
            display.getRequireSSP().setValue(false);
        else
            display.getRequireSSP().setValue(jobObject.get("require_ssp").isBoolean().booleanValue());
        if (staticData.getData("drone_sets_enabled").isBoolean().booleanValue()) {
            if (cloneObject.get("drone_set").isNull() == null) {
                display.getDroneSet().selectByName(Utils.jsonToString(cloneObject.get("drone_set")));
            }
        }

        controlTypeSelect.setControlType(
                jobObject.get("control_type").isString().stringValue());
        display.getSynchCountInput().setText(Utils.jsonToString(jobObject.get("synch_count")));
        setSelectedDependencies(jobObject.get("dependencies").isArray());
        display.getControlFile().setText(
                jobObject.get("control_file").isString().stringValue());
        controlReadyForSubmit = true;

        JSONArray hostInfo = cloneObject.get("hosts").isArray();
        List<String> hostnames = new ArrayList<String>();
        for (JSONObject host : new JSONArrayList<JSONObject>(hostInfo)) {
            hostnames.add(Utils.jsonToString(host.get("hostname")));
        }
        hostSelector.setSelectedHostnames(hostnames, true);

        JSONObject metaHostCounts = cloneObject.get("meta_host_counts").isObject();

        for (String label : metaHostCounts.keySet()) {
            String number = Integer.toString(
                (int) metaHostCounts.get(label).isNumber().doubleValue());
            hostSelector.addMetaHosts(label, number);
        }

        hostSelector.refresh();
    }

    private void openControlFileEditor() {
        display.getControlFile().setReadOnly(false);
        display.getEditControlButton().setText(UNEDIT_CONTROL_STRING);
        display.setControlFilePanelOpen(true);
        controlTypeSelect.setEnabled(true);
        display.getSynchCountInput().setEnabled(true);
        display.getEditControlButton().setEnabled(true);
    }

    private void populatePriorities(JSONArray priorities) {
        JSONValue maxSchedulableValue = staticData.getData("max_schedulable_priority");
        double maxPriority = maxSchedulableValue.isNumber().getValue();
        for(int i = 0; i < priorities.size(); i++) {
            JSONArray priorityData = priorities.get(i).isArray();
            String priority = priorityData.get(1).isString().stringValue();
            double priorityValue = priorityData.get(0).isNumber().getValue();
            String strPriorityValue = Double.toString(priorityValue);
            if (priorityValue <= maxPriority) {
                display.getPriorityList().addItem(priority, strPriorityValue);
            }
        }

        resetPriorityToDefault();
    }

    private void resetPriorityToDefault() {
        JSONValue defaultValue = staticData.getData("default_priority");
        String defaultPriority = defaultValue.isString().stringValue();
        display.getPriorityList().selectByName(defaultPriority);
    }

    private void populateProfilers() {
        JSONArray tests = staticData.getData("profilers").isArray();

        for(JSONObject profiler : new JSONArrayList<JSONObject>(tests)) {
            String name = profiler.get("name").isString().stringValue();
            ICheckBox checkbox = profilersPanel.generateCheckBox();
            checkbox.setText(name);
            checkbox.addClickHandler(new ClickHandler() {
                public void onClick(ClickEvent event) {
                    updateNonProfiledRunControl();
                    generateControlFile(false);
                    setInputsEnabled();
                }
            });
            profilersPanel.add(checkbox);
        }

        display.getRunNonProfiledIteration().addClickHandler(new ClickHandler() {
            @Override
            public void onClick(ClickEvent event) {
                generateControlFile(false);
            }
        });
        // default to checked -- run a non-profiled iteration by default
        display.getRunNonProfiledIteration().setValue(true);
    }

    private void updateNonProfiledRunControl() {
        boolean anyProfilersChecked = !profilersPanel.getChecked().isEmpty();
        display.getRunNonProfiledIteration().setVisible(anyProfilersChecked);
    }

    private void populateRebootChoices() {
        AfeUtils.populateRadioChooser(rebootBefore, "reboot_before");
        AfeUtils.populateRadioChooser(rebootAfter, "reboot_after");
    }


    private JSONArray getKernelParams(String kernel_list, String cmdline) {
        JSONArray result = new JSONArray();

        for(String version: kernel_list.split("[, ]+")) {
            Map<String, String> item = new HashMap<String, String>();

            item.put("version", version);
            // if there is a cmdline part, put it for all versions in the map
            if (cmdline.length() > 0) {
                item.put("cmdline", cmdline);
            }

            result.set(result.size(), Utils.mapToJsonObject(item));
        }

        return result;
    }
    /**
     * Get parameters to submit to the generate_control_file RPC.
     * @param readyForSubmit are we getting a control file that's ready to submit for a job, or just
     * an intermediate control file to be viewed by the user?
     */
    protected JSONObject getControlFileParams(boolean readyForSubmit) {
        JSONObject params = new JSONObject();

        String kernelString = display.getKernel().getText();
        if (!kernelString.equals("")) {
            params.put(
                    "kernel", getKernelParams(kernelString, display.getKernelCmdline().getText()));
        }

        boolean testsFromBuild = testSelector.usingTestsFromBuild();
        params.put("db_tests", JSONBoolean.getInstance(!testsFromBuild));

        JSONArray tests = new JSONArray();
        for (JSONObject test : testSelector.getSelectedTests()) {
            if (testsFromBuild) {
                tests.set(tests.size(), test);
            }
            else {
                tests.set(tests.size(), test.get("id"));
            }
        }
        params.put("tests", tests);

        JSONArray profilers = new JSONArray();
        for (ICheckBox profiler : profilersPanel.getChecked()) {
            profilers.set(profilers.size(), new JSONString(profiler.getText()));
        }
        params.put("profilers", profilers);

        if (display.getRunNonProfiledIteration().isVisible()) {
            boolean profileOnly = !display.getRunNonProfiledIteration().getValue();
            params.put("profile_only", JSONBoolean.getInstance(profileOnly));
        }

        return params;
    }

    private void generateControlFile(final boolean readyForSubmit,
                                       final SimpleCallback finishedCallback,
                                       final SimpleCallback errorCallback) {
        JSONObject params = getControlFileParams(readyForSubmit);
        rpcProxy.rpcCall("generate_control_file", params, new JsonRpcCallback() {
            @Override
            public void onSuccess(JSONValue result) {
                JSONObject controlInfo = result.isObject();
                String controlFileText = controlInfo.get("control_file").isString().stringValue();
                boolean isServer = controlInfo.get("is_server").isBoolean().booleanValue();
                String synchCount = Utils.jsonToString(controlInfo.get("synch_count"));
                setSelectedDependencies(controlInfo.get("dependencies").isArray());
                display.getControlFile().setText(controlFileText);
                controlTypeSelect.setControlType(isServer ? TestSelector.SERVER_TYPE :
                                                            TestSelector.CLIENT_TYPE);
                display.getSynchCountInput().setText(synchCount);
                controlReadyForSubmit = readyForSubmit;
                if (finishedCallback != null) {
                    finishedCallback.doCallback(this);
                }
            }

            @Override
            public void onError(JSONObject errorObject) {
                super.onError(errorObject);
                if (errorCallback != null) {
                    errorCallback.doCallback(this);
                }
            }
        });
    }

    protected void generateControlFile(boolean readyForSubmit) {
        generateControlFile(readyForSubmit, null, null);
    }

    public void handleSkipVerify() {
        boolean shouldSkipVerify = false;
        for (JSONObject test : testSelector.getSelectedTests()) {
            boolean runVerify = test.get("run_verify").isBoolean().booleanValue();
            if (!runVerify) {
                shouldSkipVerify = true;
                break;
            }
        }

        if (shouldSkipVerify) {
            display.getSkipVerify().setValue(true);
            display.getSkipVerify().setEnabled(false);
        } else {
            display.getSkipVerify().setEnabled(true);
        }
    }

    public void handleSkipReset() {
        boolean shouldSkipReset = false;
        for (JSONObject test : testSelector.getSelectedTests()) {
            boolean runReset = test.get("run_reset").isBoolean().booleanValue();
            if (!runReset) {
                shouldSkipReset = true;
                break;
            }
        }

        if (shouldSkipReset) {
            display.getSkipReset().setValue(true);
            display.getSkipReset().setEnabled(false);
        } else {
            display.getSkipReset().setEnabled(true);
        }
    }

    protected int getMaximumRetriesCount() {
        int maxRetries = 0;
        for (JSONObject test : testSelector.getSelectedTests()) {
            maxRetries = (int) Math.max(maxRetries, test.get("test_retry").isNumber().getValue());
        }
        return maxRetries;
    }

    protected void handleBuildChange() {
        ChangeHandler handler = new ChangeHandler() {
            @Override
            public void onChange(ChangeEvent event) {
                String image = display.getImageUrl().getText();
                if (image.isEmpty()) {
                    display.getFirmwareRWBuild().setText("");
                    display.getFirmwareROBuild().setText("");
                    display.getTestSourceBuildList().clear();
                    display.getFirmwareRWBuild().setEnabled(false);
                    display.getFirmwareROBuild().setEnabled(false);
                    display.getTestSourceBuildList().setEnabled(false);
                }
                else {
                    display.getFirmwareRWBuild().setEnabled(true);
                    display.getFirmwareROBuild().setEnabled(true);
                    display.getTestSourceBuildList().setEnabled(true);
                    ArrayList<String> builds = new ArrayList<String>();
                    builds.add(image);
                    if (!display.getFirmwareRWBuild().getText().isEmpty())
                        builds.add(display.getFirmwareRWBuild().getText());
                    if (!display.getFirmwareROBuild().getText().isEmpty())
                        builds.add(display.getFirmwareROBuild().getText());
                    String currentTestSourceBuild = display.getTestSourceBuildList().getSelectedValue();
                    int testSourceBuildIndex = builds.indexOf(currentTestSourceBuild);
                    display.getTestSourceBuildList().clear();
                    for (String build : builds) {
                        display.getTestSourceBuildList().addItem(build);
                    }
                    if (testSourceBuildIndex >= 0) {
                        display.getTestSourceBuildList().setSelectedIndex(testSourceBuildIndex);
                    }
                }
            }
        };

        display.getImageUrl().addChangeHandler(handler);
        display.getFirmwareRWBuild().addChangeHandler(handler);
        display.getFirmwareROBuild().addChangeHandler(handler);
    }

    protected void setInputsEnabled() {
        testSelector.setEnabled(true);
        profilersPanel.setEnabled(true);
        handleSkipVerify();
        handleSkipReset();
        display.getKernel().setEnabled(true);
        display.getKernelCmdline().setEnabled(true);
        display.getImageUrl().setEnabled(true);
    }

    protected void disableInputs() {
        testSelector.setEnabled(false);
        profilersPanel.setEnabled(false);
        display.getKernel().setEnabled(false);
        display.getKernelCmdline().setEnabled(false);
        display.getImageUrl().setEnabled(false);
    }

    public void initialize() {
        profilersPanel.bindDisplay(display.getCheckBoxPanelDisplay());
        controlTypeSelect.bindDisplay(display.getControlTypeSelectDisplay());
        testSelector.bindDisplay(display.getTestSelectorDisplay());
        rebootBefore.bindDisplay(display.getRebootBefore());
        rebootAfter.bindDisplay(display.getRebootAfter());

        display.getEditControlButton().setText(EDIT_CONTROL_STRING);
        display.getViewLink().setText(VIEW_CONTROL_STRING);

        hostSelector = new HostSelector();
        hostSelector.initialize();
        hostSelector.bindDisplay(display.getHostSelectorDisplay());

        populatePriorities(staticData.getData("priorities").isArray());

        BlurHandler kernelBlurHandler = new BlurHandler() {
            public void onBlur(BlurEvent event) {
                generateControlFile(false);
            }
        };

        display.getKernel().addBlurHandler(kernelBlurHandler);
        display.getKernelCmdline().addBlurHandler(kernelBlurHandler);

        KeyPressHandler kernelKeyPressHandler = new KeyPressHandler() {
            public void onKeyPress(KeyPressEvent event) {
                if (event.getCharCode() == (char) KeyCodes.KEY_ENTER) {
                    generateControlFile(false);
                }
            }
        };

        display.getKernel().addKeyPressHandler(kernelKeyPressHandler);
        display.getKernelCmdline().addKeyPressHandler(kernelKeyPressHandler);

        populateProfilers();
        updateNonProfiledRunControl();

        populateRebootChoices();
        onPreferencesChanged();

        if (parameterizedJobsEnabled()) {
            display.getEditControlButton().setEnabled(false);
        }

        display.getEditControlButton().addClickHandler(new ClickHandler() {
            public void onClick(ClickEvent event) {
                DOM.eventCancelBubble(DOM.eventGetCurrentEvent(), true);

                if (display.getEditControlButton().getText().equals(EDIT_CONTROL_STRING)) {
                    disableInputs();
                    display.getEditControlButton().setEnabled(false);
                    SimpleCallback onGotControlFile = new SimpleCallback() {
                        public void doCallback(Object source) {
                            openControlFileEditor();
                        }
                    };
                    SimpleCallback onControlFileError = new SimpleCallback() {
                        public void doCallback(Object source) {
                            setInputsEnabled();
                            display.getEditControlButton().setEnabled(true);
                        }
                    };
                    generateControlFile(true, onGotControlFile, onControlFileError);
                }
                else {
                    if (controlEdited &&
                        !Window.confirm("Are you sure you want to revert your" +
                                        " changes?")) {
                        return;
                    }
                    generateControlFile(false);
                    display.getControlFile().setReadOnly(true);
                    setInputsEnabled();
                    display.getEditControlButton().setText(EDIT_CONTROL_STRING);
                    controlTypeSelect.setEnabled(false);
                    display.getSynchCountInput().setEnabled(false);
                    controlEdited = false;
                }
            }
        });

        display.getControlFile().addChangeHandler(new ChangeHandler() {
            public void onChange(ChangeEvent event) {
                controlEdited = true;
            }
        });

        display.getControlFilePanelClose().addCloseHandler(new CloseHandler<DisclosurePanel>() {
            public void onClose(CloseEvent<DisclosurePanel> event) {
                display.getViewLink().setText(VIEW_CONTROL_STRING);
            }
        });

        display.getControlFilePanelOpen().addOpenHandler(new OpenHandler<DisclosurePanel>() {
            public void onOpen(OpenEvent<DisclosurePanel> event) {
                display.getViewLink().setText(HIDE_CONTROL_STRING);
            }
        });

        display.getSubmitJobButton().addClickHandler(new ClickHandler() {
            public void onClick(ClickEvent event) {
                submitJob(false);
            }
        });

        display.getCreateTemplateJobButton().addClickHandler(new ClickHandler() {
            public void onClick(ClickEvent event) {
                submitJob(true);
            }
        });

        display.getResetButton().addClickHandler(new ClickHandler() {
            public void onClick(ClickEvent event) {
                reset();
            }
        });

        display.getHostless().addClickHandler(new ClickHandler() {
            @Override
            public void onClick(ClickEvent event) {
                hostSelector.setEnabled(!display.getHostless().getValue());
            }
        });

        display.getFetchImageTestsButton().addClickHandler(new ClickHandler() {
            public void onClick(ClickEvent event) {
                String imageUrl = display.getImageUrl().getText();
                if (imageUrl == null || imageUrl.isEmpty()) {
                    NotifyManager.getInstance().showMessage(
                        "No build was specified for fetching tests.");
                }
                fetchImageTests();
            }
        });

        handleBuildChange();

        reset();

        if (staticData.getData("drone_sets_enabled").isBoolean().booleanValue()) {
            AfeUtils.populateListBox(display.getDroneSet(), "drone_sets");
        } else {
            AfeUtils.removeElement("create_drone_set_wrapper");
        }

        testSelector.setListener(this);
    }

    public void reset() {
        StaticDataRepository repository = StaticDataRepository.getRepository();

        display.getJobName().setText("");
        resetPriorityToDefault();
        rebootBefore.reset();
        rebootAfter.reset();
        display.getParseFailedRepair().setValue(
                repository.getData("parse_failed_repair_default").isBoolean().booleanValue());
        display.getHostless().setValue(false);
        // Default require_ssp to False, since it's not applicable to client side test.
        display.getRequireSSP().setValue(false);
        display.getKernel().setText("");
        display.getKernelCmdline().setText("");
        display.getImageUrl().setText("");
        display.getTimeout().setText(Utils.jsonToString(repository.getData("job_timeout_mins_default")));
        display.getMaxRuntime().setText(
                Utils.jsonToString(repository.getData("job_max_runtime_mins_default")));
        display.getTestRetry().setText("");
        display.getEmailList().setText("");
        testSelector.reset();
        display.getSkipVerify().setValue(true);
        display.getSkipReset().setValue(false);
        profilersPanel.reset();
        setInputsEnabled();
        controlTypeSelect.setControlType(TestSelector.CLIENT_TYPE);
        controlTypeSelect.setEnabled(false);
        display.getSynchCountInput().setEnabled(false);
        display.getSynchCountInput().setText("1");
        display.getControlFile().setText("");
        display.getControlFile().setReadOnly(true);
        controlEdited = false;
        display.setControlFilePanelOpen(false);
        display.getEditControlButton().setText(EDIT_CONTROL_STRING);
        hostSelector.reset();
        dependencies = new JSONArray();
        display.getPool().setText("");
        display.getArgs().setText("");
        display.getImageUrl().setText("");
        fetchImageTests();
    }

    private void submitJob(final boolean isTemplate) {
        final int timeoutValue, maxRuntimeValue, testRetryValue;
        final JSONValue synchCount;
        try {
            timeoutValue = parsePositiveIntegerInput(display.getTimeout().getText(), "timeout");
            maxRuntimeValue = parsePositiveIntegerInput(
                    display.getMaxRuntime().getText(), "max runtime");
            String testRetryText = display.getTestRetry().getText();
            if (testRetryText == "")
                testRetryValue = getMaximumRetriesCount();
            else
                testRetryValue = parseNonnegativeIntegerInput(testRetryText, "test retries");

            if (display.getHostless().getValue()) {
                synchCount = JSONNull.getInstance();
            } else {
                synchCount = new JSONNumber(parsePositiveIntegerInput(
                    display.getSynchCountInput().getText(),
                    "number of machines used per execution"));
            }
        } catch (IllegalArgumentException exc) {
            return;
        }

        // disallow accidentally clicking submit twice
        display.getSubmitJobButton().setEnabled(false);

        final SimpleCallback doSubmit = new SimpleCallback() {
            public void doCallback(Object source) {
                JSONObject args = new JSONObject();
                args.put("name", new JSONString(display.getJobName().getText()));
                String priority = display.getPriorityList().getSelectedValue();
                args.put("priority", new JSONNumber(Double.parseDouble(priority)));
                args.put("control_file", new JSONString(display.getControlFile().getText()));
                args.put("control_type",
                         new JSONString(controlTypeSelect.getControlType()));
                args.put("synch_count", synchCount);
                args.put("timeout_mins", new JSONNumber(timeoutValue));
                args.put("max_runtime_mins", new JSONNumber(maxRuntimeValue));
                args.put("test_retry", new JSONNumber(testRetryValue));
                args.put("email_list", new JSONString(display.getEmailList().getText()));
                args.put("run_verify", JSONBoolean.getInstance(
                        !display.getSkipVerify().getValue()));
                args.put("run_reset", JSONBoolean.getInstance(
                        !display.getSkipReset().getValue()));
                args.put("is_template", JSONBoolean.getInstance(isTemplate));
                args.put("dependencies", getSelectedDependencies());
                args.put("reboot_before", new JSONString(rebootBefore.getSelectedChoice()));
                args.put("reboot_after", new JSONString(rebootAfter.getSelectedChoice()));
                args.put("parse_failed_repair",
                         JSONBoolean.getInstance(display.getParseFailedRepair().getValue()));
                args.put("hostless", JSONBoolean.getInstance(display.getHostless().getValue()));
                args.put("require_ssp", JSONBoolean.getInstance(display.getRequireSSP().getValue()));
                args.put("pool", new JSONString(display.getPool().getText()));

                if (staticData.getData("drone_sets_enabled").isBoolean().booleanValue()) {
                    args.put("drone_set", new JSONString(display.getDroneSet().getSelectedName()));
                }

                HostSelector.HostSelection hosts = hostSelector.getSelectedHosts();
                args.put("hosts", Utils.stringsToJSON(hosts.hosts));
                args.put("meta_hosts", Utils.stringsToJSON(hosts.metaHosts));
                args.put("one_time_hosts",
                    Utils.stringsToJSON(hosts.oneTimeHosts));

                String imageUrlString = display.getImageUrl().getText();
                if (!imageUrlString.equals("")) {
                    args.put("image", new JSONString(imageUrlString));
                }
                else if (display.getHostless().getValue() &&
                         testSuiteSelected()) {
                    display.getSubmitJobButton().setEnabled(true);
                    NotifyManager.getInstance().showError("Please specify an image to use.");
                    return;
                }


                String argsString = display.getArgs().getText().trim();
                if (!argsString.equals("")) {
                    JSONArray argsArray = new JSONArray();
                    for (String arg : argsString.split(",")) {
                        argsArray.set(argsArray.size(), new JSONString(arg.trim()));
                    }
                    args.put("args", argsArray);
                }

                // TODO(crbug.com/464962): Fall back to build in cros-version label if possible.
                // Validate server-side packaging requirements
                if (display.getRequireSSP().getValue()) {
                    String error = "";
                    if (controlTypeSelect.getControlType() == "Client") {
                        error = "Client side test does not need server-side packaging.";
                    }
                    else if (imageUrlString.equals("")) {
                      boolean has_cros_version_dependency = false;
                      for (int i = 0; i < getSelectedDependencies().size(); i++) {
                        String dep = getSelectedDependencies().get(i).toString();
                        if (dep.startsWith("\"cros-version:")) {
                          has_cros_version_dependency = true;
                          break;
                        }
                      }
                      if (!has_cros_version_dependency)
                        error = "You must specify an image to run test with server-side packaging.";
                    }

                    if (error != "") {
                        display.getSubmitJobButton().setEnabled(true);
                        NotifyManager.getInstance().showError(error);
                        return;
                    }
                }

                String firmwareRWBuild = display.getFirmwareRWBuild().getText();
                String firmwareROBuild = display.getFirmwareROBuild().getText();
                String testSourceBuild = display.getTestSourceBuildList().getSelectedValue();
                if (!firmwareRWBuild.isEmpty() || !firmwareROBuild.isEmpty()) {
                    String error = "";
                    if (testSourceBuild.isEmpty())
                        error = "You must specify which build should be used to retrieve test code.";

                    // TODO(crbug.com/502638): Enable create test job with updating firmware.
                    if (!display.getHostless().getValue())
                        error = "Only suite job supports updating both ChromeOS build and firmware build.";
                    if (error != "") {
                        display.getSubmitJobButton().setEnabled(true);
                        NotifyManager.getInstance().showError(error);
                        return;
                    }
                    if (!firmwareRWBuild.isEmpty())
                        args.put(FIRMWARE_RW_BUILD, new JSONString(firmwareRWBuild));
                    if (!firmwareROBuild.isEmpty())
                        args.put(FIRMWARE_RO_BUILD, new JSONString(firmwareROBuild));
                    args.put(TEST_SOURCE_BUILD, new JSONString(testSourceBuild));
                }

                rpcProxy.rpcCall("create_job_page_handler", args, new JsonRpcCallback() {
                    @Override
                    public void onSuccess(JSONValue result) {
                        int id = (int) result.isNumber().doubleValue();
                        NotifyManager.getInstance().showMessage(
                                    "Job " + Integer.toString(id) + " created");
                        reset();
                        if (listener != null) {
                            listener.onJobCreated(id);
                        }
                        display.getSubmitJobButton().setEnabled(true);
                    }

                    @Override
                    public void onError(JSONObject errorObject) {
                        super.onError(errorObject);
                        display.getSubmitJobButton().setEnabled(true);
                    }
                });
            }
        };

        // ensure control file is ready for submission
        if (!controlReadyForSubmit) {
            generateControlFile(true, doSubmit, new SimpleCallback() {
                public void doCallback(Object source) {
                    display.getSubmitJobButton().setEnabled(true);
                }
            });
        } else {
            doSubmit.doCallback(this);
        }
    }

    private JSONArray getSelectedDependencies() {
        return dependencies;
    }

    private void setSelectedDependencies(JSONArray dependencies) {
        this.dependencies = dependencies;
    }

    private int parseNonnegativeIntegerInput(String input, String fieldName) {
        return parsePredicatedIntegerInput(input, fieldName,
                                           nonnegativeIntegerPredicate,
                                           "non-negative");
    }

    private int parsePositiveIntegerInput(String input, String fieldName) {
        return parsePredicatedIntegerInput(input, fieldName,
                                           positiveIntegerPredicate,
                                           "positive");
    }

    private int parseAnyIntegerInput(String input, String fieldName) {
        return parsePredicatedIntegerInput(input, fieldName,
                                           anyIntegerPredicate,
                                           "integer");
    }

    private int parsePredicatedIntegerInput(String input, String fieldName,
                                            IPredicate<Integer> predicate,
                                            String domain) {
        final int parsedInt;
        try {
            if (input.equals("") ||
                !predicate.apply(parsedInt = Integer.parseInt(input))) {
                    String error = "Please enter a " + domain + " " + fieldName;
                    NotifyManager.getInstance().showError(error);
                    throw new IllegalArgumentException();
            }
        } catch (NumberFormatException e) {
            String error = "Invalid " + fieldName + ": \"" + input + "\"";
            NotifyManager.getInstance().showError(error);
            throw new IllegalArgumentException();
        }
        return parsedInt;
    }

    public void refresh() {
        hostSelector.refresh();
    }

    public void onTestSelectionChanged() {
        generateControlFile(false);
        setInputsEnabled();

        // Set hostless selection to true if the test name contains test_suites
        display.getHostless().setValue(false);
        hostSelector.setEnabled(true);

        if (testSuiteSelected()) {
            display.getHostless().setValue(true);
            hostSelector.setEnabled(false);
        }
    }

    private boolean testSuiteSelected() {
        for (JSONObject test : testSelector.getSelectedTests()) {
            if (Utils.jsonToString(test.get("name")).toLowerCase().contains("test_suites:"))
            {
                return true;
            }
        }

        return false;
    }

    private void setRebootSelectorDefault(RadioChooser chooser, String name) {
        JSONObject user = staticData.getData("current_user").isObject();
        String defaultOption = Utils.jsonToString(user.get(name));
        chooser.setDefaultChoice(defaultOption);
    }

    private void selectPreferredDroneSet() {
        JSONObject user = staticData.getData("current_user").isObject();
        JSONValue droneSet = user.get("drone_set");
        if (droneSet.isNull() == null) {
            String preference = Utils.jsonToString(user.get("drone_set"));
            display.getDroneSet().selectByName(preference);
        }
    }

    public void onPreferencesChanged() {
        setRebootSelectorDefault(rebootBefore, "reboot_before");
        setRebootSelectorDefault(rebootAfter, "reboot_after");
        selectPreferredDroneSet();
        testSelector.reset();
    }

    private boolean parameterizedJobsEnabled() {
        return staticData.getData("parameterized_jobs").isBoolean().booleanValue();
    }

    private void fetchImageTests() {
        testSelector.setImageTests(new JSONArray());

        String imageUrl = display.getImageUrl().getText();
        if (imageUrl == null || imageUrl.isEmpty()) {
            testSelector.reset();
            return;
        }

        JSONObject params = new JSONObject();
        params.put("build", new JSONString(imageUrl));

        rpcProxy.rpcCall("get_tests_by_build", params, new JsonRpcCallback() {
            @Override
            public void onSuccess(JSONValue result) {
                JSONArray tests = result.isArray();
                testSelector.setImageTests(tests);
                testSelector.reset();
            }

            @Override
            public void onError(JSONObject errorObject) {
                super.onError(errorObject);
                NotifyManager.getInstance().showError(
                    "Failed to update tests for given build.");
                testSelector.reset();
            }
        });
    }
}
