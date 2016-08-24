package com.android.bluetooth.tests;

import com.android.bluetooth.map.BluetoothMapMasInstance;
import junit.framework.Assert;

public class MockMasInstance extends BluetoothMapMasInstance {

    private final int mMasId;
    private final int mRemoteFeatureMask;

    public MockMasInstance(int masId, int remoteFeatureMask) {
        super();
        this.mMasId = masId;
        this.mRemoteFeatureMask = remoteFeatureMask;
    }

    public int getMasId() {
        return mMasId;
    }

   @Override
    public int getRemoteFeatureMask() {
        return mRemoteFeatureMask;
    }

   @Override
    public void restartObexServerSession() {
        Assert.fail("restartObexServerSession() should not occur");
    }
}
