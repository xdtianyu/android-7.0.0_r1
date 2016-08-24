package autotest.common.table;

import com.google.gwt.event.logical.shared.ValueChangeEvent;
import com.google.gwt.event.logical.shared.ValueChangeHandler;
import com.google.gwt.user.client.ui.HorizontalPanel;
import com.google.gwt.user.client.ui.Panel;
import com.google.gwt.user.client.ui.RadioButton;
import com.google.gwt.user.client.ui.Widget;

import java.util.Vector;

public abstract class RadioButtonSetFilter extends Filter implements ValueChangeHandler<Boolean> {
    private Panel panel;
    private String name;
    private Vector<RadioButton> buttons;
    private int selected;

    public RadioButtonSetFilter(String name) {
        this(new HorizontalPanel(), name);
    }

    public RadioButtonSetFilter(Panel panel, String name) {
        this.panel = panel;
        this.name = name;
        buttons = new Vector();
    }

    @Override
    public Widget getWidget() {
        return panel;
    }

    public void addRadioButon(String label) {
        RadioButton radioButton = new RadioButton(name, label);
        int formValue = buttons.size();
        radioButton.setFormValue(Integer.toString(formValue));
        radioButton.addValueChangeHandler(this);
        buttons.add(radioButton);
        panel.add(radioButton);
    }

    public void setSelectedButton(int index) {
        if (index < buttons.size())
            selected = index;
            buttons.get(index).setChecked(true);
    }

    public int getSelectedButtonIndex() {
        return selected;
    }

    public int getButtonNum() {
        return buttons.size();
    }

    @Override
    public void onValueChange(ValueChangeEvent<Boolean> event) {
        selected = Integer.parseInt(((RadioButton) event.getSource()).getFormValue());
        notifyListeners();
    }
}