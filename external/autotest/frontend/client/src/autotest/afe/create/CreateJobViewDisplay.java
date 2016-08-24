package autotest.afe.create;

import autotest.afe.CheckBoxPanel;
import autotest.afe.CheckBoxPanelDisplay;
import autotest.afe.ControlTypeSelect;
import autotest.afe.ControlTypeSelectDisplay;
import autotest.afe.HostSelector;
import autotest.afe.HostSelectorDisplay;
import autotest.afe.IButton;
import autotest.afe.IButton.ButtonImpl;
import autotest.afe.ICheckBox;
import autotest.afe.ICheckBox.CheckBoxImpl;
import autotest.afe.ITextArea;
import autotest.afe.ITextArea.TextAreaImpl;
import autotest.afe.ITextBox;
import autotest.afe.ITextBox.TextBoxImpl;
import autotest.afe.RadioChooser;
import autotest.afe.RadioChooserDisplay;
import autotest.afe.TestSelector;
import autotest.afe.TestSelectorDisplay;
import autotest.common.ui.ExtendedListBox;
import autotest.common.ui.SimplifiedList;
import autotest.common.ui.ToolTip;

import com.google.gwt.event.dom.client.HasClickHandlers;
import com.google.gwt.event.logical.shared.HasCloseHandlers;
import com.google.gwt.event.logical.shared.HasOpenHandlers;
import com.google.gwt.user.client.ui.Anchor;
import com.google.gwt.user.client.ui.Button;
import com.google.gwt.user.client.ui.CheckBox;
import com.google.gwt.user.client.ui.DisclosurePanel;
import com.google.gwt.user.client.ui.FlexTable;
import com.google.gwt.user.client.ui.HTMLPanel;
import com.google.gwt.user.client.ui.HTMLTable;
import com.google.gwt.user.client.ui.HasText;
import com.google.gwt.user.client.ui.HasValue;
import com.google.gwt.user.client.ui.HorizontalPanel;
import com.google.gwt.user.client.ui.Label;
import com.google.gwt.user.client.ui.Panel;
import com.google.gwt.user.client.ui.TextBox;
import com.google.gwt.user.client.ui.VerticalPanel;

public class CreateJobViewDisplay implements CreateJobViewPresenter.Display {
    public static final int CHECKBOX_PANEL_COLUMNS = 5;

    private TextBox jobName = new TextBox();
    private ToolTip jobNameToolTip = new ToolTip(
        "?",
        "Name for the job. The string should be meaningful when viewing a list of jobs.");
    private ExtendedListBox priorityList = new ExtendedListBox();
    private ToolTip priorityListToolTip = new ToolTip(
        "?",
        "Lowest to highest: Weekly, Daily, PostBuild, Default.");
    private TextBoxImpl kernel = new TextBoxImpl();
    private ToolTip kernelToolTip = new ToolTip(
        "?",
        "A URL pointing to a kernel source tarball or a .rpm or .deb package to " +
        "install on the test machine before testing. Leave blank to skip this step. " +
        "Example: \"2.6.18-rc3\" or \"http://example.com/kernel-2.6.30.rpm\". " +
        "Separate multiple kernels with a comma and/or space.");
    private TextBoxImpl kernel_cmdline = new TextBoxImpl();
    private TextBoxImpl image_url = new TextBoxImpl();
    private ToolTip image_urlToolTip = new ToolTip(
        "?",
        "Name of the test image to use. Example: \"x86-alex-release/R27-3837.0.0\". " +
        "If no image is specified, regular tests will use current image on the Host. " +
        "Please note that an image is required to run a test suite.");
    private Button fetchImageTestsButton = new Button("Fetch Tests from Build");
    private TextBox timeout = new TextBox();
    private ToolTip timeoutToolTip = new ToolTip(
        "?",
        "The number of minutes after the job creation before the scheduler " +
        "automatically aborts an incomplete job.");
    private TextBox maxRuntime = new TextBox();
    private ToolTip maxRuntimeToolTip = new ToolTip(
        "?",
        "The number of minutes after the job starts running before the scheduler " +
        "automatically aborts an incomplete job.");
    private TextBox testRetry = new TextBox();
    private ToolTip testRetryToolTip = new ToolTip(
        "?",
        "Number of times to retry test if the test did not complete successfully.");
    private TextBox emailList = new TextBox();
    private ToolTip emailListToolTip = new ToolTip(
        "?",
        "Email addresses to notify when this job completes. " +
        "Use a comma or space to separate multiple addresses.");
    private CheckBoxImpl skipVerify = new CheckBoxImpl();
    private ToolTip skipVerifyToolTip = new ToolTip(
        "?",
        "Skips the host verification step before running the job. " +
        "This is useful for machine reinstalls, for example.");
    private CheckBoxImpl skipReset = new CheckBoxImpl();
    private ToolTip skipResetToolTip = new ToolTip(
        "?",
        "Skips the host reset step before running the job.");
    private RadioChooserDisplay rebootBefore = new RadioChooserDisplay();
    private ToolTip rebootBeforeToolTip = new ToolTip(
        "?",
        "Reboots all assigned hosts before the job runs. " +
        "Click If dirty to reboot the host only if it hasn’t been rebooted " +
        "since it was added, locked, or after running the last job.");
    private RadioChooserDisplay rebootAfter = new RadioChooserDisplay();
    private ToolTip rebootAfterToolTip = new ToolTip(
        "?",
        "Reboots all assigned hosts after the job runs. Click If all tests passed " +
        "to skip rebooting the host if any test in the job fails.");
    private CheckBox parseFailedRepair = new CheckBox();
    private ToolTip parseFailedRepairToolTip = new ToolTip(
        "?",
        "When a host fails repair, displays repair and verify test entries in " +
        "the results database along with a SERVER_JOB entry. " +
        "Otherwise, no information is displayed in TKO (Result Database).");
    private CheckBoxImpl hostless = new CheckBoxImpl();
    private ToolTip hostlessToolTip = new ToolTip(
        "?",
        "Check to run a suite of tests, and select Server from the Test type dropdown list.");
    private CheckBoxImpl require_ssp = new CheckBoxImpl();
    private ToolTip require_sspToolTip = new ToolTip(
        "?",
        "Check to force a server side test to use server-side packaging. This " +
        "option has no effect on suite job. Test jobs created by a suite job " +
        "will use SSP if enable_ssp_container is set to True in global config " +
        "and there exists a drone supporting SSP.");
    private TextBox pool = new TextBox();
    private ToolTip poolToolTip = new ToolTip(
        "?",
        "Specify the pool of machines to use for suite job.");
    private TextBoxImpl args = new TextBoxImpl();
    private ToolTip argsToolTip = new ToolTip(
        "?",
        "Example: \"device_addrs=00:1F:20:33:6A:1E, arg2=value2, arg3=value3\". " +
        "Separate multiple args with commas.");
    private TextBoxImpl firmwareRWBuild = new TextBoxImpl();
    private ToolTip firmwareRWBuildToolTip = new ToolTip(
        "?",
        "Name of the firmware build to update RW firmware of the DUT. Example: " +
        "\"x86-alex-firmware/R41-6588.9.0\". If no firmware build is specified, " +
        "the RW firmware of the DUT will not be updated.");
    private TextBoxImpl firmwareROBuild = new TextBoxImpl();
    private ToolTip firmwareROBuildToolTip = new ToolTip(
        "?",
        "Name of the firmware build to update RO firmware of the DUT. Example: " +
        "\"x86-alex-firmware/R41-6588.9.0\". If no firmware RO build is specified, " +
        "the RO firmware of the DUT will not be updated.");
    private ExtendedListBox testSourceBuildList = new ExtendedListBox();
    private ToolTip testSourceBuildListToolTip = new ToolTip(
        "?",
        "The image/build from which the tests will be fetched and ran from. It can " +
        "be one of the specified Build Image, Firmware RW Build or the Firmware RO Build.");
    private TestSelectorDisplay testSelector = new TestSelectorDisplay();
    private CheckBoxPanelDisplay profilersPanel = new CheckBoxPanelDisplay(CHECKBOX_PANEL_COLUMNS);
    private CheckBoxImpl runNonProfiledIteration =
        new CheckBoxImpl("Run each test without profilers first");
    private ExtendedListBox droneSet = new ExtendedListBox();
    private TextAreaImpl controlFile = new TextAreaImpl();
    private DisclosurePanel controlFilePanel = new DisclosurePanel("");
    private ControlTypeSelectDisplay controlTypeSelect = new ControlTypeSelectDisplay();
    private TextBoxImpl synchCountInput = new TextBoxImpl();
    private ButtonImpl editControlButton = new ButtonImpl();
    private HostSelectorDisplay hostSelector = new HostSelectorDisplay();
    private ButtonImpl submitJobButton = new ButtonImpl("Submit Job");
    private Button createTemplateJobButton = new Button("Create Template Job");
    private Button resetButton = new Button("Reset");
    private Label viewLink = new Label("");
    private DisclosurePanel advancedOptionsPanel = new DisclosurePanel("");
    private DisclosurePanel firmwareBuildOptionsPanel = new DisclosurePanel("");

    public void initialize(HTMLPanel panel) {
        jobName.addStyleName("jobname-image-boundedwidth");
        image_url.addStyleName("jobname-image-boundedwidth");

        Panel profilerControls = new VerticalPanel();
        profilerControls.add(profilersPanel);
        profilerControls.add(runNonProfiledIteration);

        controlFile.setSize("100%", "30em");

        HorizontalPanel controlOptionsPanel = new HorizontalPanel();
        controlOptionsPanel.setVerticalAlignment(HorizontalPanel.ALIGN_BOTTOM);
        controlOptionsPanel.add(controlTypeSelect);
        Label useLabel = new Label("Use");
        useLabel.getElement().getStyle().setProperty("marginLeft", "1em");
        synchCountInput.setSize("3em", ""); // set width only
        synchCountInput.getElement().getStyle().setProperty("margin", "0 0.5em 0 0.5em");
        controlOptionsPanel.add(useLabel);
        controlOptionsPanel.add(synchCountInput);
        controlOptionsPanel.add(new Label("host(s) per execution"));
        Panel controlEditPanel = new VerticalPanel();
        controlEditPanel.add(controlOptionsPanel);
        controlEditPanel.add(controlFile);
        controlEditPanel.setWidth("100%");

        Panel controlHeaderPanel = new HorizontalPanel();
        controlHeaderPanel.add(controlFilePanel.getHeader());
        controlHeaderPanel.add(viewLink);
        controlHeaderPanel.add(editControlButton);

        controlFilePanel.setHeader(controlHeaderPanel);
        controlFilePanel.add(controlEditPanel);
        controlFilePanel.addStyleName("panel-boundedwidth");

        // Setup the Advanced options panel
        advancedOptionsPanel.getHeaderTextAccessor().setText("Advanced Options");

        FlexTable advancedOptionsLayout = new FlexTable();

        Panel priorityPanel = new HorizontalPanel();
        priorityPanel.add(priorityList);
        priorityPanel.add(priorityListToolTip);
        advancedOptionsLayout.setWidget(0, 0, new Label("Priority:"));
        advancedOptionsLayout.setWidget(0, 1, priorityPanel);

        Panel kernelPanel = new HorizontalPanel();
        kernelPanel.add(kernel);
        kernelPanel.add(kernelToolTip);
        advancedOptionsLayout.setWidget(1, 0, new Label("Kernel(s): (optional)"));
        advancedOptionsLayout.setWidget(1, 1, kernelPanel);

        advancedOptionsLayout.setWidget(2, 0, new Label("Kernel 'cmd': (optional)"));
        advancedOptionsLayout.setWidget(2, 1, kernel_cmdline);

        Panel timeoutPanel = new HorizontalPanel();
        timeoutPanel.add(timeout);
        timeoutPanel.add(timeoutToolTip);
        advancedOptionsLayout.setWidget(3, 0, new Label("Timeout (minutes):"));
        advancedOptionsLayout.setWidget(3, 1, timeoutPanel);

        Panel maxRuntimePanel = new HorizontalPanel();
        maxRuntimePanel.add(maxRuntime);
        maxRuntimePanel.add(maxRuntimeToolTip);
        advancedOptionsLayout.setWidget(4, 0, new Label("Max runtime (minutes):"));
        advancedOptionsLayout.setWidget(4, 1, maxRuntimePanel);

        Panel testRetryPanel = new HorizontalPanel();
        testRetryPanel.add(testRetry);
        testRetryPanel.add(testRetryToolTip);
        advancedOptionsLayout.setWidget(5, 0, new Label("Test Retries: (optional)"));
        advancedOptionsLayout.setWidget(5, 1, testRetryPanel);

        Panel emailListPanel = new HorizontalPanel();
        emailListPanel.add(emailList);
        emailListPanel.add(emailListToolTip);
        advancedOptionsLayout.setWidget(6, 0, new Label("Email List: (optional)"));
        advancedOptionsLayout.setWidget(6, 1, emailListPanel);

        Panel skipVerifyPanel = new HorizontalPanel();
        skipVerifyPanel.add(skipVerify);
        skipVerifyPanel.add(skipVerifyToolTip);
        advancedOptionsLayout.setWidget(7, 0, new Label("Skip verify:"));
        advancedOptionsLayout.setWidget(7, 1, skipVerifyPanel);

        Panel skipResetPanel = new HorizontalPanel();
        skipResetPanel.add(skipReset);
        skipResetPanel.add(skipResetToolTip);
        advancedOptionsLayout.setWidget(8, 0, new Label("Skip reset:"));
        advancedOptionsLayout.setWidget(8, 1, skipResetPanel);

        Panel rebootBeforePanel = new HorizontalPanel();
        rebootBeforePanel.add(rebootBefore);
        rebootBeforePanel.add(rebootBeforeToolTip);
        advancedOptionsLayout.setWidget(9, 0, new Label("Reboot before:"));
        advancedOptionsLayout.setWidget(9, 1, rebootBeforePanel);

        Panel rebootAfterPanel = new HorizontalPanel();
        rebootAfterPanel.add(rebootAfter);
        rebootAfterPanel.add(rebootAfterToolTip);
        advancedOptionsLayout.setWidget(10, 0, new Label("Reboot after:"));
        advancedOptionsLayout.setWidget(10, 1, rebootAfterPanel);

        Panel parseFailedRepairPanel = new HorizontalPanel();
        parseFailedRepairPanel.add(parseFailedRepair);
        parseFailedRepairPanel.add(parseFailedRepairToolTip);
        advancedOptionsLayout.setWidget(11, 0, new Label("Include failed repair results:"));
        advancedOptionsLayout.setWidget(11, 1, parseFailedRepairPanel);

        Panel hostlessPanel = new HorizontalPanel();
        hostlessPanel.add(hostless);
        hostlessPanel.add(hostlessToolTip);
        advancedOptionsLayout.setWidget(12, 0, new Label("Hostless (Suite Job):"));
        advancedOptionsLayout.setWidget(12, 1, hostlessPanel);

        Panel require_sspPanel = new HorizontalPanel();
        require_sspPanel.add(require_ssp);
        require_sspPanel.add(require_sspToolTip);
        advancedOptionsLayout.setWidget(13, 0, new Label("Require server-side packaging:"));
        advancedOptionsLayout.setWidget(13, 1, require_sspPanel);

        Panel poolPanel = new HorizontalPanel();
        poolPanel.add(pool);
        poolPanel.add(poolToolTip);
        advancedOptionsLayout.setWidget(14, 0, new Label("Pool: (optional)"));
        advancedOptionsLayout.setWidget(14, 1, poolPanel);

        Panel argsPanel = new HorizontalPanel();
        argsPanel.add(args);
        argsPanel.add(argsToolTip);
        advancedOptionsLayout.setWidget(15, 0, new Label("Args: (optional)"));
        advancedOptionsLayout.setWidget(15, 1, argsPanel);

        advancedOptionsLayout.setWidget(16, 0, new Label("Profilers: (optional)"));
        advancedOptionsLayout.setWidget(16, 1, profilerControls);

        HTMLTable.RowFormatter advOptLayoutFormatter = advancedOptionsLayout.getRowFormatter();
        for (int row = 0; row < advancedOptionsLayout.getRowCount(); ++row)
        {
          if (row % 2 == 0) {
              advOptLayoutFormatter.addStyleName(row, "data-row");
          }
          else {
              advOptLayoutFormatter.addStyleName(row, "data-row-alternate");
          }

        }
        advancedOptionsLayout.setWidth("100%");
        advancedOptionsPanel.addStyleName("panel-boundedwidth");
        advancedOptionsPanel.add(advancedOptionsLayout);

        // Setup the Firmware Build options panel
        firmwareBuildOptionsPanel.getHeaderTextAccessor().setText("Firmware Build Options (optional)");
        FlexTable firmwareBuildOptionsLayout = new FlexTable();

        firmwareBuildOptionsLayout.getFlexCellFormatter().setColSpan(0, 0, 2);
        firmwareBuildOptionsLayout.setWidget(0, 0, new Label("Image URL/Build must be specified for " +
            "updating the firmware of the test device with given firmware build. A servo may be " +
            "required to be attached to the test device in order to have firmware updated."));

        Panel firmwareRWBuildPanel = new HorizontalPanel();
        firmwareRWBuild.addStyleName("jobname-image-boundedwidth");
        firmwareRWBuildPanel.add(firmwareRWBuild);
        firmwareRWBuildPanel.add(firmwareRWBuildToolTip);
        firmwareBuildOptionsLayout.setWidget(1, 0, new Label("Firmware RW build:"));
        firmwareBuildOptionsLayout.setWidget(1, 1, firmwareRWBuildPanel);

        Panel firmwareROBuildPanel = new HorizontalPanel();
        firmwareROBuild.addStyleName("jobname-image-boundedwidth");
        firmwareROBuildPanel.add(firmwareROBuild);
        firmwareROBuildPanel.add(firmwareROBuildToolTip);
        firmwareBuildOptionsLayout.setWidget(2, 0, new Label("Firmware RO build:"));
        firmwareBuildOptionsLayout.setWidget(2, 1, firmwareROBuildPanel);

        firmwareBuildOptionsLayout.setWidth("100%");
        firmwareBuildOptionsPanel.addStyleName("panel-boundedwidth");
        firmwareBuildOptionsPanel.add(firmwareBuildOptionsLayout);
        firmwareRWBuild.setEnabled(false);
        firmwareROBuild.setEnabled(false);

        testSourceBuildList.getElement().getStyle().setProperty("minWidth", "15em");

        // Add the remaining widgets to the main panel
        panel.add(jobName, "create_job_name");
        panel.add(jobNameToolTip, "create_job_name");
        panel.add(image_url, "create_image_url");
        panel.add(image_urlToolTip, "create_image_url");
        panel.add(testSourceBuildList, "create_test_source_build");
        panel.add(testSourceBuildListToolTip, "create_test_source_build");
        panel.add(fetchImageTestsButton, "fetch_image_tests");
        panel.add(testSelector, "create_tests");
        panel.add(controlFilePanel, "create_edit_control");
        panel.add(hostSelector, "create_host_selector");
        panel.add(submitJobButton, "create_submit");
        panel.add(createTemplateJobButton, "create_template_job");
        panel.add(resetButton, "create_reset");
        panel.add(droneSet, "create_drone_set");

        panel.add(advancedOptionsPanel, "create_advanced_options");
        panel.add(firmwareBuildOptionsPanel, "create_firmware_build_options");
    }

    public CheckBoxPanel.Display getCheckBoxPanelDisplay() {
        return profilersPanel;
    }

    public ControlTypeSelect.Display getControlTypeSelectDisplay() {
        return controlTypeSelect;
    }

    public ITextArea getControlFile() {
        return controlFile;
    }

    public HasCloseHandlers<DisclosurePanel> getControlFilePanelClose() {
        return controlFilePanel;
    }

    public HasOpenHandlers<DisclosurePanel> getControlFilePanelOpen() {
        return controlFilePanel;
    }

    public HasClickHandlers getCreateTemplateJobButton() {
        return createTemplateJobButton;
    }

    public SimplifiedList getDroneSet() {
        return droneSet;
    }

    public IButton getEditControlButton() {
        return editControlButton;
    }

    public HasText getEmailList() {
        return emailList;
    }

    public HostSelector.Display getHostSelectorDisplay() {
        return hostSelector;
    }

    public ICheckBox getHostless() {
        return hostless;
    }

    public ICheckBox getRequireSSP() {
      return require_ssp;
    }

    public HasText getPool() {
        return pool;
    }

    public ITextBox getArgs() {
        return args;
    }

    public HasText getJobName() {
        return jobName;
    }

    public ITextBox getKernel() {
        return kernel;
    }

    public ITextBox getKernelCmdline() {
        return kernel_cmdline;
    }

    public ITextBox getImageUrl() {
        return image_url;
    }

    public HasText getMaxRuntime() {
        return maxRuntime;
    }

    public HasText getTestRetry() {
        return testRetry;
    }

    public HasValue<Boolean> getParseFailedRepair() {
        return parseFailedRepair;
    }

    public ExtendedListBox getPriorityList() {
        return priorityList;
    }

    public RadioChooser.Display getRebootAfter() {
        return rebootAfter;
    }

    public RadioChooser.Display getRebootBefore() {
        return rebootBefore;
    }

    public HasClickHandlers getResetButton() {
        return resetButton;
    }

    public ICheckBox getRunNonProfiledIteration() {
        return runNonProfiledIteration;
    }

    public ICheckBox getSkipVerify() {
        return skipVerify;
    }

    public ICheckBox getSkipReset() {
      return skipReset;
    }

    public IButton getSubmitJobButton() {
        return submitJobButton;
    }

    public ITextBox getSynchCountInput() {
        return synchCountInput;
    }

    public TestSelector.Display getTestSelectorDisplay() {
        return testSelector;
    }

    public HasText getTimeout() {
        return timeout;
    }

    public HasText getViewLink() {
        return viewLink;
    }

    public void setControlFilePanelOpen(boolean isOpen) {
        controlFilePanel.setOpen(isOpen);
    }

    public HasClickHandlers getFetchImageTestsButton() {
        return fetchImageTestsButton;
    }

    public ITextBox getFirmwareRWBuild() {
      return firmwareRWBuild;
    }

    public ITextBox getFirmwareROBuild() {
      return firmwareROBuild;
    }

    public ExtendedListBox getTestSourceBuildList() {
      return testSourceBuildList;
    }
}
