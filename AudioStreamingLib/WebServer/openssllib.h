#ifndef OPENSSLLIB_H
#define OPENSSLLIB_H

#include <QtCore>

#include <memory>

#include <openssl/bn.h>
#include <openssl/evp.h>
#include <openssl/aes.h>
#include <openssl/sha.h>
#include <openssl/rand.h>
#include <openssl/err.h>

class OpenSslLib : public QObject
{
    Q_OBJECT
public:
    explicit OpenSslLib(QObject *parent = nullptr);

    //Functions
    bool isLoaded();
    static QByteArray RANDbytes(int size);
    static QByteArray SHA256(const QByteArray &data, const QByteArray &salt);
    QByteArray encrypt(QByteArray data);
    QByteArray decrypt(QByteArray data);
    void setPassword(const QByteArray &password);

private:
    //Functions
    void loadFunctions();

    void startEncrypt(const QByteArray &salt);
    QByteArray encryptPrivate(const QByteArray &input);
    void stopEncrypt();

    void startDecrypt(const QByteArray &salt);
    QByteArray decryptPrivate(const QByteArray &input);
    void stopDecrypt();

    //Variables
    EVP_CIPHER_CTX *enc_ctx;
    EVP_CIPHER_CTX *dec_ctx;
    QByteArray m_password;

    bool m_loaded;

    //Imports
    struct BIGNUMDeleter
    {
        static inline void cleanup(BIGNUM *pointer)
        {
            OpenSslLib openssl;
            if (openssl.isLoaded())
                openssl.pBN_free(pointer);
        }
    };

    struct BIODeleter
    {
        static inline void cleanup(BIO *pointer)
        {
            OpenSslLib openssl;
            if (openssl.isLoaded())
                openssl.pBIO_free_all(pointer);
        }
    };

    typedef int(*tRAND_bytes)(unsigned char *buf, int num);
    tRAND_bytes pRAND_bytes;

    typedef int (*tSHA256_Init)(SHA256_CTX *c);
    tSHA256_Init pSHA256_Init;

    typedef int (*tSHA256_Update)(SHA256_CTX *c, const void *data, size_t len);
    tSHA256_Update pSHA256_Update;

    typedef int (*tSHA256_Final)(unsigned char *md, SHA256_CTX *c);
    tSHA256_Final pSHA256_Final;

    typedef BIGNUM*(*tBN_new)(void);
    tBN_new pBN_new;

    typedef void(*tBN_free)(BIGNUM *a);
    tBN_free pBN_free;

    typedef int(*tBN_set_word)(BIGNUM *a, BN_ULONG w);
    tBN_set_word pBN_set_word;

    typedef BIO*(*tBIO_new)(BIO_METHOD *type);
    tBIO_new pBIO_new;

    typedef BIO_METHOD*(*tBIO_s_mem)(void);
    tBIO_s_mem pBIO_s_mem;

    typedef void(*tBIO_free_all)(BIO *a);
    tBIO_free_all pBIO_free_all;

    typedef int(*tBIO_read)(BIO *b, void *data, int len);
    tBIO_read pBIO_read;

    typedef int(*tEVP_BytesToKey)(const EVP_CIPHER *type,const EVP_MD *md,
                                  const unsigned char *salt,
                                  const unsigned char *data, int datal, int count,
                                  unsigned char *key,unsigned char *iv);
    tEVP_BytesToKey pEVP_BytesToKey;

    typedef const EVP_CIPHER*(*tEVP_aes_256_cbc)(void);
    tEVP_aes_256_cbc pEVP_aes_256_cbc;

    typedef const EVP_MD*(*tEVP_sha256)(void);
    tEVP_sha256 pEVP_sha256;

    typedef void(*tEVP_CIPHER_CTX_init)(EVP_CIPHER_CTX *a);
    tEVP_CIPHER_CTX_init pEVP_CIPHER_CTX_init;

    typedef int(*tEVP_EncryptInit_ex)(EVP_CIPHER_CTX *ctx, const EVP_CIPHER *cipher,
                                      ENGINE *impl, const unsigned char *key,
                                      const unsigned char *iv);
    tEVP_EncryptInit_ex pEVP_EncryptInit_ex;

    typedef int(*tEVP_EncryptUpdate)(EVP_CIPHER_CTX *ctx, unsigned char *out, int *outl,
                                     const unsigned char *in, int inl);
    tEVP_EncryptUpdate pEVP_EncryptUpdate;

    typedef int(*tEVP_EncryptFinal_ex)(EVP_CIPHER_CTX *ctx, unsigned char *out, int *outl);
    tEVP_EncryptFinal_ex pEVP_EncryptFinal_ex;

    typedef int(*tEVP_CIPHER_CTX_cleanup)(EVP_CIPHER_CTX *a);
    tEVP_CIPHER_CTX_cleanup pEVP_CIPHER_CTX_cleanup;

    typedef int(*tEVP_DecryptInit_ex)(EVP_CIPHER_CTX *ctx, const EVP_CIPHER *cipher,
                                      ENGINE *impl, const unsigned char *key,
                                      const unsigned char *iv);
    tEVP_DecryptInit_ex pEVP_DecryptInit_ex;

    typedef int(*tEVP_DecryptUpdate)(EVP_CIPHER_CTX *ctx, unsigned char *out, int *outl,
                                     const unsigned char *in, int inl);
    tEVP_DecryptUpdate pEVP_DecryptUpdate;

    typedef int(*tEVP_DecryptFinal_ex)(EVP_CIPHER_CTX *ctx, unsigned char *outm, int *outl);
    tEVP_DecryptFinal_ex pEVP_DecryptFinal_ex;
};

#endif // OPENSSLLIB_H
