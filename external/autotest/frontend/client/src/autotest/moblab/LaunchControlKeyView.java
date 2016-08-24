package autotest.moblab;

public class LaunchControlKeyView extends KeyUploadView {

    @Override
    public String getElementId() {
        return "launch_control_key";
    }

    @Override
    public void initialize() {
        fileUploadName = "launchcontrolkey";
        uploadViewName = "view_launch_control_key";
        submitButtonName = "view_submit_launch_control_key";
        rpcName = "set_launch_control_key";
        rpcArgName = "launch_control_key";
        successMessage = "Launch Control Key uploaded.";

        super.initialize();
    }
}
