package autotest.common.ui;

import com.google.gwt.dom.client.Document;
import com.google.gwt.dom.client.InputElement;
import com.google.gwt.user.client.ui.TextBoxBase;

public class DateTimeBox extends TextBoxBase {
    public DateTimeBox() {
        super(Document.get().createTextInputElement());
        getElement().setAttribute("type", "datetime-local");
    }
}