package com.android.bluetooth.pbapclient;

import com.android.vcard.VCardEntry;

import android.accounts.Account;
import com.android.bluetooth.pbapclient.BluetoothPbapClient;
import android.content.ContentProviderOperation;
import android.content.Context;
import android.content.OperationApplicationException;
import android.provider.ContactsContract;
import android.database.Cursor;
import android.net.Uri;
import android.os.Bundle;
import android.os.RemoteException;
import android.provider.ContactsContract.Data;
import android.provider.ContactsContract.RawContacts;
import android.provider.ContactsContract.RawContactsEntity;
import android.provider.ContactsContract.Contacts.Entity;
import android.provider.ContactsContract.CommonDataKinds.Phone;
import android.provider.ContactsContract.CommonDataKinds.StructuredName;
import android.util.Log;

import com.android.vcard.VCardEntry;

import java.lang.InterruptedException;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;

public class PhonebookPullRequest extends PullRequest {
    private static final int MAX_OPS = 200;
    private static final boolean DBG = true;
    private static final String TAG = "PbapPhonebookPullRequest";

    private final Account mAccount;
    private final Context mContext;
    public boolean complete = false;

    public PhonebookPullRequest(Context context, Account account) {
        mContext = context;
        mAccount = account;
        path = BluetoothPbapClient.PB_PATH;
    }

    private PhonebookEntry fetchContact(String id) {
        PhonebookEntry entry = new PhonebookEntry();
        entry.id = id;
        Cursor c = null;
        try {
            c = mContext.getContentResolver().query(
                    Data.CONTENT_URI,
                    null,
                    Data.RAW_CONTACT_ID + " = ?",
                    new String[] { id },
                    null);
            if (c != null) {
                int mimeTypeIndex = c.getColumnIndex(Data.MIMETYPE);
                int familyNameIndex = c.getColumnIndex(StructuredName.FAMILY_NAME);
                int givenNameIndex = c.getColumnIndex(StructuredName.GIVEN_NAME);
                int middleNameIndex = c.getColumnIndex(StructuredName.MIDDLE_NAME);
                int prefixIndex = c.getColumnIndex(StructuredName.PREFIX);
                int suffixIndex = c.getColumnIndex(StructuredName.SUFFIX);

                int phoneTypeIndex = c.getColumnIndex(Phone.TYPE);
                int phoneNumberIndex = c.getColumnIndex(Phone.NUMBER);

                while (c.moveToNext()) {
                    String mimeType = c.getString(mimeTypeIndex);
                    if (mimeType.equals(StructuredName.CONTENT_ITEM_TYPE)) {
                        entry.name.family = c.getString(familyNameIndex);
                        entry.name.given = c.getString(givenNameIndex);
                        entry.name.middle = c.getString(middleNameIndex);
                        entry.name.prefix = c.getString(prefixIndex);
                        entry.name.suffix = c.getString(suffixIndex);
                    } else if (mimeType.equals(Phone.CONTENT_ITEM_TYPE)) {
                        PhonebookEntry.Phone p = new PhonebookEntry.Phone();
                        p.type = c.getInt(phoneTypeIndex);
                        p.number = c.getString(phoneNumberIndex);
                        entry.phones.add(p);
                    }
                }
            }
        } finally {
            if (c != null) {
                c.close();
            }
        }
        return entry;
    }

    private HashMap<PhonebookEntry.Name, PhonebookEntry> fetchExistingContacts() {
        HashMap<PhonebookEntry.Name, PhonebookEntry> entries = new HashMap<>();

        Cursor c = null;
        try {
            // First find all the contacts present. Fetch all rows.
            Uri uri = RawContacts.CONTENT_URI.buildUpon()
                    .appendQueryParameter(RawContacts.ACCOUNT_NAME, mAccount.name)
                    .appendQueryParameter(RawContacts.ACCOUNT_TYPE, mAccount.type)
                    .build();
            // First get all the raw contact ids.
            c = mContext.getContentResolver().query(uri,
                    new String[]  { RawContacts._ID },
                    null, null, null);

            if (c != null) {
                while (c.moveToNext()) {
                    // For each raw contact id, fetch all the data.
                    PhonebookEntry e = fetchContact(c.getString(0));
                    entries.put(e.name, e);
                }
            }
        } finally {
            if (c != null) {
                c.close();
            }
        }

        return entries;
    }

    private void addContacts(List<PhonebookEntry> entries)
            throws RemoteException, OperationApplicationException, InterruptedException {
        ArrayList<ContentProviderOperation> ops = new ArrayList<ContentProviderOperation>();
        for (PhonebookEntry e : entries) {
            if (Thread.currentThread().isInterrupted()) {
                throw new InterruptedException();
            }
            int index = ops.size();
            // Add an entry.
            ops.add(ContentProviderOperation.newInsert(RawContacts.CONTENT_URI)
                    .withValue(RawContacts.ACCOUNT_TYPE, mAccount.type)
                    .withValue(RawContacts.ACCOUNT_NAME, mAccount.name)
                    .build());

            // Populate the name.
            ops.add(ContentProviderOperation.newInsert(Data.CONTENT_URI)
                    .withValueBackReference(Data.RAW_CONTACT_ID, index)
                    .withValue(Data.MIMETYPE, StructuredName.CONTENT_ITEM_TYPE)
                    .withValue(StructuredName.FAMILY_NAME , e.name.family)
                    .withValue(StructuredName.GIVEN_NAME , e.name.given)
                    .withValue(StructuredName.MIDDLE_NAME , e.name.middle)
                    .withValue(StructuredName.PREFIX , e.name.prefix)
                    .withValue(StructuredName.SUFFIX , e.name.suffix)
                    .build());

            // Populate the phone number(s) if any.
            for (PhonebookEntry.Phone p : e.phones) {
                ops.add(ContentProviderOperation.newInsert(Data.CONTENT_URI)
                        .withValueBackReference(Data.RAW_CONTACT_ID, index)
                        .withValue(Data.MIMETYPE, Phone.CONTENT_ITEM_TYPE)
                        .withValue(Phone.NUMBER, p.number)
                        .withValue(Phone.TYPE, p.type)
                        .build());
            }

            // Commit MAX_OPS at a time so that the binder transaction doesn't get too large.
            if (ops.size() > MAX_OPS) {
                mContext.getContentResolver().applyBatch(ContactsContract.AUTHORITY, ops);
                ops.clear();
            }
        }

        if (ops.size() > 0) {
            // Commit remaining entries.
            mContext.getContentResolver().applyBatch(ContactsContract.AUTHORITY, ops);
        }
    }

    private void deleteContacts(List<PhonebookEntry> entries)
            throws RemoteException, OperationApplicationException {
        ArrayList<ContentProviderOperation> ops = new ArrayList<ContentProviderOperation>();
        for (PhonebookEntry e : entries) {
            ops.add(ContentProviderOperation.newDelete(RawContacts.CONTENT_URI.buildUpon()
                        .appendQueryParameter(ContactsContract.CALLER_IS_SYNCADAPTER, "true")
                        .build())
                .withSelection(RawContacts._ID + "=?", new String[] { e.id })
                .build());
        }
        mContext.getContentResolver().applyBatch(ContactsContract.AUTHORITY, ops);
    }

    @Override
    public void onPullComplete() {
        if (mEntries == null) {
            Log.e(TAG, "onPullComplete entries is null.");
            return;
        }

        if (DBG) {
            Log.d(TAG, "onPullComplete with " + mEntries.size() + " count.");
        }
        try {

            HashMap<PhonebookEntry.Name, PhonebookEntry> contacts = fetchExistingContacts();

            List<PhonebookEntry> contactsToAdd = new ArrayList<PhonebookEntry>();
            List<PhonebookEntry> contactsToDelete = new ArrayList<PhonebookEntry>();

            for (VCardEntry e : mEntries) {
                PhonebookEntry current = new PhonebookEntry(e);
                PhonebookEntry.Name key = current.name;

                PhonebookEntry contact = contacts.get(key);
                if (contact == null) {
                    contactsToAdd.add(current);
                } else if (!contact.equals(current)) {
                    // Instead of trying to figure out what changed on an update, do a delete
                    // and an add. Sure, it churns contact ids but a contact being updated
                    // while someone is connected is a low enough frequency event that the
                    // complexity of doing an update is just not worth it.
                    contactsToAdd.add(current);
                    // Don't remove it from the hashmap so it will get deleted.
                } else {
                    contacts.remove(key);
                }
            }
            contactsToDelete.addAll(contacts.values());

            if (!contactsToDelete.isEmpty()) {
                deleteContacts(contactsToDelete);
            }

            if (!contactsToAdd.isEmpty()) {
                addContacts(contactsToAdd);
            }

            Log.d(TAG, "Sync complete: add=" + contactsToAdd.size()
                    + " delete=" + contactsToDelete.size());
        } catch (OperationApplicationException | RemoteException | NumberFormatException e) {
            Log.d(TAG, "Got exception: ", e);
        } catch (InterruptedException e) {
            Log.d(TAG, "Interrupted durring insert.");
        } finally {
            complete = true;
        }
    }
}
