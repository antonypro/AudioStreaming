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

    if (!(pSHA256_Init = (tSHA256_Init)openssl.resolve("SHA256_Init")))
        return;

    if (!(pSHA256_Update = (tSHA256_Update)openssl.resolve("SHA256_Update")))
        return;

    if (!(pSHA256_Final = (tSHA256_Final)openssl.resolve("SHA256_Final")))
        return;

    if (!(pBN_new = (tBN_new)openssl.resolve("BN_new")))
        return;

    if (!(pBN_free = (tBN_free)openssl.resolve("BN_free")))
        return;

    if (!(pBN_set_word = (tBN_set_word)openssl.resolve("BN_set_word")))
        return;

    if (!(pBIO_new = (tBIO_new)openssl.resolve("BIO_new")))
        return;

    if (!(pBIO_s_mem = (tBIO_s_mem)openssl.resolve("BIO_s_mem")))
        return;

    if (!(pBIO_free_all = (tBIO_free_all)openssl.resolve("BIO_free_all")))
        return;

    if (!(pBIO_read = (tBIO_read)openssl.resolve("BIO_read")))
        return;

    if (!(pEVP_BytesToKey = (tEVP_BytesToKey)openssl.resolve("EVP_BytesToKey")))
        return;

    if (!(pEVP_aes_256_cbc = (tEVP_aes_256_cbc)openssl.resolve("EVP_aes_256_cbc")))
        return;

    if (!(pEVP_sha256 = (tEVP_sha256)openssl.resolve("EVP_sha256")))
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
#else
    pRAND_bytes = (tRAND_bytes)&RAND_bytes;

    pSHA256_Init = (tSHA256_Init)&SHA256_Init;

    pSHA256_Update = (tSHA256_Update)&SHA256_Update;

    pSHA256_Final = (tSHA256_Final)&SHA256_Final;

    pBN_new = (tBN_new)&BN_new;

    pBN_free = (tBN_free)&BN_free;

    pBN_set_word = (tBN_set_word)&BN_set_word;

    pBIO_new = (tBIO_new)&BIO_new;

    pBIO_s_mem = (tBIO_s_mem)&BIO_s_mem;

    pBIO_free_all = (tBIO_free_all)&BIO_free_all;

    pBIO_read = (tBIO_read)&BIO_read;

    pEVP_BytesToKey = (tEVP_BytesToKey)&EVP_BytesToKey;

    pEVP_aes_256_cbc = (tEVP_aes_256_cbc)&EVP_aes_256_cbc;

    pEVP_sha256 = (tEVP_sha256)&EVP_sha256;

    pEVP_CIPHER_CTX_init = (tEVP_CIPHER_CTX_init)&EVP_CIPHER_CTX_init;

    pEVP_EncryptInit_ex = (tEVP_EncryptInit_ex)&EVP_EncryptInit_ex;

    pEVP_EncryptUpdate = (tEVP_EncryptUpdate)&EVP_EncryptUpdate;

    pEVP_EncryptFinal_ex = (tEVP_EncryptFinal_ex)&EVP_EncryptFinal_ex;

    pEVP_CIPHER_CTX_cleanup = (tEVP_CIPHER_CTX_cleanup)&EVP_CIPHER_CTX_cleanup;

    pEVP_DecryptInit_ex = (tEVP_DecryptInit_ex)&EVP_DecryptInit_ex;

    pEVP_DecryptUpdate = (tEVP_DecryptUpdate)&EVP_DecryptUpdate;

    pEVP_DecryptFinal_ex = (tEVP_DecryptFinal_ex)&EVP_DecryptFinal_ex;
#endif
    loaded = true;
}

bool OpenSslLib::isLoaded()
{
    return loaded;
}

QByteArray OpenSslLib::RANDbytes(int size)
{
    OpenSslLib ssl;

    if (!ssl.loaded)
        return QByteArray();

    QByteArray output;
    output.resize(size);

    ssl.pRAND_bytes((uchar*)output.data(), size);

    return output;
}

QByteArray OpenSslLib::SHA256(const QByteArray &data, const QByteArray &salt)
{
    OpenSslLib ssl;

    SHA256_CTX ctx;

    unsigned char md[SHA256_DIGEST_LENGTH];

    ssl.pSHA256_Init(&ctx);
    ssl.pSHA256_Update(&ctx, (unsigned char*)data.data(), data.size());
    ssl.pSHA256_Update(&ctx, (unsigned char*)salt.data(), salt.size());
    ssl.pSHA256_Final(md, &ctx);

    QByteArray hash = QByteArray((char*)md, sizeof(md));

    return hash;
}

void OpenSslLib::startEncrypt(const QByteArray &salt)
{
    if (enc_ctx)
        return;

    enc_ctx = new EVP_CIPHER_CTX;

    unsigned char *key_data;
    int key_data_len;

    key_data = (unsigned char*)m_password.data();
    key_data_len = m_password.size();

    int i, nrounds = 1;
    unsigned char key[EVP_MAX_KEY_LENGTH], iv[EVP_MAX_IV_LENGTH];

    i = pEVP_BytesToKey(pEVP_aes_256_cbc(), pEVP_sha256(), (unsigned char*)salt.data(), key_data, key_data_len, nrounds, key, iv);

    if (i != 32)
        return;

    pEVP_CIPHER_CTX_init(enc_ctx);
    pEVP_EncryptInit_ex(enc_ctx, pEVP_aes_256_cbc(), NULL, key, iv);
}

QByteArray OpenSslLib::encryptPrivate(const QByteArray &input)
{
    if (!enc_ctx)
        return QByteArray();

    QByteArray preencrypted;
    preencrypted.append(input);

    int len = preencrypted.size();

    int c_len = len + AES_BLOCK_SIZE, f_len = 0;
    QByteArray cipherdata;
    cipherdata.resize(c_len);

    if (pEVP_EncryptInit_ex(enc_ctx, NULL, NULL, NULL, NULL) != 1)
        return QByteArray();
    if (pEVP_EncryptUpdate(enc_ctx, (unsigned char*)cipherdata.data(), &c_len, (unsigned char*)preencrypted.data(), len) != 1)
        return QByteArray();
    if (pEVP_EncryptFinal_ex(enc_ctx, (unsigned char*)cipherdata.data() + c_len, &f_len) != 1)
        return QByteArray();

    QByteArray output = QByteArray((char*)cipherdata.data(), c_len + f_len);

    return output;
}

void OpenSslLib::stopEncrypt()
{
    if (!enc_ctx)
        return;

    pEVP_CIPHER_CTX_cleanup(enc_ctx);
    delete enc_ctx;
    enc_ctx = nullptr;
}

void OpenSslLib::startDecrypt(const QByteArray &salt)
{
    if (dec_ctx)
        return;

    dec_ctx = new EVP_CIPHER_CTX;

    unsigned char *key_data;
    int key_data_len;

    key_data = (unsigned char*)m_password.data();
    key_data_len = m_password.size();

    int i, nrounds = 1;
    unsigned char key[EVP_MAX_KEY_LENGTH], iv[EVP_MAX_IV_LENGTH];

    i = pEVP_BytesToKey(pEVP_aes_256_cbc(), pEVP_sha256(), (unsigned char*)salt.data(), key_data, key_data_len, nrounds, key, iv);

    if (i != 32)
        return;

    pEVP_CIPHER_CTX_init(dec_ctx);
    pEVP_DecryptInit_ex(dec_ctx, pEVP_aes_256_cbc(), NULL, key, iv);
}

QByteArray OpenSslLib::decryptPrivate(const QByteArray &input)
{
    if (!dec_ctx)
        return QByteArray();

    int len = input.size();

    int p_len = len, f_len = 0;
    QByteArray plaindata;
    plaindata.resize(p_len + AES_BLOCK_SIZE);

    if (pEVP_DecryptInit_ex(dec_ctx, NULL, NULL, NULL, NULL) != 1)
        return QByteArray();
    if (pEVP_DecryptUpdate(dec_ctx, (unsigned char*)plaindata.data(), &p_len, (unsigned char*)input.data(), len) != 1)
        return QByteArray();
    if (pEVP_DecryptFinal_ex(dec_ctx, (unsigned char*)plaindata.data() + p_len, &f_len) != 1)
        return QByteArray();

    QByteArray output = QByteArray((char*)plaindata.data(), p_len + f_len);

    return output;
}

void OpenSslLib::stopDecrypt()
{
    if (!dec_ctx)
        return;

    pEVP_CIPHER_CTX_cleanup(dec_ctx);
    delete dec_ctx;
    dec_ctx = nullptr;
}

QByteArray OpenSslLib::encrypt(QByteArray data)
{
    QByteArray salt;
    QByteArray encrypted;
    QByteArray encryptedheader;

    salt = OpenSslLib::RANDbytes(8);

    startEncrypt(salt);
    encrypted = encryptPrivate(data);
    stopEncrypt();

    encryptedheader.append(QByteArray("Salted__", 8));
    encryptedheader.append(salt);
    encryptedheader.append(encrypted);

    return encryptedheader;
}

QByteArray OpenSslLib::decrypt(QByteArray data)
{
    QByteArray decrypted;
    QByteArray salt;

    data.remove(0, 8); //Remove 'Salted__'
    salt = data.mid(0, 8);
    data.remove(0, 8);

    startDecrypt(salt);
    decrypted = decryptPrivate(data);
    stopDecrypt();

    return decrypted;
}

void OpenSslLib::setPassword(const QByteArray &password)
{
    m_password = password;
}
