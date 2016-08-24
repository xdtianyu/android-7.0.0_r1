package autotest.common.ui;

import com.google.gwt.event.dom.client.MouseOverHandler;
import com.google.gwt.event.dom.client.MouseOverEvent;
import com.google.gwt.event.dom.client.MouseOutHandler;
import com.google.gwt.event.dom.client.MouseOutEvent;
import com.google.gwt.user.client.Window;
import com.google.gwt.user.client.ui.Label;
import com.google.gwt.user.client.ui.PopupPanel;

public class ToolTip extends Label implements MouseOverHandler, MouseOutHandler {
    protected PopupPanel popup;
    protected final int LEFT_OFFSET = 20;
    protected final int TOP_OFFSET = -20;

    public ToolTip(String labelMessage, String toolTipMessage) {
        super(labelMessage);
        popup = new PopupPanel();
        setStyleName("tooltip_label");
        popup.setStyleName("tooltip");
        popup.add(new Label(toolTipMessage));
        addMouseOverHandler(this);
        addMouseOutHandler(this);
    }

    public void showAtWindow(int left, int top) {
        popup.setPopupPosition(left + Window.getScrollLeft() + LEFT_OFFSET,
                               top + Window.getScrollTop() + TOP_OFFSET);
        popup.show();
    }

    public void hide() {
        popup.hide();
    }

    public void onMouseOver(MouseOverEvent event) {
        showAtWindow(event.getClientX(), event.getClientY());
    }

    public void onMouseOut(MouseOutEvent event) {
        hide();
    }
}
