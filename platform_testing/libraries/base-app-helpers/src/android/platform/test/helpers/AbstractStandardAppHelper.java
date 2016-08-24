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

package android.platform.test.helpers;

import android.app.Instrumentation;
import android.content.pm.PackageInfo;
import android.content.pm.PackageManager;
import android.content.pm.PackageManager.NameNotFoundException;
import android.support.test.launcherhelper.ILauncherStrategy;
import android.support.test.launcherhelper.LauncherStrategyFactory;
import android.support.test.uiautomator.By;
import android.support.test.uiautomator.UiDevice;

public abstract class AbstractStandardAppHelper implements IStandardAppHelper {
    public UiDevice mDevice;
    public Instrumentation mInstrumentation;
    public ILauncherStrategy mLauncherStrategy;

    public AbstractStandardAppHelper(Instrumentation instr) {
        mInstrumentation = instr;
        mDevice = UiDevice.getInstance(instr);
        mLauncherStrategy = LauncherStrategyFactory.getInstance(mDevice).getLauncherStrategy();
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void open() {
        String pkg = getPackage();
        String id = getLauncherName();
        if (!mDevice.hasObject(By.pkg(pkg).depth(0))) {
            mLauncherStrategy.launch(id, pkg);
        }
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void exit() {
        int maxBacks = 4;
        while (!mDevice.hasObject(mLauncherStrategy.getWorkspaceSelector()) && maxBacks > 0) {
            mDevice.pressBack();
            mDevice.waitForIdle();
            maxBacks--;
        }

        if (maxBacks == 0) {
            mDevice.pressHome();
        }
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public String getVersion() throws NameNotFoundException {
        String pkg = getPackage();

        if (null == pkg || pkg.isEmpty()) {
            throw new RuntimeException("Cannot find version of empty package");
        }
        PackageManager pm = mInstrumentation.getContext().getPackageManager();
        PackageInfo pInfo = pm.getPackageInfo(pkg, 0);
        String version = pInfo.versionName;
        if (null == version || version.isEmpty()) {
            throw new RuntimeException(String.format("Version isn't found for package, %s", pkg));
        }

        return version;
    }

    protected int getOrientation() {
        return mInstrumentation.getContext().getResources().getConfiguration().orientation;
    }
}
