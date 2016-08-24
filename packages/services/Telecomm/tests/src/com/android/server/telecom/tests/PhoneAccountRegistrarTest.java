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

package com.android.server.telecom.tests;

import android.content.ComponentName;
import android.content.Context;
import android.graphics.BitmapFactory;
import android.graphics.Rect;
import android.graphics.drawable.Icon;
import android.net.Uri;
import android.os.Bundle;
import android.os.Parcel;
import android.os.Process;
import android.os.UserHandle;
import android.telecom.PhoneAccount;
import android.telecom.PhoneAccountHandle;
import android.telecom.TelecomManager;
import android.test.suitebuilder.annotation.MediumTest;
import android.util.Xml;

import com.android.internal.telecom.IConnectionService;
import com.android.internal.util.FastXmlSerializer;
import com.android.server.telecom.Log;
import com.android.server.telecom.PhoneAccountRegistrar;
import com.android.server.telecom.PhoneAccountRegistrar.DefaultPhoneAccountHandle;

import org.mockito.Mock;
import org.mockito.Mockito;
import org.mockito.MockitoAnnotations;
import org.xmlpull.v1.XmlPullParser;
import org.xmlpull.v1.XmlSerializer;

import java.io.BufferedInputStream;
import java.io.BufferedOutputStream;
import java.io.ByteArrayInputStream;
import java.io.ByteArrayOutputStream;
import java.io.File;
import java.util.Arrays;
import java.util.Set;

public class PhoneAccountRegistrarTest extends TelecomTestCase {

    private static final int MAX_VERSION = Integer.MAX_VALUE;
    private static final String FILE_NAME = "phone-account-registrar-test-1223.xml";
    private PhoneAccountRegistrar mRegistrar;
    @Mock
    private TelecomManager mTelecomManager;

    @Override
    public void setUp() throws Exception {
        super.setUp();
        MockitoAnnotations.initMocks(this);
        mComponentContextFixture.setTelecomManager(mTelecomManager);
        new File(
                mComponentContextFixture.getTestDouble().getApplicationContext().getFilesDir(),
                FILE_NAME)
                .delete();
        mRegistrar = new PhoneAccountRegistrar(
                mComponentContextFixture.getTestDouble().getApplicationContext(),
                FILE_NAME);
    }

    @Override
    public void tearDown() throws Exception {
        mRegistrar = null;
        new File(
                mComponentContextFixture.getTestDouble().getApplicationContext().getFilesDir(),
                FILE_NAME)
                .delete();
        super.tearDown();
    }

    @MediumTest
    public void testPhoneAccountHandle() throws Exception {
        PhoneAccountHandle input = new PhoneAccountHandle(new ComponentName("pkg0", "cls0"), "id0");
        PhoneAccountHandle result = roundTripXml(this, input,
                PhoneAccountRegistrar.sPhoneAccountHandleXml, mContext);
        assertPhoneAccountHandleEquals(input, result);

        PhoneAccountHandle inputN = new PhoneAccountHandle(new ComponentName("pkg0", "cls0"), null);
        PhoneAccountHandle resultN = roundTripXml(this, inputN,
                PhoneAccountRegistrar.sPhoneAccountHandleXml, mContext);
        Log.i(this, "inputN = %s, resultN = %s", inputN, resultN);
        assertPhoneAccountHandleEquals(inputN, resultN);
    }

    @MediumTest
    public void testPhoneAccount() throws Exception {
        Bundle testBundle = new Bundle();
        testBundle.putInt("EXTRA_INT_1", 1);
        testBundle.putInt("EXTRA_INT_100", 100);
        testBundle.putBoolean("EXTRA_BOOL_TRUE", true);
        testBundle.putBoolean("EXTRA_BOOL_FALSE", false);
        testBundle.putString("EXTRA_STR1", "Hello");
        testBundle.putString("EXTRA_STR2", "There");

        PhoneAccount input = makeQuickAccountBuilder("id0", 0)
                .addSupportedUriScheme(PhoneAccount.SCHEME_TEL)
                .addSupportedUriScheme(PhoneAccount.SCHEME_VOICEMAIL)
                .setExtras(testBundle)
                .setIsEnabled(true)
                .build();
        PhoneAccount result = roundTripXml(this, input, PhoneAccountRegistrar.sPhoneAccountXml,
                mContext);

        assertPhoneAccountEquals(input, result);
    }

    /**
     * Test to ensure non-supported values
     * @throws Exception
     */
    @MediumTest
    public void testPhoneAccountExtrasEdge() throws Exception {
        Bundle testBundle = new Bundle();
        // Ensure null values for string are not persisted.
        testBundle.putString("EXTRA_STR2", null);
        //

        // Ensure unsupported data types are not persisted.
        testBundle.putShort("EXTRA_SHORT", (short) 2);
        testBundle.putByte("EXTRA_BYTE", (byte) 1);
        testBundle.putParcelable("EXTRA_PARC", new Rect(1, 1, 1, 1));
        // Put in something valid so the bundle exists.
        testBundle.putString("EXTRA_OK", "OK");

        PhoneAccount input = makeQuickAccountBuilder("id0", 0)
                .addSupportedUriScheme(PhoneAccount.SCHEME_TEL)
                .addSupportedUriScheme(PhoneAccount.SCHEME_VOICEMAIL)
                .setExtras(testBundle)
                .build();
        PhoneAccount result = roundTripXml(this, input, PhoneAccountRegistrar.sPhoneAccountXml,
                mContext);

        Bundle extras = result.getExtras();
        assertFalse(extras.keySet().contains("EXTRA_STR2"));
        assertFalse(extras.keySet().contains("EXTRA_SHORT"));
        assertFalse(extras.keySet().contains("EXTRA_BYTE"));
        assertFalse(extras.keySet().contains("EXTRA_PARC"));
    }

    @MediumTest
    public void testState() throws Exception {
        PhoneAccountRegistrar.State input = makeQuickState();
        PhoneAccountRegistrar.State result = roundTripXml(this, input,
                PhoneAccountRegistrar.sStateXml,
                mContext);
        assertStateEquals(input, result);
    }

    private void registerAndEnableAccount(PhoneAccount account) {
        mRegistrar.registerPhoneAccount(account);
        mRegistrar.enablePhoneAccount(account.getAccountHandle(), true);
    }

    @MediumTest
    public void testAccounts() throws Exception {
        int i = 0;

        mComponentContextFixture.addConnectionService(
                makeQuickConnectionServiceComponentName(),
                Mockito.mock(IConnectionService.class));

        registerAndEnableAccount(makeQuickAccountBuilder("id" + i, i++)
                .setCapabilities(PhoneAccount.CAPABILITY_CONNECTION_MANAGER
                        | PhoneAccount.CAPABILITY_CALL_PROVIDER)
                .build());
        registerAndEnableAccount(makeQuickAccountBuilder("id" + i, i++)
                .setCapabilities(PhoneAccount.CAPABILITY_CONNECTION_MANAGER
                        | PhoneAccount.CAPABILITY_CALL_PROVIDER)
                .build());
        registerAndEnableAccount(makeQuickAccountBuilder("id" + i, i++)
                .setCapabilities(PhoneAccount.CAPABILITY_CONNECTION_MANAGER
                        | PhoneAccount.CAPABILITY_CALL_PROVIDER)
                .build());
        registerAndEnableAccount(makeQuickAccountBuilder("id" + i, i++)
                .setCapabilities(PhoneAccount.CAPABILITY_CONNECTION_MANAGER)
                .build());

        assertEquals(4, mRegistrar.getAllPhoneAccountsOfCurrentUser().size());
        assertEquals(3, mRegistrar.getCallCapablePhoneAccountsOfCurrentUser(null, false).size());
        assertEquals(null, mRegistrar.getSimCallManagerOfCurrentUser());
        assertEquals(null, mRegistrar.getOutgoingPhoneAccountForSchemeOfCurrentUser(
                PhoneAccount.SCHEME_TEL));
    }

    @MediumTest
    public void testSimCallManager() throws Exception {
        // TODO
    }

    @MediumTest
    public void testDefaultOutgoing() throws Exception {
        mComponentContextFixture.addConnectionService(
                makeQuickConnectionServiceComponentName(),
                Mockito.mock(IConnectionService.class));

        // By default, there is no default outgoing account (nothing has been registered)
        assertNull(
                mRegistrar.getOutgoingPhoneAccountForSchemeOfCurrentUser(PhoneAccount.SCHEME_TEL));

        // Register one tel: account
        PhoneAccountHandle telAccount = makeQuickAccountHandle("tel_acct");
        registerAndEnableAccount(new PhoneAccount.Builder(telAccount, "tel_acct")
                .setCapabilities(PhoneAccount.CAPABILITY_CALL_PROVIDER)
                .addSupportedUriScheme(PhoneAccount.SCHEME_TEL)
                .build());
        PhoneAccountHandle defaultAccount =
                mRegistrar.getOutgoingPhoneAccountForSchemeOfCurrentUser(PhoneAccount.SCHEME_TEL);
        assertEquals(telAccount, defaultAccount);

        // Add a SIP account, make sure tel: doesn't change
        PhoneAccountHandle sipAccount = makeQuickAccountHandle("sip_acct");
        registerAndEnableAccount(new PhoneAccount.Builder(sipAccount, "sip_acct")
                .setCapabilities(PhoneAccount.CAPABILITY_CALL_PROVIDER)
                .addSupportedUriScheme(PhoneAccount.SCHEME_SIP)
                .build());
        defaultAccount = mRegistrar.getOutgoingPhoneAccountForSchemeOfCurrentUser(
                PhoneAccount.SCHEME_SIP);
        assertEquals(sipAccount, defaultAccount);
        defaultAccount = mRegistrar.getOutgoingPhoneAccountForSchemeOfCurrentUser(
                PhoneAccount.SCHEME_TEL);
        assertEquals(telAccount, defaultAccount);

        // Add a connection manager, make sure tel: doesn't change
        PhoneAccountHandle connectionManager = makeQuickAccountHandle("mgr_acct");
        registerAndEnableAccount(new PhoneAccount.Builder(connectionManager, "mgr_acct")
                .setCapabilities(PhoneAccount.CAPABILITY_CONNECTION_MANAGER)
                .addSupportedUriScheme(PhoneAccount.SCHEME_TEL)
                .build());
        defaultAccount = mRegistrar.getOutgoingPhoneAccountForSchemeOfCurrentUser(
                PhoneAccount.SCHEME_TEL);
        assertEquals(telAccount, defaultAccount);

        // Unregister the tel: account, make sure there is no tel: default now.
        mRegistrar.unregisterPhoneAccount(telAccount);
        assertNull(
                mRegistrar.getOutgoingPhoneAccountForSchemeOfCurrentUser(PhoneAccount.SCHEME_TEL));
    }

    @MediumTest
    public void testPhoneAccountParceling() throws Exception {
        PhoneAccountHandle handle = makeQuickAccountHandle("foo");
        roundTripPhoneAccount(new PhoneAccount.Builder(handle, null).build());
        roundTripPhoneAccount(new PhoneAccount.Builder(handle, "foo").build());
        roundTripPhoneAccount(
                new PhoneAccount.Builder(handle, "foo")
                        .setAddress(Uri.parse("tel:123456"))
                        .setCapabilities(23)
                        .setHighlightColor(0xf0f0f0)
                        .setIcon(Icon.createWithResource(
                                "com.android.server.telecom.tests", R.drawable.stat_sys_phone_call))
                        // TODO: set icon tint (0xfefefe)
                        .setShortDescription("short description")
                        .setSubscriptionAddress(Uri.parse("tel:2345678"))
                        .setSupportedUriSchemes(Arrays.asList("tel", "sip"))
                        .build());
        roundTripPhoneAccount(
                new PhoneAccount.Builder(handle, "foo")
                        .setAddress(Uri.parse("tel:123456"))
                        .setCapabilities(23)
                        .setHighlightColor(0xf0f0f0)
                        .setIcon(Icon.createWithBitmap(
                                BitmapFactory.decodeResource(
                                        getContext().getResources(),
                                        R.drawable.stat_sys_phone_call)))
                        .setShortDescription("short description")
                        .setSubscriptionAddress(Uri.parse("tel:2345678"))
                        .setSupportedUriSchemes(Arrays.asList("tel", "sip"))
                        .build());
    }

    private static ComponentName makeQuickConnectionServiceComponentName() {
        return new ComponentName(
                "com.android.server.telecom.tests",
                "com.android.server.telecom.tests.MockConnectionService");
    }

    private static PhoneAccountHandle makeQuickAccountHandle(String id) {
        return new PhoneAccountHandle(
                makeQuickConnectionServiceComponentName(),
                id,
                Process.myUserHandle());
    }

    private PhoneAccount.Builder makeQuickAccountBuilder(String id, int idx) {
        return new PhoneAccount.Builder(
                makeQuickAccountHandle(id),
                "label" + idx);
    }

    private PhoneAccount makeQuickAccount(String id, int idx) {
        return makeQuickAccountBuilder(id, idx)
                .setAddress(Uri.parse("http://foo.com/" + idx))
                .setSubscriptionAddress(Uri.parse("tel:555-000" + idx))
                .setCapabilities(idx)
                .setIcon(Icon.createWithResource(
                            "com.android.server.telecom.tests", R.drawable.stat_sys_phone_call))
                .setShortDescription("desc" + idx)
                .setIsEnabled(true)
                .build();
    }

    private static void roundTripPhoneAccount(PhoneAccount original) throws Exception {
        PhoneAccount copy = null;

        {
            Parcel parcel = Parcel.obtain();
            parcel.writeParcelable(original, 0);
            parcel.setDataPosition(0);
            copy = parcel.readParcelable(PhoneAccountRegistrarTest.class.getClassLoader());
            parcel.recycle();
        }

        assertPhoneAccountEquals(original, copy);
    }

    private static <T> T roundTripXml(
            Object self,
            T input,
            PhoneAccountRegistrar.XmlSerialization<T> xml,
            Context context)
            throws Exception {
        Log.d(self, "Input = %s", input);

        byte[] data;
        {
            XmlSerializer serializer = new FastXmlSerializer();
            ByteArrayOutputStream baos = new ByteArrayOutputStream();
            serializer.setOutput(new BufferedOutputStream(baos), "utf-8");
            xml.writeToXml(input, serializer, context);
            serializer.flush();
            data = baos.toByteArray();
        }

        Log.i(self, "====== XML data ======\n%s", new String(data));

        T result = null;
        {
            XmlPullParser parser = Xml.newPullParser();
            parser.setInput(new BufferedInputStream(new ByteArrayInputStream(data)), null);
            parser.nextTag();
            result = xml.readFromXml(parser, MAX_VERSION, context);
        }

        Log.i(self, "result = " + result);

        return result;
    }

    private static void assertPhoneAccountHandleEquals(PhoneAccountHandle a, PhoneAccountHandle b) {
        if (a != b) {
            assertEquals(
                    a.getComponentName().getPackageName(),
                    b.getComponentName().getPackageName());
            assertEquals(
                    a.getComponentName().getClassName(),
                    b.getComponentName().getClassName());
            assertEquals(a.getId(), b.getId());
        }
    }

    private static void assertIconEquals(Icon a, Icon b) {
        if (a != b) {
            if (a != null && b != null) {
                assertEquals(a.toString(), b.toString());
            } else {
                fail("Icons not equal: " + a + ", " + b);
            }
        }
    }

    private static void assertDefaultPhoneAccountHandleEquals(DefaultPhoneAccountHandle a,
            DefaultPhoneAccountHandle b) {
        if (a != b) {
            if (a!= null && b != null) {
                assertEquals(a.userHandle, b.userHandle);
                assertPhoneAccountHandleEquals(a.phoneAccountHandle, b.phoneAccountHandle);
            } else {
                fail("Default phone account handles are not equal: " + a + ", " + b);
            }
        }
    }

    private static void assertPhoneAccountEquals(PhoneAccount a, PhoneAccount b) {
        if (a != b) {
            if (a != null && b != null) {
                assertPhoneAccountHandleEquals(a.getAccountHandle(), b.getAccountHandle());
                assertEquals(a.getAddress(), b.getAddress());
                assertEquals(a.getSubscriptionAddress(), b.getSubscriptionAddress());
                assertEquals(a.getCapabilities(), b.getCapabilities());
                assertIconEquals(a.getIcon(), b.getIcon());
                assertEquals(a.getHighlightColor(), b.getHighlightColor());
                assertEquals(a.getLabel(), b.getLabel());
                assertEquals(a.getShortDescription(), b.getShortDescription());
                assertEquals(a.getSupportedUriSchemes(), b.getSupportedUriSchemes());
                assertBundlesEqual(a.getExtras(), b.getExtras());
                assertEquals(a.isEnabled(), b.isEnabled());
            } else {
                fail("Phone accounts not equal: " + a + ", " + b);
            }
        }
    }

    private static void assertBundlesEqual(Bundle a, Bundle b) {
        if (a == null && b == null) {
            return;
        }

        assertNotNull(a);
        assertNotNull(b);
        Set<String> keySetA = a.keySet();
        Set<String> keySetB = b.keySet();

        assertTrue("Bundle keys not the same", keySetA.containsAll(keySetB));
        assertTrue("Bundle keys not the same", keySetB.containsAll(keySetA));

        for (String keyA : keySetA) {
            assertEquals("Bundle value not the same", a.get(keyA), b.get(keyA));
        }
    }

    private static void assertStateEquals(
            PhoneAccountRegistrar.State a, PhoneAccountRegistrar.State b) {
        assertEquals(a.defaultOutgoingAccountHandles.size(),
                b.defaultOutgoingAccountHandles.size());
        for (int i = 0; i < a.defaultOutgoingAccountHandles.size(); i++) {
            assertDefaultPhoneAccountHandleEquals(a.defaultOutgoingAccountHandles.get(i),
                    b.defaultOutgoingAccountHandles.get(i));
        }
        assertEquals(a.accounts.size(), b.accounts.size());
        for (int i = 0; i < a.accounts.size(); i++) {
            assertPhoneAccountEquals(a.accounts.get(i), b.accounts.get(i));
        }
    }

    private PhoneAccountRegistrar.State makeQuickState() {
        PhoneAccountRegistrar.State s = new PhoneAccountRegistrar.State();
        s.accounts.add(makeQuickAccount("id0", 0));
        s.accounts.add(makeQuickAccount("id1", 1));
        s.accounts.add(makeQuickAccount("id2", 2));
        PhoneAccountHandle phoneAccountHandle = new PhoneAccountHandle(
                new ComponentName("pkg0", "cls0"), "id0");
        UserHandle userHandle = phoneAccountHandle.getUserHandle();
        s.defaultOutgoingAccountHandles
                .put(userHandle, new DefaultPhoneAccountHandle(userHandle, phoneAccountHandle));
        return s;
    }
}
