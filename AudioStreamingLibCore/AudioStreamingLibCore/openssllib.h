#ifndef OPENSSLLIB_H
#define OPENSSLLIB_H

#include <QtCore>
#include "common.h"
#include "securebytearray.h"

#include <memory>

#include <openssl/bn.h>
#include <openssl/rsa.h>
#include <openssl/pem.h>
#include <openssl/evp.h>
#include <openssl/aes.h>
#include <openssl/rand.h>
#include <openssl/err.h>

class OpenSslLib : public QObject
{
    Q_OBJECT
public:
    explicit OpenSslLib(QObject *parent = nullptr);
    ~OpenSslLib();

    //Functions
    bool isLoaded();

    static SecureByteArray RANDbytes(int size);

    static bool generateKeys(SecureByteArray *private_key, SecureByteArray *public_key);

    static SecureByteArray publicEncrypt(const SecureByteArray &public_key, const SecureByteArray &data);
    static SecureByteArray privateDecrypt(const SecureByteArray &private_key, const SecureByteArray &encrypted);

    static SecureByteArray privateEncrypt(const SecureByteArray &private_key, const SecureByteArray &data);
    static SecureByteArray publicDecrypt(const SecureByteArray &public_key, const SecureByteArray &encrypted);

    void EncryptInit(const SecureByteArray &password, const SecureByteArray &salt);
    SecureByteArray Encrypt(const SecureByteArray &input);
    void EncryptFinish();

    void DecryptInit(const SecureByteArray &password, const SecureByteArray &salt);
    SecureByteArray Decrypt(const SecureByteArray &input);
    void DecryptFinish();

private:
    //Functions
    void loadFunctions();
    static RSA *createRSA(const SecureByteArray &key, bool ispublic);

    //Variables
    EVP_CIPHER_CTX *enc_ctx;
    EVP_CIPHER_CTX *dec_ctx;

    bool loaded;

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

    struct RSADeleter
    {
        static inline void cleanup(RSA *pointer)
        {
            OpenSslLib openssl;
            if (openssl.isLoaded())
                openssl.pRSA_free(pointer);
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

    typedef RSA*(*tRSA_new)(void);
    tRSA_new pRSA_new;

    typedef void(*tRSA_free)(RSA *r);
    tRSA_free pRSA_free;

    typedef BIGNUM*(*tBN_new)(void);
    tBN_new pBN_new;

    typedef void(*tBN_free)(BIGNUM *a);
    tBN_free pBN_free;

    typedef int(*tBN_set_word)(BIGNUM *a, BN_ULONG w);
    tBN_set_word pBN_set_word;

    typedef int(*tRSA_generate_key_ex)(RSA *rsa, int bits, BIGNUM *e, BN_GENCB *cb);
    tRSA_generate_key_ex pRSA_generate_key_ex;

    typedef RSA*(*tRSAPublicKey_dup)(RSA *rsa);
    tRSAPublicKey_dup pRSAPublicKey_dup;

    typedef RSA*(*tRSAPrivateKey_dup)(RSA *rsa);
    tRSAPrivateKey_dup pRSAPrivateKey_dup;

    typedef BIO*(*tBIO_new)(BIO_METHOD *type);
    tBIO_new pBIO_new;

    typedef BIO_METHOD*(*tBIO_s_mem)(void);
    tBIO_s_mem pBIO_s_mem;

    typedef void(*tBIO_free_all)(BIO *a);
    tBIO_free_all pBIO_free_all;

    typedef int(*tPEM_write_bio_RSAPublicKey)(BIO *bp, RSA *x);
    tPEM_write_bio_RSAPublicKey pPEM_write_bio_RSAPublicKey;

    typedef int(*tBIO_read)(BIO *b, void *data, int len);
    tBIO_read pBIO_read;

    typedef int(*tPEM_write_bio_RSAPrivateKey)(BIO *bp, RSA *x, const EVP_CIPHER *enc,
                                              unsigned char *kstr, int klen,
                                              pem_password_cb *cb, void *u);
    tPEM_write_bio_RSAPrivateKey pPEM_write_bio_RSAPrivateKey;

    typedef BIO*(*tBIO_new_mem_buf)(void *buf, int len);
    tBIO_new_mem_buf pBIO_new_mem_buf;

    typedef RSA*(*tPEM_read_bio_RSAPublicKey)(BIO *bp, RSA **x,
                                              pem_password_cb *cb, void *u);
    tPEM_read_bio_RSAPublicKey pPEM_read_bio_RSAPublicKey;

    typedef RSA *(*tPEM_read_bio_RSAPrivateKey)(BIO *bp, RSA **x,
                                                pem_password_cb *cb, void *u);
    tPEM_read_bio_RSAPrivateKey pPEM_read_bio_RSAPrivateKey;

    typedef int(*tRSA_public_encrypt)(int flen, unsigned char *from,
                                      unsigned char *to, RSA *rsa, int padding);
    tRSA_public_encrypt pRSA_public_encrypt;

    typedef int(*tRSA_private_decrypt)(int flen, unsigned char *from,
                                       unsigned char *to, RSA *rsa, int padding);
    tRSA_private_decrypt pRSA_private_decrypt;

    typedef int(*tRSA_private_encrypt)(int flen, unsigned char *from,
                                        unsigned char *to, RSA *rsa, int padding);
    tRSA_private_encrypt pRSA_private_encrypt;

    typedef int (*tRSA_public_decrypt)(int flen, unsigned char *from,
                                       unsigned char *to, RSA *rsa, int padding);
    tRSA_public_decrypt pRSA_public_decrypt;

    typedef int(*tEVP_BytesToKey)(const EVP_CIPHER *type,const EVP_MD *md,
                                  const unsigned char *salt,
                                  const unsigned char *data, int datal, int count,
                                  unsigned char *key,unsigned char *iv);
    tEVP_BytesToKey pEVP_BytesToKey;

    typedef const EVP_CIPHER*(*tEVP_aes_256_cbc)(void);
    tEVP_aes_256_cbc pEVP_aes_256_cbc;

    typedef const EVP_MD*(*tEVP_whirlpool)(void);
    tEVP_whirlpool pEVP_whirlpool;

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

    typedef long(*tBIO_ctrl)(BIO *bp, int cmd, long larg, void *parg);
    tBIO_ctrl pBIO_ctrl;
};

#endif // OPENSSLLIB_H
