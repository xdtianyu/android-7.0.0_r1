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

package com.google.polo.pairing;

import com.google.polo.exception.PoloException;

import java.math.BigInteger;
import java.security.cert.Certificate;

import javax.net.ssl.SSLPeerUnverifiedException;
import javax.net.ssl.SSLSession;

/**
 * Utility methods of general usefulness to the Polo library.
 */
public class PoloUtil {

  /**
   * Returns the peer {@link Certificate} for an {@link SSLSession}.
   * 
   * @throws PoloException  if the peer certificate could not be obtained
   *                        from the {@link SSLSession}.
   * @return                the {@link Certificate} of the peer
   */
  public static Certificate getPeerCert(SSLSession session)
      throws PoloException {
    try {      
      // Peer certificate
      Certificate[] certs = session.getPeerCertificates();
      if (certs == null || certs.length < 1) {
        throw new PoloException("No peer certificate.");
      }
      return certs[0];
    } catch (SSLPeerUnverifiedException e) {
      throw new PoloException(e);
    }
  }
  
  /**
   * Return the local {@link Certificate} for an {@link SSLSession}.
   * 
   * @throws PoloException  if the local certificate could not be obtained
   *                        from the {@link SSLSession}
   * @return                the {@link Certificate} of the peer
   */
  public static Certificate getLocalCert(SSLSession session)
      throws PoloException {
    Certificate[] certs = session.getLocalCertificates();
    if (certs == null || certs.length < 1) {
      throw new PoloException("No local certificate.");
    }
    return certs[0];
  }
  
  /**
   * Converts an array of bytes to a string of hexadecimal characters.
   * Leading null bytes are preserved in the output.
   * <p>
   * The input byte stream is assumed to be a positive, two's complement
   * representation of an integer.  The return value is the hexadecimal string
   * representation of this value.
   * 
   * @param bytes  the bytes to convert
   * @return       the string representation
   */
  public static String bytesToHexString(byte[] bytes) {
    if (bytes == null || bytes.length == 0) {
      return "";
    }
    BigInteger bigint = new BigInteger(1, bytes);
    int formatLen = bytes.length * 2;
    return String.format("%0" + formatLen + "x", bigint);
  }
  
  /**
   * Converts a string of hex characters to a byte array.
   * 
   * @param hexstr  the string of hex characters
   * @return        a byte array representation
   */
  public static byte[] hexStringToBytes(String hexstr) {
    if (hexstr == null || hexstr.length() == 0 || (hexstr.length() % 2) != 0) {
      throw new IllegalArgumentException("Bad input string.");
    }
    
    byte[] result = new byte[hexstr.length() / 2];
    for (int i=0; i < result.length; i++) {
      result[i] = (byte) Integer.parseInt(hexstr.substring(2 * i, 2 * (i + 1)),
          16);
    }
    return result;
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
   * Converts a 4-byte array of bytes to an unsigned long value.
   */
  public static final long intBigEndianBytesToLong(byte[] input) {
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

}
