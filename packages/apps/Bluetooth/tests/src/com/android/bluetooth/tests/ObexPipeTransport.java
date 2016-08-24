/*
* Copyright (C) 2015 Samsung System LSI
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

package com.android.bluetooth.tests;

import java.io.DataInputStream;
import java.io.DataOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.io.PipedInputStream;
import java.io.PipedOutputStream;

import javax.obex.ObexTransport;

public class ObexPipeTransport implements ObexTransport {
    InputStream mInStream;
    OutputStream mOutStream;
    boolean mEnableSrm;

    public ObexPipeTransport(InputStream inStream, 
            OutputStream outStream, boolean enableSrm) {
        mInStream = inStream;
        mOutStream = outStream;
        mEnableSrm = enableSrm;
    }

    public void close() throws IOException {
        mInStream.close();
        mOutStream.close();
    }

    public DataInputStream openDataInputStream() throws IOException {
        return new DataInputStream(openInputStream());
    }

    public DataOutputStream openDataOutputStream() throws IOException {
        return new DataOutputStream(openOutputStream());
    }

    public InputStream openInputStream() throws IOException {
        return mInStream;
    }

    public OutputStream openOutputStream() throws IOException {
        return mOutStream;
    }

    public void connect() throws IOException {
    }

    public void create() throws IOException {
    }

    public void disconnect() throws IOException {
    }

    public void listen() throws IOException {
    }

    public boolean isConnected() throws IOException {
        return true;
    }

    public int getMaxTransmitPacketSize() {
        return 3*15432;
    }

    public int getMaxReceivePacketSize() {
        return 2*23450;
    }

    @Override
    public boolean isSrmSupported() {
        return mEnableSrm;
    }

}

