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

import java.security.MessageDigest;
import java.security.NoSuchAlgorithmException;
import java.security.PublicKey;
import java.security.cert.Certificate;
import java.security.interfaces.RSAPublicKey;
import java.util.Arrays;

/**
 * Class to represent the out-of-band secret transmitted during pairing.
 */
public class PoloChallengeResponse {
  
  /**
   * Hash algorithm to generate secret.
   */
  private static final String HASH_ALGORITHM = "SHA-256";
  
  /**
   * Optional handler for debug log messages.
   */
  private DebugLogger mLogger;

  /**
   * Certificate of the local peer in the protocol.
   */
  private Certificate mClientCertificate;
  
  /**
   * Certificate of the remote peer in the protocol.
   */
  private Certificate mServerCertificate;
  
  /**
   * Creates a new callenge-response generator object.
   * 
   * @param clientCert  the certificate of the client node
   * @param serverCert  the certificate of the server node
   * @param logger      a listener for debugging messages; may be null
   */
  public PoloChallengeResponse(Certificate clientCert, Certificate serverCert,
      DebugLogger logger) {
    mClientCertificate = clientCert;
    mServerCertificate = serverCert;
    mLogger = logger;
  }
  
  /**
   * Returns the alpha value to be used in pairing.
   * <p>
   * From the Polo design document, `alpha` is the value h(K_a | K_b | R_a):
   * for an RSA public key, that is:
   * <ul>
   * <li>the client key's modulus,</li>
   * <li>the client key's public exponent,</li>
   * <li>the server key's modulus,</li>
   * <li>the server key's public exponent,</li>
   * <li>the random nonce.</li>
   * 
   * @param   nonce          the nonce to use for computation
   * @return                 the alpha value, as a byte array
   * @throws  PoloException  if the secret could not be computed
   */
  public byte[] getAlpha(byte[] nonce) throws PoloException {
    PublicKey clientPubKey = mClientCertificate.getPublicKey();
    PublicKey serverPubKey = mServerCertificate.getPublicKey();

    logDebug("getAlpha, nonce=" + PoloUtil.bytesToHexString(nonce));
    
    if (!(clientPubKey instanceof RSAPublicKey) ||
        !(serverPubKey instanceof RSAPublicKey)) {
      throw new PoloException("Polo only supports RSA public keys");
    }

    RSAPublicKey clientPubRsa = (RSAPublicKey) clientPubKey;
    RSAPublicKey serverPubRsa = (RSAPublicKey) serverPubKey;
    
    MessageDigest digest;
    try {
      digest = MessageDigest.getInstance(HASH_ALGORITHM);
    } catch (NoSuchAlgorithmException e) {
      throw new PoloException("Could not get digest algorithm", e);
    }
    
    byte[] digestBytes;
    byte[] clientModulus = clientPubRsa.getModulus().abs().toByteArray();
    byte[] clientExponent =
        clientPubRsa.getPublicExponent().abs().toByteArray();
    byte[] serverModulus = serverPubRsa.getModulus().abs().toByteArray();
    byte[] serverExponent =
        serverPubRsa.getPublicExponent().abs().toByteArray();
    
    // Per "Polo Implementation Overview", section 6.1, leading null bytes must
    // be removed prior to hashing the key material.
    clientModulus = removeLeadingNullBytes(clientModulus);
    clientExponent = removeLeadingNullBytes(clientExponent);
    serverModulus = removeLeadingNullBytes(serverModulus);
    serverExponent = removeLeadingNullBytes(serverExponent);
    
    logVerbose("Hash inputs, in order: ");
    logVerbose("   client modulus: " + PoloUtil.bytesToHexString(clientModulus));
    logVerbose("  client exponent: " + PoloUtil.bytesToHexString(clientExponent));
    logVerbose("   server modulus: " + PoloUtil.bytesToHexString(serverModulus));
    logVerbose("  server exponent: " + PoloUtil.bytesToHexString(serverExponent));
    logVerbose("            nonce: " + PoloUtil.bytesToHexString(nonce));

    // Per "Polo Implementation Overview", section 6.1, client key material is
    // hashed first, followed by the server key material, followed by the
    // nonce.
    digest.update(clientModulus);
    digest.update(clientExponent);
    digest.update(serverModulus);
    digest.update(serverExponent);
    digest.update(nonce);
    
    digestBytes = digest.digest();
    logDebug("Generated hash: " + PoloUtil.bytesToHexString(digestBytes));
    return digestBytes;
  }
    
  /**
   * Returns the gamma value to be used in pairing, i.e. the concatenation
   * of the alpha value with the nonce.
   * <p>
   * The returned value with be twice the byte length of the nonce.
   * 
   * @throws PoloException  if the secret could not be computed
   */
  public byte[] getGamma(byte[] nonce) throws PoloException {
      byte[] alphaBytes = getAlpha(nonce);
      assert(alphaBytes.length >= nonce.length);
      
      byte[] result = new byte[nonce.length * 2];
      
      System.arraycopy(alphaBytes, 0, result, 0, nonce.length);
      System.arraycopy(nonce, 0, result, nonce.length, nonce.length);
      
      return result;
  }
  
  /**
   * Extracts and returns the nonce portion of a given gamma value.
   */
  public byte[] extractNonce(byte[] gamma) {
    if ((gamma.length < 2) || (gamma.length % 2 != 0)) {
      throw new IllegalArgumentException();
    }
    int nonceLength = gamma.length / 2;
    byte[] nonce = new byte[nonceLength];
    System.arraycopy(gamma, nonceLength, nonce, 0, nonceLength);
    return nonce;
  }
  
  /**
   * Returns {@code true} if the gamma value matches the locally computed value.
   * <p>
   * The computed value is determined by extracting the nonce portion of the
   * gamma value.
   * 
   * @throws PoloException  if the value could not be computed
   */
  public boolean checkGamma(byte[] gamma) throws PoloException {
    
    byte[] nonce;
    try {
      nonce = extractNonce(gamma);
    } catch (IllegalArgumentException e) {
      logDebug("Illegal nonce value.");
      return false;
    }
    logDebug("Nonce is: " + PoloUtil.bytesToHexString(nonce));
    logDebug("User gamma is: " + PoloUtil.bytesToHexString(gamma));
    logDebug("Generated gamma is: " + PoloUtil.bytesToHexString(getGamma(nonce)));
    return Arrays.equals(gamma, getGamma(nonce));
  }
  
  /**
   * Strips leading null bytes from a byte array, returning a new copy.
   * <p>
   * As a special case, if the input array consists entirely of null bytes,
   * then an array with a single null element will be returned.
   */
  private byte[] removeLeadingNullBytes(byte[] inArray) {
    int offset = 0;
    while (offset < inArray.length & inArray[offset] == 0) {
      offset += 1;
    }
    byte[] result = new byte[inArray.length - offset];
    for (int i=offset; i < inArray.length; i++) {
      result[i - offset] = inArray[i];
    }
    return result;
  }

  private void logDebug(String message) {
    if (mLogger != null) {
      mLogger.debug(message);
    }
  }
  
  private void logVerbose(String message) {
    if (mLogger != null) {
      mLogger.verbose(message);
    }
  }
  
  public static interface DebugLogger {
    /**
     * Logs debugging information from challenge-response generation.
     */
    public void debug(String message);
    
    /**
     * Logs verbose debugging information from challenge-response generation.
     */
    public void verbose(String message);
    
  }
  
}
