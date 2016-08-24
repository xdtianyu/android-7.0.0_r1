package autotest.afe;

import autotest.afe.ITextBox;
import autotest.afe.ITextBox.TextBoxImpl;
import autotest.afe.TestSelector.IDataTable;
import autotest.afe.TestSelector.IDataTable.DataTableImpl;
import autotest.afe.TestSelector.ISelectionManager;
import autotest.afe.TestSelector.ISelectionManager.SelectionManagerImpl;
import autotest.common.table.DataTable;
import autotest.common.ui.ExtendedListBox;
import autotest.common.ui.SimplifiedList;
import autotest.common.ui.ToolTip;

import com.google.gwt.user.client.ui.Composite;
import com.google.gwt.user.client.ui.HTML;
import com.google.gwt.user.client.ui.HasHTML;
import com.google.gwt.user.client.ui.HorizontalPanel;
import com.google.gwt.user.client.ui.HorizontalSplitPanel;
import com.google.gwt.user.client.ui.Label;
import com.google.gwt.user.client.ui.Panel;
import com.google.gwt.user.client.ui.VerticalPanel;

public class TestSelectorDisplay extends Composite implements TestSelector.Display {
    private static final String[][] testTableColumns = new String[][] {
        {DataTable.WIDGET_COLUMN, ""},
        {"name", "Test"},
    };

    private ExtendedListBox testTypeSelect = new ExtendedListBox();
    private TextBoxImpl testNameFilter = new TextBoxImpl();
    private DataTableImpl testTable = new DataTableImpl(testTableColumns);
    private SelectionManagerImpl testSelection = new SelectionManagerImpl(testTable, false);
    private HTML testInfo = new HTML("Click a test to view its description");
    private HorizontalSplitPanel mainPanel = new HorizontalSplitPanel();
    private ToolTip testTypeToolTip = new ToolTip(
        "?",
        "Client tests run asynchronously, as hosts become available. " +
        "Server tests run synchronously, when all hosts are available.");

    public TestSelectorDisplay() {
        testInfo.setStyleName("test-description");

        testTable.fillParent();
        testTable.setClickable(true);

        Panel testTypePanel = new HorizontalPanel();
        testTypePanel.add(new Label("Filter by test type:"));
        testTypePanel.add(testTypeSelect);
        testTypePanel.add(testTypeToolTip);

        Panel testFilterPanel = new HorizontalPanel();
        testFilterPanel.add(new Label("Filter by test name:"));
        testFilterPanel.add(testNameFilter);

        Panel testInfoPanel = new VerticalPanel();
        testInfoPanel.add(testInfo);

        mainPanel.setLeftWidget(testTable);
        mainPanel.setRightWidget(testInfoPanel);
        mainPanel.setSize("100%", "30em");
        mainPanel.setSplitPosition("30%");
        mainPanel.addStyleName("test-selector");
        mainPanel.addStyleName("noborder");

        Panel container = new VerticalPanel();
        container.add(testTypePanel);
        container.add(testFilterPanel);
        container.add(mainPanel);
        container.addStyleName("panel-boundedwidth");
        container.addStyleName("data-table-outlined-gray");

        initWidget(container);
    }

    public SimplifiedList getTestTypeSelect() {
        return testTypeSelect;
    }

    public ITextBox getTestNameFilter() {
        return testNameFilter;
    }

    public HasHTML getTestInfo() {
        return testInfo;
    }

    public ISelectionManager getTestSelection() {
        return testSelection;
    }

    public IDataTable getTestTable() {
        return testTable;
    }
}
