package autotest.afe;

import com.google.gwt.event.dom.client.HasBlurHandlers;
import com.google.gwt.event.dom.client.HasChangeHandlers;
import com.google.gwt.event.dom.client.HasKeyPressHandlers;
import com.google.gwt.user.client.ui.HasText;
import com.google.gwt.user.client.ui.TextBox;

public interface ITextBox extends HasText, HasBlurHandlers, HasChangeHandlers,
                                  HasKeyPressHandlers {
    public void setEnabled(boolean enabled);

    public static class TextBoxImpl extends TextBox implements ITextBox {}
}
