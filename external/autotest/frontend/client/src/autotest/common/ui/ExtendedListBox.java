package autotest.common.ui;

import com.google.gwt.dom.client.Document;
import com.google.gwt.dom.client.OptionElement;
import com.google.gwt.dom.client.SelectElement;
import com.google.gwt.user.client.ui.ListBox;

public class ExtendedListBox extends ListBox implements SimplifiedList {
    private int findItemByName(String name) {
        for (int i = 0; i < getItemCount(); i++) {
            if (getItemText(i).equals(name)) {
                return i;
            }
        }
        throw new IllegalArgumentException("No such name found: " + name);
    }
    
    private int findItemByValue(String value) {
        for (int i = 0; i < getItemCount(); i++) {
            if (getValue(i).equals(value)) {
                return i;
            }
        }
        throw new IllegalArgumentException("No such value found: " + value);
    }

    private native void selectAppend(SelectElement select,
                                     OptionElement option) /*-{
        select.appendChild(option);
    }-*/;

    public void addItem(String name) {
        addItem(name, name);
    }

    public void addItem(String name, String value) {
        SelectElement select = getElement().cast();
        OptionElement option = Document.get().createOptionElement();
        setOptionText(option, name, null);
        option.setValue(value);
        selectAppend(select, option);
    }

    public void removeItemByName(String name) {
        removeItem(findItemByName(name));
    }
    
    private boolean isNothingSelected() {
        return getSelectedIndex() == -1;
    }
    
    public String getSelectedName() {
        if (isNothingSelected()) {
            return null;
        }
        return getItemText(getSelectedIndex());
    }

    public String getSelectedValue() {
        if (isNothingSelected()) {
            return null;
        }
        return getValue(getSelectedIndex());
    }

    public void selectByName(String name) {
        setSelectedIndex(findItemByName(name));
    }

    public void selectByValue(String value) {
        setSelectedIndex(findItemByValue(value));
    }
}
