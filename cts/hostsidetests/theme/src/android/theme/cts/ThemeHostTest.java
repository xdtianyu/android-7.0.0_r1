/*
 * Copyright (C) 2014 The Android Open Source Project
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

package android.theme.cts;

import com.android.compatibility.common.util.AbiUtils;
import com.android.cts.migration.MigrationHelper;
import com.android.ddmlib.Log;
import com.android.ddmlib.Log.LogLevel;
import com.android.tradefed.build.IBuildInfo;
import com.android.tradefed.device.CollectingOutputReceiver;
import com.android.tradefed.device.DeviceNotAvailableException;
import com.android.tradefed.device.ITestDevice;
import com.android.tradefed.testtype.DeviceTestCase;
import com.android.tradefed.testtype.IAbi;
import com.android.tradefed.testtype.IAbiReceiver;
import com.android.tradefed.testtype.IBuildReceiver;

import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.util.HashMap;
import java.util.Map;
import java.util.concurrent.ExecutorCompletionService;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;
import java.util.concurrent.TimeUnit;
import java.util.zip.ZipEntry;
import java.util.zip.ZipInputStream;

/**
 * Test to check non-modifiable themes have not been changed.
 */
public class ThemeHostTest extends DeviceTestCase implements IAbiReceiver, IBuildReceiver {

    private static final String LOG_TAG = "ThemeHostTest";
    private static final String APK_NAME = "CtsThemeDeviceApp";
    private static final String APP_PACKAGE_NAME = "android.theme.app";

    private static final String GENERATED_ASSETS_ZIP = "/sdcard/cts-theme-assets.zip";

    /** The class name of the main activity in the APK. */
    private static final String TEST_CLASS = "android.support.test.runner.AndroidJUnitRunner";

    /** The command to launch the main instrumentation test. */
    private static final String START_CMD = String.format(
            "am instrument -w --no-window-animation %s/%s", APP_PACKAGE_NAME, TEST_CLASS);

    private static final String CLEAR_GENERATED_CMD = "rm -rf %s/*.png";
    private static final String STOP_CMD = String.format("am force-stop %s", APP_PACKAGE_NAME);
    private static final String HARDWARE_TYPE_CMD = "dumpsys | grep android.hardware.type";
    private static final String DENSITY_PROP_DEVICE = "ro.sf.lcd_density";
    private static final String DENSITY_PROP_EMULATOR = "qemu.sf.lcd_density";

    /** Overall test timeout is 30 minutes. Should only take about 5. */
    private static final int TEST_RESULT_TIMEOUT = 30 * 60 * 1000;

    /** Map of reference image names and files. */
    private Map<String, File> mReferences;

    /** The ABI to use. */
    private IAbi mAbi;

    /** A reference to the build. */
    private IBuildInfo mBuildInfo;

    /** A reference to the device under test. */
    private ITestDevice mDevice;

    private ExecutorService mExecutionService;

    private ExecutorCompletionService<Boolean> mCompletionService;

    @Override
    public void setAbi(IAbi abi) {
        mAbi = abi;
    }

    @Override
    public void setBuild(IBuildInfo buildInfo) {
        mBuildInfo = buildInfo;
    }

    @Override
    protected void setUp() throws Exception {
        super.setUp();

        mDevice = getDevice();
        mDevice.uninstallPackage(APP_PACKAGE_NAME);

        // Get the APK from the build.
        final File app = MigrationHelper.getTestFile(mBuildInfo, String.format("%s.apk", APK_NAME));
        final String[] options = {AbiUtils.createAbiFlag(mAbi.getName())};
        mDevice.installPackage(app, true, true, options);

        final String density = getDensityBucketForDevice(mDevice);
        final String zipFile = String.format("/%s.zip", density);
        mReferences = extractReferenceImages(zipFile);

        final int numCores = Runtime.getRuntime().availableProcessors();
        mExecutionService = Executors.newFixedThreadPool(numCores * 2);
        mCompletionService = new ExecutorCompletionService<>(mExecutionService);
    }

    private Map<String, File> extractReferenceImages(String zipFile) {
        final Map<String, File> references = new HashMap<>();
        final InputStream zipStream = ThemeHostTest.class.getResourceAsStream(zipFile);
        if (zipStream != null) {
            try (ZipInputStream in = new ZipInputStream(zipStream)) {
                ZipEntry ze;
                final byte[] buffer = new byte[1024];
                while ((ze = in.getNextEntry()) != null) {
                    final String name = ze.getName();
                    final File tmp = File.createTempFile("ref_" + name, ".png");
                    final FileOutputStream out = new FileOutputStream(tmp);

                    int count;
                    while ((count = in.read(buffer)) != -1) {
                        out.write(buffer, 0, count);
                    }

                    out.flush();
                    out.close();
                    references.put(name, tmp);
                }
            } catch (IOException e) {
                fail("Failed to unzip assets: " + zipFile);
            }
        } else {
            fail("Failed to get resource: " + zipFile);
        }

        return references;
    }

    @Override
    protected void tearDown() throws Exception {
        // Delete the temp files
        mReferences.values().forEach(File::delete);

        mExecutionService.shutdown();

        // Remove the APK.
        mDevice.uninstallPackage(APP_PACKAGE_NAME);

        // Remove generated images.
        mDevice.executeShellCommand(CLEAR_GENERATED_CMD);

        super.tearDown();
    }

    public void testThemes() throws Exception {
        if (checkHardwareTypeSkipTest(mDevice.executeShellCommand(HARDWARE_TYPE_CMD).trim())) {
            Log.logAndDisplay(LogLevel.INFO, LOG_TAG, "Skipped themes test for watch");
            return;
        }

        if (mReferences.isEmpty()) {
            Log.logAndDisplay(LogLevel.INFO, LOG_TAG,
                    "Skipped themes test due to missing reference images");
            return;
        }

        assertTrue("Aborted image generation", generateDeviceImages());

        // Pull ZIP file from remote device.
        final File localZip = File.createTempFile("generated", ".zip");
        assertTrue("Failed to pull generated assets from device",
                mDevice.pullFile(GENERATED_ASSETS_ZIP, localZip));

        final int numTasks = extractGeneratedImages(localZip, mReferences);

        int failures = 0;
        for (int i = numTasks; i > 0; i--) {
            failures += mCompletionService.take().get() ? 0 : 1;
        }

        assertTrue(failures + " failures in theme test", failures == 0);
    }

    private int extractGeneratedImages(File localZip, Map<String, File> references)
            throws IOException {
        int numTasks = 0;

        // Extract generated images to temporary files.
        final byte[] data = new byte[4096];
        try (ZipInputStream zipInput = new ZipInputStream(new FileInputStream(localZip))) {
            ZipEntry entry;
            while ((entry = zipInput.getNextEntry()) != null) {
                final String name = entry.getName();
                final File expected = references.get(name);
                if (expected != null && expected.exists()) {
                    final File actual = File.createTempFile("actual_" + name, ".png");
                    final FileOutputStream pngOutput = new FileOutputStream(actual);

                    int count;
                    while ((count = zipInput.read(data, 0, data.length)) != -1) {
                        pngOutput.write(data, 0, count);
                    }

                    pngOutput.flush();
                    pngOutput.close();

                    mCompletionService.submit(new ComparisonTask(expected, actual));
                    numTasks++;
                } else {
                    Log.logAndDisplay(LogLevel.INFO, LOG_TAG,
                            "Missing reference image for " + name);
                }

                zipInput.closeEntry();
            }
        }

        return numTasks;
    }

    private boolean generateDeviceImages() throws Exception {
        // Stop any existing instances.
        mDevice.executeShellCommand(STOP_CMD);

        // Start instrumentation test.
        final CollectingOutputReceiver receiver = new CollectingOutputReceiver();
        mDevice.executeShellCommand(START_CMD, receiver, TEST_RESULT_TIMEOUT,
                TimeUnit.MILLISECONDS, 0);

        return receiver.getOutput().contains("OK ");
    }

    private static String getDensityBucketForDevice(ITestDevice device) {
        final String densityProp;
        if (device.getSerialNumber().startsWith("emulator-")) {
            densityProp = DENSITY_PROP_EMULATOR;
        } else {
            densityProp = DENSITY_PROP_DEVICE;
        }

        final int density;
        try {
            density = Integer.parseInt(device.getProperty(densityProp));
        } catch (DeviceNotAvailableException e) {
            return "unknown";
        }

        switch (density) {
            case 120:
                return "ldpi";
            case 160:
                return "mdpi";
            case 213:
                return "tvdpi";
            case 240:
                return "hdpi";
            case 320:
                return "xhdpi";
            case 480:
                return "xxhdpi";
            case 640:
                return "xxxhdpi";
            default:
                return density + "dpi";
        }
    }

    private static boolean checkHardwareTypeSkipTest(String hardwareTypeString) {
        return hardwareTypeString.contains("android.hardware.type.watch")
                || hardwareTypeString.contains("android.hardware.type.television");
    }
}
