package autotest.common.table;

import autotest.common.ui.DateTimeBox;

import com.google.gwt.event.logical.shared.ValueChangeHandler;
import com.google.gwt.i18n.client.DateTimeFormat;
import com.google.gwt.user.client.ui.HorizontalPanel;
import com.google.gwt.user.client.ui.Label;
import com.google.gwt.user.client.ui.Panel;
import com.google.gwt.user.client.ui.Widget;

import java.util.Date;

public class DatetimeSegmentFilter extends SimpleFilter {
    protected DateTimeBox startDatetimeBox;
    protected DateTimeBox endDatetimeBox;
    protected Panel panel;
    protected Label fromLabel;
    protected Label toLabel;
    private String placeHolderDatetime;

    public DatetimeSegmentFilter() {
        startDatetimeBox = new DateTimeBox();
        endDatetimeBox = new DateTimeBox();
        fromLabel = new Label("From");
        toLabel = new Label("to");

        panel = new HorizontalPanel();
        panel.add(fromLabel);
        panel.add(startDatetimeBox);
        panel.add(toLabel);
        panel.add(endDatetimeBox);

        DateTimeFormat dateTimeFormat = DateTimeFormat.getFormat("yyyy-MM-dd");
        placeHolderDatetime = dateTimeFormat.format(new Date()) + "T00:00";
        setStartTimeToPlaceHolderValue();
        setEndTimeToPlaceHolderValue();
    }

    @Override
    public Widget getWidget() {
        return panel;
    }

    public void setStartTimeToPlaceHolderValue() {
        startDatetimeBox.setValue(placeHolderDatetime);
    }

    public void setEndTimeToPlaceHolderValue() {
        endDatetimeBox.setValue(placeHolderDatetime);
    }

    public void addValueChangeHandler(ValueChangeHandler<String> startTimeHandler,
                                      ValueChangeHandler<String> endTimeHandler) {
        startDatetimeBox.addValueChangeHandler(startTimeHandler);
        endDatetimeBox.addValueChangeHandler(endTimeHandler);
    }
}