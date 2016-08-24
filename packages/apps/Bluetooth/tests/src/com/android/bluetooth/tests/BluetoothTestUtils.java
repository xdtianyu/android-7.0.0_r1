package com.android.bluetooth.tests;

import android.annotation.TargetApi;
import android.bluetooth.BluetoothAdapter;
import android.os.Build;
import android.test.AndroidTestCase;
import android.util.Log;


@TargetApi(Build.VERSION_CODES.ECLAIR)
public class BluetoothTestUtils extends AndroidTestCase {

    protected static String TAG = "BluetoothTestUtils";
    protected static final boolean D = true;

    static final int POLL_TIME = 500;
    static final int ENABLE_TIMEOUT = 5000;

    /** Helper to turn BT on.
     * This method will either fail on an assert, or return with BT turned on.
     * Behavior of getState() and isEnabled() are validated along the way.
     */
    public static void enableBt(BluetoothAdapter adapter) {
        if (adapter.getState() == BluetoothAdapter.STATE_ON) {
            assertTrue(adapter.isEnabled());
            return;
        }
        assertEquals(BluetoothAdapter.STATE_OFF, adapter.getState());
        assertFalse(adapter.isEnabled());
        adapter.enable();
        for (int i=0; i<ENABLE_TIMEOUT/POLL_TIME; i++) {
            int state = adapter.getState();
            switch (state) {
            case BluetoothAdapter.STATE_ON:
                assertTrue(adapter.isEnabled());
                Log.i(TAG, "Bluetooth enabled...");
                return;
            case BluetoothAdapter.STATE_OFF:
                Log.i(TAG, "STATE_OFF: Still waiting for enable to begin...");
                break;
            default:
                Log.i(TAG, "Status is: " + state);
                assertEquals(BluetoothAdapter.STATE_TURNING_ON, adapter.getState());
                assertFalse(adapter.isEnabled());
                break;
            }
            try {
                Thread.sleep(POLL_TIME);
            } catch (InterruptedException e) {}
        }
        fail("enable() timeout");
    }

}
