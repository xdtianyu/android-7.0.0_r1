package com.google.polo.ssl;

import org.bouncycastle.asn1.x509.AuthorityKeyIdentifier;
import org.bouncycastle.asn1.x509.BasicConstraints;
import org.bouncycastle.asn1.x509.ExtendedKeyUsage;
import org.bouncycastle.asn1.x509.GeneralName;
import org.bouncycastle.asn1.x509.GeneralNames;
import org.bouncycastle.asn1.x509.KeyPurposeId;
import org.bouncycastle.asn1.x509.KeyUsage;
import org.bouncycastle.asn1.x509.SubjectKeyIdentifier;
import org.bouncycastle.asn1.x509.X509Extensions;
import org.bouncycastle.asn1.x509.X509Name;
import org.bouncycastle.x509.X509V3CertificateGenerator;
import org.bouncycastle.x509.extension.AuthorityKeyIdentifierStructure;

import java.math.BigInteger;
import java.security.GeneralSecurityException;
import java.security.KeyPair;
import java.security.PublicKey;
import java.security.cert.X509Certificate;
import java.util.Calendar;
import java.util.Date;

/**
 * Utility class to generate X509 Root Certificates and Issue X509 Certificates signed by a root
 * Certificate.
 */
public class CsrUtil {
    private static final String SIGNATURE_ALGORITHM = "SHA256WithRSAEncryption";
    private static final String EMAIL = "android-tv-remote-support@google.com";
    private static final int NOT_BEFORE_NUMBER_OF_DAYS = -30;
    private static final int NOT_AFTER_NUMBER_OF_DAYS = 10 * 365;

    /**
     * Generate a X509 Certificate that should be used as an authority/root certificate only.
     *
     * This certificate shouldn't be used for communications, only as an authority as it won't have
     * the correct flags.
     *
     * @param rootName Common Name used in certificate.
     * @param rootPair Key Pair used to signed the certificate
     * @return
     * @throws GeneralSecurityException
     */
    public static X509Certificate generateX509V3AuthorityCertificate(String rootName,
            KeyPair rootPair)
            throws GeneralSecurityException {
        Calendar calendar = Calendar.getInstance();
        calendar.add(Calendar.DAY_OF_YEAR, NOT_BEFORE_NUMBER_OF_DAYS);
        Date notBefore  = new Date(calendar.getTimeInMillis());
        calendar.add(Calendar.DAY_OF_YEAR, NOT_AFTER_NUMBER_OF_DAYS);
        Date notAfter = new Date(calendar.getTimeInMillis());

        BigInteger serialNumber = BigInteger.valueOf(Math.abs(System.currentTimeMillis()));

        return generateX509V3AuthorityCertificate(rootName, rootPair, notBefore, notAfter, serialNumber);
    }


    @SuppressWarnings("deprecation")
    static X509Certificate generateX509V3AuthorityCertificate(String rootName,
            KeyPair rootPair, Date notBefore, Date notAfter, BigInteger serialNumber)
            throws GeneralSecurityException {
        X509V3CertificateGenerator certGen = new X509V3CertificateGenerator();
        X509Name dnName = new X509Name(rootName);

        certGen.setSerialNumber(serialNumber);
        certGen.setIssuerDN(dnName);
        certGen.setSubjectDN(dnName);
        certGen.setNotBefore(notBefore);
        certGen.setNotAfter(notAfter);
        certGen.setPublicKey(rootPair.getPublic());
        certGen.setSignatureAlgorithm(SIGNATURE_ALGORITHM);

        certGen.addExtension(X509Extensions.BasicConstraints, true,
                new BasicConstraints(0));

        certGen.addExtension(X509Extensions.KeyUsage, true, new KeyUsage(KeyUsage.digitalSignature
                | KeyUsage.keyEncipherment | KeyUsage.keyCertSign));

        AuthorityKeyIdentifier authIdentifier = SslUtil.createAuthorityKeyIdentifier(
                rootPair.getPublic(), dnName, serialNumber);

        certGen.addExtension(X509Extensions.AuthorityKeyIdentifier, true, authIdentifier);
        certGen.addExtension(X509Extensions.SubjectKeyIdentifier, true,
                SubjectKeyIdentifier.getInstance(rootPair.getPublic().getEncoded()));

        certGen.addExtension(X509Extensions.SubjectAlternativeName, false, new GeneralNames(
                new GeneralName(GeneralName.rfc822Name, EMAIL)));

        X509Certificate cert = certGen.generate(rootPair.getPrivate());
        return cert;
    }


    /**
     * Given a public key and an authority certificate and key pair, issue an X509 Certificate
     * chain signed by the provided authority certificate.
     *
     * @param name Common name used in the issued certificate.
     * @param publicKey Public key to use in issued certificate.
     * @param rootCert Root certificate used to issue the new certificate.
     * @param rootPair Root key pair used to issue the new certificate.
     * @return Array containing the issued certificate and the provided root certificate.
     * @throws GeneralSecurityException
     */
    public static X509Certificate[] issueX509V3Certificate(String name, PublicKey publicKey,
            X509Certificate rootCert, KeyPair rootPair) throws GeneralSecurityException {
        Calendar calendar = Calendar.getInstance();
        calendar.add(Calendar.DAY_OF_YEAR, NOT_BEFORE_NUMBER_OF_DAYS);
        Date notBefore  = new Date(calendar.getTimeInMillis());
        calendar.add(Calendar.DAY_OF_YEAR, NOT_AFTER_NUMBER_OF_DAYS);
        Date notAfter = new Date(calendar.getTimeInMillis());

        BigInteger serialNumber = BigInteger.valueOf(Math.abs(System.currentTimeMillis()));

        return issueX509V3Certificate(name, publicKey, rootCert, rootPair, notBefore, notAfter, serialNumber);
    }

    @SuppressWarnings("deprecation")
    static X509Certificate[] issueX509V3Certificate(String name, PublicKey publicKey,
            X509Certificate rootCert, KeyPair rootPair, Date notBefore, Date notAfter,
            BigInteger serialNumber) throws GeneralSecurityException {

        X509V3CertificateGenerator certGen = new X509V3CertificateGenerator();

        X509Name dnName = new X509Name(name);

        certGen.setSerialNumber(serialNumber);
        certGen.setIssuerDN(rootCert.getSubjectX500Principal());
        certGen.setNotBefore(notBefore);
        certGen.setNotAfter(notAfter);
        certGen.setSubjectDN(dnName);
        certGen.setPublicKey(publicKey);
        certGen.setSignatureAlgorithm(SIGNATURE_ALGORITHM);

        // Use Root Certificate as the authority
        certGen.addExtension(X509Extensions.AuthorityKeyIdentifier, false,
                new AuthorityKeyIdentifierStructure(rootCert));
        // Use provided public key for the subject
        certGen.addExtension(X509Extensions.SubjectKeyIdentifier, false,
                SubjectKeyIdentifier.getInstance(publicKey.getEncoded()));
        // This is not a CA certificate, do not allow
        certGen.addExtension(X509Extensions.BasicConstraints, true, new BasicConstraints(false));
        // This can be used for signature and encryption
        certGen.addExtension(X509Extensions.KeyUsage, true, new KeyUsage(KeyUsage.digitalSignature
                | KeyUsage.keyEncipherment));
        // This is used for server authentication
        certGen.addExtension(X509Extensions.ExtendedKeyUsage, true, new ExtendedKeyUsage(
                KeyPurposeId.id_kp_serverAuth));

        X509Certificate issuedCert = certGen.generate(rootPair.getPrivate());

        return new X509Certificate[] { issuedCert, rootCert };
    }
}
