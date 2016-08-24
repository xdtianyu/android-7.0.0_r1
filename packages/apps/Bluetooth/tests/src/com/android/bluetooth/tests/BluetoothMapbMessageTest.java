package com.android.bluetooth.tests;

import java.io.ByteArrayInputStream;
import java.io.ByteArrayOutputStream;
import java.io.InputStream;
import java.io.UnsupportedEncodingException;
import java.text.SimpleDateFormat;
import java.util.Calendar;
import java.util.Date;

import android.test.AndroidTestCase;
import android.util.Log;

import com.android.bluetooth.map.BluetoothMapAppParams;
import com.android.bluetooth.map.BluetoothMapSmsPdu;
import com.android.bluetooth.map.BluetoothMapUtils;
import com.android.bluetooth.map.BluetoothMapUtils.TYPE;
import com.android.bluetooth.map.BluetoothMapbMessage;
import com.android.bluetooth.map.BluetoothMapbMessageMime;
import com.android.bluetooth.map.BluetoothMapbMessageSms;

/***
 *
 * Test cases for the bMessage class. (encoding and decoding)
 *
 */
public class BluetoothMapbMessageTest extends AndroidTestCase {
    protected static String TAG = "BluetoothMapbMessageTest";
    protected static final boolean D = true;

    public BluetoothMapbMessageTest() {
        super();
    }

    /***
     * Test encoding of a simple SMS text message (UTF8). This validates most parameters.
     */
    public void testSmsEncodeText() {
        BluetoothMapbMessageSms msg = new BluetoothMapbMessageSms();
        String str1 =
                 "BEGIN:BMSG\r\n" +
                    "VERSION:1.0\r\n" +
                    "STATUS:UNREAD\r\n" +
                    "TYPE:SMS_GSM\r\n" +
                    "FOLDER:telecom/msg/inbox\r\n" +
                    "BEGIN:VCARD\r\n" +
                        "VERSION:3.0\r\n" +
                        "FN:Casper Bonde\r\n" +
                        "N:Bonde,Casper\r\n" +
                        "TEL:+4512345678\r\n" +
                        "TEL:+4587654321\r\n" +
                        "EMAIL:casper@email.add\r\n" +
                        "EMAIL:bonde@email.add\r\n" +
                    "END:VCARD\r\n" +
                    "BEGIN:VCARD\r\n" +
                    "VERSION:3.0\r\n" +
                    "FN:Casper Bonde\r\n" +
                    "N:Bonde,Casper\r\n" +
                    "TEL:+4512345678\r\n" +
                    "TEL:+4587654321\r\n" +
                    "EMAIL:casper@email.add\r\n" +
                    "EMAIL:bonde@email.add\r\n" +
                "END:VCARD\r\n" +
                    "BEGIN:BENV\r\n" +
                        "BEGIN:VCARD\r\n" +
                            "VERSION:3.0\r\n" +
                            "FN:Jens Hansen\r\n" +
                            "N:\r\n" +
                            "TEL:+4512345678\r\n" +
                            "TEL:+4587654321\r\n" +
                            "EMAIL:casper@email.add\r\n" +
                            "EMAIL:bonde@email.add\r\n" +
                        "END:VCARD\r\n" +
                        "BEGIN:VCARD\r\n" +
                            "VERSION:3.0\r\n" +
                            "FN:Jens Hansen\r\n" +
                            "N:\r\n" +
                            "TEL:+4512345678\r\n" +
                            "TEL:+4587654321\r\n" +
                            "EMAIL:casper@email.add\r\n" +
                            "EMAIL:bonde@email.add\r\n" +
                        "END:VCARD\r\n" +
                        "BEGIN:BBODY\r\n" +
                            "CHARSET:UTF-8\r\n" +
                            "LENGTH:45\r\n" +
                            "BEGIN:MSG\r\n" +
                                "This is a short message\r\n" +
                            "END:MSG\r\n" +
                        "END:BBODY\r\n" +
                    "END:BENV\r\n" +
                 "END:BMSG\r\n";

        String encoded;
        String[] phone = {"+4512345678", "+4587654321"};
        String[] email = {"casper@email.add", "bonde@email.add"};
        msg.addOriginator("Bonde,Casper", "Casper Bonde", phone, email, null, null);
        msg.addOriginator("Bonde,Casper", "Casper Bonde", phone, email, null, null);
        msg.addRecipient("", "Jens Hansen", phone, email, null, null);
        msg.addRecipient("", "Jens Hansen", phone, email, null, null);
        msg.setFolder("inbox");
        msg.setSmsBody("This is a short message");
        msg.setStatus(false);
        msg.setType(TYPE.SMS_GSM);
        try {
            encoded = new String(msg.encode());
            if(D) Log.d(TAG, encoded);
            assertTrue(str1.equals(encoded));
        } catch (UnsupportedEncodingException e) {
            Log.d(TAG, "Encoding failed.",e);
            assertTrue("Encoding failed.", true);
        }
    }

    /***
     * Test native Deliver PDU encoding (decoding not possible), based on the example in the MAP 1.1 specification.
     * The difference between this PDU, and the one in the specification:
     *  - The invalid SC address 0191 is replaced with no address 00
     *  - The "No more messages flag" is set (bit 2 in the second byte)
     *  - The phone number type is changed from private 91 to international 81
     *  - The time is changed to local time, since the time zone cannot be controlled through the API
     */
    public void testSmsEncodeNativeDeliverPdu() {
        BluetoothMapbMessageSms msg = new BluetoothMapbMessageSms();
        SimpleDateFormat format = new SimpleDateFormat("yyMMddHHmmss");
        Date date = new Date(System.currentTimeMillis());
        String timeStr = format.format(date); // Format to YYMMDDTHHMMSS UTC time
        ByteArrayOutputStream scTime = new ByteArrayOutputStream(7);
        StringBuilder scTimeSb = new StringBuilder();
        byte[] timeChars;
        try {
            timeChars = timeStr.getBytes("US-ASCII");
        } catch (UnsupportedEncodingException e1) {
            assertTrue("Failed to extract bytes from string using US-ASCII", true);
            return;
        }

        for(int i = 0, n = timeStr.length(); i < n; i+=2) {
            scTime.write((timeChars[i+1]-0x30) << 4 | (timeChars[i]-0x30)); // Offset from ascii char to decimal value
        }

        Calendar cal = Calendar.getInstance();
        int offset = (cal.get(Calendar.ZONE_OFFSET) + cal.get(Calendar.DST_OFFSET)) / (15 * 60 * 1000); /* offset in quarters of an hour */
        String offsetString;
        if(offset < 0) {
            offsetString = String.format("%1$02d", -(offset));
            char[] offsetChars = offsetString.toCharArray();
            scTime.write((offsetChars[1]-0x30) << 4 | 0x40 | (offsetChars[0]-0x30));
        }
        else {
            offsetString = String.format("%1$02d", offset);
            char[] offsetChars = offsetString.toCharArray();
            scTime.write((offsetChars[1]-0x30) << 4 | (offsetChars[0]-0x30));
        }
        byte[] scTimeData = scTime.toByteArray();
        for(int i = 0; i < scTimeData.length; i++) {
            scTimeSb.append(Integer.toString((scTimeData[i] >> 4) & 0x0f,16)); // MS-nibble first
            scTimeSb.append(Integer.toString( scTimeData[i]       & 0x0f,16));
        }
        if(D) Log.v(TAG, "Generated time string: " + scTimeSb.toString());
        String expected =
                 "BEGIN:BMSG\r\n" +
                    "VERSION:1.0\r\n" +
                    "STATUS:UNREAD\r\n" +
                    "TYPE:SMS_GSM\r\n" +
                    "FOLDER:telecom/msg/inbox\r\n" +
                    "BEGIN:VCARD\r\n" +
                        "VERSION:3.0\r\n" +
                        "FN:Casper Bonde\r\n" +
                        "N:Bonde,Casper\r\n" +
                        "TEL:00498912345678\r\n" +
                        "TEL:+4587654321\r\n" +
                        "EMAIL:casper@email.add\r\n" +
                        "EMAIL:bonde@email.add\r\n" +
                    "END:VCARD\r\n" +
                    "BEGIN:BENV\r\n" +
                        "BEGIN:VCARD\r\n" +
                            "VERSION:3.0\r\n" +
                            "FN:Jens Hansen\r\n" +
                            "N:\r\n" +
                            "TEL:00498912345678\r\n" +
                            "TEL:+4587654321\r\n" +
                            "EMAIL:casper@email.add\r\n" +
                            "EMAIL:bonde@email.add\r\n" +
                        "END:VCARD\r\n" +
                        "BEGIN:BBODY\r\n" +
                            "ENCODING:G-7BIT\r\n" +
                            "LENGTH:94\r\n" +
                            "BEGIN:MSG\r\n" +
                                "00040E81009498214365870000" + scTimeSb.toString() +
                                "11CC32FD34079DDF20737A8E4EBBCF21\r\n" +
                            "END:MSG\r\n" +
                        "END:BBODY\r\n" +
                    "END:BENV\r\n" +
                 "END:BMSG\r\n";

        String encoded;
        String[] phone = {"00498912345678", "+4587654321"};
        String[] email = {"casper@email.add", "bonde@email.add"};
        msg.addOriginator("Bonde,Casper", "Casper Bonde", phone, email, null, null);
        msg.addRecipient("", "Jens Hansen", phone, email, null, null);
        msg.setFolder("inbox");
        /* TODO: extract current time, and build the expected string */
        msg.setSmsBodyPdus(BluetoothMapSmsPdu.getDeliverPdus("Let's go fishing!", "00498912345678", date.getTime()));
        msg.setStatus(false);
        msg.setType(TYPE.SMS_GSM);
        try {
            byte[] encodedBytes = msg.encode();
//            InputStream is = new ByteArrayInputStream(encodedBytes);
            encoded = new String(encodedBytes);
//            BluetoothMapbMessage newMsg = BluetoothMapbMessage.parse(is, BluetoothMapAppParams.CHARSET_NATIVE);
//            String decoded = ((BluetoothMapbMessageSms) newMsg).getSmsBody();
            if(D) Log.d(TAG, "\nExpected: \n" + expected);
            if(D) Log.d(TAG, "\nEncoded: \n" + encoded);
//            if(D) Log.d(TAG, "\nDecoded: \n" + decoded);
            assertTrue(expected.equalsIgnoreCase(encoded));
        } catch (UnsupportedEncodingException e) {
            Log.d(TAG, "Encoding failed.",e);
            assertTrue("Encoding failed.", true);
        }
    }

    /***
     * Test native Submit PDU encoding and decoding, based on the example in the MAP 1.1 specification.
     * The difference between this PDU, and the one in the specification:
     *  - The invalid SC address 0191 is replaced with no address 00
     *  - The PDU is converted to a submit PDU by adding the TP-MR and removing the service center time stamp.
     *  - The phone number type is changed from private 91 to international 81
     */
    public void testSmsEncodeDecodeNativeSubmitPdu() {
        BluetoothMapbMessageSms msg = new BluetoothMapbMessageSms();
        String expected =
                 "BEGIN:BMSG\r\n" +
                    "VERSION:1.0\r\n" +
                    "STATUS:UNREAD\r\n" +
                    "TYPE:SMS_GSM\r\n" +
                    "FOLDER:telecom/msg/outbox\r\n" +
                    "BEGIN:VCARD\r\n" +
                        "VERSION:3.0\r\n" +
                        "FN:Casper Bonde\r\n" +
                        "N:Bonde,Casper\r\n" +
                        "TEL:00498912345678\r\n" +
                        "TEL:+4587654321\r\n" +
                        "EMAIL:casper@email.add\r\n" +
                        "EMAIL:bonde@email.add\r\n" +
                    "END:VCARD\r\n" +
                    "BEGIN:BENV\r\n" +
                        "BEGIN:VCARD\r\n" +
                            "VERSION:3.0\r\n" +
                            "FN:Jens Hansen\r\n" +
                            "N:\r\n" +
                            "TEL:00498912345678\r\n" +
                            "TEL:+4587654321\r\n" +
                            "EMAIL:casper@email.add\r\n" +
                            "EMAIL:bonde@email.add\r\n" +
                        "END:VCARD\r\n" +
                        "BEGIN:BBODY\r\n" +
                            "ENCODING:G-7BIT\r\n" +
                            "LENGTH:82\r\n" +
                            "BEGIN:MSG\r\n" + /*Length 11 */
                                "0001000E8100949821436587000011CC32FD34079DDF20737A8E4EBBCF21\r\n" + /* Length 62 */
                            "END:MSG\r\n" + /* Length 9 */
                        "END:BBODY\r\n" +
                    "END:BENV\r\n" +
                 "END:BMSG\r\n";

        String encoded;
        String[] phone = {"00498912345678", "+4587654321"};
        String[] email = {"casper@email.add", "bonde@email.add"};
        msg.addOriginator("Bonde,Casper", "Casper Bonde", phone, email, null, null);
        msg.addRecipient("", "Jens Hansen", phone, email, null, null);
        msg.setFolder("outbox");
        /* TODO: extract current time, and build the expected string */
        msg.setSmsBodyPdus(BluetoothMapSmsPdu.getSubmitPdus("Let's go fishing!", "00498912345678"));
        msg.setStatus(false);
        msg.setType(TYPE.SMS_GSM);
        try {
            byte[] encodedBytes = msg.encode();
            InputStream is = new ByteArrayInputStream(encodedBytes);
            encoded = new String(encodedBytes);
            BluetoothMapbMessage newMsg = BluetoothMapbMessage.parse(is, BluetoothMapAppParams.CHARSET_NATIVE);
            String decoded = ((BluetoothMapbMessageSms) newMsg).getSmsBody();
            if(D) Log.d(TAG, "\nCalling encoder on decoded message to log its content");
            newMsg.encode();
            if(D) Log.d(TAG, "\nExpected: \n" + expected);
            if(D) Log.d(TAG, "\nEncoded: \n" + encoded);
            if(D) Log.d(TAG, "\nDecoded: \n" + decoded);
            assertTrue("The encoded bMessage do not match the expected.", expected.equalsIgnoreCase(encoded));
            assertTrue("The decoded text is \"" + decoded + "\" - expected \"Let's go fishing!\"", decoded.equalsIgnoreCase("Let's go fishing!"));
        } catch (UnsupportedEncodingException e) {
            Log.d(TAG, "Encoding failed.",e);
            assertTrue("Encoding failed.", true);
        }
    }

    /***
     * Test native Submit PDU encoding and decoding, based on the example in the MAP 1.1 specification.
     * The difference between this PDU, and the one in the specification:
     *  - The invalid SC address 0191 is replaced with no address 00
     *  - The PDU is converted to a submit PDU by adding the TP-MR and removing the service center time stamp.
     *  - The phone number type is changed from private 91 to international 81
     */
    public void testSmsEncodeDecodeNativeSubmitPduWithSc() {
        BluetoothMapbMessageSms msg = new BluetoothMapbMessageSms();
        String encoded =
                 "BEGIN:BMSG\r\n" +
                    "VERSION:1.0\r\n" +
                    "STATUS:UNREAD\r\n" +
                    "TYPE:SMS_GSM\r\n" +
                    "FOLDER:telecom/msg/outbox\r\n" +
                    "BEGIN:VCARD\r\n" +
                        "VERSION:3.0\r\n" +
                        "FN:Casper Bonde\r\n" +
                        "N:Bonde,Casper\r\n" +
                        "TEL:00498912345678\r\n" +
                        "TEL:+4587654321\r\n" +
                        "EMAIL:casper@email.add\r\n" +
                        "EMAIL:bonde@email.add\r\n" +
                    "END:VCARD\r\n" +
                    "BEGIN:BENV\r\n" +
                        "BEGIN:VCARD\r\n" +
                            "VERSION:3.0\r\n" +
                            "FN:Jens Hansen\r\n" +
                            "N:\r\n" +
                            "TEL:00498912345678\r\n" +
                            "TEL:+4587654321\r\n" +
                            "EMAIL:casper@email.add\r\n" +
                            "EMAIL:bonde@email.add\r\n" +
                        "END:VCARD\r\n" +
                        "BEGIN:BBODY\r\n" +
                            "ENCODING:G-7BIT\r\n" +
                            "LENGTH:58 \r\n" +
                            "BEGIN:MSG\r\n" + /*Length 11 */
                                "018001000B912184254590F500000346F61B\r\n" + /* Length 38 */
                            "END:MSG\r\n" + /* Length 9 */
                        "END:BBODY\r\n" +
                    "END:BENV\r\n" +
                 "END:BMSG\r\n";
        try {
            String expected = "Flo";
            InputStream is = new ByteArrayInputStream(encoded.getBytes("UTF-8"));
            BluetoothMapbMessage newMsg = BluetoothMapbMessage.parse(is, BluetoothMapAppParams.CHARSET_NATIVE);
            String decoded = ((BluetoothMapbMessageSms) newMsg).getSmsBody();
            if(D) Log.d(TAG, "\nEncoded: \n" + encoded);
            if(D) Log.d(TAG, "\nDecoded: \n" + decoded);
            assertTrue("Decoded string (" + decoded + ") did not match expected (" + expected + ")", expected.equals(decoded));
        } catch (UnsupportedEncodingException e) {
            Log.d(TAG, "Encoding failed.",e);
            assertTrue("Encoding failed.", false);
        }
    }

    /***
     * Validate that the folder is correctly truncated to 512 bytes, if a longer folder path
     * is supplied.
     */
    public void testFolderLengthTruncation() {
        String folder = "";
        int levelCount = 0;
        while(folder.length()<640)
            folder += "/folder" + levelCount++;

        String expected = folder.substring(folder.length()-512, folder.length());

        BluetoothMapbMessageSms msg = new BluetoothMapbMessageSms();
        msg.setFolder(folder);
        msg.setStatus(false);
        msg.setType(TYPE.SMS_GSM);

        try {
            byte[] encoded = msg.encode();
            InputStream is = new ByteArrayInputStream(encoded);
            if(D) Log.d(TAG, new String(encoded));
            BluetoothMapbMessage newMsg = BluetoothMapbMessage.parse(is, BluetoothMapAppParams.CHARSET_UTF8);
            assertTrue("Wrong length expected 512, got " + expected.length(), expected.length() == 512);
            Log.d(TAG, "expected:           " + expected);
            Log.d(TAG, "newMsg.getFolder(): " + newMsg.getFolder());
            assertTrue("Folder string did not match", expected.equals(newMsg.getFolder()));

        } catch (UnsupportedEncodingException e) {
            Log.d(TAG, "Encoding failed.",e);
            assertTrue("Encoding failed", false);
        }
    }

    /***
     * Test multipart message decoding.
     */
    public void testSmsMultipartDecode() {
        BluetoothMapbMessageSms msg = new BluetoothMapbMessageSms();
        String encoded =
                 "BEGIN:BMSG\r\n" +
                 "VERSION:1.0\r\n" +
                 "STATUS:READ\r\n" +
                 "TYPE:SMS_GSM\r\n" +
                 "FOLDER:/telecom/msg/outbox\r\n" +
                 "BEGIN:VCARD\r\n" +
                 "VERSION:2.1\r\n" +
                 "N:12485254094 \r\n" +
                 "TEL:12485254094\r\n" +
                 "END:VCARD\r\n" +
                 "BEGIN:BENV\r\n" +
                 "BEGIN:VCARD\r\n" +
                 "VERSION:2.1\r\n" +
                 "N:+12485254095 \r\n" +
                 "TEL:+12485254095\r\n" +
                 "END:VCARD\r\n" +
                 "BEGIN:BBODY\r\n" +
                 "ENCODING:G-7BIT\r\n" +
                 "LENGTH:762\r\n" +
                 "BEGIN:MSG\r\n" +
                 "018041000B912184254590F50000A0050003010301A8E8F41C949E83C220F69B0E7A9B41F7B79C3C07D1DF20F35BDE068541E3B77B1CA697DD617A990C6A97E7F3F0B90C0ABBC9203ABA0C32A7E5733A889E6E9741F437888E2E83E66537B92C07A5DBED32391DA697D97990FB4D4F9BF3A07619B476BFEFA03B3A4C07E5DF7550585E06B9DF74D0BC2E2F83F2EF3A681C7683C86F509A0EBABFEB\r\n" +
                 "END:MSG\r\n" +
                 "BEGIN:MSG\r\n" +
                 "018041000B912184254590F50000A0050003010302C820767A5D06D1D16550DA4D2FBBC96532485E1EA7E1E9B29B0E82B3CBE17919644EBBC9A0779D0EA2A3CB20735A3EA783E8E8B4FB0CA2BF41E437C8FDA683E6757919947FD741E3B01B447E83D274D0FD5D679341ECB7BD0C4AD341F7F01C44479741E6B47C4E0791C379D0DB0C6AE741F2F2BCDE2E83CC6F3928FFAECB41E57638CD06A5E7\r\n" +
                 "END:MSG\r\n" +
                 "BEGIN:MSG\r\n" +
                 "018041000B912184254590F500001A050003010303DC6F3A685E979741F9771D340EBB41E437\r\n" +
                 "END:MSG\r\n" +
                 "END:BBODY\r\n" +
                 "END:BENV\r\n" +
                 "END:BMSG\r\n";
        try {
            InputStream is = new ByteArrayInputStream(encoded.getBytes("UTF-8"));
            BluetoothMapbMessage newMsg = BluetoothMapbMessage.parse(is, BluetoothMapAppParams.CHARSET_NATIVE);
            String decoded = ((BluetoothMapbMessageSms) newMsg).getSmsBody();
            if(D) Log.d(TAG, "\nEncoded: \n" + encoded);
            if(D) Log.d(TAG, "\nDecoded: \n" + decoded);
        } catch (UnsupportedEncodingException e) {
            Log.d(TAG, "Decoding failed.",e);
            assertTrue("Decoding failed.", false);
        }
    }

    /***
     * Test encoding of a simple MMS text message (UTF8). This validates most parameters.
     */
    public void testMmsEncodeText() {
        BluetoothMapbMessageMime  msg = new BluetoothMapbMessageMime ();
        String str1 =
                 "BEGIN:BMSG\r\n" +
                    "VERSION:1.0\r\n" +
                    "STATUS:UNREAD\r\n" +
                    "TYPE:MMS\r\n" +
                    "FOLDER:telecom/msg/inbox\r\n" +
                    "BEGIN:VCARD\r\n" +
                        "VERSION:3.0\r\n" +
                        "FN:Casper Bonde\r\n" +
                        "N:Bonde,Casper\r\n" +
                        "TEL:+4512345678\r\n" +
                        "TEL:+4587654321\r\n" +
                        "EMAIL:casper@email.add\r\n" +
                        "EMAIL:bonde@email.add\r\n" +
                    "END:VCARD\r\n" +
                    "BEGIN:BENV\r\n" +
                        "BEGIN:VCARD\r\n" +
                            "VERSION:3.0\r\n" +
                            "FN:Jørn Hansen\r\n" +
                            "N:\r\n" +
                            "TEL:+4512345678\r\n" +
                            "TEL:+4587654321\r\n" +
                            "EMAIL:casper@email.add\r\n" +
                            "EMAIL:bonde@email.add\r\n" +
                        "END:VCARD\r\n" +
                        "BEGIN:BBODY\r\n" +
                            "CHARSET:UTF-8\r\n" +
                            "LENGTH:184\r\n" +
                            "BEGIN:MSG\r\n" +
                            "From: \"Jørn Hansen\" <bonde@email.add>;\r\n" +
                            "To: \"Jørn Hansen\" <bonde@email.add>;\r\n" +
                            "Cc: Jens Hansen <bonde@email.add>;\r\n" +
                            "\r\n" +
                            "This is a short message\r\n" +
                            "\r\n" +
                            "<partNameimage>\r\n" +
                            "\r\n" +
                            "END:MSG\r\n" +
                        "END:BBODY\r\n" +
                    "END:BENV\r\n" +
                 "END:BMSG\r\n";

        String encoded;
        String[] phone = {"+4512345678", "+4587654321"};
        String[] email = {"casper@email.add", "bonde@email.add"};
        msg.addOriginator("Bonde,Casper", "Casper Bonde", phone, email, null, null);
        msg.addRecipient("", "Jørn Hansen", phone, email, null, null);
        msg.setFolder("inbox");
        msg.setIncludeAttachments(false);
        msg.addTo("Jørn Hansen", "bonde@email.add");
        msg.addCc("Jens Hansen", "bonde@email.add");
        msg.addFrom("Jørn Hansen", "bonde@email.add");
        BluetoothMapbMessageMime .MimePart part = msg.addMimePart();
        part.mPartName = "partNameText";
        part.mContentType ="dsfajfdlk/text/asdfafda";
        try {
            part.mData = new String("This is a short message\r\n").getBytes("UTF-8");
        }
        catch (UnsupportedEncodingException e) {
            if(D) Log.e(TAG, "UnsupportedEncodingException should never happen???", e);
            assertTrue(false);
        }

        part = msg.addMimePart();
        part.mPartName = "partNameimage";
        part.mContentType = "dsfajfdlk/image/asdfafda";
        part.mData = null;

        msg.setStatus(false);
        msg.setType(TYPE.MMS);
        msg.updateCharset();

        try {
            encoded = new String(msg.encode());
            if(D) Log.d(TAG, encoded);
            assertTrue(str1.equals(encoded));
        } catch (UnsupportedEncodingException e) {
            Log.d(TAG, "Encoding failed.",e);
            assertTrue("Encoding failed.", true);
        }
    }

    public void testQuotedPrintable() {
        testQuotedPrintableIso8859_1();
        testQuotedPrintableUTF_8();
    }

    public void testQuotedPrintableIso8859_1() {
        String charset = "iso-8859-1";
        String input = "Hello, here are some danish letters: =E6=F8=E5.\r\n" +
                       "Please check that you are able to remove soft " +
                       "line breaks and handle '=3D' =\r\ncharacters within the text. \r\n" +
                       "Just a sequence of non optimal characters to make " +
                       "it complete: !\"#$@[\\]^{|}=\r\n~\r\n\r\n" +
                       "Thanks\r\n" +
                       "Casper";
        String expected = "Hello, here are some danish letters: æøå.\r\n" +
                "Please check that you are able to remove soft " +
                "line breaks and handle '=' characters within the text. \r\n" +
                "Just a sequence of non optimal characters to make " +
                "it complete: !\"#$@[\\]^{|}~\r\n\r\n" +
                "Thanks\r\n" +
                "Casper";
        String output;
        output = new String(BluetoothMapUtils.quotedPrintableToUtf8(input, charset));
        if(D) Log.d(TAG, "\nExpected: \n" + expected);
        if(D) Log.d(TAG, "\nOutput: \n" + output);
        assertTrue(output.equals(expected));
    }

    public void testQuotedPrintableUTF_8() {
        String charset = "utf-8";
        String input = "Hello, here are some danish letters: =C3=A6=C3=B8=C3=A5.\r\n" +
                       "Please check that you are able to remove soft " +
                       "line breaks and handle '=3D' =\r\ncharacters within the text. \r\n" +
                       "Just a sequence of non optimal characters to make " +
                       "it complete: !\"#$@[\\]^{|}=\r\n~\r\n\r\n" +
                       "Thanks\r\n" +
                       "Casper";
        String expected = "Hello, here are some danish letters: æøå.\r\n" +
                "Please check that you are able to remove soft " +
                "line breaks and handle '=' characters within the text. \r\n" +
                "Just a sequence of non optimal characters to make " +
                "it complete: !\"#$@[\\]^{|}~\r\n\r\n" +
                "Thanks\r\n" +
                "Casper";
        String output;
        output = new String(BluetoothMapUtils.quotedPrintableToUtf8(input, charset));
        if(D) Log.d(TAG, "\nExpected: \n" + expected);
        if(D) Log.d(TAG, "\nOutput: \n" + output);
        assertTrue(output.equals(expected));
    }

}

