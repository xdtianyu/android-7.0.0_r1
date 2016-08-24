/*
 * Copyright (C) 2010 The Android Open Source Project
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

package android.provider.cts;

import android.content.ContentProviderClient;
import android.content.ContentResolver;
import android.content.ContentUris;
import android.content.ContentValues;
import android.content.Context;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.content.pm.ResolveInfo;
import android.net.Uri;
import android.os.SystemClock;
import android.provider.ContactsContract;
import android.provider.ContactsContract.CommonDataKinds.StructuredName;
import android.provider.ContactsContract.Contacts;
import android.provider.cts.ContactsContract_TestDataBuilder.TestContact;
import android.provider.cts.ContactsContract_TestDataBuilder.TestRawContact;
import android.provider.cts.contacts.ContactUtil;
import android.provider.cts.contacts.DatabaseAsserts;
import android.provider.cts.contacts.RawContactUtil;
import android.provider.cts.contacts.account.StaticAccountAuthenticator;
import android.test.AndroidTestCase;

import java.util.List;

public class ContactsContract_ContactsTest extends AndroidTestCase {

    private StaticAccountAuthenticator mAuthenticator;
    private ContentResolver mResolver;
    private ContactsContract_TestDataBuilder mBuilder;

    @Override
    protected void setUp() throws Exception {
        super.setUp();
        mResolver = getContext().getContentResolver();
        ContentProviderClient provider =
                mResolver.acquireContentProviderClient(ContactsContract.AUTHORITY);
        mBuilder = new ContactsContract_TestDataBuilder(provider);

        mAuthenticator = new StaticAccountAuthenticator(getContext());
    }

    @Override
    protected void tearDown() throws Exception {
        super.tearDown();
        mBuilder.cleanup();
    }

    public void testMarkAsContacted() throws Exception {
        TestRawContact rawContact = mBuilder.newRawContact().insert().load();
        TestContact contact = rawContact.getContact().load();
        long oldLastContacted = contact.getLong(Contacts.LAST_TIME_CONTACTED);

        Contacts.markAsContacted(mResolver, contact.getId());
        contact.load(); // Reload

        long lastContacted = contact.getLong(Contacts.LAST_TIME_CONTACTED);
        assertTrue(oldLastContacted < lastContacted);
        oldLastContacted = lastContacted;

        Contacts.markAsContacted(mResolver, contact.getId());
        contact.load();

        lastContacted = contact.getLong(Contacts.LAST_TIME_CONTACTED);
        assertTrue(oldLastContacted < lastContacted);
    }

    public void testContentUri() {
        Context context = getContext();
        PackageManager packageManager = context.getPackageManager();
        Intent intent = new Intent(Intent.ACTION_VIEW, ContactsContract.Contacts.CONTENT_URI);
        List<ResolveInfo> resolveInfos = packageManager.queryIntentActivities(intent, 0);
        assertFalse("Device does not support the activity intent: " + intent,
                resolveInfos.isEmpty());
    }

    public void testLookupUri() throws Exception {
        TestRawContact rawContact = mBuilder.newRawContact().insert().load();
        TestContact contact = rawContact.getContact().load();

        Uri contactUri = contact.getUri();
        long contactId = contact.getId();
        String lookupKey = contact.getString(Contacts.LOOKUP_KEY);

        Uri lookupUri = Contacts.getLookupUri(contactId, lookupKey);
        assertEquals(ContentUris.withAppendedId(Uri.withAppendedPath(Contacts.CONTENT_LOOKUP_URI,
                lookupKey), contactId), lookupUri);

        Uri nullLookupUri = Contacts.getLookupUri(contactId, null);
        assertNull(nullLookupUri);

        Uri emptyLookupUri = Contacts.getLookupUri(contactId, "");
        assertNull(emptyLookupUri);

        Uri lookupUri2 = Contacts.getLookupUri(mResolver, contactUri);
        assertEquals(lookupUri, lookupUri2);

        Uri contactUri2 = Contacts.lookupContact(mResolver, lookupUri);
        assertEquals(contactUri, contactUri2);
    }

    public void testInsert_isUnsupported() {
        DatabaseAsserts.assertInsertIsUnsupported(mResolver, Contacts.CONTENT_URI);
    }

    public void testContactDelete_removesContactRecord() {
        assertContactCreateDelete();
    }

    public void testContactDelete_hasDeleteLog() {
        long start = System.currentTimeMillis();
        DatabaseAsserts.ContactIdPair ids = assertContactCreateDelete();
        DatabaseAsserts.assertHasDeleteLogGreaterThan(mResolver, ids.mContactId, start);

        // Clean up. Must also remove raw contact.
        RawContactUtil.delete(mResolver, ids.mRawContactId, true);
    }

    public void testContactDelete_marksRawContactsForDeletion() {
        DatabaseAsserts.ContactIdPair ids = assertContactCreateDelete();

        String[] projection = new String[] {
                ContactsContract.RawContacts.DIRTY,
                ContactsContract.RawContacts.DELETED
        };
        List<String[]> records = RawContactUtil.queryByContactId(mResolver, ids.mContactId,
                projection);
        for (String[] arr : records) {
            assertEquals("1", arr[0]);
            assertEquals("1", arr[1]);
        }

        // Clean up
        RawContactUtil.delete(mResolver, ids.mRawContactId, true);
    }

    public void testContactUpdate_updatesContactUpdatedTimestamp() {
        DatabaseAsserts.ContactIdPair ids = DatabaseAsserts.assertAndCreateContact(mResolver);

        long baseTime = ContactUtil.queryContactLastUpdatedTimestamp(mResolver, ids.mContactId);

        ContentValues values = new ContentValues();
        values.put(ContactsContract.Contacts.STARRED, 1);

        SystemClock.sleep(1);
        ContactUtil.update(mResolver, ids.mContactId, values);

        long newTime = ContactUtil.queryContactLastUpdatedTimestamp(mResolver, ids.mContactId);
        assertTrue(newTime > baseTime);

        // Clean up
        RawContactUtil.delete(mResolver, ids.mRawContactId, true);
    }

    public void testProjection() throws Exception {
        final TestRawContact rawContact = mBuilder.newRawContact().insert().load();
        rawContact.newDataRow(StructuredName.CONTENT_ITEM_TYPE)
                .with(StructuredName.GIVEN_NAME, "xxx")
                .insert();

        final TestContact contact = rawContact.getContact().load();
        final long contactId = contact.getId();
        final String lookupKey = contact.getString(Contacts.LOOKUP_KEY);

        final String[] PROJECTION = new String[]{
                Contacts._ID,
                Contacts.DISPLAY_NAME,
                Contacts.DISPLAY_NAME_PRIMARY,
                Contacts.DISPLAY_NAME_ALTERNATIVE,
                Contacts.DISPLAY_NAME_SOURCE,
                Contacts.PHONETIC_NAME,
                Contacts.PHONETIC_NAME_STYLE,
                Contacts.SORT_KEY_PRIMARY,
                Contacts.SORT_KEY_ALTERNATIVE,
                Contacts.LAST_TIME_CONTACTED,
                Contacts.TIMES_CONTACTED,
                Contacts.STARRED,
                Contacts.PINNED,
                Contacts.IN_DEFAULT_DIRECTORY,
                Contacts.IN_VISIBLE_GROUP,
                Contacts.PHOTO_ID,
                Contacts.PHOTO_FILE_ID,
                Contacts.PHOTO_URI,
                Contacts.PHOTO_THUMBNAIL_URI,
                Contacts.CUSTOM_RINGTONE,
                Contacts.HAS_PHONE_NUMBER,
                Contacts.SEND_TO_VOICEMAIL,
                Contacts.IS_USER_PROFILE,
                Contacts.LOOKUP_KEY,
                Contacts.NAME_RAW_CONTACT_ID,
                Contacts.CONTACT_PRESENCE,
                Contacts.CONTACT_CHAT_CAPABILITY,
                Contacts.CONTACT_STATUS,
                Contacts.CONTACT_STATUS_TIMESTAMP,
                Contacts.CONTACT_STATUS_RES_PACKAGE,
                Contacts.CONTACT_STATUS_LABEL,
                Contacts.CONTACT_STATUS_ICON,
                Contacts.CONTACT_LAST_UPDATED_TIMESTAMP
        };

        // Contacts.CONTENT_URI
        DatabaseAsserts.checkProjection(mResolver,
                Contacts.CONTENT_URI,
                PROJECTION,
                new long[]{contact.getId()}
        );

        // Contacts.CONTENT_FILTER_URI
        DatabaseAsserts.checkProjection(mResolver,
                Contacts.CONTENT_FILTER_URI.buildUpon().appendEncodedPath("xxx").build(),
                PROJECTION,
                new long[]{contact.getId()}
        );

        // Contacts.CONTENT_LOOKUP_URI
        DatabaseAsserts.checkProjection(mResolver,
                Contacts.getLookupUri(contactId, lookupKey),
                PROJECTION,
                new long[]{contact.getId()}
        );
    }

    /**
     * Create a contact.  Delete it.  And assert that the contact record is no longer present.
     *
     * @return The contact id and raw contact id that was created.
     */
    private DatabaseAsserts.ContactIdPair assertContactCreateDelete() {
        DatabaseAsserts.ContactIdPair ids = DatabaseAsserts.assertAndCreateContact(mResolver);

        SystemClock.sleep(1);
        ContactUtil.delete(mResolver, ids.mContactId);

        assertFalse(ContactUtil.recordExistsForContactId(mResolver, ids.mContactId));

        return ids;
    }
}
