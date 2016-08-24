package com.android.cts.intent.sender;

import android.content.Context;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.test.InstrumentationTestCase;

public class SuspendPackageTest extends InstrumentationTestCase {
    private IntentSenderActivity mActivity;
    private Context mContext;
    private PackageManager mPackageManager;

    private static final String INTENT_RECEIVER_PKG = "com.android.cts.intent.receiver";
    private static final String TARGET_ACTIVTIY_NAME
            = "com.android.cts.intent.receiver.SimpleIntentReceiverActivity";

    @Override
    protected void setUp() throws Exception {
        super.setUp();
        mContext = getInstrumentation().getTargetContext();
        mActivity = launchActivity(mContext.getPackageName(), IntentSenderActivity.class, null);
        mPackageManager = mContext.getPackageManager();
    }

    @Override
    public void tearDown() throws Exception {
        mActivity.finish();
        super.tearDown();
    }

    public void testPackageSuspended() throws Exception {
        assertPackageSuspended(true);
    }

    public void testPackageNotSuspended() throws Exception {
        assertPackageSuspended(false);
    }

    /**
     * Verify the package is suspended by trying start the activity inside it. If the package
     * is not suspended, the target activity will return the result.
     */
    private void assertPackageSuspended(boolean suspended) throws Exception {
        Intent intent = new Intent();
        intent.setClassName(INTENT_RECEIVER_PKG, TARGET_ACTIVTIY_NAME);
        Intent result = mActivity.getResult(intent);
        if (suspended) {
            assertNull(result);
        } else {
            assertNotNull(result);
        }
        // No matter it is suspended or not, we should able to resolve the activity.
        assertNotNull(mPackageManager.resolveActivity(intent, 0));
    }
}
