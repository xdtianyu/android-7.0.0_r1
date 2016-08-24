package autotest.moblab;

public class BotoKeyView extends KeyUploadView {

    @Override
    public String getElementId() {
        return "boto_key";
    }

    @Override
    public void initialize() {
        fileUploadName = "botokey";
        uploadViewName = "view_boto_key";
        submitButtonName = "view_submit_boto_key";
        rpcName = "set_boto_key";
        rpcArgName = "boto_key";
        successMessage = "Boto Key uploaded.";

        super.initialize();
    }
}
