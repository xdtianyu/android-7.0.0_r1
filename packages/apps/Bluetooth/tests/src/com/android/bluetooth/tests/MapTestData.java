package com.android.bluetooth.tests;

import java.io.IOException;
import java.util.Date;

import javax.obex.HeaderSet;
import javax.obex.Operation;

import android.annotation.TargetApi;
import android.content.ContentResolver;
import android.content.ContentValues;
import android.content.Context;
import android.net.Uri;
import android.provider.ContactsContract;
import android.provider.Telephony.Sms;
import android.test.AndroidTestCase;
import android.util.Log;

import com.android.bluetooth.map.BluetoothMapConvoContactElement;
import com.android.bluetooth.map.BluetoothMapConvoListing;
import com.android.bluetooth.map.BluetoothMapConvoListingElement;
import com.android.bluetooth.mapapi.BluetoothMapContract;

/**
 * Class to hold test data - both the server side data to insert into the databases, and the
 * validation data to validate the result, when reading back the data.
 *
 * Should be data only, not operation specific functionality (client).
 *
 * Please try to keep useful functionality call-able from a test case, to make it possible
 * to call a single test case to e.g. inject some contacts or messages into the database.
 *
 */
@TargetApi(20)
public class MapTestData extends AndroidTestCase {
    private static final String TAG = "MapTestData";

    /* Test validation variables */
    static final String TEST_CONTACT_NAME = "Jesus Überboss";
    static final String TEST_CONTACT_PHONE = "55566688";
    static final String TEST_CONTACT_EMAIL = "boss@the.skyes";
    static final int TEST_NUM_CONTACTS = 3;

    static final int TEST_ADD_CONTACT_PER_ITERATIONS = 4;
    /* I do know this function is deprecated, but I'm unable to find a good alternative
     * except from taking a copy of the Date.UTC function as suggested. */
    // NOTE: This will only set the data on the message - not the lastActivity on SMS/MMS threads
    static final long TEST_ACTIVITY_BEGIN = Date.UTC(
            2014-1900,
            8-1, /* month 0-11*/
            22, /*day 1-31 */
            22, /*hour*/
            15, /*minute*/
            20 /*second*/);

    static final String TEST_ACTIVITY_BEGIN_STRING = "20150102T150047";
    static final String TEST_ACTIVITY_END_STRING = "20160102T150047";

    static final int TEST_ACTIVITY_INTERVAL = 5*60*1000; /*ms*/

    static Context sContext = null;
    public static void init(Context context){
        sContext = context;
    }
    /**
     * Adds messages to the SMS message database.
     */
    public static class MapAddSmsMessages implements ISeqStepAction {
        int mCount;
        /**
         *
         * @param count the number of iterations to execute
         */
        public MapAddSmsMessages(int count) {
            mCount = count;
        }

        @Override
        public void execute(SeqStep step, HeaderSet request, Operation op)
                throws IOException {
            int count = mCount; // Number of messages in each conversation
            ContentResolver resolver = sContext.getContentResolver();

            // Insert some messages
            insertTestMessages(resolver, step.index, count);

            // Cleanup if needed to avoid duplicates
            deleteTestContacts(resolver);

            // And now add the contacts
            setupTestContacts(resolver);
        }
    }

    /**
     * TODO: Only works for filter on TEST_CONTACT_NAME
     * @param maxCount
     * @param offset
     * @param filterContact
     * @param read
     * @param reportRead
     * @param msgCount
     * @return
     */
    public static BluetoothMapConvoListing getConvoListingReference(int maxCount, int offset,
            boolean filterContact, boolean read, boolean reportRead, int msgCount){
        BluetoothMapConvoListing list = new BluetoothMapConvoListing();
        BluetoothMapConvoListingElement element;
        BluetoothMapConvoContactElement contact;
        element = new BluetoothMapConvoListingElement();
        element.setRead(read, reportRead);
        element.setVersionCounter(0);
        contact = new BluetoothMapConvoContactElement();
        contact.setName(TEST_CONTACT_NAME);
        contact.setLastActivity(TEST_ACTIVITY_BEGIN +
                msgCount*TEST_ADD_CONTACT_PER_ITERATIONS*TEST_ACTIVITY_INTERVAL);
        element.addContact(contact);
        list.add(element);
        return null;
    }

    public static void insertTestMessages(ContentResolver resolver, int tag, int count) {
        ContentValues values[] = new ContentValues[count*4]; // 4 messages/iteration
        long date = TEST_ACTIVITY_BEGIN;
        Log.i(TAG, "Preparing messages... with data = " + date);

        for (int x = 0;x < count;x++){
            /* NOTE: Update TEST_ADD_CONTACT_PER_ITERATIONS if more messages are added */
            ContentValues item = new ContentValues(5);
            item.put("address", "98765432");
            item.put("body", "test message " + x + " step index: " + tag);
            item.put("date", date+=TEST_ACTIVITY_INTERVAL);
            item.put("read", "0");
            if(x%2 == 0) {
                item.put("type", Sms.MESSAGE_TYPE_INBOX);
            } else {
                item.put("type", Sms.MESSAGE_TYPE_SENT);
            }
            values[x] = item;

            item = new ContentValues(5);
            item.put("address", "23456780");
            item.put("body", "test message " + x + " step index: " + tag);
            item.put("date", date += TEST_ACTIVITY_INTERVAL);
            item.put("read", "0");
            if(x%2 == 0) {
                item.put("type", Sms.MESSAGE_TYPE_INBOX);
            } else {
                item.put("type", Sms.MESSAGE_TYPE_SENT);
            }
            values[count+x] = item;

            item = new ContentValues(5);
            item.put("address", "+4523456780");
            item.put("body", "test message "+x+" step index: " + tag);
            item.put("date", date += TEST_ACTIVITY_INTERVAL);
            item.put("read", "0");
            if(x%2 == 0) {
                item.put("type", Sms.MESSAGE_TYPE_INBOX);
            } else {
                item.put("type", Sms.MESSAGE_TYPE_SENT);
            }
            values[2*count+x] = item;

            /* This is the message used for test */
            item = new ContentValues(5);
            item.put("address", TEST_CONTACT_PHONE);
            item.put("body", "test message "+x+" step index: " + tag);
            item.put("date", date += TEST_ACTIVITY_INTERVAL);
            item.put("read", "0");
            if(x%2 == 0) {
                item.put("type", Sms.MESSAGE_TYPE_INBOX);
            } else {
                item.put("type", Sms.MESSAGE_TYPE_SENT);
            }
            values[3*count+x] = item;
        }

        Log.i(TAG, "Starting bulk insert...");
        resolver.bulkInsert(Uri.parse("content://sms"), values);
        Log.i(TAG, "Bulk insert done.");
    }

    /**
     * Insert a few contacts in the main contact database, using a test account.
     */
    public static void setupTestContacts(ContentResolver resolver){
        /*TEST_NUM_CONTACTS must be updated if this function is changed */
        insertContact(resolver, "Hans Hansen", "98765432", "hans@hansens.global");
        insertContact(resolver, "Helle Børgesen", "23456780", "hb@gmail.com");
        insertContact(resolver, TEST_CONTACT_NAME, TEST_CONTACT_PHONE, TEST_CONTACT_EMAIL);
    }

    /**
     * Helper function to insert a contact
     * @param name
     * @param phone
     * @param email
     */
    private static void insertContact(ContentResolver resolver, String name, String phone, String email) {
        // Get the account info
        //Cursor c = resolver.query(uri, projection, selection, selectionArgs, sortOrder)
        ContentValues item = new ContentValues(3);
        item.put(ContactsContract.RawContacts.ACCOUNT_TYPE, "test_account");
        item.put(ContactsContract.RawContacts.ACCOUNT_NAME, "MAP account");
        Uri uri = resolver.insert(ContactsContract.RawContacts.CONTENT_URI, item);
        Log.i(TAG, "Inserted RawContact: " + uri);
        long rawId = Long.parseLong(uri.getLastPathSegment());

        //Now add contact information
        item = new ContentValues(3);
        item.put(ContactsContract.Data.RAW_CONTACT_ID, rawId);
        item.put(ContactsContract.Data.MIMETYPE,
                ContactsContract.CommonDataKinds.StructuredName.CONTENT_ITEM_TYPE);
        item.put(ContactsContract.CommonDataKinds.StructuredName.DISPLAY_NAME,
                name);
        resolver.insert(ContactsContract.Data.CONTENT_URI, item);

        if(phone != null) {
            item = new ContentValues(3);
            item.put(ContactsContract.Data.RAW_CONTACT_ID, rawId);
            item.put(ContactsContract.Data.MIMETYPE,
                    ContactsContract.CommonDataKinds.Phone.CONTENT_ITEM_TYPE);
            item.put(ContactsContract.CommonDataKinds.Phone.NUMBER, phone);
            resolver.insert(ContactsContract.Data.CONTENT_URI, item);
        }

        if(email != null) {
            item = new ContentValues(3);
            item.put(ContactsContract.Data.RAW_CONTACT_ID, rawId);
            item.put(ContactsContract.Data.MIMETYPE,
                    ContactsContract.CommonDataKinds.Email.CONTENT_ITEM_TYPE);
            item.put(ContactsContract.CommonDataKinds.Email.ADDRESS, email);
            resolver.insert(ContactsContract.Data.CONTENT_URI, item);
        }
    }

    /**
     * Delete all contacts belonging to the test_account.
     */
    public static void deleteTestContacts(ContentResolver resolver){
        resolver.delete(ContactsContract.RawContacts.CONTENT_URI,
                ContactsContract.RawContacts.ACCOUNT_TYPE + "=\"test_account\"", null);
    }

    /****************************************************************************
     * Small test cases to trigger the functionality without running a sequence.
     ****************************************************************************/
    /**
     * Insert a few contacts in the main contact database, using a test account.
     */
    public void testInsertMessages() {
        ContentResolver resolver = mContext.getContentResolver();
        insertTestMessages(resolver, 1234, 10);
    }

    public void testInsert1000Messages() {
        ContentResolver resolver = mContext.getContentResolver();
        insertTestMessages(resolver, 1234, 1000);
    }

    /**
     * Insert a few contacts in the main contact database, using a test account.
     */
    public void testSetupContacts() {
        ContentResolver resolver = mContext.getContentResolver();
        setupTestContacts(resolver);
    }

    /**
     * Delete all contacts belonging to the test_account.
     */
    public void testDeleteTestContacts() {
        ContentResolver resolver = mContext.getContentResolver();
        deleteTestContacts(resolver);
    }

    public void testSetup1000Contacts() {
        ContentResolver resolver = mContext.getContentResolver();
        for(int i = 0; i < 1000; i++) {
            insertContact(resolver, "Hans Hansen " + i,
                    "98765431" + i, "hans" + i + "@hansens.global");
        }
    }

}
