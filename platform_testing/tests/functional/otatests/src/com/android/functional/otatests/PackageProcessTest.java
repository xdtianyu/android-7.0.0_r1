/*
 * Copyright (C) 2016 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
package com.android.functional.otatests;

import static org.easymock.EasyMock.createNiceMock;

import android.content.Context;
import android.content.ContextWrapper;
import android.os.IPowerManager;
import android.os.PowerManager;
import android.os.RecoverySystem;
import android.test.InstrumentationTestCase;

import java.io.File;

public class PackageProcessTest extends InstrumentationTestCase {

    private static final String PACKAGE_DATA_PATH =
            "/data/data/com.google.android.gms/app_download/update.zip";
    private static final String BLOCK_MAP = "/cache/recovery/block.map";
    private static final String UNCRYPT_FILE = "/cache/recovery/uncrypt_file";

    private Context mMockContext;
    private Context mContext;
    private PowerManager mMockPowerManager;

    private class PackageProcessMockContext extends ContextWrapper {

        private Context mInternal;

        public PackageProcessMockContext(Context base) {
            super(base);
            mInternal = base;
        }

        @Override
        public Object getSystemService(String name) {
            if (name.equals(Context.POWER_SERVICE)) {
                return mMockPowerManager;
            }
            return mInternal.getSystemService(name);
        }
    }

    @Override
    public void setUp() throws Exception {
        super.setUp();
        mContext = getInstrumentation().getContext();
        mMockContext = new PackageProcessMockContext(mContext);
        // Set a mocked out power manager into the mocked context, so the device
        // won't reboot at the end of installPackage
        IPowerManager mockIPowerManager = createNiceMock(IPowerManager.class);
        mMockPowerManager = new PowerManager(mContext, mockIPowerManager, null);
    }

    public void testPackageProcessOnly() throws Exception {
        File pkg = new File(PACKAGE_DATA_PATH);
        RecoverySystem.verifyPackage(pkg, null, null);
        RecoverySystem.processPackage(mMockContext, pkg, null);
        // uncrypt will push block.map to this location if and only if it finishes successfully
        assertTrue(new File(BLOCK_MAP).exists());
    }

    public void testPackageProcessViaInstall() throws Exception {
        File pkg = new File(PACKAGE_DATA_PATH);
        RecoverySystem.verifyPackage(pkg, null, null);
        RecoverySystem.installPackage(mMockContext, pkg);
        // uncrypt will push block.map to this location if and only if it finishes successfully
        assertTrue(new File(UNCRYPT_FILE).exists());
    }
}
