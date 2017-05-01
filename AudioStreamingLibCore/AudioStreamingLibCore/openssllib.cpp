#include "openssllib.h"

OpenSslLib::OpenSslLib(QObject *parent) : QObject(parent)
{
    enc_ctx = nullptr;
    dec_ctx = nullptr;

    loaded = false;

    loadFunctions();
}

OpenSslLib::~OpenSslLib()
{
    EncryptFinish();
    DecryptFinish();
}

void OpenSslLib::loadFunctions()
{
    if (loaded)
        return;

#ifndef OPENSSL
#if defined(Q_OS_WIN) || defined(Q_OS_WINPHONE)
    QLibrary openssl("libeay32");
#else
    QLibrary openssl("crypto");
#endif
#endif

#ifndef OPENSSL
    if (!openssl.load())
        return;

    if (!(pRAND_bytes = (tRAND_bytes)openssl.resolve("RAND_bytes")))
        return;

    if (!(pRSA_new = (tRSA_new)openssl.resolve("RSA_new")))
        return;

    if (!(pRSA_free = (tRSA_free)openssl.resolve("RSA_free")))
        return;

    if (!(pBN_new = (tBN_new)openssl.resolve("BN_new")))
        return;

    if (!(pBN_free = (tBN_free)openssl.resolve("BN_free")))
        return;

    if (!(pBN_set_word = (tBN_set_word)openssl.resolve("BN_set_word")))
        return;

    if (!(pRSA_generate_key_ex = (tRSA_generate_key_ex)openssl.resolve("RSA_generate_key_ex")))
        return;

    if (!(pRSAPublicKey_dup = (tRSAPublicKey_dup)openssl.resolve("RSAPublicKey_dup")))
        return;

    if (!(pRSAPrivateKey_dup = (tRSAPrivateKey_dup)openssl.resolve("RSAPrivateKey_dup")))
        return;

    if (!(pBIO_new = (tBIO_new)openssl.resolve("BIO_new")))
        return;

    if (!(pBIO_s_mem = (tBIO_s_mem)openssl.resolve("BIO_s_mem")))
        return;

    if (!(pBIO_free_all = (tBIO_free_all)openssl.resolve("BIO_free_all")))
        return;

    if (!(pPEM_write_bio_RSAPublicKey = (tPEM_write_bio_RSAPublicKey)openssl.resolve("PEM_write_bio_RSAPublicKey")))
        return;

    if (!(pBIO_read = (tBIO_read)openssl.resolve("BIO_read")))
        return;

    if (!(pPEM_write_bio_RSAPrivateKey = (tPEM_write_bio_RSAPrivateKey)openssl.resolve("PEM_write_bio_RSAPrivateKey")))
        return;

    if (!(pBIO_new_mem_buf = (tBIO_new_mem_buf)openssl.resolve("BIO_new_mem_buf")))
        return;

    if (!(pPEM_read_bio_RSAPublicKey = (tPEM_read_bio_RSAPublicKey)openssl.resolve("PEM_read_bio_RSAPublicKey")))
        return;

    if (!(pPEM_read_bio_RSAPrivateKey = (tPEM_read_bio_RSAPrivateKey)openssl.resolve("PEM_read_bio_RSAPrivateKey")))
        return;

    if (!(pRSA_public_encrypt = (tRSA_public_encrypt)openssl.resolve("RSA_public_encrypt")))
        return;

    if (!(pRSA_private_decrypt = (tRSA_private_decrypt)openssl.resolve("RSA_private_decrypt")))
        return;

    if (!(pRSA_private_encrypt = (tRSA_private_encrypt)openssl.resolve("RSA_private_encrypt")))
        return;

    if (!(pRSA_public_decrypt = (tRSA_public_decrypt)openssl.resolve("RSA_public_decrypt")))
        return;

    if (!(pEVP_BytesToKey = (tEVP_BytesToKey)openssl.resolve("EVP_BytesToKey")))
        return;

    if (!(pEVP_aes_256_cbc = (tEVP_aes_256_cbc)openssl.resolve("EVP_aes_256_cbc")))
        return;

    if (!(pEVP_whirlpool = (tEVP_whirlpool)openssl.resolve("EVP_whirlpool")))
        return;

    if (!(pEVP_CIPHER_CTX_init = (tEVP_CIPHER_CTX_init)openssl.resolve("EVP_CIPHER_CTX_init")))
        return;

    if (!(pEVP_EncryptInit_ex = (tEVP_EncryptInit_ex)openssl.resolve("EVP_EncryptInit_ex")))
        return;

    if (!(pEVP_EncryptUpdate = (tEVP_EncryptUpdate)openssl.resolve("EVP_EncryptUpdate")))
        return;

    if (!(pEVP_EncryptFinal_ex = (tEVP_EncryptFinal_ex)openssl.resolve("EVP_EncryptFinal_ex")))
        return;

    if (!(pEVP_CIPHER_CTX_cleanup = (tEVP_CIPHER_CTX_cleanup)openssl.resolve("EVP_CIPHER_CTX_cleanup")))
        return;

    if (!(pEVP_DecryptInit_ex = (tEVP_DecryptInit_ex)openssl.resolve("EVP_DecryptInit_ex")))
        return;

    if (!(pEVP_DecryptUpdate = (tEVP_DecryptUpdate)openssl.resolve("EVP_DecryptUpdate")))
        return;

    if (!(pEVP_DecryptFinal_ex = (tEVP_DecryptFinal_ex)openssl.resolve("EVP_DecryptFinal_ex")))
        return;

    if (!(pBIO_ctrl = (tBIO_ctrl)openssl.resolve("BIO_ctrl")))
        return;
#else
    pRAND_bytes = (tRAND_bytes)&RAND_bytes;

    pRSA_new = (tRSA_new)&RSA_new;

    pRSA_free = (tRSA_free)&RSA_free;

    pBN_new = (tBN_new)&BN_new;

    pBN_free = (tBN_free)&BN_free;

    pBN_set_word = (tBN_set_word)&BN_set_word;

    pRSA_generate_key_ex = (tRSA_generate_key_ex)&RSA_generate_key_ex;

    pRSAPublicKey_dup = (tRSAPublicKey_dup)&RSAPublicKey_dup;

    pRSAPrivateKey_dup = (tRSAPrivateKey_dup)&RSAPrivateKey_dup;

    pBIO_new = (tBIO_new)&BIO_new;

    pBIO_s_mem = (tBIO_s_mem)&BIO_s_mem;

    pBIO_free_all = (tBIO_free_all)&BIO_free_all;

    pPEM_write_bio_RSAPublicKey = (tPEM_write_bio_RSAPublicKey)&PEM_write_bio_RSAPublicKey;

    pBIO_read = (tBIO_read)&BIO_read;

    pPEM_write_bio_RSAPrivateKey = (tPEM_write_bio_RSAPrivateKey)&PEM_write_bio_RSAPrivateKey;

    pBIO_new_mem_buf = (tBIO_new_mem_buf)&BIO_new_mem_buf;

    pPEM_read_bio_RSAPublicKey = (tPEM_read_bio_RSAPublicKey)&PEM_read_bio_RSAPublicKey;

    pPEM_read_bio_RSAPrivateKey = (tPEM_read_bio_RSAPrivateKey)&PEM_read_bio_RSAPrivateKey;

    pRSA_public_encrypt = (tRSA_public_encrypt)&RSA_public_encrypt;

    pRSA_private_decrypt = (tRSA_private_decrypt)&RSA_private_decrypt;

    pRSA_private_encrypt = (tRSA_private_encrypt)&RSA_private_encrypt;

    pRSA_public_decrypt = (tRSA_public_decrypt)&RSA_public_decrypt;

    pEVP_BytesToKey = (tEVP_BytesToKey)&EVP_BytesToKey;

    pEVP_aes_256_cbc = (tEVP_aes_256_cbc)&EVP_aes_256_cbc;

    pEVP_whirlpool = (tEVP_whirlpool)&EVP_whirlpool;

    pEVP_CIPHER_CTX_init = (tEVP_CIPHER_CTX_init)&EVP_CIPHER_CTX_init;

    pEVP_EncryptInit_ex = (tEVP_EncryptInit_ex)&EVP_EncryptInit_ex;

    pEVP_EncryptUpdate = (tEVP_EncryptUpdate)&EVP_EncryptUpdate;

    pEVP_EncryptFinal_ex = (tEVP_EncryptFinal_ex)&EVP_EncryptFinal_ex;

    pEVP_CIPHER_CTX_cleanup = (tEVP_CIPHER_CTX_cleanup)&EVP_CIPHER_CTX_cleanup;

    pEVP_DecryptInit_ex = (tEVP_DecryptInit_ex)&EVP_DecryptInit_ex;

    pEVP_DecryptUpdate = (tEVP_DecryptUpdate)&EVP_DecryptUpdate;

    pEVP_DecryptFinal_ex = (tEVP_DecryptFinal_ex)&EVP_DecryptFinal_ex;

    pBIO_ctrl = (tBIO_ctrl)&BIO_ctrl;
#endif
    loaded = true;
}

bool OpenSslLib::isLoaded()
{
    return loaded;
}

SecureByteArray OpenSslLib::RANDbytes(int size)
{
    OpenSslLib ssl;

    if (!ssl.loaded)
        return SecureByteArray();

    SecureByteArray output;
    output.resize(size);

    ssl.pRAND_bytes((uchar*)output.data(), size);

    return output;
}

bool OpenSslLib::generateKeys(SecureByteArray *private_key, SecureByteArray *public_key)
{
    OpenSslLib ssl;

    if (!ssl.loaded)
        return false;

    int rc;

    QScopedPointer<BIGNUM, BIGNUMDeleter> bn(ssl.pBN_new());
    QScopedPointer<RSA, RSADeleter> rsa(ssl.pRSA_new());

    rc = ssl.pBN_set_word(bn.data(), RSA_F4);

    if (!rc)
        return false;

    rc = ssl.pRSA_generate_key_ex(rsa.data(), 4096, bn.data(), NULL);

    if (!rc)
        return false;

    QScopedPointer<RSA, RSADeleter> rsa_priv(ssl.pRSAPrivateKey_dup(rsa.data()));
    QScopedPointer<RSA, RSADeleter> rsa_pub(ssl.pRSAPrivateKey_dup(ssl.pRSAPublicKey_dup(rsa.data())));

    QScopedPointer<BIO, BIODeleter> bio_private(ssl.pBIO_new(ssl.pBIO_s_mem()));
    QScopedPointer<BIO, BIODeleter> bio_public(ssl.pBIO_new(ssl.pBIO_s_mem()));

    //Private key

    ssl.pPEM_write_bio_RSAPrivateKey(bio_private.data(), rsa.data(), NULL, NULL, 0, NULL, NULL);

    private_key->resize((int)ssl.pBIO_ctrl(bio_private.data(), BIO_CTRL_PENDING, 0, NULL));

    ssl.pBIO_read(bio_private.data(), private_key->data(), private_key->size());

    //Public key

    ssl.pPEM_write_bio_RSAPublicKey(bio_public.data(), rsa.data());

    public_key->resize((int)ssl.pBIO_ctrl(bio_public.data(), BIO_CTRL_PENDING, 0, NULL));

    ssl.pBIO_read(bio_public.data(), public_key->data(), public_key->size());

    return true;
}

RSA *OpenSslLib::createRSA(const SecureByteArray &key, bool ispublic)
{
    OpenSslLib ssl;

    if (!ssl.loaded)
        return nullptr;

    RSA *rsa = nullptr;
    QScopedPointer<BIO, BIODeleter> keybio(ssl.pBIO_new_mem_buf((char*)key.constData(), key.size()));

    if (!keybio.data())
        return nullptr;

    if (ispublic)
        rsa = ssl.pPEM_read_bio_RSAPublicKey(keybio.data(), &rsa, NULL, NULL);
    else
        rsa = ssl.pPEM_read_bio_RSAPrivateKey(keybio.data(), &rsa, NULL, NULL);

    return rsa;
}

SecureByteArray OpenSslLib::publicEncrypt(const SecureByteArray &public_key, const SecureByteArray &data)
{
    OpenSslLib ssl;

    if (!ssl.loaded)
        return SecureByteArray();

    QScopedPointer<RSA, RSADeleter> rsa(createRSA(public_key, true));

    if (!rsa.data())
        return SecureByteArray();

    SecureByteArray output;
    output.resize(512);

    int result = ssl.pRSA_public_encrypt(data.size(), (uchar*)data.data(), (uchar*)output.data(), rsa.data(), RSA_PKCS1_PADDING);

    if (result >= 0)
        output.resize(result);
    else
        output = SecureByteArray();

    return output;
}

SecureByteArray OpenSslLib::privateDecrypt(const SecureByteArray &private_key, const SecureByteArray &encrypted)
{
    OpenSslLib ssl;

    if (!ssl.loaded)
        return SecureByteArray();

    QScopedPointer<RSA, RSADeleter> rsa(createRSA(private_key, false));

    if (!rsa.data())
        return SecureByteArray();

    SecureByteArray output;
    output.resize(512);

    int result = ssl.pRSA_private_decrypt(encrypted.size(), (uchar*)encrypted.data(), (uchar*)output.data(), rsa.data(), RSA_PKCS1_PADDING);

    if (result >= 0)
        output.resize(result);
    else
        output = SecureByteArray();

    return output;
}

SecureByteArray OpenSslLib::privateEncrypt(const SecureByteArray &private_key, const SecureByteArray &data)
{
    OpenSslLib ssl;

    if (!ssl.loaded)
        return SecureByteArray();

    QScopedPointer<RSA, RSADeleter> rsa(createRSA(private_key, false));

    if (!rsa.data())
        return SecureByteArray();

    SecureByteArray output;
    output.resize(512);

    int result = ssl.pRSA_private_encrypt(data.size(), (uchar*)data.data(), (uchar*)output.data(), rsa.data(), RSA_PKCS1_PADDING);

    if (result >= 0)
        output.resize(result);
    else
        output = SecureByteArray();

    return output;
}

SecureByteArray OpenSslLib::publicDecrypt(const SecureByteArray &public_key, const SecureByteArray &encrypted)
{
    OpenSslLib ssl;

    if (!ssl.loaded)
        return SecureByteArray();

    QScopedPointer<RSA, RSADeleter> rsa(createRSA(public_key, true));

    if (!rsa.data())
        return SecureByteArray();

    SecureByteArray output;
    output.resize(512);

    int result = ssl.pRSA_public_decrypt(encrypted.size(), (uchar*)encrypted.data(), (uchar*)output.data(), rsa.data(), RSA_PKCS1_PADDING);

    if (result >= 0)
        output.resize(result);
    else
        output = SecureByteArray();

    return output;
}

void OpenSslLib::EncryptInit(const SecureByteArray &password, const SecureByteArray &salt)
{
    if (!loaded || enc_ctx)
        return;

    enc_ctx = new EVP_CIPHER_CTX;

    uchar *key_data;
    int key_data_len;

    key_data = (uchar*)password.constData();
    key_data_len = password.size();

    int i, nrounds = qPow(10, 4);
    uchar key[32], iv[32];

    i = pEVP_BytesToKey(pEVP_aes_256_cbc(), pEVP_whirlpool(), (uchar*)salt.constData(), key_data, key_data_len, nrounds, key, iv);

    if (i != 32)
        return;

    pEVP_CIPHER_CTX_init(enc_ctx);
    pEVP_EncryptInit_ex(enc_ctx, pEVP_aes_256_cbc(), NULL, key, iv);
}

SecureByteArray OpenSslLib::Encrypt(const SecureByteArray &input)
{
    if (!loaded || !enc_ctx)
        return SecureByteArray();

    SecureByteArray preencrypted;
    preencrypted.append(getBytes(qChecksum(input.data(), input.size())));
    preencrypted.append(RANDbytes(4));
    preencrypted.append(input);

    int len = preencrypted.size();

    int c_len = len + AES_BLOCK_SIZE, f_len = 0;
    uchar *cipherdata = new uchar[c_len];

    pEVP_EncryptInit_ex(enc_ctx, NULL, NULL, NULL, NULL);
    pEVP_EncryptUpdate(enc_ctx, cipherdata, &c_len, (uchar*)preencrypted.constData(), len);
    pEVP_EncryptFinal_ex(enc_ctx, cipherdata + c_len, &f_len);

    SecureByteArray output = SecureByteArray((char*)cipherdata, c_len + f_len);

    delete[] cipherdata;

    return output;
}

void OpenSslLib::EncryptFinish()
{
    if (!loaded || !enc_ctx)
        return;

    pEVP_CIPHER_CTX_cleanup(enc_ctx);
    delete enc_ctx;
    enc_ctx = nullptr;
}

void OpenSslLib::DecryptInit(const SecureByteArray &password, const SecureByteArray &salt)
{
    if (!loaded || dec_ctx)
        return;

    dec_ctx = new EVP_CIPHER_CTX;

    uchar *key_data;
    int key_data_len;

    key_data = (uchar*)password.constData();
    key_data_len = password.size();

    int i, nrounds = qPow(10, 4);
    uchar key[32], iv[32];

    i = pEVP_BytesToKey(pEVP_aes_256_cbc(), pEVP_whirlpool(), (uchar*)salt.constData(), key_data, key_data_len, nrounds, key, iv);

    if (i != 32)
        return;

    pEVP_CIPHER_CTX_init(dec_ctx);
    pEVP_DecryptInit_ex(dec_ctx, pEVP_aes_256_cbc(), NULL, key, iv);
}

SecureByteArray OpenSslLib::Decrypt(const SecureByteArray &input)
{
    if (!loaded || !dec_ctx)
        return SecureByteArray();

    int len = input.size();

    int p_len = len, f_len = 0;
    uchar *plaindata = new uchar[p_len + AES_BLOCK_SIZE];

    pEVP_DecryptInit_ex(dec_ctx, NULL, NULL, NULL, NULL);
    pEVP_DecryptUpdate(dec_ctx, plaindata, &p_len, (uchar*)input.constData(), len);
    pEVP_DecryptFinal_ex(dec_ctx, plaindata + p_len, &f_len);

    SecureByteArray output = SecureByteArray((char*)plaindata, p_len + f_len);

    delete[] plaindata;

    if (output.size() < 6)
        return SecureByteArray();

    SecureByteArray crc16 = output.mid(0, 2);
    output.remove(0, 6);

    quint16 checksum = getValue<quint16>(crc16);
    quint16 currentchecksum = qChecksum(output.data(), output.size());

    if (checksum != currentchecksum)
        output = SecureByteArray();

    return output;
}

void OpenSslLib::DecryptFinish()
{
    if (!loaded || !dec_ctx)
        return;

    pEVP_CIPHER_CTX_cleanup(dec_ctx);
    delete dec_ctx;
    dec_ctx = nullptr;
}
