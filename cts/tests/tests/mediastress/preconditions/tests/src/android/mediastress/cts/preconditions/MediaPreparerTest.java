/*
 * Copyright (C) 2015 The Android Open Source Project
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

package android.mediastress.cts.preconditions;

import com.android.ddmlib.IDevice;
import com.android.tradefed.build.BuildInfo;
import com.android.tradefed.build.IBuildInfo;
import com.android.tradefed.config.OptionSetter;
import com.android.tradefed.device.ITestDevice;
import com.android.tradefed.targetprep.TargetSetupError;

import java.awt.Dimension;

import junit.framework.TestCase;

import org.easymock.EasyMock;

public class MediaPreparerTest extends TestCase {

    private MediaPreparer mMediaPreparer;
    private IBuildInfo mMockBuildInfo;
    private ITestDevice mMockDevice;
    private OptionSetter mOptionSetter;

    private final Dimension DEFAULT_DIMENSION =
            MediaPreparer.resolutions[MediaPreparer.RES_DEFAULT];

    @Override
    public void setUp() throws Exception {
        super.setUp();
        mMediaPreparer = new MediaPreparer();
        mMockDevice = EasyMock.createMock(ITestDevice.class);
        mMockBuildInfo = new BuildInfo("0", "", "");
        mOptionSetter = new OptionSetter(mMediaPreparer);
    }

    public void testSetMountPoint() throws Exception {
        EasyMock.expect(mMockDevice.getMountPoint(IDevice.MNT_EXTERNAL_STORAGE)).andReturn(
                "/sdcard").once();
        EasyMock.replay(mMockDevice);
        mMediaPreparer.setMountPoint(mMockDevice);
        assertEquals(mMediaPreparer.baseDeviceShortDir, "/sdcard/test/bbb_short/");
        assertEquals(mMediaPreparer.baseDeviceFullDir, "/sdcard/test/bbb_full/");
    }

    public void testCopyMediaFiles() throws Exception {
        // by jumping directly into copyMediaFiles, the baseDeviceShortDir variable won't be set
        // thus, the string "null" replaces the variable
        EasyMock.expect(mMockDevice.doesFileExist("null176x144")).andReturn(true).anyTimes();
        EasyMock.expect(mMockDevice.doesFileExist("null480x360")).andReturn(true).anyTimes();
        EasyMock.replay(mMockDevice);
        mMediaPreparer.copyMediaFiles(mMockDevice, DEFAULT_DIMENSION);
    }

    public void testMediaFilesExistOnDeviceTrue() throws Exception {
        // by jumping directly into copyMediaFiles, the baseDeviceShortDir variable won't be set
        // thus, the string "null" replaces the variable
        EasyMock.expect(mMockDevice.doesFileExist("null176x144")).andReturn(true).anyTimes();
        EasyMock.expect(mMockDevice.doesFileExist("null480x360")).andReturn(true).anyTimes();
        EasyMock.replay(mMockDevice);
        assertTrue(mMediaPreparer.mediaFilesExistOnDevice(mMockDevice, DEFAULT_DIMENSION));
    }

    public void testMediaFilesExistOnDeviceFalse() throws Exception {
        // by jumping directly into copyMediaFiles, the baseDeviceShortDir variable won't be set
        // thus, the string "null" replaces the variable
        EasyMock.expect(mMockDevice.doesFileExist("null176x144")).andReturn(false).anyTimes();
        EasyMock.expect(mMockDevice.doesFileExist("null480x360")).andReturn(true).anyTimes();
        EasyMock.replay(mMockDevice);
        assertFalse(mMediaPreparer.mediaFilesExistOnDevice(mMockDevice, DEFAULT_DIMENSION));
    }

    public void testGetMaxVideoPlaybackResolutionFound() throws Exception {
        String mockDumpsysOutput = "mBaseDisplayInfo=DisplayInfo{\"Built-in Screen\", uniqueId " +
                "\"local:0\", app 1440 x 2560, real 1440 x 2560, largest app 1440 x 2560, " +
                "smallest app 360 x 480, mode 1, defaultMode 1, modes [{id=1, width=1440, " +
                "height=2560, fps=60.0}], rotation 0, density 560 (494.27 x 492.606) dpi, " +
                "layerStack 0, appVsyncOff 2500000, presDeadline 17666667, type BUILT_IN, state " +
                "ON, FLAG_SECURE, FLAG_SUPPORTS_PROTECTED_BUFFERS}\n";
        EasyMock.expect(mMockDevice.executeShellCommand(
                "dumpsys display | grep mBaseDisplayInfo")).andReturn(mockDumpsysOutput).once();
        EasyMock.replay(mMockDevice);
        Dimension result = mMediaPreparer.getMaxVideoPlaybackResolution(mMockDevice);
        assertEquals(result, DEFAULT_DIMENSION);
    }

    public void testGetMaxVideoPlaybackResolutionNotFound() throws Exception {
        String mockDumpsysOutput = "incorrect output";
        EasyMock.expect(mMockDevice.executeShellCommand(
                "dumpsys display | grep mBaseDisplayInfo")).andReturn(mockDumpsysOutput).once();
        EasyMock.replay(mMockDevice);
        Dimension result = mMediaPreparer.getMaxVideoPlaybackResolution(mMockDevice);
        assertEquals(result, MediaPreparer.resolutions[MediaPreparer.RES_1920_1080]);
    }

    public void testSkipMediaDownload() throws Exception {
        mOptionSetter.setOptionValue("skip-media-download", "true");
        EasyMock.replay();
        mMediaPreparer.run(mMockDevice, mMockBuildInfo);
    }

}
