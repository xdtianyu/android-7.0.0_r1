package com.android.bluetooth.tests;

import android.net.LocalSocket;
import android.net.LocalSocketAddress;
import android.test.AndroidTestCase;
import android.util.Log;

import org.android.btsap.SapApi;
import org.android.btsap.SapApi.MsgHeader;
import org.android.btsap.SapApi.RIL_SIM_SAP_CONNECT_REQ;

import com.google.protobuf.micro.ByteStringMicro;
import com.google.protobuf.micro.CodedOutputStreamMicro;
import com.google.protobuf.micro.CodedInputStreamMicro;

import java.io.IOException;
import java.io.OutputStream;
import java.io.InputStream;
import java.util.Arrays;

public class SapSocketTest extends AndroidTestCase {

    protected static String TAG = "SapSocketTest";
    protected static final boolean D = true;

    private static final String SOCKET_NAME_RIL_BT = "sap_uim_socket1";

    public SapSocketTest() {
        super();
    }

    private void writeLegacyLength(int length, OutputStream rawOut) throws IOException {
        byte[] dataLength = new byte[4];
        dataLength[0] = dataLength[1] = 0;
        dataLength[2] = (byte)((length >> 8) & 0xff);
        dataLength[3] = (byte)((length) & 0xff);
        rawOut.write(dataLength);
    }

    private void dumpMsgHeader(MsgHeader msg){
        Log.d(TAG,"MsgHeader: ID =   " + msg.getId());
        Log.d(TAG,"           Type=  " + msg.getType());
        Log.d(TAG,"           Token= " + msg.getToken());
        Log.d(TAG,"           Error= " + msg.getError());
        Log.d(TAG,"           Length=" + msg.getSerializedSize());
        if(msg.hasPayload()){
        Log.d(TAG,"Payload:   Length=" + msg.getPayload().size());
        Log.d(TAG,"           Data=  " + Arrays.toString(msg.getPayload().toByteArray()));
        }

    }
    private void readLegacyLength(InputStream rawIn) throws IOException{
        byte[] buffer = new byte[4];
        int countRead;
        int offset;
        int remaining;
        int messageLength;

        // Read in the length of the message
        offset = 0;
        remaining = 4;
        do {
            countRead = rawIn.read(buffer, offset, remaining);

            if (countRead < 0 ) {
                Log.e(TAG, "Hit EOS reading message length");
                return;
            }

            offset += countRead;
            remaining -= countRead;
        } while (remaining > 0);

        messageLength = ((buffer[0] & 0xff) << 24)
                | ((buffer[1] & 0xff) << 16)
                | ((buffer[2] & 0xff) << 8)
                | (buffer[3] & 0xff);

        Log.d(TAG, "The length is: " + messageLength + " - discarding as we do not need it");

    }

/**
Precondition:
Add the sap_uim_socket1 to rild in init.rc:
    socket sap_uim_socket1 stream 666 root bluetooth

Ensure the socket is present in /dev/socket:
srw-rw-rw- root     bluetooth          1970-04-16 06:27 sap_uim_socket1

Build:
mmm packages/apps/Bluetooth/tests

rebuild with a make in the root folder to get the
android.test.InstrumentationTestRunner class included.

Run the test(remove line breaks):
adb shell am instrument -w -e class com.android.bluetooth.
tests.SapSocketTest#testSapServerConnectSimple com.android.
bluetooth.tests/android.test.InstrumentationTestRunner

Validate you do not get a permission denied IOException.

Validate you do not get an error in the kernel log:
type=1400 audit(1404244298.582:25): avc:  denied  { write }
for  pid=2421 comm="ationTestRunner" name="sap_uim_socket1"
dev="tmpfs" ino=6703 scontext=u:r:bluetooth:s0
tcontext=u:object_r:socket_device:s0 tclass=sock_file
*/

    /**
     * Precondition: Add the sap_uim_socket1 to rild in init.rc: socket
     * sap_uim_socket1 stream 666 root bluetooth
     *
     * Ensure the socket is present in /dev/socket: srw-rw-rw- root bluetooth
     * 1970-04-16 06:27 sap_uim_socket1
     *
     * Build: mmm packages/apps/Bluetooth/tests
     *
     * rebuild with a make in the root folder to get the
     * android.test.InstrumentationTestRunner class included.
     *
     * Run the test(remove line breaks): adb shell am instrument -w -e class
     * com.android.bluetooth. tests.SapSocketTest#testSapServerConnectSimple
     * com.android. bluetooth.tests/android.test.InstrumentationTestRunner
     *
     * Validate you do not get a permission denied IOException.
     *
     * Validate you do not get an error in the kernel log: type=1400
     * audit(1404244298.582:25): avc: denied { write } for pid=2421
     * comm="ationTestRunner" name="sap_uim_socket1" dev="tmpfs" ino=6703
     * scontext=u:r:bluetooth:s0 tcontext=u:object_r:socket_device:s0
     * tclass=sock_file
     */
    public void testSapServerConnectSimple() {
        LocalSocketAddress address;
        LocalSocket rilSocket = new LocalSocket();
        try {
            address = new LocalSocketAddress(SOCKET_NAME_RIL_BT,
                    LocalSocketAddress.Namespace.RESERVED);
            rilSocket.connect(address);
            CodedInputStreamMicro in = CodedInputStreamMicro.newInstance(rilSocket.getInputStream());
            OutputStream rawOut = rilSocket.getOutputStream();
            CodedOutputStreamMicro out = CodedOutputStreamMicro.newInstance(rilSocket.getOutputStream());
            InputStream rawIn = rilSocket.getInputStream();
            int rilSerial = 1;
            SapApi.MsgHeader msg = new MsgHeader();
            /* Common variables for all requests */
            msg.setToken(rilSerial);
            msg.setType(SapApi.REQUEST);
            msg.setError(SapApi.RIL_E_UNUSED);
            SapApi.RIL_SIM_SAP_CONNECT_REQ reqMsg = new RIL_SIM_SAP_CONNECT_REQ();
            reqMsg.setMaxMessageSize(1234);
            msg.setId(SapApi.RIL_SIM_SAP_CONNECT);
            msg.setPayload(ByteStringMicro.copyFrom(reqMsg.toByteArray()));
            writeLegacyLength(msg.getSerializedSize(), rawOut);
            msg.writeTo(out);
            out.flush();
            readLegacyLength(rawIn);
            msg = MsgHeader.parseFrom(in);
            dumpMsgHeader(msg);
            assertTrue("Invalid response type", msg.getType()== SapApi.RESPONSE);
            assertTrue("Invalid response id", msg.getId()== SapApi.RIL_SIM_SAP_CONNECT);
        } catch (IOException e){
            Log.e(TAG, "IOException:", e);
            assertTrue("Failed to connect to the socket " + SOCKET_NAME_RIL_BT + ":" + e, false);
        } finally {
            try {
                rilSocket.close();
            } catch (IOException e2) {}

        }
    }
}
