#ifndef CERTIFICATE_H
#define CERTIFICATE_H

#include <QtCore>

#include <cstdio>
#include <cstdlib>
#include <stdio.h>

#include <memory>

#include <openssl/pem.h>
#include <openssl/x509.h>

#define pEVP_PKEY_assign_RSA(pkey,rsa) \
pEVP_PKEY_assign((pkey),EVP_PKEY_RSA,reinterpret_cast<char*>(rsa))

class Certificate : public QObject
{
    Q_OBJECT
public:
    explicit Certificate(QObject *parent = nullptr);

public slots:
    bool generate(const QString &country, const QString &commonname, const QString &domain, const QByteArray &key_password = QByteArray());

    bool hasError();
    QString errorString();

private:
    //Functions
    void loadFunctions();

    EVP_PKEY *generate_key();
    X509 *generate_x509(EVP_PKEY *pkey);

    //Variables
    bool m_has_error;
    QString m_error_str;

    char m_country[255];
    char m_common_name[255];
    char m_domain[255];

    bool m_loaded;

    //Imports
    typedef EVP_PKEY *(*tEVP_PKEY_new)();
    tEVP_PKEY_new pEVP_PKEY_new;

    typedef const EVP_CIPHER *(*tEVP_des_ede3_cbc)();
    tEVP_des_ede3_cbc pEVP_des_ede3_cbc;

    typedef int (*tSSL_library_init)();
    tSSL_library_init pSSL_library_init;

    typedef RSA *(*tRSA_generate_key)(int bits, unsigned long e, void (*callback) (int, int, void *), void *cb_arg);
    tRSA_generate_key pRSA_generate_key;

    typedef int (*tEVP_PKEY_assign)(EVP_PKEY *pkey, int type, void *key);
    tEVP_PKEY_assign pEVP_PKEY_assign;

    typedef void (*tEVP_PKEY_free)(EVP_PKEY *pkey);
    tEVP_PKEY_free pEVP_PKEY_free;

    typedef X509 *(*tX509_new)();
    tX509_new pX509_new;

    typedef int (*tASN1_INTEGER_set)(ASN1_INTEGER *a, long v);
    tASN1_INTEGER_set pASN1_INTEGER_set;

    typedef ASN1_INTEGER *(*tX509_get_serialNumber)(X509 *x);
    tX509_get_serialNumber pX509_get_serialNumber;

    typedef ASN1_TIME *(*tX509_gmtime_adj)(ASN1_TIME *s, long adj);
    tX509_gmtime_adj pX509_gmtime_adj;

    typedef int (*tX509_set_pubkey)(X509 *x, EVP_PKEY *pkey);
    tX509_set_pubkey pX509_set_pubkey;

    typedef X509_NAME *(*tX509_get_subject_name)(X509 *a);
    tX509_get_subject_name pX509_get_subject_name;

    typedef int (*tX509_NAME_add_entry_by_txt)(X509_NAME *name, const char *field, int type,
                                               const unsigned char *bytes, int len, int loc,
                                               int set);
    tX509_NAME_add_entry_by_txt pX509_NAME_add_entry_by_txt;

    typedef int (*tX509_set_issuer_name)(X509 *x, X509_NAME *name);
    tX509_set_issuer_name pX509_set_issuer_name;

    typedef const EVP_MD *(*tEVP_sha1)(void);
    tEVP_sha1 pEVP_sha1;

    typedef int (*tX509_sign)(X509 *x, EVP_PKEY *pkey, const EVP_MD *md);
    tX509_sign pX509_sign;

    typedef void (*tX509_free)(X509 *a);
    tX509_free pX509_free;

    typedef int (*tPEM_write_PrivateKey)(FILE *fp, EVP_PKEY *x,
                                         const EVP_CIPHER *enc, unsigned char *kstr, int klen,
                                         pem_password_cb *cb, void *u);
    tPEM_write_PrivateKey pPEM_write_PrivateKey;

    typedef int (*tPEM_write_X509)(FILE *fp, X509 *x);
    tPEM_write_X509 pPEM_write_X509;

    typedef BIO*(*tBIO_new)(BIO_METHOD *type);
    tBIO_new pBIO_new;

    typedef BIO_METHOD*(*tBIO_s_mem)(void);
    tBIO_s_mem pBIO_s_mem;

    typedef void(*tBIO_free_all)(BIO *a);
    tBIO_free_all pBIO_free_all;

    typedef int(*tBIO_read)(BIO *b, void *data, int len);
    tBIO_read pBIO_read;

    typedef int(*tPEM_write_bio_PrivateKey)(BIO *bp, EVP_PKEY *x, const EVP_CIPHER *enc,
                                            unsigned char *kstr, int klen,
                                            pem_password_cb *cb, void *u);
    tPEM_write_bio_PrivateKey pPEM_write_bio_PrivateKey;

    typedef int(*tPEM_write_bio_X509)(BIO *bp, X509 *x);
    tPEM_write_bio_X509 pPEM_write_bio_X509;

    typedef long(*tBIO_ctrl)(BIO *bp, int cmd, long larg, void *parg);
    tBIO_ctrl pBIO_ctrl;

    typedef ASN1_TIME *(*tX509_getm_notBefore)(const X509 *x);
    tX509_getm_notBefore pX509_getm_notBefore;

    typedef ASN1_TIME *(*tX509_getm_notAfter)(const X509 *x);
    tX509_getm_notAfter pX509_getm_notAfter;
};

#endif // CERTIFICATE_H
