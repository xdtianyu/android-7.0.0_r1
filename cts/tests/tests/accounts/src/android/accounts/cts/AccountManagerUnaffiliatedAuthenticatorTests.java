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

package android.accounts.cts;

import android.accounts.Account;
import android.accounts.AccountManager;
import android.accounts.AccountManagerFuture;
import android.accounts.AuthenticatorException;
import android.accounts.OperationCanceledException;
import android.accounts.cts.common.AuthenticatorContentProvider;
import android.accounts.cts.common.Fixtures;
import android.content.ContentProviderClient;
import android.content.ContentResolver;
import android.os.RemoteException;
import android.test.AndroidTestCase;

import java.io.IOException;

/**
 * Tests for AccountManager and AbstractAccountAuthenticator related behavior using {@link
 * android.accounts.cts.common.TestAccountAuthenticator} instances signed with different keys than
 * the caller. This is important to test that portion of the {@link AccountManager} API intended
 * for {@link android.accounts.AbstractAccountAuthenticator} implementers.
 * <p>
 * You can run those unit tests with the following command line:
 * <p>
 *  adb shell am instrument
 *   -e debug false -w
 *   -e class android.accounts.cts.AccountManagerUnaffiliatedAuthenticatorTests
 * android.accounts.cts/android.support.test.runner.AndroidJUnitRunner
 */
public class AccountManagerUnaffiliatedAuthenticatorTests extends AndroidTestCase {

    private AccountManager mAccountManager;
    private ContentProviderClient mProviderClient;

    @Override
    public void setUp() throws Exception {
        // bind to the diagnostic service and set it up.
        mAccountManager = AccountManager.get(getContext());
        ContentResolver resolver = getContext().getContentResolver();
        mProviderClient = resolver.acquireContentProviderClient(
                AuthenticatorContentProvider.AUTHORITY);
        /*
         * This will install a bunch of accounts on the device
         * (see Fixtures.getFixtureAccountNames()).
         */
        mProviderClient.call(AuthenticatorContentProvider.METHOD_SETUP, null, null);
    }

    @Override
    public void tearDown() throws RemoteException {
        try {
            mProviderClient.call(AuthenticatorContentProvider.METHOD_TEARDOWN, null, null);
        } finally {
            mProviderClient.release();
        }
    }

    public void testNotifyAccountAuthenticated() {
        try {
            mAccountManager.notifyAccountAuthenticated(
                    Fixtures.ACCOUNT_UNAFFILIATED_FIXTURE_SUCCESS);
            fail("Expected to just barf if the caller doesn't share a signature.");
        } catch (SecurityException expected) {}
    }

    public void testEditProperties()  {
        try {
            mAccountManager.editProperties(
                    Fixtures.TYPE_STANDARD_UNAFFILIATED,
                    null, // activity
                    null, // callback
                    null); // handler
            fail("Expecting a OperationCanceledException.");
        } catch (SecurityException expected) {
            
        }
    }

    public void testAddAccountExplicitly() {
        try {
            mAccountManager.addAccountExplicitly(
                    Fixtures.ACCOUNT_UNAFFILIATED_FIXTURE_SUCCESS,
                    "shouldn't matter", // password
                    null); // bundle
            fail("addAccountExplicitly should just barf if the caller isn't permitted.");
        } catch (SecurityException expected) {}
    }

    public void testRemoveAccount_withBooleanResult() {
        try {
            mAccountManager.removeAccount(
                    Fixtures.ACCOUNT_UNAFFILIATED_FIXTURE_SUCCESS,
                    null,
                    null);
            fail("removeAccount should just barf if the caller isn't permitted.");
        } catch (SecurityException expected) {}
    }

    public void testRemoveAccount_withBundleResult() {
        try {
            mAccountManager.removeAccount(
                    Fixtures.ACCOUNT_UNAFFILIATED_FIXTURE_SUCCESS,
                    null, // Activity
                    null,
                    null);
            fail("removeAccount should just barf if the caller isn't permitted.");
        } catch (SecurityException expected) {}
    }

    public void testRemoveAccountExplicitly() {
        try {
            mAccountManager.removeAccountExplicitly(
                    Fixtures.ACCOUNT_UNAFFILIATED_FIXTURE_SUCCESS);
            fail("removeAccountExplicitly should just barf if the caller isn't permitted.");
        } catch (SecurityException expected) {}
    }

    public void testGetPassword() {
        try {
            mAccountManager.getPassword(
                    Fixtures.ACCOUNT_UNAFFILIATED_FIXTURE_SUCCESS);
            fail("getPassword should just barf if the caller isn't permitted.");
        } catch (SecurityException expected) {}
    }

    public void testSetPassword() {
        try {
            mAccountManager.setPassword(
                    Fixtures.ACCOUNT_UNAFFILIATED_FIXTURE_SUCCESS,
                    "Doesn't matter");
            fail("setPassword should just barf if the caller isn't permitted.");
        } catch (SecurityException expected) {}
    }

    public void testClearPassword() {
        try {
            mAccountManager.clearPassword(
                    Fixtures.ACCOUNT_UNAFFILIATED_FIXTURE_SUCCESS);
            fail("clearPassword should just barf if the caller isn't permitted.");
        } catch (SecurityException expected) {}
    }

    public void testGetUserData() {
        try {
            mAccountManager.getUserData(
                    Fixtures.ACCOUNT_UNAFFILIATED_FIXTURE_SUCCESS,
                    "key");
            fail("getUserData should just barf if the caller isn't permitted.");
        } catch (SecurityException expected) {}
    }

    public void testSetUserData() {
        try {
            mAccountManager.setUserData(
                    Fixtures.ACCOUNT_UNAFFILIATED_FIXTURE_SUCCESS,
                    "key",
                    "value");
            fail("setUserData should just barf if the caller isn't permitted.");
        } catch (SecurityException expected) {}
    }

    public void setAuthToken() {
        try {
            mAccountManager.setAuthToken(
                    Fixtures.ACCOUNT_UNAFFILIATED_FIXTURE_SUCCESS,
                    "tokenType",
                    "token");
            fail("setAuthToken should just barf if the caller isn't permitted.");
        } catch (SecurityException expected) {}
    }

    public void testPeekAuthToken() {
        try {
            mAccountManager.peekAuthToken(
                    Fixtures.ACCOUNT_UNAFFILIATED_FIXTURE_SUCCESS,
                    "tokenType");
            fail("peekAuthToken should just barf if the caller isn't permitted.");
        } catch (SecurityException expected) {}
    }

    public void testGetAccounts() {
        Account[] accounts = mAccountManager.getAccounts();
        assertEquals(0, accounts.length);
    }

    public void testGetAccountsByType() {
        Account[] accounts = mAccountManager.getAccountsByType(
                Fixtures.TYPE_STANDARD_UNAFFILIATED);
        assertEquals(0, accounts.length);
    }

    public void testGetAccountsByTypeAndFeatures()
            throws OperationCanceledException, AuthenticatorException, IOException {
        AccountManagerFuture<Account[]> future = mAccountManager.getAccountsByTypeAndFeatures(
                Fixtures.TYPE_STANDARD_UNAFFILIATED,
                new String[] { "doesn't matter" },
                null,  // Callback
                null);  // Handler
        Account[] accounts = future.getResult();
        assertEquals(0, accounts.length);
    }

    public void testGetAccountsByTypeForPackage() {
        Account[] accounts = mAccountManager.getAccountsByTypeForPackage(
                Fixtures.TYPE_STANDARD_UNAFFILIATED,
                getContext().getPackageName());
        assertEquals(0, accounts.length);
    }
}

