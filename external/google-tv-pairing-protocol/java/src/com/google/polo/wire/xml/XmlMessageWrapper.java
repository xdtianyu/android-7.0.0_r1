/*
 * Copyright (C) 2009 Google Inc.  All rights reserved.
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

package com.google.polo.wire.xml;

import java.io.ByteArrayInputStream;
import java.io.ByteArrayOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;

/**
 * Representation of a message sent by the XML protocol.
 */
public class XmlMessageWrapper {

    /**
     * Number of bytes in the header for the "receiver id" field.
     */
    private static final int HEADER_FIELD_RECEIVER_ID_LENGTH = 32;

    /**
     * Number of bytes in the header for the "payload length" field.
     */
    private static final int HEADER_FIELD_PAYLOAD_LENGTH = 4;

    /**
     * Number of bytes in the header for the "protocol version" field.
     */
    private static final int HEADER_FIELD_PROTOCOL_VERSION_LENGTH = 2;

    /**
     * Number of bytes in the header reserved for future use.
     */
    private static final int HEADER_FIELD_PADDING_LENGTH = 25;

    private static final int HEADER_SIZE = 64;

    /**
     * The id of the receiver.
     */
    private String mReceiverId;

    /**
     * Protocol version.
     */
    private int mProtocolVersion;

    /**
     * Creator ID
     */
    private byte mCreatorId;

    /**
     * XML message.
     */
    private byte[] mPayload;

    public XmlMessageWrapper(String recieverId, int protocolVersion,
            byte creatorId, byte[] payload) {
        mReceiverId = recieverId;
        mProtocolVersion = protocolVersion;
        mCreatorId = creatorId;
        mPayload = payload;
    }

    /**
     * Writes the serialized form of this message to an {@link OutputStream}
     *
     * @param  outputStream  the destination output stream
     * @throws IOException  if an error occurred during write
     */
    public void serializeToOutputStream(OutputStream outputStream)
            throws IOException {
        // Receiver ID
        outputStream.write(stringToBytesPadded(mReceiverId,
                HEADER_FIELD_RECEIVER_ID_LENGTH));

        // Payload length
        outputStream.write(intToBigEndianIntBytes(mPayload.length));

        // Protocol version
        outputStream.write(intToBigEndianShortBytes(mProtocolVersion));

        // Creator ID
        outputStream.write(mCreatorId);

        // Padding
        byte[] pad = new byte[HEADER_FIELD_PADDING_LENGTH];
        outputStream.write(pad);

        // Payload
        outputStream.write(mPayload);
    }

    /**
     * Returns the serialized form of this message in a newly-allocated byte
     * array.
     *
     * @return  a new byte array
     * @throws  IOException  if an error occurred during write
     */
    public byte[] serializeToByteArray() throws IOException {
        int len = mPayload.length + HEADER_SIZE;
        ByteArrayOutputStream outputStream = new ByteArrayOutputStream(len);
        serializeToOutputStream(outputStream);
        return outputStream.toByteArray();
    }

    /**
     * Construct a new {@link XmlMessageWrapper} from an InputStream.
     *
     * @param stream  the {@link InputStream} to read
     * @return  a new {@link XmlMessageWrapper}
     * @throws IOException  if an error occurs during read
     */
    public static XmlMessageWrapper fromInputStream(InputStream stream)
            throws IOException {
        String receiverId = new String(readBytes(stream,
                HEADER_FIELD_RECEIVER_ID_LENGTH));
        receiverId = receiverId.replace("\0", "");

        byte[] payloadLenBytes = readBytes(stream, HEADER_FIELD_PAYLOAD_LENGTH);
        long payloadLen = intBigEndianBytesToLong(payloadLenBytes);

        int protocolVersion = shortBigEndianBytesToInt(readBytes(stream,
                HEADER_FIELD_PROTOCOL_VERSION_LENGTH));

        byte createorId = readBytes(stream, 1)[0];
        byte[] padding = readBytes(stream, HEADER_FIELD_PADDING_LENGTH);
        byte[] payload = readBytes(stream, (int)payloadLen);

        return new XmlMessageWrapper(receiverId, protocolVersion, createorId,
                payload);

    }

    /**
     * Get creator id to indicate the program is playing on TV1 or TV2.
     */
    public byte getCreatorId() {
        return mCreatorId;
    }

    public byte[] getPayload() {
        return mPayload;
    }

    /**
     * Get the message payload as an {@link InputStream}.
     */
    public InputStream getPayloadStream() {
        return new ByteArrayInputStream(mPayload);
    }

    /**
     * Converts a 4-byte array of bytes to an unsigned long value.
     */
    private static final long intBigEndianBytesToLong(byte[] input) {
        assert (input.length == 4);
        long ret = (long)(input[0]) & 0xff;
        ret <<= 8;
        ret |= (long)(input[1]) & 0xff;
        ret <<= 8;
        ret |= (long)(input[2]) & 0xff;
        ret <<= 8;
        ret |= (long)(input[3]) & 0xff;
        return ret;
    }

    /**
     * Converts an integer value to the big endian 4-byte representation.
     */
    public static final byte[] intToBigEndianIntBytes(int intVal) {
        byte[] outBuf = new byte[4];
        outBuf[0] = (byte)((intVal >> 24) & 0xff);
        outBuf[1] = (byte)((intVal >> 16) & 0xff);
        outBuf[2] = (byte)((intVal >> 8) & 0xff);
        outBuf[3] = (byte)(intVal & 0xff);
        return outBuf;
    }

    /**
     * Converts a 2-byte array of bytes to an unsigned long value.
     */
    public static final int shortBigEndianBytesToInt(byte[] input) {
        assert (input.length == 2);
        int ret = (input[0]) & 0xff;
        ret <<= 8;
        ret |= input[1] & 0xff;
        return ret;
    }

    /**
     * Converts an integer value to the 2-byte short representation.  The two
     * most significant bytes are ignored.
     */
    public static final byte[] intToBigEndianShortBytes(int intVal) {
        byte[] outBuf = new byte[2];
        outBuf[0] = (byte)((intVal >> 8) & 0xff);
        outBuf[1] = (byte)(intVal & 0xff);
        return outBuf;
    }

    /**
     * Converts a string to a byte sequence of exactly byteLen bytes,
     * padding with null characters if needed.
     *
     * @param byteLen  the size of the byte array to return
     * @return  a byte array
     */
    public static final byte[] stringToBytesPadded(String string, int byteLen) {
        byte[] outBuf = new byte[byteLen];
        byte[] stringBytes = string.getBytes();

        for (int i=0; i < outBuf.length; i++) {
            if (i < stringBytes.length) {
                outBuf[i] = stringBytes[i];
            } else {
                outBuf[i] = '\0';
            }
        }
        return outBuf;
    }

    /**
     * Reads an exact number of bytes from an input stream.
     *
     * @param stream  the stream to read
     * @param numBytes  the number of bytes desired
     * @return  a byte array of results
     * @throws IOException  if an error occurred during read, or stream closed
     */
    private static byte[] readBytes(InputStream stream, int numBytes)
            throws IOException {
        byte buffer[] = new byte[numBytes];
        int bytesRead = 0;

        while (bytesRead < numBytes) {
            int inc = stream.read(buffer, bytesRead, numBytes - bytesRead);
            if (inc < 0) {
                throw new IOException("Stream closed while reading.");
            }
            bytesRead += inc;
        }
        
        return buffer;
    }
}
