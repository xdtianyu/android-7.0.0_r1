package autotest.moblab;

import autotest.common.JsonRpcProxy;
import autotest.common.Utils;
import autotest.common.ui.CustomTabPanel;
import autotest.common.ui.NotifyManager;

import com.google.gwt.core.client.EntryPoint;
import com.google.gwt.user.client.ui.RootPanel;


public class MoblabSetupClient implements EntryPoint {
    private ConfigSettingsView configSettingsView;
    private BotoKeyView botoKeyView;
    private LaunchControlKeyView launchControlKeyView;

    public CustomTabPanel mainTabPanel = new CustomTabPanel();

    /**
     * Application entry point.
     */
    public void onModuleLoad() {
        JsonRpcProxy.setDefaultBaseUrl(JsonRpcProxy.AFE_BASE_URL);
        NotifyManager.getInstance().initialize();

        configSettingsView = new ConfigSettingsView();
        botoKeyView = new BotoKeyView();
        launchControlKeyView = new LaunchControlKeyView();
        mainTabPanel.addTabView(configSettingsView);
        mainTabPanel.addTabView(botoKeyView);
        mainTabPanel.addTabView(launchControlKeyView);

        final RootPanel rootPanel = RootPanel.get("tabs");
        rootPanel.add(mainTabPanel);
        mainTabPanel.initialize();
        rootPanel.setStyleName("");
    }

}
