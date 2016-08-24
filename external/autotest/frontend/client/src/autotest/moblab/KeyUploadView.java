package autotest.moblab;

import autotest.common.JsonRpcCallback;
import autotest.common.JsonRpcProxy;
import autotest.common.SimpleCallback;
import autotest.common.ui.TabView;
import autotest.common.ui.NotifyManager;

import com.google.gwt.event.dom.client.ClickHandler;
import com.google.gwt.event.dom.client.ClickEvent;
import com.google.gwt.json.client.JSONObject;
import com.google.gwt.json.client.JSONString;
import com.google.gwt.json.client.JSONValue;
import com.google.gwt.user.client.ui.Button;
import com.google.gwt.user.client.ui.FileUpload;
import com.google.gwt.user.client.ui.FormPanel;
import com.google.gwt.user.client.ui.FormPanel.SubmitCompleteEvent;
import com.google.gwt.user.client.ui.FormPanel.SubmitCompleteHandler;
import com.google.gwt.user.client.ui.FormPanel.SubmitEvent;
import com.google.gwt.user.client.ui.FormPanel.SubmitHandler;


public class KeyUploadView extends TabView {
    private FileUpload keyUpload;
    private Button submitButton;
    private FormPanel keyUploadForm;

    protected String fileUploadName = "key";
    protected String uploadViewName = "view_key";
    protected String submitButtonName = "view_submit_key";
    protected String rpcName = "";
    protected String rpcArgName = "key";
    protected String successMessage = "Key uploaded.";

    @Override
    public String getElementId() {
        return "key";
    }

    @Override
    public void initialize() {
        super.initialize();

        keyUpload = new FileUpload();
        keyUpload.setName(fileUploadName);

        keyUploadForm = new FormPanel();
        keyUploadForm.setAction(JsonRpcProxy.AFE_BASE_URL + "upload/");
        keyUploadForm.setEncoding(FormPanel.ENCODING_MULTIPART);
        keyUploadForm.setMethod(FormPanel.METHOD_POST);
        keyUploadForm.setWidget(keyUpload);

        submitButton = new Button("Submit", new ClickHandler() {
            public void onClick(ClickEvent event) {
                keyUploadForm.submit();
            }
        });

        keyUploadForm.addSubmitCompleteHandler(new SubmitCompleteHandler() {
            public void onSubmitComplete(SubmitCompleteEvent event) {
                String fileName = event.getResults();
                JSONObject params = new JSONObject();
                params.put(rpcArgName, new JSONString(fileName));
                rpcCall(params);
            }
        });

        addWidget(keyUploadForm, uploadViewName);
        addWidget(submitButton, submitButtonName);
    }

    public void rpcCall(JSONObject params) {
        JsonRpcProxy rpcProxy = JsonRpcProxy.getProxy();
        rpcProxy.rpcCall(rpcName, params, new JsonRpcCallback() {
            @Override
            public void onSuccess(JSONValue result) {
                NotifyManager.getInstance().showMessage(successMessage);
            }
        });
    }
}
