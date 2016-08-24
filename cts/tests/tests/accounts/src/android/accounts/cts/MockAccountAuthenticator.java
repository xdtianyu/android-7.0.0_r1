/*
 * Copyright (C) 2009 The Android Open Source Project
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

import android.accounts.AbstractAccountAuthenticator;
import android.accounts.Account;
import android.accounts.AccountAuthenticatorResponse;
import android.accounts.AccountManager;
import android.accounts.NetworkErrorException;
import android.content.Context;
import android.content.Intent;
import android.os.Bundle;
import android.util.Log;

import java.util.ArrayList;
import java.util.concurrent.atomic.AtomicBoolean;
import java.util.concurrent.atomic.AtomicInteger;

/**
 * A simple Mock Account Authenticator
 */
public class MockAccountAuthenticator extends AbstractAccountAuthenticator {
    private static String TAG = "AccountManagerTest";

    public static String KEY_ACCOUNT_INFO = "key_account_info";
    public static String KEY_ACCOUNT_AUTHENTICATOR_RESPONSE = "key_account_authenticator_response";
    public static String ACCOUNT_NAME_FOR_NEW_REMOVE_API = "call new removeAccount api";
    public static String ACCOUNT_NAME_FOR_DEFAULT_IMPL = "call super api";
    // Key for triggering return intent flow
    public static String KEY_RETURN_INTENT = "return an intent";
    public static String ACCOUNT_NAME_FOR_NEW_REMOVE_API1 = "call new removeAccount api";

    private final Context mContext;
    private final AtomicInteger mTokenCounter  = new AtomicInteger(0);
    private final AtomicBoolean mIsRecentlyCalled = new AtomicBoolean(false);

    AccountAuthenticatorResponse mResponse;
    String mAccountType;
    String mAuthTokenType;
    String[] mRequiredFeatures;
    public Bundle mOptionsUpdateCredentials;
    public Bundle mOptionsConfirmCredentials;
    public Bundle mOptionsAddAccount;
    public Bundle mOptionsGetAuthToken;
    Account mAccount;
    String[] mFeatures;

    final ArrayList<String> mockFeatureList = new ArrayList<String>();
    private final long mTokenDurationMillis = 1000; // 1 second

    public MockAccountAuthenticator(Context context) {
        super(context);
        mContext = context;

        // Create some mock features
        mockFeatureList.add(AccountManagerTest.FEATURE_1);
        mockFeatureList.add(AccountManagerTest.FEATURE_2);
    }

    public long getTokenDurationMillis() {
        return mTokenDurationMillis;
    }

    public boolean isRecentlyCalled() {
        return mIsRecentlyCalled.getAndSet(false);
    }

    public String getLastTokenServed() {
        return Integer.toString(mTokenCounter.get());
    }

    public AccountAuthenticatorResponse getResponse() {
        return mResponse;
    }

    public String getAccountType() {
        return mAccountType;
    }

    public String getAuthTokenType() {
        return mAuthTokenType;
    }

    public String[] getRequiredFeatures() {
        return mRequiredFeatures;
    }

    public Account getAccount() {
        return mAccount;
    }

    public String[] getFeatures() {
        return mFeatures;
    }

    public void clearData() {
        mResponse = null;
        mAccountType = null;
        mAuthTokenType = null;
        mRequiredFeatures = null;
        mOptionsUpdateCredentials = null;
        mOptionsAddAccount = null;
        mOptionsGetAuthToken = null;
        mOptionsConfirmCredentials = null;
        mAccount = null;
        mFeatures = null;
    }

    public void callAccountAuthenticated() {
        AccountManager am = AccountManager.get(mContext);
        am.notifyAccountAuthenticated(mAccount);
    }

    public void callSetPassword() {
        AccountManager am = AccountManager.get(mContext);
        am.setPassword(mAccount, "password");
    }

    private Bundle createResultBundle() {
        Bundle result = new Bundle();
        result.putString(AccountManager.KEY_ACCOUNT_NAME, AccountManagerTest.ACCOUNT_NAME);
        result.putString(AccountManager.KEY_ACCOUNT_TYPE, AccountManagerTest.ACCOUNT_TYPE);
        result.putString(
                AccountManager.KEY_AUTHTOKEN,
                Integer.toString(mTokenCounter.incrementAndGet()));
        return result;
    }

    /**
     * Adds an account of the specified accountType.
     */
    @Override
    public Bundle addAccount(AccountAuthenticatorResponse response, String accountType,
            String authTokenType, String[] requiredFeatures, Bundle options)
            throws NetworkErrorException {
        this.mResponse = response;
        this.mAccountType = accountType;
        this.mAuthTokenType = authTokenType;
        this.mRequiredFeatures = requiredFeatures;
        this.mOptionsAddAccount = options;
        AccountManager am = AccountManager.get(mContext);
        am.addAccountExplicitly(AccountManagerTest.ACCOUNT, "fakePassword", null);
        return createResultBundle();
    }

    /**
     * Update the locally stored credentials for an account.
     */
    @Override
    public Bundle updateCredentials(AccountAuthenticatorResponse response, Account account,
            String authTokenType, Bundle options) throws NetworkErrorException {
        this.mResponse = response;
        this.mAccount = account;
        this.mAuthTokenType = authTokenType;
        this.mOptionsUpdateCredentials = options;
        return createResultBundle();
    }

    /**
     * Returns a Bundle that contains the Intent of the activity that can be used to edit the
     * properties. In order to indicate success the activity should call response.setResult()
     * with a non-null Bundle.
     */
    @Override
    public Bundle editProperties(AccountAuthenticatorResponse response, String accountType) {
        this.mResponse = response;
        this.mAccountType = accountType;
        return createResultBundle();
    }

    /**
     * Checks that the user knows the credentials of an account.
     */
    @Override
    public Bundle confirmCredentials(AccountAuthenticatorResponse response, Account account,
            Bundle options) throws NetworkErrorException {
        this.mResponse = response;
        this.mAccount = account;
        this.mOptionsConfirmCredentials = options;
        Bundle result = new Bundle();
        if (options.containsKey(KEY_RETURN_INTENT)) {
            Intent intent = new Intent();
            intent.setClassName("android.accounts.cts", "android.accounts.cts.AccountDummyActivity");
            result.putParcelable(AccountManager.KEY_INTENT, intent);
        } else {
            result.putBoolean(AccountManager.KEY_BOOLEAN_RESULT, true);
        }

        return result;
    }

    /**
     * Gets the authtoken for an account.
     */
    @Override
    public Bundle getAuthToken(
            AccountAuthenticatorResponse response,
            Account account,
            String authTokenType,
            Bundle options) throws NetworkErrorException {
        Log.w(TAG, "MockAuth - getAuthToken@" + System.currentTimeMillis());
        mIsRecentlyCalled.set(true);
        this.mResponse = response;
        this.mAccount = account;
        this.mAuthTokenType = authTokenType;
        this.mOptionsGetAuthToken = options;
        Bundle result = new Bundle();

        result.putString(AccountManager.KEY_ACCOUNT_NAME, account.name);
        result.putString(AccountManager.KEY_ACCOUNT_TYPE, account.type);
        String token;
        if (AccountManagerTest.AUTH_EXPIRING_TOKEN_TYPE.equals(authTokenType)) {
            /*
             * The resultant token should simply be the expiration timestamp. E.g. the time after
             * which getting a new AUTH_EXPIRING_TOKEN_TYPE typed token will return a different
             * value.
             */
            long expiry = System.currentTimeMillis() + mTokenDurationMillis;
            result.putLong(AbstractAccountAuthenticator.KEY_CUSTOM_TOKEN_EXPIRY, expiry);
        }
        result.putString(
                AccountManager.KEY_AUTHTOKEN,
                Integer.toString(mTokenCounter.incrementAndGet()));
        return result;
    }

    /**
     * Ask the authenticator for a localized label for the given authTokenType.
     */
    @Override
    public String getAuthTokenLabel(String authTokenType) {
        this.mAuthTokenType = authTokenType;
        return AccountManagerTest.AUTH_TOKEN_LABEL;
    }

    /**
     * Checks if the account supports all the specified authenticator specific features.
     */
    @Override
    public Bundle hasFeatures(AccountAuthenticatorResponse response, Account account,
            String[] features) throws NetworkErrorException {

        this.mResponse = response;
        this.mAccount = account;
        this.mFeatures = features;

        Bundle result = new Bundle();
        if (null == features) {
            result.putBoolean(AccountManager.KEY_BOOLEAN_RESULT, true);
        }
        else {
            boolean booleanResult = true;
            for (String feature: features) {
                if (!mockFeatureList.contains(feature)) {
                    booleanResult = false;
                    break;
                }
            }
            result.putBoolean(AccountManager.KEY_BOOLEAN_RESULT, booleanResult);
        }
        return result;
    }

    @Override
    public Bundle getAccountRemovalAllowed(AccountAuthenticatorResponse response,
            Account account) throws NetworkErrorException {
        final Bundle result = new Bundle();
        if (ACCOUNT_NAME_FOR_NEW_REMOVE_API.equals(account.name)) {
            Intent intent = AccountRemovalDummyActivity.createIntent(mContext);
            // Pass in the authenticator response, so that account removal can
            // be
            // completed
            intent.putExtra(KEY_ACCOUNT_AUTHENTICATOR_RESPONSE, response);
            intent.putExtra(KEY_ACCOUNT_INFO, account);
            result.putParcelable(AccountManager.KEY_INTENT, intent);
            // Adding this following line to reject account installation
            // requests
            // coming from old removeAccount API.
            result.putBoolean(AccountManager.KEY_BOOLEAN_RESULT, false);
        } else if (ACCOUNT_NAME_FOR_DEFAULT_IMPL.equals(account.name)) {
            return super.getAccountRemovalAllowed(response, account);
        } else {
            result.putBoolean(AccountManager.KEY_BOOLEAN_RESULT, true);
        }
        return result;
    }

    @Override
    public Bundle addAccountFromCredentials(final AccountAuthenticatorResponse response,
            Account account,
            Bundle accountCredentials) throws NetworkErrorException {
        return super.addAccountFromCredentials(response, account, accountCredentials);
    }

    @Override
    public Bundle getAccountCredentialsForCloning(final AccountAuthenticatorResponse response,
            final Account account) throws NetworkErrorException {
        return super.getAccountCredentialsForCloning(response, account);
    }

}
