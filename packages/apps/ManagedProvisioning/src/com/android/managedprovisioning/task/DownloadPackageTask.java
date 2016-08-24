/*
 * Copyright 2014, The Android Open Source Project
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
package com.android.managedprovisioning.task;

import android.app.DownloadManager;
import android.app.DownloadManager.Query;
import android.app.DownloadManager.Request;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.pm.PackageInfo;
import android.content.pm.PackageManager;
import android.content.pm.PackageManager.NameNotFoundException;
import android.content.pm.Signature;
import android.database.Cursor;
import android.net.Uri;
import android.text.TextUtils;

import com.android.managedprovisioning.NetworkMonitor;
import com.android.managedprovisioning.ProvisionLogger;
import com.android.managedprovisioning.common.Utils;
import com.android.managedprovisioning.model.PackageDownloadInfo;

import java.io.InputStream;
import java.io.IOException;
import java.io.File;
import java.io.FileInputStream;
import java.security.MessageDigest;
import java.security.NoSuchAlgorithmException;
import java.util.Arrays;
import java.util.HashSet;
import java.util.LinkedList;
import java.util.List;
import java.util.Set;

/**
 * Downloads all packages that were added. Also verifies that the downloaded files are the ones that
 * are expected.
 */
public class DownloadPackageTask {
    private static final boolean DEBUG = false; // To control logging.

    public static final int ERROR_HASH_MISMATCH = 0;
    public static final int ERROR_DOWNLOAD_FAILED = 1;
    public static final int ERROR_OTHER = 2;

    private static final String SHA1_TYPE = "SHA-1";
    private static final String SHA256_TYPE = "SHA-256";

    private final Context mContext;
    private final Callback mCallback;
    private BroadcastReceiver mReceiver;
    private final DownloadManager mDlm;
    private final PackageManager mPm;
    private int mFileNumber = 0;

    private final Utils mUtils = new Utils();

    private Set<DownloadStatusInfo> mDownloads;

    public DownloadPackageTask (Context context, Callback callback) {
        mCallback = callback;
        mContext = context;
        mDlm = (DownloadManager) mContext.getSystemService(Context.DOWNLOAD_SERVICE);
        mDlm.setAccessFilename(true);
        mPm = context.getPackageManager();

        mDownloads = new HashSet<DownloadStatusInfo>();
    }

    public void addDownloadIfNecessary(
            String packageName, PackageDownloadInfo downloadInfo, String label) {
        if (downloadInfo != null
                && mUtils.packageRequiresUpdate(packageName, downloadInfo.minVersion, mContext)) {
            mDownloads.add(new DownloadStatusInfo(downloadInfo, label));
        }
    }

    public void run() {
        if (mDownloads.size() == 0) {
            mCallback.onSuccess();
            return;
        }
        if (!mUtils.isConnectedToNetwork(mContext)) {
            ProvisionLogger.loge("DownloadPackageTask: not connected to the network, can't download"
                    + " the package");
            mCallback.onError(ERROR_OTHER);
        }
        mReceiver = createDownloadReceiver();
        mContext.registerReceiver(mReceiver,
                new IntentFilter(DownloadManager.ACTION_DOWNLOAD_COMPLETE));

        DownloadManager dm = (DownloadManager) mContext
                .getSystemService(Context.DOWNLOAD_SERVICE);
        for (DownloadStatusInfo info : mDownloads) {
            if (DEBUG) {
                ProvisionLogger.logd("Starting download from " +
                        info.mPackageDownloadInfo.location);
            }

            Request request = new Request(Uri.parse(info.mPackageDownloadInfo.location));
            // All we want is to have a different file for each apk
            // Note that the apk may not actually be downloaded to this path. This could happen if
            // this file already exists.
            String path = mContext.getExternalFilesDir(null)
                    + "/download_cache/managed_provisioning_downloaded_app_" + mFileNumber + ".apk";
            mFileNumber++;
            File downloadedFile = new File(path);
            downloadedFile.getParentFile().mkdirs(); // If the folder doesn't exists it is created
            request.setDestinationUri(Uri.fromFile(downloadedFile));
            if (info.mPackageDownloadInfo.cookieHeader != null) {
                request.addRequestHeader("Cookie", info.mPackageDownloadInfo.cookieHeader);
                if (DEBUG) {
                    ProvisionLogger.logd("Downloading with http cookie header: "
                            + info.mPackageDownloadInfo.cookieHeader);
                }
            }
            info.mDownloadId = dm.enqueue(request);
        }
    }

    private BroadcastReceiver createDownloadReceiver() {
        return new BroadcastReceiver() {
            /**
             * Whenever the download manager finishes a download, record the successful download for
             * the corresponding DownloadStatusInfo.
             */
            @Override
            public void onReceive(Context context, Intent intent) {
                if (DownloadManager.ACTION_DOWNLOAD_COMPLETE.equals(intent.getAction())) {
                    Query q = new Query();
                    for (DownloadStatusInfo info : mDownloads) {
                        q.setFilterById(info.mDownloadId);
                        Cursor c = mDlm.query(q);
                        if (c.moveToFirst()) {
                            long downloadId =
                                    c.getLong(c.getColumnIndex(DownloadManager.COLUMN_ID));
                            String filePath = c.getString(c.getColumnIndex(
                                    DownloadManager.COLUMN_LOCAL_FILENAME));
                            int columnIndex = c.getColumnIndex(DownloadManager.COLUMN_STATUS);
                            if (DownloadManager.STATUS_SUCCESSFUL == c.getInt(columnIndex)) {
                                c.close();
                                onDownloadSuccess(downloadId, filePath);
                            } else if (DownloadManager.STATUS_FAILED == c.getInt(columnIndex)){
                                int reason = c.getInt(
                                        c.getColumnIndex(DownloadManager.COLUMN_REASON));
                                c.close();
                                onDownloadFail(reason);
                            }
                        }
                    }
                }
            }
        };
    }

    /**
     * For a given successful download, check that the downloaded file is the expected file.
     * If the package hash is provided then that is used, otherwise a signature hash is used.
     * Then check if this was the last file the task had to download and finish the
     * DownloadPackageTask if that is the case.
     * @param downloadId the unique download id for the completed download.
     * @param location the file location of the downloaded file.
     */
    private void onDownloadSuccess(long downloadId, String filePath) {
        DownloadStatusInfo info = null;
        for (DownloadStatusInfo infoToMatch : mDownloads) {
            if (downloadId == infoToMatch.mDownloadId) {
                info = infoToMatch;
            }
        }
        if (info == null || info.mDoneDownloading) {
            // DownloadManager can send success more than once. Only act first time.
            return;
        } else {
            info.mDoneDownloading = true;
            info.mLocation = filePath;
        }
        ProvisionLogger.logd("Downloaded succesfully to: " + info.mLocation);

        boolean downloadedContentsCorrect = false;
        if (info.mPackageDownloadInfo.packageChecksum.length > 0) {
            downloadedContentsCorrect = doesPackageHashMatch(info);
        } else if (info.mPackageDownloadInfo.signatureChecksum.length > 0) {
            downloadedContentsCorrect = doesASignatureHashMatch(info);
        }

        if (downloadedContentsCorrect) {
            info.mSuccess = true;
            checkSuccess();
        } else {
            mCallback.onError(ERROR_HASH_MISMATCH);
        }
    }

    /**
     * Check whether package hash of downloaded file matches the hash given in DownloadStatusInfo.
     * By default, SHA-256 is used to verify the file hash.
     * If mPackageDownloadInfo.packageChecksumSupportsSha1 == true, SHA-1 hash is also supported for
     * backwards compatibility.
     */
    private boolean doesPackageHashMatch(DownloadStatusInfo info) {
        byte[] packageSha256Hash, packageSha1Hash = null;

        ProvisionLogger.logd("Checking file hash of entire apk file.");
        packageSha256Hash = computeHashOfFile(info.mLocation, SHA256_TYPE);
        if (packageSha256Hash == null) {
            // Error should have been reported in computeHashOfFile().
            return false;
        }

        if (Arrays.equals(info.mPackageDownloadInfo.packageChecksum, packageSha256Hash)) {
            return true;
        }

        // Fall back to SHA-1
        if (info.mPackageDownloadInfo.packageChecksumSupportsSha1) {
            packageSha1Hash = computeHashOfFile(info.mLocation, SHA1_TYPE);
            if (Arrays.equals(info.mPackageDownloadInfo.packageChecksum, packageSha1Hash)) {
                return true;
            }
        }

        ProvisionLogger.loge("Provided hash does not match file hash.");
        ProvisionLogger.loge("Hash provided by programmer: "
                + mUtils.byteArrayToString(info.mPackageDownloadInfo.packageChecksum));
        ProvisionLogger.loge("SHA-256 Hash computed from file: " + mUtils.byteArrayToString(
                packageSha256Hash));
        if (packageSha1Hash != null) {
            ProvisionLogger.loge("SHA-1 Hash computed from file: " + mUtils.byteArrayToString(
                    packageSha1Hash));
        }
        return false;
    }

    private boolean doesASignatureHashMatch(DownloadStatusInfo info) {
        // Check whether a signature hash of downloaded apk matches the hash given in constructor.
        ProvisionLogger.logd("Checking " + SHA256_TYPE
                + "-hashes of all signatures of downloaded package.");
        List<byte[]> sigHashes = computeHashesOfAllSignatures(info.mLocation);
        if (sigHashes == null) {
            // Error should have been reported in computeHashesOfAllSignatures().
            return false;
        }
        if (sigHashes.isEmpty()) {
            ProvisionLogger.loge("Downloaded package does not have any signatures.");
            return false;
        }
        for (byte[] sigHash : sigHashes) {
            if (Arrays.equals(sigHash, info.mPackageDownloadInfo.signatureChecksum)) {
                return true;
            }
        }

        ProvisionLogger.loge("Provided hash does not match any signature hash.");
        ProvisionLogger.loge("Hash provided by programmer: "
                + mUtils.byteArrayToString(info.mPackageDownloadInfo.signatureChecksum));
        ProvisionLogger.loge("Hashes computed from package signatures: ");
        for (byte[] sigHash : sigHashes) {
            ProvisionLogger.loge(mUtils.byteArrayToString(sigHash));
        }

        return false;
    }

    private void checkSuccess() {
        for (DownloadStatusInfo info : mDownloads) {
            if (!info.mSuccess) {
                return;
            }
        }
        mCallback.onSuccess();
    }

    private void onDownloadFail(int errorCode) {
        ProvisionLogger.loge("Downloading package failed.");
        ProvisionLogger.loge("COLUMN_REASON in DownloadManager response has value: "
                + errorCode);
        mCallback.onError(ERROR_DOWNLOAD_FAILED);
    }

    private byte[] computeHashOfFile(String fileLocation, String hashType) {
        InputStream fis = null;
        MessageDigest md;
        byte hash[] = null;
        try {
            md = MessageDigest.getInstance(hashType);
        } catch (NoSuchAlgorithmException e) {
            ProvisionLogger.loge("Hashing algorithm " + hashType + " not supported.", e);
            mCallback.onError(ERROR_OTHER);
            return null;
        }
        try {
            fis = new FileInputStream(fileLocation);

            byte[] buffer = new byte[256];
            int n = 0;
            while (n != -1) {
                n = fis.read(buffer);
                if (n > 0) {
                    md.update(buffer, 0, n);
                }
            }
            hash = md.digest();
        } catch (IOException e) {
            ProvisionLogger.loge("IO error.", e);
            mCallback.onError(ERROR_OTHER);
        } finally {
            // Close input stream quietly.
            try {
                if (fis != null) {
                    fis.close();
                }
            } catch (IOException e) {
                // Ignore.
            }
        }
        return hash;
    }

    public String getDownloadedPackageLocation(String label) {
        for (DownloadStatusInfo info : mDownloads) {
            if (info.mLabel.equals(label)) {
                return info.mLocation;
            }
        }
        return "";
    }

    private List<byte[]> computeHashesOfAllSignatures(String packageArchiveLocation) {
        PackageInfo info = mPm.getPackageArchiveInfo(packageArchiveLocation,
                PackageManager.GET_SIGNATURES);
        if (info == null) {
            ProvisionLogger.loge("Unable to get package archive info from "
                    + packageArchiveLocation);
            mCallback.onError(ERROR_OTHER);
            return null;
        }

        List<byte[]> hashes = new LinkedList<byte[]>();
        Signature signatures[] = info.signatures;
        try {
            for (Signature signature : signatures) {
               byte[] hash = computeHashOfByteArray(signature.toByteArray());
               hashes.add(hash);
            }
        } catch (NoSuchAlgorithmException e) {
            ProvisionLogger.loge("Hashing algorithm " + SHA256_TYPE + " not supported.", e);
            mCallback.onError(ERROR_OTHER);
            return null;
        }
        return hashes;
    }

    private byte[] computeHashOfByteArray(byte[] bytes) throws NoSuchAlgorithmException {
        MessageDigest md = MessageDigest.getInstance(SHA256_TYPE);
        md.update(bytes, 0, bytes.length);
        return md.digest();
    }

    public void cleanUp() {
        if (mReceiver != null) {
            //Unregister receiver.
            mContext.unregisterReceiver(mReceiver);
            mReceiver = null;
        }

        //Remove download.
        DownloadManager dm = (DownloadManager) mContext
                .getSystemService(Context.DOWNLOAD_SERVICE);
        for (DownloadStatusInfo info : mDownloads) {
            boolean removeSuccess = dm.remove(info.mDownloadId) == 1;
            if (removeSuccess) {
                ProvisionLogger.logd("Successfully removed installer file.");
            } else {
                ProvisionLogger.loge("Could not remove installer file.");
                // Ignore this error. Failing cleanup should not stop provisioning flow.
            }
        }
    }

    public abstract static class Callback {
        public abstract void onSuccess();
        public abstract void onError(int errorCode);
    }

    private static class DownloadStatusInfo {
        public final PackageDownloadInfo mPackageDownloadInfo;
        public final String mLabel;
        public long mDownloadId;
        public String mLocation; // Location where the package is downloaded to.
        public boolean mDoneDownloading;
        public boolean mSuccess;

        public DownloadStatusInfo(PackageDownloadInfo packageDownloadInfo,String label) {
            mPackageDownloadInfo = packageDownloadInfo;
            mLabel = label;
            mDoneDownloading = false;
        }
    }
}
