#include "openssllib.h"

OpenSslLib::OpenSslLib(QObject *parent) : QObject(parent)
{
    enc_ctx = nullptr;
    dec_ctx = nullptr;

    m_loaded = false;

    loadFunctions();
}

void OpenSslLib::loadFunctions()
{
    if (m_loaded)
        return;

#if defined(Q_OS_WIN) || defined(Q_OS_WINPHONE)
    QLibrary openssl("libeay32");
#else
    QLibrary openssl("crypto");
#endif

    if (!openssl.load())
        return;

    if (!(pRAND_bytes = reinterpret_cast<tRAND_bytes>(openssl.resolve("RAND_bytes"))))
        return;

    if (!(pSHA256_Init = reinterpret_cast<tSHA256_Init>(openssl.resolve("SHA256_Init"))))
        return;

    if (!(pSHA256_Update = reinterpret_cast<tSHA256_Update>(openssl.resolve("SHA256_Update"))))
        return;

    if (!(pSHA256_Final = reinterpret_cast<tSHA256_Final>(openssl.resolve("SHA256_Final"))))
        return;

    if (!(pBN_new = reinterpret_cast<tBN_new>(openssl.resolve("BN_new"))))
        return;

    if (!(pBN_free = reinterpret_cast<tBN_free>(openssl.resolve("BN_free"))))
        return;

    if (!(pBN_set_word = reinterpret_cast<tBN_set_word>(openssl.resolve("BN_set_word"))))
        return;

    if (!(pBIO_new = reinterpret_cast<tBIO_new>(openssl.resolve("BIO_new"))))
        return;

    if (!(pBIO_s_mem = reinterpret_cast<tBIO_s_mem>(openssl.resolve("BIO_s_mem"))))
        return;

    if (!(pBIO_free_all = reinterpret_cast<tBIO_free_all>(openssl.resolve("BIO_free_all"))))
        return;

    if (!(pBIO_read = reinterpret_cast<tBIO_read>(openssl.resolve("BIO_read"))))
        return;

    if (!(pEVP_BytesToKey = reinterpret_cast<tEVP_BytesToKey>(openssl.resolve("EVP_BytesToKey"))))
        return;

    if (!(pEVP_aes_256_cbc = reinterpret_cast<tEVP_aes_256_cbc>(openssl.resolve("EVP_aes_256_cbc"))))
        return;

    if (!(pEVP_sha256 = reinterpret_cast<tEVP_sha256>(openssl.resolve("EVP_sha256"))))
        return;

    if (!(pEVP_CIPHER_CTX_init = reinterpret_cast<tEVP_CIPHER_CTX_init>(openssl.resolve("EVP_CIPHER_CTX_init"))))
        return;

    if (!(pEVP_EncryptInit_ex = reinterpret_cast<tEVP_EncryptInit_ex>(openssl.resolve("EVP_EncryptInit_ex"))))
        return;

    if (!(pEVP_EncryptUpdate = reinterpret_cast<tEVP_EncryptUpdate>(openssl.resolve("EVP_EncryptUpdate"))))
        return;

    if (!(pEVP_EncryptFinal_ex = reinterpret_cast<tEVP_EncryptFinal_ex>(openssl.resolve("EVP_EncryptFinal_ex"))))
        return;

    if (!(pEVP_CIPHER_CTX_cleanup = reinterpret_cast<tEVP_CIPHER_CTX_cleanup>(openssl.resolve("EVP_CIPHER_CTX_cleanup"))))
        return;

    if (!(pEVP_DecryptInit_ex = reinterpret_cast<tEVP_DecryptInit_ex>(openssl.resolve("EVP_DecryptInit_ex"))))
        return;

    if (!(pEVP_DecryptUpdate = reinterpret_cast<tEVP_DecryptUpdate>(openssl.resolve("EVP_DecryptUpdate"))))
        return;

    if (!(pEVP_DecryptFinal_ex = reinterpret_cast<tEVP_DecryptFinal_ex>(openssl.resolve("EVP_DecryptFinal_ex"))))
        return;

    m_loaded = true;
}

bool OpenSslLib::isLoaded()
{
    return m_loaded;
}

QByteArray OpenSslLib::RANDbytes(int size)
{
    OpenSslLib ssl;

    if (!ssl.m_loaded)
        return QByteArray();

    QByteArray output;
    output.resize(size);

    ssl.pRAND_bytes(reinterpret_cast<unsigned char*>(output.data()), size);

    return output;
}

QByteArray OpenSslLib::SHA256(const QByteArray &data, const QByteArray &salt)
{
    OpenSslLib ssl;

    SHA256_CTX ctx;

    unsigned char md[SHA256_DIGEST_LENGTH];

    ssl.pSHA256_Init(&ctx);
    ssl.pSHA256_Update(&ctx, reinterpret_cast<const unsigned char*>(data.data()), uint(data.size()));
    ssl.pSHA256_Update(&ctx, reinterpret_cast<const unsigned char*>(salt.data()), uint(salt.size()));
    ssl.pSHA256_Final(md, &ctx);

    QByteArray hash = QByteArray(reinterpret_cast<char*>(md), sizeof(md));

    return hash;
}

void OpenSslLib::startEncrypt(const QByteArray &salt)
{
    if (enc_ctx)
        return;

    enc_ctx = new EVP_CIPHER_CTX;

    unsigned char *key_data;
    int key_data_len;

    key_data = reinterpret_cast<unsigned char*>(m_password.data());
    key_data_len = m_password.size();

    int i, nrounds = 1;
    unsigned char key[EVP_MAX_KEY_LENGTH], iv[EVP_MAX_IV_LENGTH];

    i = pEVP_BytesToKey(pEVP_aes_256_cbc(), pEVP_sha256(), reinterpret_cast<const unsigned char*>(salt.data()), key_data, key_data_len, nrounds, key, iv);

    if (i != 32)
        return;

    pEVP_CIPHER_CTX_init(enc_ctx);
    pEVP_EncryptInit_ex(enc_ctx, pEVP_aes_256_cbc(), nullptr, key, iv);
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

    if (pEVP_EncryptInit_ex(enc_ctx, nullptr, nullptr, nullptr, nullptr) != 1)
        return QByteArray();
    if (pEVP_EncryptUpdate(enc_ctx, reinterpret_cast<unsigned char*>(cipherdata.data()), &c_len, reinterpret_cast<unsigned char*>(preencrypted.data()), len) != 1)
        return QByteArray();
    if (pEVP_EncryptFinal_ex(enc_ctx, reinterpret_cast<unsigned char*>(cipherdata.data()) + c_len, &f_len) != 1)
        return QByteArray();

    QByteArray output = QByteArray(cipherdata.data(), c_len + f_len);

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

    key_data = reinterpret_cast<unsigned char*>(m_password.data());
    key_data_len = m_password.size();

    int i, nrounds = 1;
    unsigned char key[EVP_MAX_KEY_LENGTH], iv[EVP_MAX_IV_LENGTH];

    i = pEVP_BytesToKey(pEVP_aes_256_cbc(), pEVP_sha256(), reinterpret_cast<const unsigned char*>(salt.data()), key_data, key_data_len, nrounds, key, iv);

    if (i != 32)
        return;

    pEVP_CIPHER_CTX_init(dec_ctx);
    pEVP_DecryptInit_ex(dec_ctx, pEVP_aes_256_cbc(), nullptr, key, iv);
}

QByteArray OpenSslLib::decryptPrivate(const QByteArray &input)
{
    if (!dec_ctx)
        return QByteArray();

    int len = input.size();

    int p_len = len, f_len = 0;
    QByteArray plaindata;
    plaindata.resize(p_len + AES_BLOCK_SIZE);

    if (pEVP_DecryptInit_ex(dec_ctx, nullptr, nullptr, nullptr, nullptr) != 1)
        return QByteArray();
    if (pEVP_DecryptUpdate(dec_ctx, reinterpret_cast<unsigned char*>(plaindata.data()), &p_len, reinterpret_cast<const unsigned char*>(input.data()), len) != 1)
        return QByteArray();
    if (pEVP_DecryptFinal_ex(dec_ctx, reinterpret_cast<unsigned char*>(plaindata.data()) + p_len, &f_len) != 1)
        return QByteArray();

    QByteArray output = QByteArray(plaindata.data(), p_len + f_len);

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
