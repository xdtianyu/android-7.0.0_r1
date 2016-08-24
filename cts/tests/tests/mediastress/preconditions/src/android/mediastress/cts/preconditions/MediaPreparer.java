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

import com.android.compatibility.common.tradefed.targetprep.PreconditionPreparer;
import com.android.compatibility.common.util.DynamicConfigHostSide;
import com.android.ddmlib.IDevice;
import com.android.tradefed.build.IBuildInfo;
import com.android.tradefed.config.Option;
import com.android.tradefed.config.OptionClass;
import com.android.tradefed.device.DeviceNotAvailableException;
import com.android.tradefed.device.ITestDevice;
import com.android.tradefed.targetprep.BuildError;
import com.android.tradefed.targetprep.TargetSetupError;
import com.android.tradefed.util.FileUtil;
import com.android.tradefed.util.ZipUtil;

import java.awt.Dimension;
import java.io.BufferedOutputStream;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.InputStream;
import java.io.IOException;
import java.net.URL;
import java.net.URLConnection;
import java.util.regex.Matcher;
import java.util.regex.Pattern;
import java.util.zip.ZipFile;

import org.xmlpull.v1.XmlPullParserException;

/**
 * Ensures that the appropriate media files exist on the device
 */
@OptionClass(alias="media-preparer")
public class MediaPreparer extends PreconditionPreparer {

    @Option(name = "local-media-path",
            description = "Absolute path of the media files directory, containing" +
            "'bbb_short' and 'bbb_full' directories")
    protected String mLocalMediaPath = null;

    @Option(name = "skip-media-download",
            description = "Whether to skip the media files precondition")
    protected boolean mSkipMediaDownload = false;

    /*
     * The default name of local directory into which media files will be downloaded, if option
     * "local-media-path" is not provided. This directory will live inside the temp directory.
     */
    protected static final String MEDIA_FOLDER_NAME = "android-cts-media";

    /* The key used to retrieve the media files URL from the dynamic configuration */
    private static final String MEDIA_FILES_URL_KEY = "media_files_url";

    /* For a target preparer, the "module" of the configuration is the test suite */
    private static final String DYNAMIC_CONFIG_MODULE = "cts";

    /*
     * The message printed when the maximum video playback resolution cannot be found in the
     * output of 'dumpsys'. When this is the case, media files of all resolutions must be pushed
     * to the device.
     */
    private static final String MAX_PLAYBACK_RES_FAILURE_MSG =
            "Unable to parse maximum video playback resolution, pushing all media files";

    private static final String LOG_TAG = MediaPreparer.class.getSimpleName();

    /* Constants identifying resolutions of the media files to be copied */
    protected static final int RES_176_144 = 0; // 176x144 resolution
    protected static final int RES_DEFAULT = 1; // default max video playback resolution, 480x360
    protected static final int RES_720_480 = 2; // 720x480 resolution
    protected static final int RES_1280_720 = 3; // 1280x720 resolution
    protected static final int RES_1920_1080 = 4; // 1920x1080 resolution

    protected static final Dimension[] resolutions = { // indices meant to align with constants above
            new Dimension(176, 144),
            new Dimension(480, 360),
            new Dimension(720, 480),
            new Dimension(1280, 720),
            new Dimension(1920, 1080)
    };

    /*
     * The pathnames of the device's directories that hold media files for the tests.
     * These depend on the device's mount point, which is retrieved in the MediaPreparer's run
     * method.
     *
     * These fields are exposed for unit testing
     */
    protected String baseDeviceShortDir;
    protected String baseDeviceFullDir;

    /*
     * Returns a string representation of the dimension
     * For dimension of width = 480 and height = 360, the resolution string is "480x360"
     */
    private static String resolutionString(Dimension resolution) {
        return String.format("%dx%d", resolution.width, resolution.height);
    }

    /*
     * Loops through the predefined maximum video playback resolutions from largest to smallest,
     * And returns the greatest resolution that is strictly smaller than the width and height
     * provided in the arguments
     */
    private Dimension getMaxVideoPlaybackResolution(int width, int height) {
        for (int resIndex = resolutions.length - 1; resIndex >= RES_DEFAULT; resIndex--) {
            Dimension resolution = resolutions[resIndex];
            if (width >= resolution.width && height >= resolution.height) {
                return resolution;
            }
        }
        return resolutions[RES_DEFAULT];
    }

    /*
     * Returns the maximum video playback resolution of the device, in the form of a Dimension
     * object. This method parses dumpsys output to find resolutions listed under the
     * 'mBaseDisplayInfo' field. The value for 'smallest app' is used as an estimate for
     * maximum video playback resolution, and is rounded down to the nearest dimension in the
     * resolutions array.
     *
     * This method is exposed for unit testing.
     */
    protected Dimension getMaxVideoPlaybackResolution(ITestDevice device)
            throws DeviceNotAvailableException {
        String dumpsysOutput =
                device.executeShellCommand("dumpsys display | grep mBaseDisplayInfo");
        Pattern pattern = Pattern.compile("smallest app (\\d+) x (\\d+)");
        Matcher matcher = pattern.matcher(dumpsysOutput);
        if(!matcher.find()) {
            // could not find resolution in dumpsysOutput, return largest max playback resolution
            // so that preparer copies all media files
            logError(MAX_PLAYBACK_RES_FAILURE_MSG);
            return resolutions[RES_1920_1080];
        }

        int first;
        int second;
        try {
            first = Integer.parseInt(matcher.group(1));
            second = Integer.parseInt(matcher.group(2));
        } catch (NumberFormatException e) {
            logError(MAX_PLAYBACK_RES_FAILURE_MSG);
            return resolutions[RES_1920_1080];
        }
        // dimensions in dumpsys output seem consistently reversed
        // here we make note of which dimension is the larger of the two
        int height = Math.min(first, second);
        int width = Math.max(first, second);
        return getMaxVideoPlaybackResolution(width, height);
    }

    /*
     * Returns true if all necessary media files exist on the device, and false otherwise.
     *
     * This method is exposed for unit testing.
     */
    protected boolean mediaFilesExistOnDevice(ITestDevice device, Dimension mvpr)
            throws DeviceNotAvailableException{
        int resIndex = RES_176_144;
        while (resIndex <= RES_1920_1080) {
            Dimension copiedResolution = resolutions[resIndex];
            if (copiedResolution.width > mvpr.width || copiedResolution.height > mvpr.height) {
                break; // we don't need to check for resolutions greater than or equal to this
            }
            String resString = resolutionString(copiedResolution);
            String deviceShortFilePath = baseDeviceShortDir + resString;
            String deviceFullFilePath = baseDeviceFullDir + resString;
            if (!device.doesFileExist(deviceShortFilePath) ||
                    !device.doesFileExist(deviceFullFilePath)) { // media files must be copied
                return false;
            }
            resIndex++;
        }
        return true;
    }

    /*
     * After downloading and unzipping the media files, mLocalMediaPath must be the path to the
     * directory containing 'bbb_short' and 'bbb_full' directories, as it is defined in its
     * description as an option.
     * After extraction, this directory exists one level below the the directory 'mediaFolder'.
     * If the 'mediaFolder' contains anything other than exactly one subdirectory, a
     * TargetSetupError is thrown. Otherwise, the mLocalMediaPath variable is set to the path of
     * this subdirectory.
     */
    private void updateLocalMediaPath(File mediaFolder) throws TargetSetupError {
        String[] subDirs = mediaFolder.list();
        if (subDirs.length != 1) {
            throw new TargetSetupError(String.format(
                    "Unexpected contents in directory %s", mLocalMediaPath));
        }
        File newMediaFolder = new File(mediaFolder, subDirs[0]);
        mLocalMediaPath = newMediaFolder.toString();
    }

    /*
     * Copies the media files to the host from a predefined URL
     * Updates mLocalMediaPath to be the pathname of the directory containing bbb_short and
     * bbb_full media directories.
     */
    private void downloadMediaToHost() throws TargetSetupError {

        URL url;
        try {
            DynamicConfigHostSide config = new DynamicConfigHostSide(DYNAMIC_CONFIG_MODULE);
            String mediaUrlString = config.getValue(MEDIA_FILES_URL_KEY);
            url = new URL(mediaUrlString);
        } catch (IOException | XmlPullParserException e) {
            throw new TargetSetupError("Trouble finding media file download location with " +
                    "dynamic configuration");
        }

        File mediaFolder = new File(mLocalMediaPath);
        File mediaFolderZip = new File(mediaFolder.getName() + ".zip");
        try {

            mediaFolder.mkdirs();
            mediaFolderZip.createNewFile();

            URLConnection conn = url.openConnection();
            InputStream in = conn.getInputStream();
            BufferedOutputStream out =
                    new BufferedOutputStream(new FileOutputStream(mediaFolderZip));
            byte[] buffer = new byte[1024];
            int count;
            logInfo("Downloading media files to host from %s", url.toString());
            while ((count = in.read(buffer)) >= 0) {
                out.write(buffer, 0, count);
            }
            out.flush();
            out.close();
            in.close();

            logInfo("Unzipping media files");
            ZipUtil.extractZip(new ZipFile(mediaFolderZip), mediaFolder);

        } catch (IOException e) {
            FileUtil.recursiveDelete(mediaFolder);
            FileUtil.recursiveDelete(mediaFolderZip);
            throw new TargetSetupError("Failed to download and open media files on host, the"
                    + " device requires these media files for CTS media tests");
        }
    }

    /*
     * Pushes directories containing media files to the device for all directories that:
     * - are not already present on the device
     * - contain video files of a resolution less than or equal to the device's
     *       max video playback resolution
     *
     * This method is exposed for unit testing.
     */
    protected void copyMediaFiles(ITestDevice device, Dimension mvpr)
            throws DeviceNotAvailableException {

        int resIndex = RES_176_144;
        while (resIndex <= RES_1920_1080) {
            Dimension copiedResolution = resolutions[resIndex];
            String resString = resolutionString(copiedResolution);
            if (copiedResolution.width > mvpr.width || copiedResolution.height > mvpr.height) {
                logInfo("Device cannot support resolutions %s and larger, media copying complete",
                        resString);
                return;
            }
            String deviceShortFilePath = baseDeviceShortDir + resString;
            String deviceFullFilePath = baseDeviceFullDir + resString;
            if (!device.doesFileExist(deviceShortFilePath) ||
                    !device.doesFileExist(deviceFullFilePath)) {
                logInfo("Copying files of resolution %s to device", resString);
                String localShortDirName = "bbb_short/" + resString;
                String localFullDirName = "bbb_full/" + resString;
                File localShortDir = new File(mLocalMediaPath, localShortDirName);
                File localFullDir = new File(mLocalMediaPath, localFullDirName);
                // push short directory of given resolution, if not present on device
                if(!device.doesFileExist(deviceShortFilePath)) {
                    device.pushDir(localShortDir, deviceShortFilePath);
                }
                // push full directory of given resolution, if not present on device
                if(!device.doesFileExist(deviceFullFilePath)) {
                    device.pushDir(localFullDir, deviceFullFilePath);
                }
            }
            resIndex++;
        }
    }

    // Initialize directory strings where media files live on device
    protected void setMountPoint(ITestDevice device) {
        String mountPoint = device.getMountPoint(IDevice.MNT_EXTERNAL_STORAGE);
        baseDeviceShortDir = String.format("%s/test/bbb_short/", mountPoint);
        baseDeviceFullDir = String.format("%s/test/bbb_full/", mountPoint);
    }

    @Override
    public void run(ITestDevice device, IBuildInfo buildInfo) throws TargetSetupError,
            BuildError, DeviceNotAvailableException {

        if (mSkipMediaDownload) {
            // skip this precondition
            return;
        }

        setMountPoint(device);
        Dimension mvpr = getMaxVideoPlaybackResolution(device);
        if (mediaFilesExistOnDevice(device, mvpr)) {
            // if files already on device, do nothing
            logInfo("Media files found on the device");
            return;
        }

        File mediaFolder;
        if (mLocalMediaPath == null) {
            // Option 'local-media-path' has not been defined
            try {
                // find system's temp directory, create folder MEDIA_FOLDER_NAME inside
                File tmpFile = File.createTempFile(MEDIA_FOLDER_NAME, null);
                String tmpDir = tmpFile.getParent();
                mediaFolder = new File(tmpDir, MEDIA_FOLDER_NAME);
                // delete temp file used for locating temp directory
                tmpFile.delete();
            } catch (IOException e) {
                throw new TargetSetupError("Unable to create host temp directory for media files");
            }
            mLocalMediaPath = mediaFolder.getAbsolutePath();
            if(!mediaFolder.exists()){
                // directory has not been created by previous runs of MediaPreparer
                // download media into mLocalMediaPath
                downloadMediaToHost();
            }
            updateLocalMediaPath(mediaFolder);
        }

        logInfo("Media files located on host at: %s", mLocalMediaPath);
        copyMediaFiles(device, mvpr);
    }

}
