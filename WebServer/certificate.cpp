#include "certificate.h"

Certificate::Certificate(QObject *parent) : QObject(parent)
{
    m_loaded = false;
    m_has_error = false;

    loadFunctions();
}

void Certificate::loadFunctions()
{
    if (m_loaded)
        return;

#if defined(Q_OS_WIN) || defined(Q_OS_WINPHONE)
    QLibrary openssl_crypto("libeay32");
    QLibrary openssl_ssl("ssleay32");
#else
    QLibrary openssl_crypto("crypto");
    QLibrary openssl_ssl("ssl");
#endif

    if (!openssl_crypto.load())
        return;

    if (!openssl_ssl.load())
        return;

    if (!(pEVP_PKEY_new = (tEVP_PKEY_new)openssl_crypto.resolve("EVP_PKEY_new")))
        return;

    if (!(pRSA_generate_key = (tRSA_generate_key)openssl_crypto.resolve("RSA_generate_key")))
        return;

    if (!(pEVP_PKEY_assign = (tEVP_PKEY_assign)openssl_crypto.resolve("EVP_PKEY_assign")))
        return;

    if (!(pEVP_PKEY_free = (tEVP_PKEY_free)openssl_crypto.resolve("EVP_PKEY_free")))
        return;

    if (!(pX509_new = (tX509_new)openssl_crypto.resolve("X509_new")))
        return;

    if (!(pASN1_INTEGER_set = (tASN1_INTEGER_set)openssl_crypto.resolve("ASN1_INTEGER_set")))
        return;

    if (!(pX509_get_serialNumber = (tX509_get_serialNumber)openssl_crypto.resolve("X509_get_serialNumber")))
        return;

    if (!(pX509_gmtime_adj = (tX509_gmtime_adj)openssl_crypto.resolve("X509_gmtime_adj")))
        return;

    if (!(pX509_set_pubkey = (tX509_set_pubkey)openssl_crypto.resolve("X509_set_pubkey")))
        return;

    if (!(pX509_get_subject_name = (tX509_get_subject_name)openssl_crypto.resolve("X509_get_subject_name")))
        return;

    if (!(pX509_NAME_add_entry_by_txt = (tX509_NAME_add_entry_by_txt)openssl_crypto.resolve("X509_NAME_add_entry_by_txt")))
        return;

    if (!(pX509_set_issuer_name = (tX509_set_issuer_name)openssl_crypto.resolve("X509_set_issuer_name")))
        return;

    if (!(pEVP_sha1 = (tEVP_sha1)openssl_crypto.resolve("EVP_sha1")))
        return;

    if (!(pX509_sign = (tX509_sign)openssl_crypto.resolve("X509_sign")))
        return;

    if (!(pX509_free = (tX509_free)openssl_crypto.resolve("X509_free")))
        return;

    if (!(pPEM_write_PrivateKey = (tPEM_write_PrivateKey)openssl_crypto.resolve("PEM_write_PrivateKey")))
        return;

    if (!(pPEM_write_X509 = (tPEM_write_X509)openssl_crypto.resolve("PEM_write_X509")))
        return;

    if (!(pBIO_new = (tBIO_new)openssl_crypto.resolve("BIO_new")))
        return;

    if (!(pBIO_s_mem = (tBIO_s_mem)openssl_crypto.resolve("BIO_s_mem")))
        return;

    if (!(pBIO_free_all = (tBIO_free_all)openssl_crypto.resolve("BIO_free_all")))
        return;

    if (!(pBIO_read = (tBIO_read)openssl_crypto.resolve("BIO_read")))
        return;

    if (!(pPEM_write_bio_PrivateKey = (tPEM_write_bio_PrivateKey)openssl_crypto.resolve("PEM_write_bio_PrivateKey")))
        return;

    if (!(pPEM_write_bio_X509 = (tPEM_write_bio_X509)openssl_crypto.resolve("PEM_write_bio_X509")))
        return;

    if (!(pBIO_ctrl = (tBIO_ctrl)openssl_crypto.resolve("BIO_ctrl")))
        return;

    if (!(pEVP_des_ede3_cbc = (tEVP_des_ede3_cbc)openssl_crypto.resolve("EVP_des_ede3_cbc")))
        return;

    if (!(pSSL_library_init = (tSSL_library_init)openssl_ssl.resolve("SSL_library_init")))
        return;

    m_loaded = true;
}

/* Generates a 4096-bit RSA key. */
EVP_PKEY *Certificate::generate_key()
{
    if (!m_loaded)
        return nullptr;

    /* Allocate memory for the EVP_PKEY structure. */
    EVP_PKEY *pkey = pEVP_PKEY_new();
    if (!pkey)
    {
        m_error_str.append("Unable to create EVP_PKEY structure.\n");
        m_has_error = true;
        return nullptr;
    }

    /* Generate the RSA key and assign it to pkey. */
    RSA *rsa = pRSA_generate_key(4096, RSA_F4, NULL, NULL);
    if (!pEVP_PKEY_assign_RSA(pkey, rsa))
    {
        m_error_str.append("Unable to generate 4096-bit RSA key.\n");
        m_has_error = true;
        pEVP_PKEY_free(pkey);
        return nullptr;
    }

    /* The key has been generated, return it. */
    return pkey;
}

/* Generates a self-signed x509 certificate. */
X509 *Certificate::generate_x509(EVP_PKEY *pkey)
{
    if (!m_loaded)
        return nullptr;

    /* Allocate memory for the X509 structure. */
    X509 *x509 = pX509_new();
    if (!x509)
    {
        m_error_str.append("Unable to create X509 structure.\n");
        m_has_error = true;
        return nullptr;
    }

    /* Set the serial number. */
    pASN1_INTEGER_set(pX509_get_serialNumber(x509), 1);

    /* This certificate is valid from now until exactly one year from now. */
    pX509_gmtime_adj(X509_get_notBefore(x509), 0);
    pX509_gmtime_adj(X509_get_notAfter(x509), 31536000L);

    /* Set the public key for our certificate. */
    pX509_set_pubkey(x509, pkey);

    /* We want to copy the subject name to the issuer name. */
    X509_NAME *name = pX509_get_subject_name(x509);

    /* Set the country code and common name. */
    pX509_NAME_add_entry_by_txt(name, "C",  MBSTRING_ASC, (unsigned char *)m_country,     -1, -1, 0);
    pX509_NAME_add_entry_by_txt(name, "O",  MBSTRING_ASC, (unsigned char *)m_common_name,  -1, -1, 0);
    pX509_NAME_add_entry_by_txt(name, "CN", MBSTRING_ASC, (unsigned char *)m_domain,      -1, -1, 0);

    /* Now set the issuer name. */
    pX509_set_issuer_name(x509, name);

    /* Actually sign the certificate with our key. */
    if (!pX509_sign(x509, pkey, pEVP_sha1()))
    {
        m_error_str.append("Error signing certificate.\n");
        m_has_error = true;
        pX509_free(x509);
        return nullptr;
    }

    return x509;
}

bool Certificate::generate(const QString &country, const QString &commonname, const QString &domain, const QByteArray &key_password)
{
    m_error_str.clear();
    m_has_error = false;

    if (!m_loaded)
    {
        m_error_str.append("Couldn't load OpenSSL.\n");
        m_has_error = true;
        return false;
    }

    pSSL_library_init();

    {
        QByteArray ba = country.toLatin1();
        qstrcpy(m_country, ba.data());
    }
    {
        QByteArray ba = commonname.toLatin1();
        qstrcpy(m_common_name, ba.data());
    }
    {
        QByteArray ba = domain.toLatin1();
        qstrcpy(m_domain, ba.data());
    }

    /* Generate the key. */
    EVP_PKEY *pkey = generate_key();
    if (!pkey)
        return false;

    /* Generate the certificate. */
    X509 *x509 = generate_x509(pkey);
    if (!x509)
    {
        pEVP_PKEY_free(pkey);
        return false;
    }

    /* Write the private key and certificate out.*/
    FILE *key_file = fopen("../data/key.pem", "wb");

    bool ret1 = pPEM_write_PrivateKey(
        key_file,                               /* write the key to the file we've opened */
        pkey,                                   /* our key from earlier */
        pEVP_des_ede3_cbc(),                    /* default cipher for encrypting the key on disk */
        (unsigned char*)key_password.data(),    /* passphrase required for decrypting the key on disk */
        key_password.size() + 1,                /* length of the passphrase string */
        NULL,                                   /* callback for requesting a password */
        NULL                                    /* data to pass to the callback */
    );

    fclose(key_file);

    FILE *cert_file = fopen("../data/cert.pem", "wb");

    bool ret2 = pPEM_write_X509(
        cert_file,      /* write the certificate to the file we've opened */
        x509            /* our certificate */
    );

    fclose(cert_file);

    pEVP_PKEY_free(pkey);
    pX509_free(x509);

    if (!ret1 || !ret2)
    {
        m_error_str.append("Couldn't store keys!\n");
        m_has_error = true;
        return false;
    }

    return true;
}

bool Certificate::hasError()
{
    return m_has_error;
}

QString Certificate::errorString()
{
    return m_error_str;
}
