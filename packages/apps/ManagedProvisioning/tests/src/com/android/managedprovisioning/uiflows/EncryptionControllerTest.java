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
package com.android.managedprovisioning.uiflows;

import static android.app.admin.DevicePolicyManager.ACTION_PROVISION_MANAGED_DEVICE;
import static android.app.admin.DevicePolicyManager.ACTION_PROVISION_MANAGED_PROFILE;
import static org.mockito.Mockito.any;
import static org.mockito.Mockito.anyInt;
import static org.mockito.Mockito.eq;
import static org.mockito.Mockito.never;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;

import android.app.Notification;
import android.app.NotificationManager;
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.res.Resources;
import android.content.pm.ApplicationInfo;
import android.content.pm.PackageManager;
import android.os.Handler;
import android.os.Looper;
import android.test.AndroidTestCase;
import android.test.suitebuilder.annotation.SmallTest;

import com.android.managedprovisioning.IntentStore;
import com.android.managedprovisioning.common.Globals;
import com.android.managedprovisioning.common.Utils;
import com.android.managedprovisioning.model.ProvisioningParams;
import com.android.managedprovisioning.parser.MessageParser;

import java.util.concurrent.Semaphore;
import java.util.concurrent.TimeUnit;
import org.mockito.ArgumentCaptor;
import org.mockito.Mock;
import org.mockito.MockitoAnnotations;

@SmallTest
public class EncryptionControllerTest extends AndroidTestCase {
    private static final int TEST_USER_ID = 10;
    private static final String MP_PACKAGE_NAME = "com.android.managedprovisioning";
    private static final ComponentName TEST_HOME_RECEIVER = new ComponentName(MP_PACKAGE_NAME,
            ".HomeReceiverActivity");
    private static final String TEST_MDM_PACKAGE = "com.admin.test";
    private static final String TEST_STRING = "Test";
    private static final int RESUME_PROVISIONING_TIMEOUT_MS = 1000;

    @Mock private Context mContext;
    @Mock private IntentStore mIntentStore;
    @Mock private Utils mUtils;
    @Mock private Resources mResources;
    @Mock private PackageManager mPackageManager;
    @Mock private EncryptionController.ResumeNotificationHelper mResumeNotificationHelper;

    private EncryptionController mController;
    private MessageParser mMessageParser;
    private Intent mIntent;

    @Override
    public void setUp() {
        // this is necessary for mockito to work
        System.setProperty("dexmaker.dexcache", getContext().getCacheDir().toString());

        MockitoAnnotations.initMocks(this);

        mMessageParser = new MessageParser();

        when(mUtils.isPhysicalDeviceEncrypted()).thenReturn(true);
        when(mContext.getApplicationContext()).thenReturn(mContext);
        when(mContext.getPackageManager()).thenReturn(mPackageManager);

        mController = new EncryptionController(mContext, mIntentStore, mUtils, mMessageParser,
                TEST_HOME_RECEIVER, mResumeNotificationHelper, TEST_USER_ID);
    }

    public void testSetEncryptionReminder_duringSuw() {
        final String action = ACTION_PROVISION_MANAGED_DEVICE;
        // GIVEN a set of parameters to be stored for resumption
        // WHEN setting an encryption reminder
        mController.setEncryptionReminder(createAndStoreProvisioningParams(action));
        // THEN the intent is stored in IntentStore and the HOME receiver is enabled
        ArgumentCaptor<Intent> intentCaptor = ArgumentCaptor.forClass(Intent.class);
        verify(mIntentStore).save(intentCaptor.capture());
        Intent intent = intentCaptor.getValue();
        assertEquals(Globals.ACTION_RESUME_PROVISIONING, intent.getAction());
        assertEquals(action, intent.getStringExtra(MessageParser.EXTRA_PROVISIONING_ACTION));
        verify(mUtils).enableComponent(TEST_HOME_RECEIVER, TEST_USER_ID);
        verify(mPackageManager).flushPackageRestrictionsAsUser(TEST_USER_ID);
    }

    public void testSetEncryptionReminder_afterSuw() {
        final String action = ACTION_PROVISION_MANAGED_PROFILE;
        // GIVEN a set of parameters to be stored for resumption and user setup has been completed
        // on the given user
        when(mUtils.isUserSetupCompleted(mContext)).thenReturn(true);
        // WHEN setting an encryption reminder
        mController.setEncryptionReminder(createAndStoreProvisioningParams(action));
        // THEN the intent is stored in IntentStore and the HOME receiver is enabled
        ArgumentCaptor<Intent> intentCaptor = ArgumentCaptor.forClass(Intent.class);
        verify(mIntentStore).save(intentCaptor.capture());
        Intent intent = intentCaptor.getValue();
        assertEquals(Globals.ACTION_RESUME_PROVISIONING, intent.getAction());
        assertEquals(action, intent.getStringExtra(MessageParser.EXTRA_PROVISIONING_ACTION));
        verify(mUtils, never()).enableComponent(any(ComponentName.class), anyInt());
    }

    public void testResumeProvisioning_profileOwnerAfterSuw() throws Exception {
        // GIVEN an intent was stored to resume managed profile provisioning after SUW
        when(mUtils.isUserSetupCompleted(mContext)).thenReturn(true);
        createAndStoreProvisioningParams(ACTION_PROVISION_MANAGED_PROFILE);
        // WHEN resuming provisioning
        runResumeProvisioningOnUiThread();
        // THEN a resume notification should be posted and no activity should be started
        verify(mResumeNotificationHelper).showResumeNotification(any(Intent.class));
        verify(mContext, never()).startActivity(any(Intent.class));
    }

    public void testResumeProvisioning_profileOwnerDuringSuw() throws Exception {
        // GIVEN an intent was stored to resume managed profile provisioning during SUW
        createAndStoreProvisioningParams(ACTION_PROVISION_MANAGED_PROFILE);
        // WHEN resuming provisioning
        runResumeProvisioningOnUiThread();
        // THEN the PreProvisioningActivity should be started and no notification should be posted
        verify(mContext).startActivity(mIntent);
        verify(mResumeNotificationHelper, never()).showResumeNotification(any(Intent.class));
    }

    public void testResumeProvisioningTwice_profileOwnerDuringSuw() throws Exception {
        // GIVEN an intent was stored to resume managed profile provisioning during SUW
        createAndStoreProvisioningParams(ACTION_PROVISION_MANAGED_PROFILE);
        // GIVEN resuming provisioning was called once.
        runResumeProvisioningOnUiThread();
        // // WHEN resuming provisioning is called again.
        runResumeProvisioningOnUiThread();
        // THEN the PreProvisioningActivity should only be started once and no notification should
        // be posted
        verify(mContext).startActivity(mIntent);
        verify(mResumeNotificationHelper, never()).showResumeNotification(any(Intent.class));
    }

    public void testResumeProvisioning_deviceOwner() throws Exception {
        // GIVEN an intent was stored to resume device owner provisioning during SUW
        createAndStoreProvisioningParams(ACTION_PROVISION_MANAGED_DEVICE);
        // WHEN resuming provisioning
        runResumeProvisioningOnUiThread();
        // THEN the PreProvisioningActivity should be started and no notification should be posted
        verify(mContext).startActivity(mIntent);
        verify(mResumeNotificationHelper, never()).showResumeNotification(any(Intent.class));
    }

    public void testResumeProvisioning_deviceNotEncrypted() throws Exception {
        // GIVEN an intent was stored to resume device owner provisioning, but the device
        // is not encrypted
        when(mUtils.isPhysicalDeviceEncrypted()).thenReturn(false);
        createAndStoreProvisioningParams(ACTION_PROVISION_MANAGED_DEVICE);
        // WHEN resuming provisioning
        runResumeProvisioningOnUiThread();
        // THEN nothing should happen
        verify(mContext, never()).startActivity(any(Intent.class));
        verify(mResumeNotificationHelper, never()).showResumeNotification(any(Intent.class));
    }

    public void testResumeProvisioning_noIntent() throws Exception {
        // GIVEN an intent was stored to resume device owner provisioning, but the device
        // is not encrypted
        when(mIntentStore.load()).thenReturn(null);
        // WHEN resuming provisioning
        runResumeProvisioningOnUiThread();
        // THEN nothing should happen
        verify(mContext, never()).startActivity(any(Intent.class));
        verify(mResumeNotificationHelper, never()).showResumeNotification(any(Intent.class));
    }

    public void testCancelProvisioningReminder() {
        // WHEN cancelling the provisioning reminder
        mController.cancelEncryptionReminder();
        // THEN the intent store should be cleared and the HOME receiver disabled
        verify(mIntentStore).clear();
        verify(mUtils).disableComponent(TEST_HOME_RECEIVER, TEST_USER_ID);
    }

    private ProvisioningParams createAndStoreProvisioningParams(String action) {
        ProvisioningParams params = new ProvisioningParams.Builder()
                .setProvisioningAction(action)
                .setDeviceAdminPackageName(TEST_MDM_PACKAGE)
                .build();
        mIntent = mMessageParser.getIntentFromProvisioningParams(params);
        assertEquals(action, mIntent.getStringExtra(MessageParser.EXTRA_PROVISIONING_ACTION));
        when(mIntentStore.load()).thenReturn(mIntent);
        return params;
    }

    private void runResumeProvisioningOnUiThread() throws InterruptedException {
        final Semaphore semaphore = new Semaphore(0);
        new Handler(Looper.getMainLooper()).post(new Runnable() {
            @Override
            public void run() {
                mController.resumeProvisioning();
                semaphore.release();
            }
        });
        assertTrue("Timeout trying to resume provisioning",
                semaphore.tryAcquire(RESUME_PROVISIONING_TIMEOUT_MS, TimeUnit.MILLISECONDS));
    }
}
